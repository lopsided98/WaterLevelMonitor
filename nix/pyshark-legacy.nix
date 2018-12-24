{ lib, fetchPypi, buildPythonPackage
, lxml
, py
, trollius
, Logbook
, mock
, pytest
}:

buildPythonPackage rec {
  pname = "pyshark-legacy";
  version = "0.3.8";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0qxcflaasd1cr5xgp5i2kxmb4x410zvfj45lxdfjvd882qrixny8";
  };

  propagatedBuildInputs = [
    lxml
    py
    trollius
    Logbook
  ];
  
  checkInputs = [ mock pytest ];
}
