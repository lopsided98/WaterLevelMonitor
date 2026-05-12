{
  mkShell,
  rustc,
  cargo,
  pkg-config,
  rustfmt,
  rust-analyzer,
  clippy,
  crate2nix,
  dbus,
  openssl,
  rustPlatform,
}:

mkShell {
  name = "base-station-env";

  nativeBuildInputs = [
    rustc
    cargo
    pkg-config
    # Stable mode is basically useless
    (rustfmt.override { asNightly = true; })
    rust-analyzer
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
