{ fetchPypi, buildPythonPackage, ipaddress }:

buildPythonPackage rec {
  pname = "piccata";
  version = "1.0.1";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1xq82nj5xzq5b36gx1hd951yhgmwk5zi32080i2x82d85s6ckxj5";
  };
  
  propagatedBuildInputs = [ ipaddress ];
}
