self: super:

with super.lib;

let
  pythonOverridesFor = python: python.override (old: {
    packageOverrides = pySelf: pySuper: {
      fuzzywuzzy = pySelf.callPackage ./fuzzywuzzy.nix { };
      git-spindle = pySelf.callPackage ./git-spindle.nix { };
      gitlint = pySelf.callPackage ./gitlint.nix {
        arrow = pySelf.arrow.overridePythonAttrs (oldAttrs: rec {
          version = "0.10.0";
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "08n7q2l69hlainds1byd4lxhwrq7zsw7s640zkqc3bs5jkq0cnc0";
          };
        });
        sh = pySelf.sh.overridePythonAttrs (oldAttrs: rec {
          version = "1.11";
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "192r0mpv6dmkysjzhc43ddffiwb5g7c76bgr1mb1z2xz9awbj3sr";
          };
        });
        click = pySelf.click.overridePythonAttrs (oldAttrs: rec {
          version = "6.6";
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "1sggipyz52crrybwbr9xvwxd4aqigvplf53k9w3ygxmzivd1jsnc";
          };
        });
      };
      icetea = pySelf.callPackage ./icetea.nix { };
      junit2html = pySelf.callPackage ./junit2html.nix { };
      junit-xml = pySelf.callPackage ./junit-xml.nix { };
      jsonmerge = pySelf.callPackage ./jsonmerge.nix { };
      manifest-tool = pySelf.callPackage ./manifest-tool.nix {
        protobuf = pySelf.protobuf.override { protobuf = self.protobuf3_5; };
      };
      mbed-cli = pySelf.callPackage ./mbed-cli.nix { };
      mbed-cloud-sdk = pySelf.callPackage ./mbed-cloud-sdk.nix { };
      mbed-flasher = pySelf.callPackage ./mbed-flasher.nix { };
      mbed-greentea = pySelf.callPackage ./mbed-greentea.nix { };
      mbed-host-tests = pySelf.callPackage ./mbed-host-tests.nix { };
      mbed-ls = pySelf.callPackage ./mbed-ls.nix { };
      nrfutil = pySelf.callPackage ./nrfutil.nix { };
      pc_ble_driver_py = pySelf.callPackage ./pc_ble_driver_py.nix { };
      piccata = pySelf.callPackage ./piccata.nix { };
      /*pyasn1 = pySuper.pyasn1.overridePythonAttrs (oldAttrs: rec {
        version = "0.2.3";
        src = pySelf.fetchPypi {
          inherit (oldAttrs) pname;
          inherit version;
          sha256 = "1b86yx23c1x74clai05a5ma8c8nfmhlx3j1mxq0ff657i2ylx33k";
        };
      });*/
      pykwalify = pySelf.callPackage ./pykwalify.nix { };
      pyocd = pySelf.callPackage ./pyocd.nix { };
      pyshark-legacy = pySelf.callPackage ./pyshark-legacy.nix {
        trollius = pySelf.trollius.overridePythonAttrs (oldAttrs: rec {
          version = "1.0.4";
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "0xny8y12x3wrflmyn6xi8a7n3m3ac80fgmgzphx5jbbaxkjcm148";
          };
        });
      };
      pyshark = pySelf.callPackage ./pyshark.nix { };
      pyspinel = pySelf.callPackage ./pyspinel.nix { };
      python-dotenv = pySelf.callPackage ./python-dotenv.nix { };
      requests = pySuper.requests.overridePythonAttrs (oldAttrs: rec {
        version = "2.20.1";
        src = pySelf.fetchPypi {
          inherit (oldAttrs) pname;
          inherit version;
          sha256 = "0qzj6cgv3k9wyj7wlxgz7xq0cfg4jbbkfm24pp8dnhczwl31527a";
        };
      });
      setuptools_scm_git_archive = pySelf.callPackage ./setuptools_scm_git_archive.nix { };
      sphinxcontrib-svg2pdfconverter = pySelf.callPackage ./sphinxcontrib-svg2pdfconverter.nix { };
      websocket-client = pySelf.callPackage ./websocket-client.nix { };
      whelk = pySelf.callPackage ./whelk.nix { };
    };
  });
in {
  python27 = pythonOverridesFor super.python27;
  python36 = pythonOverridesFor super.python36;
  python37 = pythonOverridesFor super.python37;
}
