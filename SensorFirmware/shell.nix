let
  pkgs = import <nixpkgs> { overlays = [ (import ../nix) ]; };
in

with pkgs.pkgsCross.arm-embedded.buildPackages;

let
  toolchain = buildEnv {
    name = "arm-toolchain";
    paths = [ gcc binutils ];
  };
in mkShell {
  name = "water-level-monitor-env";
  nativeBuildInputs = with python3Packages; [
    which
    git
    # gcc
    # binutils
    cmake
    dtc
    gperf
    west
    pyelftools
    gcovr
    openocd
    gdb
  ];

  # Have to use Arch's GCC for now because Nix's causes crashes
  GNUARMEMB_TOOLCHAIN_PATH = "/usr"; #toolchain;
}

