{ lib, fetchPypi, buildPythonPackage, six, backports_ssl_match_hostname }:

buildPythonPackage rec {
  pname = "websocket_client";
  version = "0.54.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0j88zmikaypf38lvpkf4aaxrjp9j07dmy5ghj7kli0fv3p4n45g5";
  };

  propagatedBuildInputs = [ six ];
  
  checkInputs = [ backports_ssl_match_hostname ];
}
