let
  pkgs = import <nixpkgs> { overlays = [ (import ../nix) ]; };
in

with pkgs;

stdenv.mkDerivation {
  name = "water-level-monitor-env";
  buildInputs = with python3Packages; [
    wheel
    breathe
    sphinx
    docutils
    sphinx_rtd_theme
    sphinxcontrib-svg2pdfconverter
    junit2html
    pyyaml
    ply
    git-spindle
    gitlint
    pyelftools
    pyocd
    pyserial
    pykwalify
    colorama
    pillow
    intelhex
    dtc
    cmake
    gperf
  ];
}

