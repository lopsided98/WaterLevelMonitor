{
  mkShell,
  which,
  git,
  gcc-arm-embedded,
  cmake,
  ninja,
  dtc,
  openocd,
  nix-prefetch-git,
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
    openocd
    nix-prefetch-git
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
