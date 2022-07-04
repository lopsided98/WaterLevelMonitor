use std::borrow::Cow;
use std::io;
use std::io::Cursor;

use bluer::{Address, DeviceEvent, DeviceProperty};
use byteorder::{LittleEndian, ReadBytesExt};
use futures::StreamExt;
use thiserror::Error;
use uuid::{uuid, Uuid};

#[derive(Debug, Error)]
pub enum Error {
    #[error("sensor with address {0} not found")]
    SensorNotFound(bluer::Address),
    #[error("sensor cannot be used: {0}")]
    SensorInvalid(Cow<'static, str>),
    #[error("GATT attribute '{0}' not found")]
    GattAttributeNotFound(Uuid),
    #[error("sensor is not connected")]
    NotConnected,
    #[error("property not found")]
    PropertyNotFound,
    #[error("invalid data received: {0:?}")]
    InvalidData(Vec<u8>),
    #[error("BlueZ error: {0}")]
    BlueZ(#[from] bluer::Error),
}

struct SensorGatt {
    bas_battery_level: bluer::gatt::remote::Characteristic,
    ess_temperature: bluer::gatt::remote::Characteristic,
    wls_water_level: bluer::gatt::remote::Characteristic,
    wls_water_distance: bluer::gatt::remote::Characteristic,
    wls_tank_depth: bluer::gatt::remote::Characteristic,
    scs_error: bluer::gatt::remote::Characteristic,
    scs_status: bluer::gatt::remote::Characteristic,
    scs_battery_voltage: bluer::gatt::remote::Characteristic,
}

/// Object for communicating over Bluetooth Low Energy (BLE) with the water
/// level sensor.
pub struct Sensor {
    device: bluer::Device,
    gatt: Option<SensorGatt>,
    new_data_rx: tokio::sync::watch::Receiver<u64>,
    new_data_count: u64,
}

impl Sensor {
    // Characteristic Presentation Format
    const CPF_UUID: &'static str = "00002904-0000-1000-8000-00805f9b34fb";

    // Battery Service
    const BAS_UUID: Uuid = uuid!("0000180f-0000-1000-8000-00805f9b34fb");
    const BAS_BATTERY_LEVEL_UUID: Uuid = uuid!("00002a19-0000-1000-8000-00805f9b34fb");

    // Environmental Sensing Service
    const ESS_UUID: Uuid = uuid!("0000181a-0000-1000-8000-00805f9b34fb");
    const ESS_TEMPERATURE_UUID: Uuid = uuid!("00002a6e-0000-1000-8000-00805f9b34fb");

    // Water Level Service
    const WLS_UUID: Uuid = uuid!("67dd9530-9b07-e58b-e811-affa00e3c701");
    const WLS_WATER_LEVEL_UUID: Uuid = uuid!("7af2e6a5-729a-a1b5-4a4e-6d5799bc4c24");
    const WLS_WATER_DISTANCE_UUID: Uuid = uuid!("fe475554-4784-3b82-9442-a74f062d3101");
    const WLS_TANK_DEPTH_UUID: Uuid = uuid!("d357ee3d-765c-debf-8c49-4a2fcbecc6d3");

    // System Control Service
    const SCS_UUID: Uuid = uuid!("89efdfcb-9661-888e-724e-c0a20f20f8a1");
    const SCS_ERROR_UUID: Uuid = uuid!("c25f2f83-847b-c6bd-a74a-cbc37714f1e3");
    const SCS_STATUS_UUID: Uuid = uuid!("57c15dae-edd4-c195-284b-61f909f5325b");
    const SCS_BATTERY_VOLTAGE_UUID: Uuid = uuid!("dd08556b-ad05-7c83-2640-90448b38c121");

    async fn new(adapter: &bluer::Adapter, device: bluer::Device) -> Result<Self, Error> {
        if !device.is_paired().await? {
            return Err(Error::SensorInvalid("not paired".into()));
        }

        let (new_data_tx, new_data_rx) = tokio::sync::watch::channel(0);

        Ok(Sensor {
            device,
            gatt: None,
            new_data_rx,
            new_data_count: 0,
        })
    }

    pub async fn find_by_address(
        adapter: &bluer::Adapter,
        address: bluer::Address,
    ) -> Result<Sensor, Error> {
        Self::new(
            adapter,
            adapter
                .device(address)
                .map_err(|_| Error::SensorNotFound(address))?,
        )
        .await
    }

    async fn find_service(
        services: &mut Vec<bluer::gatt::remote::Service>,
        uuid: Uuid,
    ) -> Result<bluer::gatt::remote::Service, Error> {
        let mut index = None;
        for (i, service) in services.iter().enumerate() {
            if service.uuid().await? == uuid {
                index = Some(i);
                break;
            }
        }
        index
            .map(|i| services.swap_remove(i))
            .ok_or(Error::GattAttributeNotFound(uuid))
    }

    async fn find_characteristic(
        chars: &mut Vec<bluer::gatt::remote::Characteristic>,
        uuid: Uuid,
    ) -> Result<bluer::gatt::remote::Characteristic, Error> {
        let mut index = None;
        for (i, c) in chars.iter().enumerate() {
            if c.uuid().await? == uuid {
                index = Some(i);
                break;
            }
        }
        index
            .map(|i| chars.swap_remove(i))
            .ok_or(Error::GattAttributeNotFound(uuid))
    }

    async fn find_gatt_attributes(&mut self) -> Result<(), Error> {
        let mut services = self.device.services().await?;

        let bas = Self::find_service(&mut services, Self::BAS_UUID).await?;
        let mut bas_chars = bas.characteristics().await?;
        let bas_battery_level =
            Self::find_characteristic(&mut bas_chars, Self::BAS_BATTERY_LEVEL_UUID).await?;

        let ess = Self::find_service(&mut services, Self::ESS_UUID).await?;
        let mut ess_chars = ess.characteristics().await?;
        let ess_temperature =
            Self::find_characteristic(&mut ess_chars, Self::ESS_TEMPERATURE_UUID).await?;

        let wls = Self::find_service(&mut services, Self::WLS_UUID).await?;
        let mut wls_chars = wls.characteristics().await?;
        let wls_water_level =
            Self::find_characteristic(&mut wls_chars, Self::WLS_WATER_LEVEL_UUID).await?;
        let wls_water_distance =
            Self::find_characteristic(&mut wls_chars, Self::WLS_WATER_DISTANCE_UUID).await?;
        let wls_tank_depth =
            Self::find_characteristic(&mut wls_chars, Self::WLS_TANK_DEPTH_UUID).await?;

        let scs = Self::find_service(&mut services, Self::SCS_UUID).await?;
        let mut scs_chars = scs.characteristics().await?;
        let scs_error = Self::find_characteristic(&mut scs_chars, Self::SCS_ERROR_UUID).await?;
        let scs_status = Self::find_characteristic(&mut scs_chars, Self::SCS_STATUS_UUID).await?;
        let scs_battery_voltage =
            Self::find_characteristic(&mut scs_chars, Self::SCS_BATTERY_VOLTAGE_UUID).await?;

        self.gatt = Some(SensorGatt {
            bas_battery_level,
            ess_temperature,
            wls_water_level,
            wls_water_distance,
            wls_tank_depth,
            scs_error,
            scs_status,
            scs_battery_voltage,
        });
        Ok(())
    }

    pub async fn connect(&mut self) -> Result<(), Error> {
        self.device.connect().await?;
        self.find_gatt_attributes().await?;
        Ok(())
    }

    pub async fn disconnect(&mut self) -> Result<(), Error> {
        self.device.disconnect().await?;
        self.gatt = None;
        Ok(())
    }

    pub async fn name(&self) -> Result<String, Error> {
        self.device.name().await?.ok_or(Error::PropertyNotFound)
    }

    pub fn address(&self) -> Address {
        self.device.address()
    }

    async fn status(&self) -> Result<u32, Error> {
        let service_data = self
            .device
            .service_data()
            .await?
            .ok_or(Error::PropertyNotFound)?;
        let status_buf = service_data
            .get(&Self::SCS_UUID)
            .ok_or(Error::PropertyNotFound)?;
        Cursor::new(&status_buf)
            .read_u32::<LittleEndian>()
            .map_err(|_| Error::InvalidData(status_buf.clone()))
    }

    pub async fn wait_new_data(&mut self, adapter: &bluer::Adapter) -> Result<(), Error> {
        let mut monitor = adapter
            .register_advertisement_monitor(bluer::adv_mon::AdvertisementMonitor {
                monitor_type: bluer::adv_mon::Type::OrPatterns,
                patterns: Some(vec![bluer::adv_mon::Pattern {
                    start_position: 0,
                    ad_data_type: 0x21,
                    content_of_pattern: vec![
                        0xa1, 0xf8, 0x20, 0x0f, 0xa2, 0xc0, 0x4e, 0x72, 0x8e, 0x88, 0x61, 0x96,
                        0xcb, 0xdf, 0xef, 0x89, 0x01, 0x00, 0x00, 0x00,
                    ],
                }]),
                ..bluer::adv_mon::AdvertisementMonitor::default()
            })
            .await?;

        while let Some(event) = monitor.next().await {
            if let bluer::adv_mon::AdvertisementMonitorEvent::DeviceFound(addr) = event {
                if self.device.address() == addr {
                    return Ok(());
                }
            }
        }

        Err(Error::SensorNotFound(self.device.address()))
    }

    fn gatt(&self) -> Result<&SensorGatt, Error> {
        self.gatt.as_ref().ok_or(Error::NotConnected)
    }

    async fn read_attr<T, F>(
        &self,
        attr: &bluer::gatt::remote::Characteristic,
        parser: F,
    ) -> Result<T, Error>
    where
        F: FnOnce(Cursor<&[u8]>) -> Result<T, io::Error>,
    {
        let value = attr.read().await?;
        parser(Cursor::new(&value)).map_err(|_| Error::InvalidData(value))
    }

    pub async fn clear_new_data(&mut self) -> Result<(), Error> {
        self.gatt()?
            .scs_status
            .write(&[0x00, 0x00, 0x00, 0x00])
            .await?;
        // Ignore any spurious new data notifications that have come in since we
        // connected
        self.new_data_count = *self.new_data_rx.borrow();
        Ok(())
    }

    pub async fn errors(&self) -> Result<u32, Error> {
        self.read_attr(&self.gatt()?.scs_error, |mut v| {
            v.read_u32::<LittleEndian>()
        })
        .await
    }

    pub async fn battery_percentage(&self) -> Result<u8, Error> {
        self.read_attr(&self.gatt()?.bas_battery_level, |mut v| v.read_u8())
            .await
    }

    pub async fn battery_voltage(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.scs_battery_voltage, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .await
        .map(|l| l as f32 / 1000.0)
    }

    pub async fn temperature(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.ess_temperature, |mut v| {
            v.read_i16::<LittleEndian>()
        })
        .await
        .map(|l| l as f32 / 100.0)
    }

    pub async fn water_level(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_water_level, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .await
        .map(|l| l as f32 / 1000.0)
    }

    pub async fn water_distance(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_water_distance, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .await
        .map(|l| l as f32 / 1000.0)
    }

    pub async fn tank_depth(&self) -> Result<f32, Error> {
        self.read_attr(&self.gatt()?.wls_tank_depth, |mut v| {
            v.read_u16::<LittleEndian>()
        })
        .await
        .map(|l| l as f32 / 1000.0)
    }
}
