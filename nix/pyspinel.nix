{ fetchPypi, buildPythonPackage, pyserial, ipaddress, future }:

buildPythonPackage rec {
  pname = "pyspinel";
  version = "1.0.0a1";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0fh8nsgb032xw4akdyns7z71xhk4sfhqi0as5qw688h901aib5y5";
  };

  propagatedBuildInputs = [ pyserial ipaddress future ];
  
  doCheck = false;
}
