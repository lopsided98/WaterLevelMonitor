{ lib, fetchPypi, buildPythonPackage
, prettytable
, pyserial
, mbed-host-tests
, mbed-ls
, junit-xml
, lockfile
, mock
, six
, colorama
, future
}:

buildPythonPackage rec {
  pname = "mbed-greentea";
  version = "1.4.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0yqlnm1y94wc7byhpz7vkny1kmgg90z6jvizw6ih0a3v3vj1yfpz";
  };

  propagatedBuildInputs = [
    prettytable
    pyserial
    mbed-host-tests
    mbed-ls
    junit-xml
    lockfile
    mock
    six
    colorama
    future
  ];

  # False is not true
  doCheck = false;
}
