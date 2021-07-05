use std::borrow::Cow;
use std::convert::TryInto;
use std::io;
use std::io::Cursor;
use std::time::Duration;

use byteorder::{LittleEndian, ReadBytesExt};
use thiserror::Error;

use crate::util::Timeout;

#[derive(Debug, Error)]
pub enum Error {
    #[error("sensor with address {address} not found")]
    SensorNotFound { address: String },
    #[error("sensor cannot be used: {0}")]
    SensorInvalid(Cow<'static, str>),
    #[error("GATT attribute '{0}' not found")]
    GattAttributeNotFound(uuid::Uuid),
    #[error("sensor is not connected")]
    NotConnected,
    #[error("battery not found")]
    BatteryNotFound,
    #[error("invalid data received: {0:?}")]
    InvalidData(Vec<u8>),
    #[error("BlueZ error: {0}")]
    BlueZ(#[from] blurst::Error),
}

struct SensorGatt {
    bas_battery_level: blurst::GattCharacteristic,
    ess_temperature: blurst::GattCharacteristic,
    wls_water_level: blurst::GattCharacteristic,
    wls_water_distance: blurst::GattCharacteristic,
    wls_tank_depth: blurst::GattCharacteristic,
    scs_error: blurst::GattCharacteristic,
    scs_status: blurst::GattCharacteristic,
    scs_battery_voltage: blurst::GattCharacteristic,
}

/// Object for communicating over Bluetooth Low Energy (BLE) with the water
/// level sensor.
pub struct Sensor {
    device: blurst::Device,
    gatt: Option<SensorGatt>,
}

impl Sensor {
    const DBUS_TIMEOUT: Duration = Duration::from_secs(10);

    // Characteristic Presentation Format
    const CPF_UUID: &'static str = "00002904-0000-1000-8000-00805f9b34fb";

    // Battery Service
    const BAS_UUID: &'static str = "0000180f-0000-1000-8000-00805f9b34fb";
    const BAS_BATTERY_LEVEL_UUID: &'static str = "00002a19-0000-1000-8000-00805f9b34fb";

    // Environmental Sensing Service
    const ESS_UUID: &'static str = "0000181a-0000-1000-8000-00805f9b34fb";
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
    const SCS_BATTERY_VOLTAGE_UUID: &'static str = "21c1388b-4490-4026-837c-05ad6b5508dd";

    fn new(device: blurst::Device) -> Result<Self, Error> {
        if device.paired()? {
            Ok(Sensor { device, gatt: None })
        } else {
            Err(Error::SensorInvalid("not paired".into()))
        }
    }

    pub fn find_by_address(
        adapter: &blurst::Adapter,
        address: &str,
        timeout: Duration,
    ) -> Result<Sensor, Error> {
        Self::new(
            adapter
                .find_device_by_address(address, Duration::from_secs(60), timeout)?
                .ok_or_else(|| Error::SensorNotFound {
                    address: address.to_string(),
                })?,
        )
    }

    fn find_service(
        device: &blurst::Device,
        uuid: &'static str,
        timeout: Duration,
    ) -> Result<blurst::GattService, Error> {
        let uuid = uuid.parse().unwrap();
        device
            .find_service_by_uuid(&uuid, Self::DBUS_TIMEOUT, timeout)?
            .ok_or_else(|| Error::GattAttributeNotFound(uuid))
    }

    fn find_characteristic(
        service: &blurst::GattService,
        uuid: &'static str,
        timeout: Duration,
    ) -> Result<blurst::GattCharacteristic, Error> {
        let uuid = uuid.parse().unwrap();
        service
            .find_characteristic_by_uuid(&uuid, Self::DBUS_TIMEOUT, timeout)?
            .ok_or_else(|| Error::GattAttributeNotFound(uuid))
    }

    fn find_gatt_attributes(&mut self, timeout: Duration) -> Result<(), Error> {
        let timeout = Timeout::start(timeout);

        let bas = Self::find_service(&self.device, Self::BAS_UUID, timeout.get())?;
        let bas_battery_level =
            Self::find_characteristic(&bas, Self::BAS_BATTERY_LEVEL_UUID, timeout.get())?;

        let ess = Self::find_service(&self.device, Self::ESS_UUID, timeout.get())?;
        let ess_temperature =
            Self::find_characteristic(&ess, Self::ESS_TEMPERATURE_UUID, timeout.get())?;

        let wls = Self::find_service(&self.device, Self::WLS_UUID, timeout.get())?;
        let wls_water_level =
            Self::find_characteristic(&wls, Self::WLS_WATER_LEVEL_UUID, timeout.get())?;
        let wls_water_distance =
            Self::find_characteristic(&wls, Self::WLS_WATER_DISTANCE_UUID, timeout.get())?;
        let wls_tank_depth =
            Self::find_characteristic(&wls, Self::WLS_TANK_DEPTH_UUID, timeout.get())?;

        let scs = Self::find_service(&self.device, Self::SCS_UUID, timeout.get())?;
        let scs_error = Self::find_characteristic(&scs, Self::SCS_ERROR_UUID, timeout.get())?;
        let scs_status = Self::find_characteristic(&scs, Self::SCS_STATUS_UUID, timeout.get())?;
        let scs_battery_voltage =
            Self::find_characteristic(&scs, Self::SCS_BATTERY_VOLTAGE_UUID, timeout.get())?;

        self.gatt = Some(SensorGatt {
            bas_battery_level,
            ess_temperature,
            wls_water_distance,
            wls_water_level,
            wls_tank_depth,
            scs_error,
            scs_status,
            scs_battery_voltage,
        });
        Ok(())
    }

    pub fn connect(&mut self, timeout: Duration) -> Result<(), Error> {
        let timeout = Timeout::start(timeout);
        self.device.connect()?;
        self.find_gatt_attributes(timeout.get())?;

        Ok(())
    }

    pub fn disconnect(&mut self) -> Result<(), Error> {
        self.device.disconnect()?;
        self.gatt = None;
        Ok(())
    }

    pub fn name(&self) -> Result<String, Error> {
        Ok(self.device.name()?)
    }

    pub fn address(&self) -> Result<String, Error> {
        Ok(self.device.address()?)
    }

    fn wait_status(
        &mut self,
        mut pred: impl FnMut(u32) -> bool,
        timeout: Duration,
    ) -> Result<bool, Error> {
        let timeout = Timeout::start(timeout);
        Ok(loop {
            if let Some(data) = self
                .device
                .service_data()?
                .remove(&Self::SCS_UUID.parse().unwrap())
            {
                let status = u32::from_le_bytes(data.try_into().map_err(Error::InvalidData)?);
                log::debug!("status update: {:0>8x}", status);
                if pred(status) {
                    break true;
                }
            }
            if !self.device.wait_property_change(timeout.get())? {
                break false;
            }
        })
    }

    pub fn wait_new_data(&mut self, timeout: Duration) -> Result<bool, Error> {
        self.wait_status(|status| status == 1, timeout)
    }

    pub fn wait_new_data_cleared(&mut self, timeout: Duration) -> Result<bool, Error> {
        self.wait_status(|status| status == 0, timeout)
    }

    fn gatt(&self) -> Result<&SensorGatt, Error> {
        self.gatt.as_ref().ok_or(Error::NotConnected)
    }

    fn read_attr<T, F>(&self, attr: &blurst::GattCharacteristic, parser: F) -> Result<T, Error>
    where
        F: FnOnce(Cursor<&[u8]>) -> Result<T, io::Error>,
    {
        let value = attr.read_value()?;
        parser(Cursor::new(&value)).map_err(|_| Error::InvalidData(value))
    }

    pub fn set_status(&self, status: u32) -> Result<(), Error> {
        self.gatt()?
            .scs_status
            .write_value(status.to_le_bytes().to_vec())?;
        Ok(())
    }

    pub fn errors(&self) -> Result<u32, Error> {
        self.read_attr(&self.gatt()?.scs_error, |mut v| {
            v.read_u32::<LittleEndian>()
        })
    }

    pub fn battery_percentage(&self) -> Result<u8, Error> {
        self.read_attr(&self.gatt()?.bas_battery_level, |mut v| v.read_u8())
    }

    pub fn battery_voltage(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.scs_battery_voltage, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .map(|l| l as f32 / 1000.0)
    }

    pub fn temperature(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.ess_temperature, |mut v| {
            v.read_i16::<LittleEndian>()
        })
        .map(|l| l as f32 / 100.0)
    }

    pub fn water_level(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_water_level, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .map(|l| l as f32 / 1000.0)
    }

    pub fn water_distance(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_water_distance, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .map(|l| l as f32 / 1000.0)
    }

    pub fn tank_depth(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_tank_depth, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .map(|l| l as f32 / 1000.0)
    }
}

impl<'a> Drop for Sensor {
    fn drop(&mut self) {
        self.device.disconnect().ok();
    }
}
