{
  mkShell,
  which,
  git,
  gcc-arm-embedded,
  cmake,
  ninja,
  dtc,
  gperf,
  gcovr,
  openocd,
  gdb,
  python3Packages,
}:

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
    gcovr
    openocd
    gdb
  ]
  ++ (with python3Packages; [
    west
    packaging
    pyelftools
    pykwalify
    pyyaml
    jsonschema
  ]);

  GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;
}
