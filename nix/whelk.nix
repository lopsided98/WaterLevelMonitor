{ fetchPypi, buildPythonPackage }:

buildPythonPackage rec {
  pname = "whelk";
  version = "2.7.1";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1g34fpfzdmz992xnw0gywigqf0d66axq9wsizfdr8y16lxqd42fi";
  };
}
