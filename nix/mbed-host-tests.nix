{ fetchPypi, buildPythonPackage
, pyserial
, prettytable
, requests
, mbed-ls
, pyocd
, intelhex
, future
}:

buildPythonPackage rec {
  pname = "mbed-host-tests";
  version = "1.4.2";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0dh3p85y1xm78n767czacxbcllkh7w4ab46fgc0kda96lrxxbbi4";
  };

  propagatedBuildInputs = [
    pyserial
    prettytable
    requests
    mbed-ls
    pyocd
    intelhex
    future
  ];

  # Something's missing
  doCheck = false;
}
