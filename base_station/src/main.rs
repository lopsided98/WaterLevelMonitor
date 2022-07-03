use std::fs::File;
use std::time::{Duration, SystemTime};

use anyhow::Context;
use serde::Deserialize;

use crate::influxdb::{TimestampPrecision, Value};
use crate::sensor::Sensor;

mod influxdb;
mod sensor;

const fn default_new_data_timeout() -> u32 {
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
    url: url::Url,
    database: String,
    certificate: CertificateConfig,
}

#[derive(Deserialize, Debug)]
struct Config {
    influxdb: InfluxDbConfig,
    address: bluer::Address,
    #[serde(default = "default_new_data_timeout")]
    new_data_timeout: u32,
}

async fn wait_new_data(sensor: &mut sensor::Sensor) -> anyhow::Result<SystemTime> {
    log::debug!("waiting for new data...");
    sensor.wait_new_data().await?;
    // Get timestamp as close as possible to when the data was collected
    Ok(SystemTime::now())
}

async fn read_data(
    sensor: &mut sensor::Sensor,
    timestamp: SystemTime,
) -> anyhow::Result<influxdb::Point> {
    log::debug!("connecting...");
    match sensor.connect().await {
        Err(sensor::Error::BlueZ(bluer::Error {
            kind: bluer::ErrorKind::AlreadyConnected,
            ..
        })) => log::warn!("already connected to sensor"),
        r => r?,
    };

    let mut point = influxdb::Point::new("water_tank".into());
    point.set_timestamp(influxdb::Timestamp::new(
        timestamp,
        TimestampPrecision::Second,
    ));

    log::debug!("reading battery percentage...");
    match sensor.battery_percentage().await {
        Ok(battery_percentage) => point.add_field(
            "battery_percentage".into(),
            Value::Integer(battery_percentage as i64),
        ),
        Err(e) => log::warn!("failed to read battery percentage: {}", e),
    }

    log::debug!("reading battery voltage...");
    match sensor.battery_voltage().await {
        Ok(battery_voltage) => point.add_field(
            "battery_voltage".into(),
            Value::Float(battery_voltage as f64),
        ),
        Err(e) => log::warn!("failed to read battery voltage: {}", e),
    }

    log::debug!("reading temperature...");
    match sensor.temperature().await {
        Ok(temperature) => point.add_field("temperature".into(), Value::Float(temperature as f64)),
        Err(e) => log::warn!("failed to read temperature: {}", e),
    }

    log::debug!("reading water level...");
    match sensor.water_level().await {
        Ok(water_level) => point.add_field("water_level".into(), Value::Float(water_level as f64)),
        Err(e) => log::warn!("failed to read water level: {}", e),
    }

    log::debug!("reading water distance...");
    match sensor.water_distance().await {
        Ok(water_distance) => {
            point.add_field("water_distance".into(), Value::Float(water_distance as f64))
        }
        Err(e) => log::warn!("failed to read water distance: {}", e),
    }

    log::debug!("reading tank depth...");
    match sensor.tank_depth().await {
        Ok(tank_depth) => point.add_field("tank_depth".into(), Value::Float(tank_depth as f64)),
        Err(e) => log::warn!("failed to read tank depth: {}", e),
    }

    log::debug!("reading errors...");
    match sensor.errors().await {
        Ok(errors) => point.add_field("errors".into(), Value::Integer(errors as i64)),
        Err(e) => log::warn!("failed to read errors: {}", e),
    }

    log::debug!("clearing status...");
    if let Err(e) = sensor.clear_new_data().await {
        log::warn!("failed to clear new data status: {}", e);
    }

    log::debug!("disconnecting...");
    sensor.disconnect().await?;

    Ok(point)
}

async fn collect_data(
    sensor: &mut sensor::Sensor,
    new_data_timeout: Duration,
) -> anyhow::Result<influxdb::Point> {
    let timestamp = match tokio::time::timeout(new_data_timeout, wait_new_data(sensor)).await {
        Ok(t) => t,
        Err(_) => {
            log::warn!("timed out waiting for new data");
            Ok(SystemTime::now())
        }
    }?;
    read_data(sensor, timestamp).await
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> anyhow::Result<()> {
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

    let influxdb_cert = isahc::config::ClientCertificate::pkcs12_file(
        config.influxdb.certificate.file,
        Some(config.influxdb.certificate.password),
    );

    let influxdb =
        influxdb::Client::new(config.influxdb.url, config.influxdb.database, influxdb_cert)?;

    let session = bluer::Session::new().await?;
    let adapter = session
        .default_adapter()
        .await
        .context("failed to get Bluetooth adapter")?;

    let mut sensor = Sensor::find_by_address(&adapter, config.address).await?;
    log::info!(
        "using device: {} ({})",
        sensor.name().await?,
        sensor.address()
    );

    loop {
        match collect_data(
            &mut sensor,
            Duration::from_millis(config.new_data_timeout as u64),
        )
        .await
        {
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
