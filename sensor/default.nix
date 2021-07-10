{ stdenv, callPackage, python3Packages, git, cmake, ninja, gcc-arm-embedded }:

stdenv.mkDerivation {
  pname = "water-level-sensor";
  version = "0.1.0";
  
  src = callPackage ./firmware/west.nix { };

  nativeBuildInputs = [
    git
    cmake
    ninja
    gcc-arm-embedded
  ] ++ (with python3Packages; [
    west
    pyelftools
  ]);

  GNUARMEMB_TOOLCHAIN_PATH = gcc-arm-embedded;

  dontConfigure = true;

  buildPhase = ''
    runHook preBuild

    west build -b nrf51_ble400 firmware -- -DZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb -DUSER_CACHE_DIR="$(pwd)/.cache"
    cat .west/config

    runHook postBuild
  '';
  
  installPhase = ''
    runHook preInstall

    mkdir -p "$out"
    cp build/zephyr/zephyr.elf "$out"

    runHook postInstall
  '';
  
  dontFixup = true;
}
