{ fetchPypi, buildPythonPackage
, mbed-ls
, six
, pyserial
, pyocd
, mock
}:

buildPythonPackage rec {
  pname = "mbed-flasher";
  version = "0.9.2";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "13rh81fvy55p2z84gsg8v1msizikivijsbd1akbr7r87ylp37ds5";
  };

  propagatedBuildInputs = [
    mbed-ls
    six
    pyserial
    pyocd
  ];
  
  checkInputs = [ mock ];
  
  # Tries to access files that don't exist
  doCheck = false;
}
