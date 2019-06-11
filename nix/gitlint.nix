{ lib, fetchPypi, buildPythonPackage, click, arrow, sh, git }:

buildPythonPackage rec {
  pname = "gitlint";
  version = "0.11.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0n25ddmgc9d3wfdjm8nsy2xdzm9cf39x75syk3qp1wv543i65a2r";
  };
  
  doCheck = false;
  
  propagatedBuildInputs = [ click arrow sh ];
  
  checkInputs = [ git ];
}
