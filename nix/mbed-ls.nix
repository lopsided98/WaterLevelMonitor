{ fetchPypi, buildPythonPackage, prettytable, fasteners, appdirs, mock, pytest }:

buildPythonPackage rec {
  pname = "mbed-ls";
  version = "1.6.2";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0nyb3cw4851cs8201q2fkna0z565j7169vj7wm2c88c8fm6qd21i";
  };

  propagatedBuildInputs = [ prettytable fasteners appdirs ];
  
  checkInputs = [ mock pytest ];
  
  # Tries to access files that do not exist
  doCheck = false;
}
