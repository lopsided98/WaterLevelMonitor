use std::fs::File;
use std::io::Read;
use std::rc::Rc;
use std::time::{Duration, SystemTime};

use failure::ResultExt;
use serde::Deserialize;

use crate::influxdb::{TimestampPrecision, Value};
use crate::sensor::Sensor;

mod influxdb;
mod sensor;
mod util;

const DEFAULT_TIMEOUT: Duration = Duration::from_secs(10);

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

fn wait_new_data(
    sensor: &mut sensor::Sensor,
    timeout: Duration,
) -> Result<SystemTime, failure::Error> {
    log::debug!("waiting for new data...");
    if !sensor.wait_new_data(timeout)? {
        log::warn!("timed out waiting for new data, collecting anyway");
    }
    // Get timestamp as close as possible to when the data was collected
    let timestamp = SystemTime::now();
    Ok(timestamp)
}

fn read_data(
    sensor: &mut sensor::Sensor,
    timestamp: SystemTime,
) -> Result<influxdb::Point, failure::Error> {
    log::debug!("connecting...");
    match sensor.connect(Duration::from_secs(10)) {
        Err(sensor::Error::BlueZ(blurst::Error::Bluez {
            kind: blurst::ErrorKind::AlreadyConnected,
            ..
        })) => log::warn!("already connected to sensor"),
        Err(e) => Err(e)?,
        Ok(_) => (),
    };

    let mut point = influxdb::Point::new("water_tank".into());
    point.set_timestamp(influxdb::Timestamp::new(
        timestamp,
        TimestampPrecision::Second,
    ));

    log::debug!("reading battery percentage...");
    match sensor.battery_percentage() {
        Ok(battery_percentage) => point.add_field(
            "battery_percentage".into(),
            Value::Integer(battery_percentage as i64),
        ),
        Err(e) => log::warn!("failed to read battery level: {}", e),
    }

    log::debug!("reading temperature...");
    match sensor.temperature() {
        Ok(temperature) => point.add_field("temperature".into(), Value::Float(temperature as f64)),
        Err(e) => log::warn!("failed to read temperature: {}", e),
    }

    log::debug!("reading water level...");
    match sensor.water_level() {
        Ok(water_level) => point.add_field("water_level".into(), Value::Float(water_level as f64)),
        Err(e) => log::warn!("failed to read water level: {}", e),
    }

    log::debug!("reading water distance...");
    match sensor.water_distance() {
        Ok(water_distance) => {
            point.add_field("water_distance".into(), Value::Float(water_distance as f64))
        }
        Err(e) => log::warn!("failed to read water distance: {}", e),
    }

    log::debug!("reading tank depth...");
    match sensor.tank_depth() {
        Ok(tank_depth) => point.add_field("tank_depth".into(), Value::Float(tank_depth as f64)),
        Err(e) => log::warn!("failed to read tank depth: {}", e),
    }

    log::debug!("clearing status...");
    if let Err(e) = sensor.set_status(0) {
        log::warn!("failed to clear new data status: {}", e);
    } else {
        log::debug!("waiting for status to clear...");
        // Once we disconnect, the sensor will stop advertising until it collects new
        // data. Therefore, we need to wait for the service data to update before
        // disconnecting, so we aren't stuck with stale data that makes us think
        // there is still new data to retrieve.
        if !sensor.wait_new_data_cleared(Duration::from_secs(30))? {
            log::warn!("timeout waiting for cleaned status");
        }
    }

    Ok(point)
}

fn cleanup(sensor: &mut sensor::Sensor) -> Result<(), failure::Error> {
    log::debug!("disconnecting...");
    sensor.disconnect()?;
    Ok(())
}

fn collect_data(
    sensor: &mut sensor::Sensor,
    timeout: Duration,
) -> Result<influxdb::Point, failure::Error> {
    let timestamp = wait_new_data(sensor, timeout)?;
    let result = read_data(sensor, timestamp);
    cleanup(sensor)?;
    result
}

pub fn main() -> Result<(), failure::Error> {
    env_logger::init();

    let matches = clap::App::new("water_level_monitor")
        .version(clap::crate_version!())
        .author("Ben Wolsieffer <benwolsieffer@gmail.com>")
        .about("Base station software to communicate with the water level sensor")
        .arg(
            clap::Arg::with_name("config")
                .help("Config file")
                .required(true)
                .takes_value(true)
                .value_name("CONFIG"),
        )
        .get_matches();

    let config_file = File::open(
        matches
            .value_of("config")
            // Clap should guarantee that the argument exists
            .unwrap(),
    )
    .context("could not open config file")?;
    let config: Config =
        serde_yaml::from_reader(config_file).context("could not parse config file")?;

    let mut influxdb_cert = Vec::new();
    File::open(config.influxdb.certificate.file)?.read_to_end(&mut influxdb_cert)?;

    let influxdb = influxdb::Client::new(
        config.influxdb.url,
        config.influxdb.database,
        reqwest::Identity::from_pkcs12_der(&influxdb_cert, &config.influxdb.certificate.password)?,
    )?;

    let bluez = Rc::new(blurst::Bluez::new(DEFAULT_TIMEOUT)?);
    let adapter = bluez
        .get_first_adapter(DEFAULT_TIMEOUT, DEFAULT_TIMEOUT)?
        .ok_or_else(|| failure::err_msg("no Bluetooth adapter found"))?;

    let mut sensor = Sensor::find_by_address(&adapter, &config.address, DEFAULT_TIMEOUT)?;
    log::info!("using device: {} ({})", sensor.name()?, sensor.address()?);

    adapter.start_discovery()?;

    loop {
        match collect_data(
            &mut sensor,
            Duration::from_millis(config.new_data_timeout as u64),
        ) {
            Ok(point) => {
                log::debug!("writing point: {}", point);
                if let Err(e) = influxdb.write_point(&point) {
                    log::error!("failed to write data to InfluxDB: {}", e);
                }
            }
            Err(e) => log::error!("failed to collect data: {}", e),
        };
    }
}
