{ lib, fetchPypi, buildPythonPackage, jsonschema }:

buildPythonPackage rec {
  pname = "jsonmerge";
  version = "1.5.2";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "07vakkx8czcfqjd0a2gj79qqr1n9l7wcf4gsngyvphambma2jln0";
  };

  propagatedBuildInputs = [ jsonschema ];
}
