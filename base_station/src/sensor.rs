use std::borrow::Cow;
use std::collections::hash_map::Entry;
use std::collections::HashMap;
use std::io;
use std::io::Cursor;
use std::marker::PhantomData;
use std::rc::Rc;

use byteorder::{LittleEndian, ReadBytesExt};
use dbus::arg::{RefArg, Variant};
use dbus::SignalArgs;
use dbus::stdintf::org_freedesktop_dbus::ObjectManager;
use dbus::stdintf::org_freedesktop_dbus::ObjectManagerInterfacesAdded;
use dbus::stdintf::org_freedesktop_dbus::ObjectManagerInterfacesRemoved;
use dbus::stdintf::org_freedesktop_dbus::Properties;
use dbus::stdintf::org_freedesktop_dbus::PropertiesPropertiesChanged;
use failure::Fail;
use log::debug;

use crate::bluez;
use crate::bluez::Battery1;
use crate::bluez::Device1;
use crate::bluez::GattCharacteristic1;
use crate::dbus::RefArgCast;

const BUS_NAME: &str = "org.bluez";
const DEVICE_INTERFACE: &str = "org.bluez.Device1";
const BATTERY_INTERFACE: &str = "org.bluez.Battery1";
const GATT_CHARACTERISTIC_INTERFACE: &str = "org.bluez.GattCharacteristic1";
const GATT_DESCRIPTOR_INTERFACE: &str = "org.bluez.GattDescriptor1";

type DBusProperties = HashMap<String, dbus::arg::Variant<Box<dbus::arg::RefArg>>>;
type DBusObject = HashMap<String, DBusProperties>;
type DBusPath = dbus::ConnPath<'static, Rc<dbus::Connection>>;
type DBusError = crate::dbus::TypedError;

#[derive(Debug, Fail)]
pub enum Error {
    #[fail(display = "Sensor with ID {} not found", id)]
    SensorNotFound { id: Cow<'static, str> },
    #[fail(display = "Sensor cannot be used: {}", 0)]
    SensorInvalid(Cow<'static, str>),
    #[fail(display = "GATT attribute '{}' ({}) not found", name, uuid)]
    GATTAttributeNotFound { name: Cow<'static, str>, uuid: Cow<'static, str> },
    #[fail(display = "Invalid data received: {}", 0)]
    InvalidData(#[fail(cause)]  io::Error),
    #[fail(display = "BlueZ error: {}", 0)]
    BlueZ(#[fail(cause)] bluez::Error),
    #[fail(display = "DBus error: {}", 0)]
    DBus(#[fail(cause)] DBusError),
}

impl From<dbus::Error> for Error {
    fn from(e: dbus::Error) -> Self {
        DBusError::from(e).into()
    }
}

impl From<DBusError> for Error {
    fn from(e: DBusError) -> Self {
        if let DBusError { kind: crate::dbus::ErrorKind::Custom, cause } = e {
            if cause.name().map_or(false, |m| m.starts_with("org.bluez.Error")) {
                Error::BlueZ(cause.into())
            } else {
                Error::DBus(cause.into())
            }
        } else {
            Error::DBus(e)
        }
    }
}

pub struct PropertyChangedIterator<'a, T> {
    manager: &'a mut SensorManager,
    obj: dbus::ConnPath<'a, Rc<dbus::Connection>>,
    iter: dbus::ConnMsgs<Rc<dbus::Connection>>,
    match_str: String,
    interface: Cow<'a, str>,
    property: Cow<'a, str>,
    d: PhantomData<T>,
}

impl<'a, T> PropertyChangedIterator<'a, T> {
    fn new(
        manager: &'a mut SensorManager,
        path: dbus::Path<'a>,
        interface: Cow<'a, str>,
        property: Cow<'a, str>,
        timeout_ms: Option<u32>,
    ) -> Result<Self, Error> {
        let bus_name: dbus::BusName = BUS_NAME.into();
        let match_str = PropertiesPropertiesChanged::match_str(
            Some(&bus_name),
            Some(&path),
        );

        let obj = DBusPath {
            conn: manager.conn.clone(),
            dest: bus_name,
            path: path.into_static(),
            timeout: 1000,
        };
        let iter = dbus::ConnMsgs { conn: manager.conn.clone(), timeout_ms };
        manager.conn.add_match(&match_str)?;

        Ok(Self { manager, obj, iter, match_str, interface, property, d: PhantomData })
    }
}

impl<'a, T: for<'b> dbus::arg::Get<'b>> PropertyChangedIterator<'a, T> {
    pub fn get(&self) -> Result<T, Error> {
        self.obj.get(&self.interface, &self.property)
            .map_err(Error::from)
    }
}

impl<'a, T: 'static + for<'b> dbus::arg::Get<'b> + RefArgCast> Iterator for PropertyChangedIterator<'a, T> {
    type Item = Result<T, Error>;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if let Some(msg) = self.iter.next() {
                if let Some(PropertiesPropertiesChanged {
                                interface_name,
                                changed_properties,
                                invalidated_properties
                            }) = PropertiesPropertiesChanged::from_message(&msg) {
                    debug!("properties changed: {}, changed: {:?} invalidated: {:?}",
                           interface_name, changed_properties, invalidated_properties);

                    if interface_name == self.interface {
                        if let Some(value) = changed_properties.get(self.property.as_ref()) {
                            return Some(T::ref_arg_cast(&value.0).map_err(Error::from));
                        } else if invalidated_properties.iter().any(|p| p == &self.property) {
                            match self.get() {
                                // Ignore missing property and continue waiting
                                Err(Error::DBus(DBusError { kind: crate::dbus::ErrorKind::InvalidArgs, .. })) => (),
                                v => return Some(v)
                            };
                        }
                    }
                } else {
                    self.manager.process_interface_signal(&msg, |_, _| ());
                }
            } else {
                break None;
            }
        }
    }
}

impl<'a, T> Drop for PropertyChangedIterator<'a, T> {
    fn drop(&mut self) {
        self.manager.conn.remove_match(&self.match_str).ok();
    }
}

pub struct SensorManager {
    conn: Rc<dbus::Connection>,
    objs: HashMap<dbus::Path<'static>, DBusObject>,
}

impl SensorManager {
    pub fn new() -> Result<Self, Error> {
        let conn = Rc::new(dbus::Connection::get_private(dbus::BusType::System)
            .map_err(Error::from)?);

        let bus_name = dbus::BusName::from(BUS_NAME);
        let root_path = dbus::Path::from("/");

        let bluez = conn.with_path(BUS_NAME, &root_path, 1000);
        let objs = bluez.get_managed_objects()?;

        conn.add_match(&ObjectManagerInterfacesAdded::match_str(Some(&bus_name), Some(&root_path)))?;
        conn.add_match(&ObjectManagerInterfacesRemoved::match_str(Some(&bus_name), Some(&root_path)))?;

        Ok(SensorManager { conn, objs })
    }

    fn process_interface_signal<F: FnOnce(&dbus::Path, &DBusObject)>(&mut self, msg: &dbus::Message, f: F) {
        if let Some(ObjectManagerInterfacesAdded { object: path, interfaces }) = ObjectManagerInterfacesAdded::from_message(&msg) {
            let all_interfaces = self.objs.entry(path.clone())
                .or_default();
            all_interfaces.extend(interfaces);
            // This method really has no business executing a closure. It should just return
            // references to the new path and object, but it is currently impossible to get a
            // reference to a key owned by a HashMap.
            f(&path, &all_interfaces);
        } else if let Some(ObjectManagerInterfacesRemoved { object: path, interfaces }) = ObjectManagerInterfacesRemoved::from_message(&msg) {
            match self.objs.entry(path) {
                Entry::Occupied(mut e) => {
                    let obj = e.get_mut();
                    interfaces.iter().for_each(|i| { obj.remove(i); });
                    if obj.is_empty() {
                        e.remove();
                    }
                }
                _ => ()
            };
        }
    }

    fn find_object<T, F: FnMut(&dbus::Path, &DBusObject) -> Option<T>>(&mut self, mut f: F, timeout_ms: Option<u32>) -> Option<T> {
        let r = self.objs.iter()
            // Try to find the object in the existing database
            .find_map(|(path, obj)| f(path, obj))
            // If we couldn't find anything, wait for a message that might contain the desired object
            .or_else(|| (dbus::ConnMsgs { conn: self.conn.clone(), timeout_ms })
                .find_map(|msg| {
                    let mut t = None;
                    self.process_interface_signal(&msg, |path, obj| {
                        t = f(path, obj);
                    });
                    t
                }));
        // Process any messages available right now and add them to the database
        (dbus::ConnMsgs { conn: self.conn.clone(), timeout_ms: None })
            .for_each(|msg| self.process_interface_signal(&msg, |_, _| ()));
        r
    }

    fn find_objects<F: FnMut(&dbus::Path, &DBusObject) -> bool>(&mut self, mut f: F, timeout_ms: Option<u32>) {
        self.find_object(|path, obj| if f(path, obj) { Some(()) } else { None }, timeout_ms);
    }

    fn property_change_iter<'a, T>(
        &'a mut self, path: dbus::Path<'a>,
        interface: Cow<'a, str>,
        property: Cow<'a, str>,
        timeout_ms: Option<u32>,
    ) -> Result<PropertyChangedIterator<'a, T>, Error> {
        PropertyChangedIterator::new(self, path, interface, property, timeout_ms)
    }

    pub fn get_sensor_by_address(&mut self, address: &str, timeout_ms: Option<u32>) -> Result<Sensor, Error> {
        let conn = self.conn.clone();
        self.find_object(|path, obj| {
            obj.get(DEVICE_INTERFACE.into())
                .and_then(|props| props.get("Address"))
                .and_then(dbus::arg::Variant::as_str)
                .and_then(|a| if a == address { Some(()) } else { None })
                .map(|_| DBusPath {
                    conn: conn.clone(),
                    dest: BUS_NAME.into(),
                    path: path.clone().into_static(),
                    timeout: 30000,
                })
        }, timeout_ms)
            .ok_or(Error::SensorNotFound { id: address.to_owned().into() })
            .and_then(move |device| Sensor::new(self, device))
    }
}

pub struct Sensor<'a> {
    manager: &'a mut SensorManager,
    device: DBusPath,
    battery: Option<DBusPath>,
    ess_temperature: Option<DBusPath>,
    wls_water_level: Option<DBusPath>,
    wls_water_distance: Option<DBusPath>,
    wls_tank_depth: Option<DBusPath>,
    scs_error: Option<DBusPath>,
    scs_status: Option<DBusPath>,
}

impl<'a> Sensor<'a> {
    // Characteristic Presentation Format
    const CPF_UUID: &'static str = "00002904-0000-1000-8000-00805f9b34fb";

    // Battery Service
    const BAS_BATTERY_LEVEL_UUID: &'static str = "00002a19-0000-1000-8000-00805f9b34fb";

    // Environmental Sensing Service
    const ESS_TEMPERATURE_UUID: &'static str = "00002a6e-0000-1000-8000-00805f9b34fb";

    // Water Level Service
    const WLS_UUID: &'static str = "67dd9530-9b07-e58b-e811-affa00e3c701";
    const WLS_WATER_LEVEL_UUID: &'static str = "7af2e6a5-729a-a1b5-4a4e-6d5799bc4c24";
    const WLS_WATER_DISTANCE_UUID: &'static str = "fe475554-4784-3b82-9442-a74f062d3101";
    const WLS_TANK_DEPTH_UUID: &'static str = "d357ee3d-765c-debf-8c49-4a2fcbecc6d3";

    // System Control Service
    const SCS_UUID: &'static str = "89efdfcb-9661-888e-724e-c0a20f20f8a1";
    const SCS_ERROR_UUID: &'static str = "c25f2f83-847b-c6bd-a74a-cbc37714f1e3";
    const SCS_STATUS_UUID: &'static str = "57c15dae-edd4-c195-284b-61f909f5325b";

    fn new(manager: &'a mut SensorManager, device: dbus::ConnPath<'static, Rc<dbus::Connection>>) -> Result<Self, Error> {
        if device.get_paired()? {
            Ok(Sensor {
                manager,
                device,
                battery: None,
                ess_temperature: None,
                wls_water_level: None,
                wls_water_distance: None,
                wls_tank_depth: None,
                scs_error: None,
                scs_status: None,
            })
        } else {
            Err(Error::SensorInvalid("not paired".into()))
        }
    }

    fn find_gatt_attributes(&mut self) -> Result<(), Error> {
        let mut battery: Option<DBusPath> = None;
        let mut ess_temperature: Option<DBusPath> = None;
        let mut wls_water_level: Option<DBusPath> = None;
        let mut wls_water_distance: Option<DBusPath> = None;
        let mut wls_tank_depth: Option<DBusPath> = None;
        let mut scs_error: Option<DBusPath> = None;
        let mut scs_status: Option<DBusPath> = None;

        let conn = self.manager.conn.clone();
        let device_path = self.device.path.clone();
        self.manager.find_objects(|path, obj| {
            if let Some(props) = obj
                .get(GATT_CHARACTERISTIC_INTERFACE.into())
                .or_else(|| obj.get(GATT_DESCRIPTOR_INTERFACE.into())) {
                props.get("UUID")
                    .and_then(dbus::arg::Variant::as_str)
                    .and_then(|uuid| match uuid {
                        Self::ESS_TEMPERATURE_UUID => Some(&mut ess_temperature),
                        Self::WLS_WATER_LEVEL_UUID => Some(&mut wls_water_level),
                        Self::WLS_WATER_DISTANCE_UUID => Some(&mut wls_water_distance),
                        Self::WLS_TANK_DEPTH_UUID => Some(&mut wls_tank_depth),
                        Self::SCS_ERROR_UUID => Some(&mut scs_error),
                        Self::SCS_STATUS_UUID => Some(&mut scs_status),
                        _ => None
                    })
            } else if path == &device_path && obj.contains_key(BATTERY_INTERFACE.into()) {
                Some(&mut battery)
            } else {
                None
            }.map(|char| {
                *char = Some(dbus::ConnPath {
                    conn: conn.clone(),
                    dest: BUS_NAME.into(),
                    path: path.clone().into_static(),
                    timeout: 1000,
                });
            });

            battery.is_some() &&
                ess_temperature.is_some() &&
                wls_water_level.is_some() &&
                wls_water_distance.is_some() &&
                wls_tank_depth.is_some() &&
                scs_status.is_some() &&
                scs_error.is_some()
        }, Some(5000));

        self.battery = battery;
        self.ess_temperature = ess_temperature;
        self.wls_water_level = wls_water_level;
        self.wls_water_distance = wls_water_distance;
        self.wls_tank_depth = wls_tank_depth;
        self.scs_error = scs_error;
        self.scs_status = scs_status;

        Ok(())
    }

    pub fn connect(&mut self) -> Result<(), Error> {
        self.device.connect().map_err(Error::from)?;
        self.find_gatt_attributes()?;

        Ok(())
    }

    pub fn disconnect(&mut self) -> Result<(), Error> {
        self.device.disconnect().map_err(Error::from)
    }

    pub fn name(&self) -> Result<String, Error> {
        self.device.get_name().map_err(Error::from)
    }

    pub fn address(&self) -> Result<String, Error> {
        self.device.get_address().map_err(Error::from)
    }

    pub fn wait_new_data(&mut self, timeout_ms: Option<u32>) -> Result<bool, Error> {
        for value in self.manager.property_change_iter::<HashMap<String, Variant<Vec<u8>>>>(
            self.device.path.clone(),
            DEVICE_INTERFACE.into(),
            "ServiceData".into(),
            timeout_ms,
        )? {
            let mut value = value?;
            if let Some(data) = value.remove(Self::SCS_UUID) {
                let status = Cursor::new(data.0)
                    .read_u32::<LittleEndian>()
                    .map_err(Error::InvalidData)?;
                debug!("Status changed: {:0>8x}", status);
                // FIXME: parse status for real
                if status == 1 {
                    return Ok(true);
                }
            }
        }
        Ok(false)
    }

    fn read_attr<T, F>(&self,
                       attr: &Option<DBusPath>,
                       name: &'static str,
                       uuid: &'static str,
                       parser: F) -> Result<T, Error>
        where F: FnOnce(Cursor<Vec<u8>>) -> Result<T, io::Error> {
        attr.as_ref()
            .ok_or(Error::GATTAttributeNotFound { name: name.into(), uuid: uuid.into() })
            .and_then(|c| c.read_value(HashMap::new())
                .map_err(Error::from))
            .map(Cursor::new)
            .and_then(|v| parser(v)
                .map_err(Error::InvalidData))
    }

    fn write_attr<T, F>(&self,
                        attr: &Option<DBusPath>,
                        value: T,
                        name: &'static str,
                        uuid: &'static str,
                        writer: F) -> Result<(), Error>
        where F: FnOnce(T) -> Vec<u8> {
        attr.as_ref()
            .ok_or(Error::GATTAttributeNotFound { name: name.into(), uuid: uuid.into() })
            .and_then(|c| c.write_value(writer(value), HashMap::new())
                .map_err(Error::from))
    }

    pub fn set_status(&self, status: u32) -> Result<(), Error> {
        self.write_attr(&self.scs_status,
                        status,
                        "SCS Status",
                        Self::SCS_STATUS_UUID,
                        |s| s.to_le_bytes().to_vec())
    }

    pub fn errors(&self) -> Result<u32, Error> {
        self.read_attr(&self.scs_error,
                       "SCS Error",
                       Self::SCS_ERROR_UUID,
                       |mut v| v.read_u32::<LittleEndian>())
    }

    pub fn battery_percentage(&self) -> Result<u8, Error> {
        self.battery.as_ref()
            .ok_or(Error::GATTAttributeNotFound {
                name: "BAS Battery Level".into(),
                uuid: Self::BAS_BATTERY_LEVEL_UUID.into(),
            }).and_then(|b| b.get_percentage()
            .map_err(Error::from))
            .map(|p| p as u8)
    }

    pub fn temperature(&self) -> Result<f32, Error> {
        self.read_attr(&self.ess_temperature,
                       "ESS Temperature",
                       Self::ESS_TEMPERATURE_UUID,
                       |mut v| v.read_i16::<LittleEndian>())
            .map(|l| l as f32 / 100.0)
    }

    pub fn water_level(&self) -> Result<f32, Error> {
        self.read_attr(&self.wls_water_level,
                       "WLS Water Level",
                       Self::WLS_WATER_LEVEL_UUID,
                       |mut v| v.read_u16::<LittleEndian>())
            .map(|l| l as f32 / 1000.0)
    }

    pub fn water_distance(&self) -> Result<f32, Error> {
        self.read_attr(&self.wls_water_distance,
                       "WLS Water Distance",
                       Self::WLS_WATER_DISTANCE_UUID,
                       |mut v| v.read_u16::<LittleEndian>())
            .map(|l| l as f32 / 1000.0)
    }

    pub fn tank_depth(&self) -> Result<f32, Error> {
        self.read_attr(&self.wls_tank_depth,
                       "WLS Tank Depth",
                       Self::WLS_TANK_DEPTH_UUID,
                       |mut v| v.read_u16::<LittleEndian>())
            .map(|l| l as f32 / 1000.0)
    }
}

impl<'a> Drop for Sensor<'a> {
    fn drop(&mut self) {
        self.device.disconnect().ok();
    }
}