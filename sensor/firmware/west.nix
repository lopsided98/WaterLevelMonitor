
{ lib, runCommand, lndir, fetchgit, fetchurl }:

runCommand "west-workspace" {
nativeBuildInputs = [ lndir ];
} ''

    mkdir -p "$out"/'firmware'
    lndir -silent \
        ${lib.escapeShellArg ("${../firmware}")} \
        "$out"/'firmware'

    mkdir -p "$out"/'zephyr'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/lopsided98/zephyr";
            rev = "487d489b6f0abbbab754a559627222def971b493";
            branchName = "manifest-rev";
            hash = "sha256-0RX6jsXcF0dw3PVdLFsED3cHLIz03ceWrxdUUdjL1CY=";
        })} \
        "$out"/'zephyr'

    mkdir -p "$out"/'tools/west-nix'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/lopsided98/west-nix";
            rev = "3f31c82804e217842dc40e21ab8f95a037dfc8f7";
            branchName = "manifest-rev";
            hash = "sha256-l30Ib09LT0+kFhq6DYqIWlLznRP0ZMxgzEzU6L2d1nQ=";
        })} \
        "$out"/'tools/west-nix'

    mkdir -p "$out"/'modules/hal/cmsis_6'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/zephyrproject-rtos/CMSIS_6";
            rev = "30a859f44ef8ab4dc8f84b03ed586fd16ccf9d74";
            branchName = "manifest-rev";
            hash = "sha256-nTehISN0pu9gnOZMpGaBQ3DFmNxAqAZPGpvbKfEM35o=";
        })} \
        "$out"/'modules/hal/cmsis_6'

    mkdir -p "$out"/'modules/hal/nordic'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/zephyrproject-rtos/hal_nordic";
            rev = "44fd3d44b15cb75f80a25b4679f91d2787e28664";
            branchName = "manifest-rev";
            hash = "sha256-vv66uJMmJdTqJtUWG9PrCuvXf23tcvwy+WpYI+OiKTE=";
        })} \
        "$out"/'modules/hal/nordic'

    mkdir -p "$out"/'modules/crypto/mbedtls'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/zephyrproject-rtos/mbedtls";
            rev = "a3e190fe44c78d1ba67f55979e1257328cc7d0d8";
            branchName = "manifest-rev";
            hash = "sha256-TwRunAuhQcBefPRHuUUEMyi9ux8kQ9wa3umUZpz1wXY=";
        })} \
        "$out"/'modules/crypto/mbedtls'

    mkdir -p "$out"/'modules/crypto/tf-psa-crypto'
    lndir -silent \
        ${lib.escapeShellArg (
        fetchgit {
            url = "https://github.com/zephyrproject-rtos/tf-psa-crypto";
            rev = "dc575a2ddcc8cb16275d24c42a52eaf79ebe2231";
            branchName = "manifest-rev";
            hash = "sha256-zA/2oIU7/rP1Ho254o2zZqwd3WgLz3N9GLmhJpX5TEY=";
        })} \
        "$out"/'modules/crypto/tf-psa-crypto'

    mkdir -p "$out"/'modules/hal/nordic/zephyr/blobs/suit/bin'
    ln -s \
        ${lib.escapeShellArg (
        fetchurl {
            url = "https://github.com/nrfconnect/sdk-nrfxlib/raw/938b057dcefe9ebeb1554f4c448cee5ecdf965ae/suit/bin/suit_manifest_starter.hex";
            hash = "sha256:7671d284ae8a7b3d7222f6869d5efd8b1efb1633ad752104ee9317476bf3566a";
        })} \
        "$out"/'modules/hal/nordic/zephyr/blobs/suit/bin/suit_manifest_starter.hex'

    cat << EOF > "$out/.zephyr-env"
    export ZEPHYR_BASE=${lib.escapeShellArg "${placeholder "out"}/zephyr"}
    export ZEPHYR_MODULES=${lib.escapeShellArg "${placeholder "out"}/firmware;${placeholder "out"}/modules/hal/cmsis_6;${placeholder "out"}/modules/hal/nordic;${placeholder "out"}/modules/crypto/mbedtls;${placeholder "out"}/modules/crypto/tf-psa-crypto"}
    EOF
''
