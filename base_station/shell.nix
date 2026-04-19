{
  mkShell,
  rustc,
  cargo,
  pkg-config,
  rustfmt,
  clippy,
  crate2nix,
  dbus,
  openssl,
}:

mkShell {
  name = "base-station-env";

  nativeBuildInputs = [
    rustc
    cargo
    pkg-config
    # Stable mode is basically useless
    (rustfmt.override { asNightly = true; })
    clippy
    crate2nix
  ];

  buildInputs = [
    dbus
    openssl
  ];

  # Doesn't work in debug builds
  hardeningDisable = [ "fortify" ];

  RUST_BACKTRACE = 1;
}
