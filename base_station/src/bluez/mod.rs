use failure::Fail;

// Import generated code
pub use gen::*;
use std::fmt;
use std::fmt::Display;

mod gen;

#[derive(Debug, Fail)]
pub struct Error {
    #[fail(cause)]
    pub cause: dbus::Error,
    pub kind: ErrorKind,
}

#[derive(Debug)]
pub enum ErrorKind {
    InvalidArguments,
    InProgress,
    AlreadyExists,
    NotSupported,
    NotConnected,
    AlreadyConnected,
    NotAvailable,
    DoesNotExist,
    NotAuthorized,
    NotPermitted,
    NoSuchAdapter,
    AgentNotAvailable,
    NotReady,
    Failed,
    InvalidValueLength,
    InvalidOffset,
    Rejected,
    Canceled,
    AuthenticationCanceled,
    AuthenticationFailed,
    AuthenticationRejected,
    AuthenticationTimeout,
    ConnectionAttemptFailed,
    OutOfRange,
    HealthError,
    NotAcquired,
    Unknown,
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
                "org.bluez.Error.InvalidArguments" => ErrorKind::InvalidArguments,
                "org.bluez.Error.InProgress" => ErrorKind::InProgress,
                "org.bluez.Error.AlreadyExists" => ErrorKind::AlreadyExists,
                "org.bluez.Error.NotSupported" => ErrorKind::NotSupported,
                "org.bluez.Error.NotConnected" => ErrorKind::NotConnected,
                "org.bluez.Error.AlreadyConnected" => ErrorKind::AlreadyConnected,
                "org.bluez.Error.NotAvailable" => ErrorKind::NotAvailable,
                "org.bluez.Error.DoesNotExist" => ErrorKind::DoesNotExist,
                "org.bluez.Error.NotAuthorized" => ErrorKind::NotAuthorized,
                "org.bluez.Error.NotPermitted" => ErrorKind::NotPermitted,
                "org.bluez.Error.NoSuchAdapter" => ErrorKind::NoSuchAdapter,
                "org.bluez.Error.AgentNotAvailable" => ErrorKind::AgentNotAvailable,
                "org.bluez.Error.NotReady" => ErrorKind::NotReady,
                "org.bluez.Error.Failed" => ErrorKind::Failed,
                "org.bluez.Error.InvalidValueLength" => ErrorKind::InvalidValueLength,
                "org.bluez.Error.InvalidOffset" => ErrorKind::InvalidOffset,
                "org.bluez.Error.Rejected" => ErrorKind::Rejected,
                "org.bluez.Error.Canceled" => ErrorKind::Canceled,
                "org.bluez.Error.AuthenticationCanceled" => ErrorKind::AuthenticationCanceled,
                "org.bluez.Error.AuthenticationFailed" => ErrorKind::AuthenticationFailed,
                "org.bluez.Error.AuthenticationRejected" => ErrorKind::AuthenticationRejected,
                "org.bluez.Error.AuthenticationTimeout" => ErrorKind::AuthenticationTimeout,
                "org.bluez.Error.ConnectionAttemptFailed" => ErrorKind::ConnectionAttemptFailed,
                "org.bluez.Error.OutOfRange" => ErrorKind::OutOfRange,
                "org.bluez.Error.HealthError" => ErrorKind::HealthError,
                "org.bluez.Error.NotAcquired" => ErrorKind::NotAcquired,
                _ => ErrorKind::Unknown
            }
        } else {
            ErrorKind::Unknown
        };
        Error { cause, kind }
    }
}