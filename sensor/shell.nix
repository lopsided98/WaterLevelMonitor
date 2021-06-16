let
  pkgs = import <nixpkgs> { };
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

  GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;
}

