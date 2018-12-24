{ lib, fetchPypi, buildPythonPackage
, lxml
, py
, Logbook
, pytest
}:

buildPythonPackage rec {
  pname = "pyshark";
  version = "0.4.2.2";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "12wxcclj85wf6xvgk7d29z66v9qpi3zixhrjpcmc0hqjarq78v6l";
  };

  propagatedBuildInputs = [
    lxml
    py
    Logbook
  ];
  
  checkInputs = [ pytest ];

  # Require trollius, which doesn't work on Python 3.7
  doCheck = false;
}
