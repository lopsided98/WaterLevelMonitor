{ fetchPypi, buildPythonPackage, colorama, pyyaml, pykwalify, configobj }:

buildPythonPackage rec {
  pname = "west";
  version = "0.5.8";
  
  propagatedBuildInputs = [ colorama pyyaml pykwalify configobj ];
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0g1w7z1bqlmf428shzs3h3z2lm6d6ydbcjj4x5rwhcnvarpmpfsv";
  };
}
