use core::fmt::Write;
use std::collections::HashMap;
use std::fmt;
use std::fmt::Display;
use std::fmt::Formatter;
use std::time::{SystemTime, UNIX_EPOCH};

use failure::Fail;
use itertools::Itertools;
use log::debug;

#[derive(Debug, Fail)]
pub enum Error {
    #[fail(display = "HTTP client error: {}", 0)]
    Network(#[fail(cause)] reqwest::Error),
    #[fail(display = "URL error: {}", 0)]
    Url(url::ParseError),
}

impl From<reqwest::Error> for Error {
    fn from(e: reqwest::Error) -> Self {
        Error::Network(e)
    }
}

pub enum Value {
    Float(f64),
    Integer(i64),
    String(String),
    Boolean(bool),
}

impl Display for Value {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        match self {
            Value::Float(v) => v.fmt(f),
            Value::Integer(v) => write!(f, "{}i", v),
            Value::String(v) => write!(f, "\"{}\"", v),
            Value::Boolean(v) => v.fmt(f),
        }
    }
}

pub enum TimestampPrecision {
    NanoSecond,
    MicroSecond,
    MilliSecond,
    Second,
    Minute,
    Hour,
}

impl Display for TimestampPrecision {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        f.write_str(match self {
            TimestampPrecision::NanoSecond => "ns",
            TimestampPrecision::MicroSecond => "u",
            TimestampPrecision::MilliSecond => "ms",
            TimestampPrecision::Second => "s",
            TimestampPrecision::Minute => "m",
            TimestampPrecision::Hour => "h",
        })
    }
}

pub struct Timestamp {
    pub timestamp: SystemTime,
    pub precision: TimestampPrecision,
}

impl Timestamp {
    pub fn new(timestamp: SystemTime, precision: TimestampPrecision) -> Self {
        Self { timestamp, precision }
    }
}

impl Display for Timestamp {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        let epoch_duration = self.timestamp.duration_since(UNIX_EPOCH)
            .map_err(|_| fmt::Error)?;
        match self.precision {
            TimestampPrecision::NanoSecond => epoch_duration.as_nanos() as u64,
            TimestampPrecision::MicroSecond => epoch_duration.as_micros() as u64,
            TimestampPrecision::MilliSecond => epoch_duration.as_millis() as u64,
            TimestampPrecision::Second => epoch_duration.as_secs(),
            TimestampPrecision::Minute => epoch_duration.as_secs() / 60,
            TimestampPrecision::Hour => epoch_duration.as_secs() / 3600,
        }.fmt(f)
    }
}

pub struct Point {
    measurement: String,
    tags: HashMap<String, String>,
    fields: HashMap<String, String>,
    timestamp: Option<Timestamp>,
}

impl Point {
    pub fn new(measurement: String) -> Self {
        Point {
            measurement,
            tags: HashMap::new(),
            fields: HashMap::new(),
            timestamp: None,
        }
    }

    pub fn add_tag(&mut self, key: String, value: String) {
        self.tags.insert(key, value);
    }

    pub fn add_field(&mut self, key: String, value: Value) {
        self.fields.insert(key, value.to_string());
    }

    pub fn set_timestamp(&mut self, timestamp: Timestamp) {
        self.timestamp = Some(timestamp);
    }
}

impl Display for Point {
    fn fmt(&self, f: &mut Formatter) -> Result<(), fmt::Error> {
        f.write_str(&self.measurement)?;
        if !self.tags.is_empty() {
            f.write_char(',')?;
            self.tags.iter()
                .format_with(",", |(key, value), f| f(&format_args!("{}={}", key, value)))
                .fmt(f)?;
        }
        if !self.fields.is_empty() {
            f.write_char(' ')?;
            self.fields.iter()
                .format_with(",", |(key, value), f| f(&format_args!("{}={}", key, value)))
                .fmt(f)?;
        }
        if let Some(timestamp) = &self.timestamp {
            f.write_char(' ')?;
            timestamp.fmt(f)?;
        }
        Ok(())
    }
}

pub struct Client {
    base_url: url::Url,
    database: String,
    client: reqwest::Client,
}

impl Client {
    pub fn new(base_url: url::Url,
               database: String,
               identity: reqwest::Identity) -> Result<Self, Error> {
        let client = reqwest::ClientBuilder::new()
            .identity(identity)
            .build()?;
        Ok(Client { base_url, database, client })
    }

    pub fn write_point(&self, point: &Point) -> Result<(), Error> {
        debug!("Writing point: {}", point);
        let mut url = self.base_url.join("write")
            .map_err(Error::Url)?;
        let mut query = format!("db={}", self.database);
        if let Some(timestamp) = &point.timestamp {
            query.push_str(&format!("&precision={}", timestamp.precision));
        }
        url.set_query(Some(&query));
        self.client.execute(self.client.post(url)
            .body(point.to_string())
            .build()?)?;
        Ok(())
    }
}

