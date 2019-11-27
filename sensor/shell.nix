let
  pkgs = import <nixpkgs> { overlays = [ (import ../nix) ]; };
in

with pkgs;
with python3Packages;

mkShell {
  name = "water-level-monitor-env";
  disableHardening = [ "all" ];
  nativeBuildInputs = [
    which
    git
    gcc-arm-embedded
    cmake
    ninja
    dtc
    gperf
    west
    pyelftools
    click
    cryptography
    intelhex
    gcovr
    openocd
    gdb
  ];

  # NOTE: I have not tested this yet
  GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;
}

