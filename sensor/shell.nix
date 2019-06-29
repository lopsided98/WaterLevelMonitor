let
  pkgs = import <nixpkgs> { overlays = [ (import ../nix) ]; };
in

with pkgs.pkgsCross.arm-embedded.buildPackages;
with python3Packages;

let
  toolchain = buildEnv {
    name = "arm-toolchain";
    paths = [ gcc binutils ];
  };
in mkShell {
  name = "water-level-monitor-env";
  nativeBuildInputs = [
    which
    git
    # gcc
    # binutils
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

  # Have to use Arch's GCC for now because Nix's causes crashes
  GNUARMEMB_TOOLCHAIN_PATH = "/usr"; #toolchain;
}

