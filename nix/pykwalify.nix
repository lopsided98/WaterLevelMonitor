{ buildPythonPackage, fetchPypi
, docopt
, python-dateutil
, pyyaml
, pytest
, testfixtures
}:
buildPythonPackage rec {
  pname = "pykwalify";
  version = "1.7.0";

  src = fetchPypi {
    inherit pname version;
    sha256 = "7e8b39c5a3a10bc176682b3bd9a7422c39ca247482df198b402e8015defcceb2";
  };

  propagatedBuildInputs = [
    docopt
    python-dateutil
    pyyaml
  ];
  
  checkInputs = [ pytest testfixtures ];
}
