use std::fs::File;
use std::io::Read;
use std::time::SystemTime;

use failure::ResultExt;
use log::{debug, info, warn};
use serde::Deserialize;

use crate::influxdb::{TimestampPrecision, Value};

mod bluez;
mod sensor;
mod influxdb;
mod dbus;

fn default_new_data_timeout() -> u32 {
    16 * 60 * 1000
}

#[derive(Deserialize, Debug)]
struct CertificateConfig {
    file: String,
    #[serde(default)]
    password: String,
}

#[derive(Deserialize, Debug)]
struct InfluxDbConfig {
    #[serde(with = "url_serde")]
    url: url::Url,
    database: String,
    certificate: CertificateConfig,
}

#[derive(Deserialize, Debug)]
struct Config {
    influxdb: InfluxDbConfig,
    address: String,
    #[serde(default = "default_new_data_timeout")]
    new_data_timeout: u32,
}

pub fn main() -> Result<(), failure::Error> {
    env_logger::init();

    let matches = clap::App::new("water_level_monitor")
        .version(clap::crate_version!())
        .author("Ben Wolsieffer <benwolsieffer@gmail.com>")
        .about("Base station software to communicate with the water level sensor")
        .arg(clap::Arg::with_name("config")
            .help("Config file")
            .required(true)
            .takes_value(true)
            .value_name("CONFIG"))
        .get_matches();

    let config_file = File::open(matches
        .value_of("config")
        // Clap should guarantee that the argument exists
        .unwrap())
        .context("Could not open config file")?;
    let config: Config = serde_yaml::from_reader(config_file)
        .context("Could not parse config file")?;

    let mut influxdb_cert = Vec::new();
    File::open(config.influxdb.certificate.file)?
        .read_to_end(&mut influxdb_cert)?;

    let influxdb = influxdb::Client::new(
        config.influxdb.url,
        config.influxdb.database,
        reqwest::Identity::from_pkcs12_der(
            &influxdb_cert,
            &config.influxdb.certificate.password)?,
    )?;

    let mut mgr = sensor::SensorManager::new()?;

    let mut sensor = mgr.get_sensor_by_address(&config.address, Some(10000))?;
    info!("Using device: {} ({})", sensor.name()?, sensor.address()?);

    let mut first = true;

    loop {
        // FIXME: hack for debugging
        if !first {
            if !sensor.wait_new_data(Some(config.new_data_timeout))? {
                warn!("Timed out waiting for new data, collecting anyway");
            }
        }
        first = false;

        // Get timestamp as close as possible to when the data was collected
        let timestamp = SystemTime::now();

        sensor.connect()?;

        let mut point = influxdb::Point::new("water_tank".into());

        match sensor.battery_level() {
            Ok(battery_level) =>
                point.add_field("battery_level".into(), Value::Integer(battery_level as i64)),
            Err(e) => warn!("Failed to read battery level: {}", e)
        }
        match sensor.temperature() {
            Ok(temperature) =>
                point.add_field("temperature".into(), Value::Float(temperature as f64)),
            Err(e) => warn!("Failed to read temperature: {}", e)
        }
        match sensor.water_level() {
            Ok(water_level) =>
                point.add_field("water_level".into(), Value::Float(water_level as f64)),
            Err(e) => warn!("Failed to read water level: {}", e)
        }
        match sensor.water_distance() {
            Ok(water_distance) =>
                point.add_field("water_distance".into(), Value::Float(water_distance as f64)),
            Err(e) => warn!("Failed to read water distance: {}", e)
        }
        match sensor.tank_depth() {
            Ok(tank_depth) =>
                point.add_field("tank_depth".into(), Value::Float(tank_depth as f64)),
            Err(e) => warn!("Failed to read tank depth: {}", e)
        }

        sensor.set_status(0)?;
        sensor.disconnect()?;

        point.set_timestamp(influxdb::Timestamp::new(timestamp, TimestampPrecision::Second));
        influxdb.write_point(&point)?;
    }
}