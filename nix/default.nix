self: super:

with super.lib;

let
  pythonOverridesFor = python: python.override (old: {
    packageOverrides = pySelf: pySuper: {
      gitlint = pySelf.callPackage ./gitlint.nix { };
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
