use failure::Fail;

// Import generated code
pub use gen::*;
use std::fmt;
use std::fmt::Display;

mod gen;

#[derive(Debug, Fail)]
pub struct Error {
    #[fail(cause)]
    cause: dbus::Error,
    kind: ErrorKind,
}

#[derive(Debug)]
pub enum ErrorKind {
    Failed,
    Unknown
}


impl Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "BlueZ error: {:?}: {}", self.kind, self.cause)
    }
}

impl From<dbus::Error> for Error {
    fn from(cause: dbus::Error) -> Self {
        let kind = if let Some(name) = cause.name() {
            match name {
                "org.bluez.Failed" => ErrorKind::Failed,
                _ => ErrorKind::Unknown
            }
        } else {
            ErrorKind::Unknown
        };
        Error { cause, kind }
    }
}