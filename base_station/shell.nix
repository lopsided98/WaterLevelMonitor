{ pkgs ? import <nixpkgs> { } }: with pkgs;

stdenv.mkDerivation {
  name = "base-station-env";

  nativeBuildInputs = [
    rustc
    cargo
    pkgconfig
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

  RUST_TOOLCHAIN = buildEnv {
    name = "rust-toolchain";
    paths = with buildPackages; [ rustc cargo ];
  } + "/bin";
  RUST_SRC_PATH = rustPlatform.rustLibSrc;
}

