{ crossSystem ? null }:

with import <nixpkgs> { inherit crossSystem; };

stdenv.mkDerivation {
  name = "base-station-env";
  nativeBuildInputs = with buildPackages; [
    rustc
    cargo
    pkgconfig
  ];
  buildInputs = [
    dbus
    openssl
  ];

  # Doesn't work in debug builds
  hardeningDisable = [ "fortify" ];

  # Set Environment Variables
  RUST_BACKTRACE = 1;
  
  RUST_TOOLCHAIN = buildEnv {
    name = "rust-toolchain";
    paths = with buildPackages; [ rustc cargo ];
  } + "/bin";
  RUST_SRC_PATH = rustPlatform.rustcSrc;

  PKG_CONFIG_ALLOW_CROSS = 1;

  CARGO_TARGET_ARM_UNKNOWN_LINUX_GNUEABIHF_LINKER = "armv6l-unknown-linux-gnueabihf-cc";
}

