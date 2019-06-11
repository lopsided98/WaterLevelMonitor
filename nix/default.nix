self: super:

with super.lib;

let
  pythonOverridesFor = python: python.override (old: {
    packageOverrides = pySelf: pySuper: {
      gitlint = pySelf.callPackage ./gitlint.nix {
        arrow = pySelf.arrow.overridePythonAttrs (oldAttrs: rec {
          version = "0.10.0";
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "08n7q2l69hlainds1byd4lxhwrq7zsw7s640zkqc3bs5jkq0cnc0";
          };
        });
        click = pySelf.click.overridePythonAttrs (oldAttrs: rec {
          version = "6.7";
          patches = [];
          src = pySelf.fetchPypi {
            inherit (oldAttrs) pname;
            inherit version;
            sha256 = "02qkfpykbq35id8glfgwc38yc430427yd05z1wc5cnld8zgicmgi";
          };
        });
      };
      junit2html = pySelf.callPackage ./junit2html.nix { };
      pykwalify = pySelf.callPackage ./pykwalify.nix { };
      pyocd = pySelf.callPackage ./pyocd.nix { };
      setuptools_scm_git_archive = pySelf.callPackage ./setuptools_scm_git_archive.nix { };
      sphinxcontrib-svg2pdfconverter = pySelf.callPackage ./sphinxcontrib-svg2pdfconverter.nix { };
      websocket-client = pySelf.callPackage ./websocket-client.nix { };
      west = pySelf.callPackage ./west.nix { };
    };
  });
in {
  python27 = pythonOverridesFor super.python27;
  python36 = pythonOverridesFor super.python36;
  python37 = pythonOverridesFor super.python37;
}
