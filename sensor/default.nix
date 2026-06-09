{
  stdenvNoCC,
  lib,
  callPackage,
  python3Packages,
  cmake,
  ninja,
  gcc-arm-embedded,
  dtc,
  buildPackages,
}:

stdenvNoCC.mkDerivation {
  pname = "water-level-sensor";
  version = "0.2.0";

  src = callPackage ./firmware/west.nix { };

  nativeBuildInputs = [
    cmake
    ninja
    gcc-arm-embedded
    dtc
  ]
  ++ (with python3Packages; [
    packaging
    pyelftools
    pykwalify
    pyyaml
    jsonschema
  ]);

  env = {
    ZEPHYR_TOOLCHAIN_VARIANT = "gnuarmemb";
    GNUARMEMB_TOOLCHAIN_PATH = buildPackages.gcc-arm-embedded;
  };

  # Default CMake flags make assumptions that don't hold when building Zephyr
  dontUseCmakeConfigure = true;

  configurePhase = ''
    runHook preConfigure

    export XDG_CACHE_HOME="$TMPDIR"
    source .zephyr-env

    cmake -S firmware -B build -G Ninja \
      -DBUILD_VERSION="$ZEPHYR_BUILD_VERSION" \
      -DEXTRA_CONF_FILE=release.conf

    runHook postConfigure
  '';

  ninjaFlags = [
    "-C"
    "build"
  ];

  installPhase = ''
    runHook preInstall

    mkdir -p "$out"
    cp build/zephyr/zephyr.{elf,bin,hex,map} "$out"

    runHook postInstall
  '';

  dontFixup = true;
}
