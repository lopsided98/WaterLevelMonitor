use std::collections::HashMap;
use std::fmt;
use std::fmt::Display;
use std::hash::Hash;

use dbus::arg::RefArg;
use dbus::arg::Variant;
use failure::Fail;
use log::debug;

#[derive(Debug, Fail)]
pub struct TypedError {
    #[fail(cause)]
    pub cause: dbus::Error,
    pub kind: ErrorKind,
}

#[derive(Debug)]
pub enum ErrorKind {
    InvalidArgs,
    AccessDenied,
    NoReply,
    Custom,
}

impl Display for TypedError {
    fn fmt(&self, f: &mut fmt::Formatter) -> Result<(), fmt::Error> {
        write!(f, "D-Bus {:?} error: {}", self.kind, self.cause)
    }
}

impl From<dbus::Error> for TypedError {
    fn from(cause: dbus::Error) -> Self {
        let kind = if let Some(name) = cause.name() {
            match name {
                "org.freedesktop.DBus.Error.InvalidArgs" => ErrorKind::InvalidArgs,
                "org.freedesktop.DBus.Error.AccessDenied" => ErrorKind::AccessDenied,
                "org.freedesktop.DBus.Error.NoReply" => ErrorKind::NoReply,
                _ => ErrorKind::Custom
            }
        } else {
            ErrorKind::Custom
        };
        TypedError { cause, kind }
    }
}

pub trait RefArgCast<C: RefArgCast = Self>: Sized {
    fn ref_arg_cast(r: &RefArg) -> Result<C, dbus::Error>;

    fn ref_arg_cast_variant(r: &RefArg) -> Result<C, dbus::Error> {
        C::ref_arg_cast(r)
    }
}

fn cast_error(from: &RefArg, to: &str) -> dbus::Error {
    dbus::Error::new_custom("Type mismatch",
                            &format!("Cannot cast from {:?} to {}", &from.arg_type(), to))
}

impl RefArgCast for String {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        r.as_str()
            .ok_or_else(|| cast_error(r, "String"))
            .map(|s| s.to_owned())
    }
}

impl RefArgCast for u8 {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        r.as_u64()
            .ok_or_else(|| cast_error(r, "u8"))
            .map(|v| v as u8)
    }
}

impl RefArgCast for f64 {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        r.as_f64()
            .ok_or_else(|| cast_error(r, "f64"))
    }
}

impl<V: RefArgCast> RefArgCast for Variant<V> {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        Ok(Variant(V::ref_arg_cast_variant(r.as_iter()
            .ok_or_else(|| cast_error(r, "Variant"))?
            .next()
            .ok_or_else(|| dbus::Error::new_custom(
                "Missing Variant contents",
                "Unable to get contents of Variant"))?)?))
    }
}

impl<V: RefArgCast> RefArgCast for Vec<V> {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        r.as_iter()
            .ok_or_else(|| cast_error(r, "Vec"))
            .and_then(|i| i.map(V::ref_arg_cast_variant).collect())
    }
}

impl<K: RefArgCast + Eq + Hash, V: RefArgCast> RefArgCast for HashMap<K, V> {
    fn ref_arg_cast(r: &RefArg) -> Result<Self, dbus::Error> {
        let mut map = HashMap::<K, V>::new();
        let mut iter = r.as_iter()
            .ok_or_else(|| cast_error(r, "HashMap"))?;
        while let Some(key) = iter.next() {
            let value = iter.next()
                .ok_or_else(|| dbus::Error::new_custom(
                    "Missing dictionary value",
                    "Dictionary does not have value corresponding to key"))?;
            debug!("key: {:?} ({:?}), value: {:?} ({:?})", key, key.arg_type(), value, value.arg_type());
            map.insert(K::ref_arg_cast_variant(key)?, V::ref_arg_cast_variant(value)?);
        }
        Ok(map)
    }
}