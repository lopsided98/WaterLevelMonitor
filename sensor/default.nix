{
  stdenvNoCC,
  lib,
  callPackage,
  python3Packages,
  cmake,
  gcc-arm-embedded,
  dtc,
}:

stdenvNoCC.mkDerivation {
  pname = "water-level-sensor";
  version = "0.2.0";

  src = callPackage ./firmware/west.nix { };

  nativeBuildInputs = [
    cmake
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

  env =
    let
      tool = name: "${lib.getBin gcc-arm-embedded}/bin/arm-none-eabi-${name}";
    in
    {
      ZEPHYR_TOOLCHAIN_VARIANT = "gnuarmemb";
      GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;
      STRIP = tool "strip";
      RANLIB = tool "ranlib";
      AR = tool "ar";
      CC = tool "gcc";
      CXX = tool "g++";
    };

  cmakeDir = "../firmware";

  preConfigure = ''
    export XDG_CACHE_HOME="$TMPDIR"
    source .zephyr-env
    cmakeFlagsArray+=("-DBUILD_VERSION=$ZEPHYR_BUILD_VERSION")
  '';

  installPhase = ''
    runHook preInstall

    mkdir -p "$out"
    cp zephyr/zephyr.{elf,bin,hex,map} "$out"

    runHook postInstall
  '';

  dontFixup = true;
}
