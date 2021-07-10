{ lib, linkFarm, fetchgit, writeText }: (linkFarm "west-workspace" [

{
    name = "firmware";
    path = "${../firmware}";
}

{
    name = "zephyr";
    path = fetchgit {
        url = "https://github.com/lopsided98/zephyr";
        rev = "d0d6731089b2b10dd0e9c5b6bdc42e4ccf2050b8";
        branchName = "manifest-rev";
        sha256 = "0r1an9x71yf08vb1qxqv0wp56dj3s05kcqr8i8c5rz0ccfr117gm";
        leaveDotGit = true;
    };
}

{
    name = "west-nix";
    path = fetchgit {
        url = "https://github.com/lopsided98/west-nix";
        rev = "3bd2db86413da592a1bf7a5e1f7a25245ceedefe";
        branchName = "manifest-rev";
        sha256 = "1wmkynmzjzy6vv77s1kq23ln288dhhcfhs66hgnxhllxxlp3scms";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/cmsis";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/cmsis";
        rev = "c3bd2094f92d574377f7af2aec147ae181aa5f8e";
        branchName = "manifest-rev";
        sha256 = "0f0iqnyiv707w0mx71hilsnpxslivl1zwz230npkyp79nj8wnaaf";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/atmel";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_atmel";
        rev = "d17b7dd92d209b20bc1e15431d068edc29bf438d";
        branchName = "manifest-rev";
        sha256 = "0saq88l7zhnnfx1m1nggm4i2dspfisil1qxck6xfi96c5n8r0yj6";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/altera";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_altera";
        rev = "23c1c1dd7a0c1cc9a399509d1819375847c95b97";
        branchName = "manifest-rev";
        sha256 = "1xsy2fblnzpsrl6qkbqz1k01c1p8244al6sabh3bz8sxh4h05iwc";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/canopennode";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/canopennode";
        rev = "468d350028a975b96563e58344de48281a0ab371";
        branchName = "manifest-rev";
        sha256 = "1c184vqv4n5xd9kn1rvq44j4qc3dk2r4xdxrg07a1w73hw88isdm";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/civetweb";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/civetweb";
        rev = "e6903b80c09d17cd1a8bb32e40080005558aad29";
        branchName = "manifest-rev";
        sha256 = "05a392j4ssjk8d92vlrk424zbzvi23p8lxqzn5330x01cnvd7az5";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/espressif";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_espressif";
        rev = "22e757632677e3579e6f20bb9955fffb2e1b3e1c";
        branchName = "manifest-rev";
        sha256 = "0i6wkj6spjcs9bsgnabd0cqydj8mzrvb2i4dgw1flzbb7a4kzy5y";
        leaveDotGit = true;
    };
}

{
    name = "modules/fs/fatfs";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/fatfs";
        rev = "1d1fcc725aa1cb3c32f366e0c53d7490d0fe1109";
        branchName = "manifest-rev";
        sha256 = "1sg5j46n8zmawqj9qsw7c7yv3jf2c7ssxgy7gdcfc7lq5bjq6r7p";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/cypress";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_cypress";
        rev = "81a059f21435bc7e315bccd720da5a9b615bbb50";
        branchName = "manifest-rev";
        sha256 = "1id06f7l8b11jpq9xy645m4s3ancn59fw4jcza46frz9v8l7wmfx";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/infineon";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_infineon";
        rev = "f1fa8241f8786198ba41155413243de36ed878a5";
        branchName = "manifest-rev";
        sha256 = "1wd5k6qy14ga19k8nfpd4awk39bl096a79smfl0kyp9d6y4rn7mv";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/nordic";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_nordic";
        rev = "574493fe29c79140df4827ab5d4a23df79d03681";
        branchName = "manifest-rev";
        sha256 = "17hm6pcyfp0yg8m4rvrn462fwcpchip00h2y0a0hj6ipf7f1b735";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/openisa";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_openisa";
        rev = "40d049f69c50b58ea20473bee14cf93f518bf262";
        branchName = "manifest-rev";
        sha256 = "1dny9dkxsa2md8480v9rg5qj1x61dq7y5hrajagyplq7kczp4b12";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/nuvoton";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_nuvoton";
        rev = "b4d31f33238713a568e23618845702fadd67386f";
        branchName = "manifest-rev";
        sha256 = "1kf9b33lifn7krm55iand55n8ai33lj37z32bw7xjvwnm1jlr99s";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/microchip";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_microchip";
        rev = "b280eec5d3b1296b231117c1999bcd0269b6ecc4";
        branchName = "manifest-rev";
        sha256 = "1z36l0zy2vw3ca84h1zqwfd7y34myp01glwmdnsgvrmzby2fz30s";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/silabs";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_silabs";
        rev = "be39d4eebeddac6e18e9c0c3ba1b31ad1e82eaed";
        branchName = "manifest-rev";
        sha256 = "1jw8llqvfgzhyf41d7ay5ylc54k0d2pszjq7vkh52bz4dl7lyrfn";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/st";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_st";
        rev = "b52fdbf4b62439be9fab9bb4bae9690a42d2fb14";
        branchName = "manifest-rev";
        sha256 = "1m0p6ac2ghf7fl0p886ianv4nz71c0dyipf4cva1fw1ljw6wafsi";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/stm32";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_stm32";
        rev = "f8ff8d25aa0a9e65948040c7b47ec67f3fa300df";
        branchName = "manifest-rev";
        sha256 = "0ff4pxw4la5i1rirciwskghi645qsijnpsxvm0vqih7pqlyyk0a3";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/ti";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_ti";
        rev = "3da6fae25fc44ec830fac4a92787b585ff55435e";
        branchName = "manifest-rev";
        sha256 = "0ma2nh76ssbp1pmvqvjhgn4sqj240516gkwyrpc4jpbw5z3kcj6l";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/libmetal";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/libmetal";
        rev = "39d049d4ae68e6f6d595fce7de1dcfc1024fb4eb";
        branchName = "manifest-rev";
        sha256 = "0s3ycdc4k6k9v0j1z3fpamsfcllqfzlqwns7qrd6m4dg146yp1gv";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/quicklogic";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_quicklogic";
        rev = "b3a66fe6d04d87fd1533a5c8de51d0599fcd08d0";
        branchName = "manifest-rev";
        sha256 = "18ygkcbgl5p7x5bdzzzm0h90hl5qpfw76x9ah5id72yr3wf3myny";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/gui/lvgl";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/lvgl";
        rev = "31acbaa36e9e74ab88ac81e3d21e7f1d00a71136";
        branchName = "manifest-rev";
        sha256 = "1xj30771igvpzpm01afh4rqlagrv05c3ais9i3ljflrnxvqzjgms";
        leaveDotGit = true;
    };
}

{
    name = "modules/crypto/mbedtls";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/mbedtls";
        rev = "5765cb7f75a9973ae9232d438e361a9d7bbc49e7";
        branchName = "manifest-rev";
        sha256 = "0dkhyfxpxlq3225kzarslfzmym9zsxzkzy4w8j1wbwad09y7haw6";
        leaveDotGit = true;
    };
}

{
    name = "bootloader/mcuboot";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/mcuboot";
        rev = "2fce9769b191411d580bbc65b043956c2ae9307e";
        branchName = "manifest-rev";
        sha256 = "10y1kh5bpw8qqlcxjb1a2anrmjvznncn1fsahkazaijqbhbdv8jr";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/mcumgr";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/mcumgr";
        rev = "5c5055f5a7565f8152d75fcecf07262928b4d56e";
        branchName = "manifest-rev";
        sha256 = "1401kdx8ipcpi3m63y62gghfnd7gffiihhs0cpm3sxq076kyjb1d";
        leaveDotGit = true;
    };
}

{
    name = "tools/net-tools";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/net-tools";
        rev = "f49bd1354616fae4093bf36e5eaee43c51a55127";
        branchName = "manifest-rev";
        sha256 = "0jxjmiqipwyv7iy61i82snxmh0mbyhzwijib68mmc3jq0ra3iwwz";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/nxp";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_nxp";
        rev = "0d11138724959e1162777d9206f841ccdf64348e";
        branchName = "manifest-rev";
        sha256 = "0lp7nwc3bwyv9j3qkm74b6cfw61cqm7ylbbz78xbalq7gjzs7smd";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/open-amp";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/open-amp";
        rev = "6010f0523cbc75f551d9256cf782f173177acdef";
        branchName = "manifest-rev";
        sha256 = "1cy8s19jfpis3dbj64lk0qr1y7vr48csr9rvlw6y26qacl99yr8c";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/loramac-node";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/loramac-node";
        rev = "2cee5f7295ff0ff804bf06fea5de006bc7cd121e";
        branchName = "manifest-rev";
        sha256 = "0vz4y6zc303c18qhq81iysmg8kfzvgk2irnhsz35hcw99jg9cw14";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/openthread";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/openthread";
        rev = "385e19da1ae15f27872c2543b97276a42f102ead";
        branchName = "manifest-rev";
        sha256 = "0f5c6xax86j5yz7bbm1nc8p4slgnfgysgja69rqlcws1zh4afxb2";
        leaveDotGit = true;
    };
}

{
    name = "modules/debug/segger";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/segger";
        rev = "3a52ab222133193802d3c3b4d21730b9b1f1d2f6";
        branchName = "manifest-rev";
        sha256 = "057327xsr9s3akz4lj09n6f4isc98zrwl8r4p0rxnckhvj4fvm94";
        leaveDotGit = true;
    };
}

{
    name = "modules/audio/sof";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/sof";
        rev = "779f28ed465c03899c8a7d4aaf399856f4e51158";
        branchName = "manifest-rev";
        sha256 = "0h6xph101mm39kksnckbgpn3y12r95fl67c6qya9nrkbfyzmq3kb";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/tinycbor";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/tinycbor";
        rev = "40daca97b478989884bffb5226e9ab73ca54b8c4";
        branchName = "manifest-rev";
        sha256 = "06rgjk2zgzd1baf6f91157rm1w8fhyngm6806mz4i7i402cw7d6l";
        leaveDotGit = true;
    };
}

{
    name = "modules/crypto/tinycrypt";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/tinycrypt";
        rev = "3e9a49d2672ec01435ffbf0d788db6d95ef28de0";
        branchName = "manifest-rev";
        sha256 = "15nbfjyfy9v4dfmm8138hj9zls3h1kj5sl7wc4f6cwp8rfygq9fb";
        leaveDotGit = true;
    };
}

{
    name = "modules/fs/littlefs";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/littlefs";
        rev = "9e4498d1c73009acd84bb36036ee5e2869112a6c";
        branchName = "manifest-rev";
        sha256 = "1zlfzlwyvfnjjjxpyl2pn4wq9qm65i3k5f532b9sj85bh4kpf938";
        leaveDotGit = true;
    };
}

{
    name = "modules/debug/mipi-sys-t";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/mipi-sys-t";
        rev = "75e671550ac1acb502f315fe4952514dc73f7bfb";
        branchName = "manifest-rev";
        sha256 = "1swrviajm9i9fclkb9nz87slc889m9l3yqb3bpbqq0nfnsm3viqx";
        leaveDotGit = true;
    };
}

{
    name = "modules/bsim_hw_models/nrf_hw_models";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/nrf_hw_models";
        rev = "a47e326ca772ddd14cc3b9d4ca30a9ab44ecca16";
        branchName = "manifest-rev";
        sha256 = "0w3yghy0v1h6mi92awn3p86p6zql3kvify9djcdnz72y5r8dm764";
        leaveDotGit = true;
    };
}

{
    name = "modules/debug/TraceRecorder";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/TraceRecorderSource";
        rev = "d9889883abb4657d71e15ff055517a9b032f8212";
        branchName = "manifest-rev";
        sha256 = "1ljhlsfpw3cklwbh1vld392f0bz1jwqy4kngpnlc55lwfwg48ffp";
        leaveDotGit = true;
    };
}

{
    name = "modules/hal/xtensa";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/hal_xtensa";
        rev = "2f04b615cd5ad3a1b8abef33f9bdd10cc1990ed6";
        branchName = "manifest-rev";
        sha256 = "0az1gigxb96y2xgxpq6gvafy3g27ipafigy1fg6912swdx64jz70";
        leaveDotGit = true;
    };
}

{
    name = "tools/edtt";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/edtt";
        rev = "7dd56fc100d79cc45c33d43e7401d1803e26f6e7";
        branchName = "manifest-rev";
        sha256 = "0yligmlan41lnmkgpx45s9sgj7q30rz4gsmp2xb5mzg1q7gjagli";
        leaveDotGit = true;
    };
}

{
    name = "modules/tee/tfm";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/trusted-firmware-m";
        rev = "e18b7a9b040b5b5324520388047c9e7d678447e6";
        branchName = "manifest-rev";
        sha256 = "116419a5799ycgvi61fzr3frsx8snqbmfj7w0qp1fvxix5h7bmxy";
        leaveDotGit = true;
    };
}

{
    name = "modules/tee/tfm-mcuboot";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/mcuboot";
        rev = "v1.7.2";
        branchName = "manifest-rev";
        sha256 = "12gbp3c5and25jz3a7ivk0d0s7hqa6fajkbf9jf071fl2sgkf27a";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/nanopb";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/nanopb";
        rev = "d148bd26718e4c10414f07a7eb1bd24c62e56c5d";
        branchName = "manifest-rev";
        sha256 = "0d5bhhcv6dnqqxlw71fifn15n33vnqiy46633grs5fvbdaq2sdi8";
        leaveDotGit = true;
    };
}

{
    name = "modules/lib/tensorflow";
    path = fetchgit {
        url = "https://github.com/zephyrproject-rtos/tensorflow";
        rev = "dc70a45a7cc12c25726a32cd91b28be59e7bc596";
        branchName = "manifest-rev";
        sha256 = "12vpmrigi3v015mqfacyclxpk3a89c01wnkdw8h3x1076ksdmd33";
        leaveDotGit = true;
    };
}

]).overrideAttrs ({ buildCommand, ... }: {
    buildCommand = buildCommand + ''
        mkdir -p .west
        cat << EOF > .west/config
        [manifest]
        path = firmware
        file = west.yml
        EOF
    '';
})
