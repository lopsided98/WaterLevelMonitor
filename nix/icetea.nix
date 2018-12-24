{ lib, fetchPypi, buildPythonPackage, isPy3k
, prettytable
, requests
, yattag
, pyserial
, jsonmerge
, jsonschema
, mbed-ls
, semver
, mbed-flasher
, six
, pyshark-legacy
, pyshark
, coverage
, mock
, sphinx
, lxml
, pylint
, astroid
}:

buildPythonPackage rec {
  pname = "icetea";
  version = "1.1.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "02bjwj5ign9dyycbg6hi56r26v0aqk8vlp33mqx3qh8vpcxx0k9f";
  };

  propagatedBuildInputs = [
    prettytable
    requests
    yattag
    pyserial
    jsonmerge
    jsonschema
    mbed-ls
    semver
    mbed-flasher
    six
  ] ++ lib.optional (!isPy3k) pyshark-legacy
    ++ lib.optional (isPy3k) pyshark;
  
  checkInputs = [
    coverage
    mock
    sphinx
    lxml
    pylint
    astroid
  ];
  
  # Tries to access files that don't exist
  doCheck = false;
}
