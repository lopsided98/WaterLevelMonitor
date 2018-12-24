{ lib, fetchPypi, buildPythonPackage, isPy3k
, setuptools_scm
, setuptools_scm_git_archive
, intelhex
, six
, future
, websocket-client
, intervaltree
, colorama
, pyelftools
, pyusb
, pyyaml
, enum34
, pytest
}:

buildPythonPackage rec {
  pname = "pyOCD";
  version = "0.12.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0z32s13nd62h07mngf1ikgmg3a7r92shkwv40b2zrjic9rrz0ycw";
  };

  prePatch = ''
    sed -i "s#'enum34',##" setup.py
  '';

  nativeBuildInputs = [ setuptools_scm setuptools_scm_git_archive ];

  propagatedBuildInputs = [
    intelhex
    six
    future
    websocket-client
    intervaltree
    colorama
    pyelftools
    pyusb
    pyyaml
  ] ++ lib.optional (!isPy3k) enum34;
  
  checkInputs = [ pytest /* elapsedtimer */ ];

  # Needs package that I don't want to package
  doCheck = false;
}
