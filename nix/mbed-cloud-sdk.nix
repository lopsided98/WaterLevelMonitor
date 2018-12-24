{ lib, fetchPypi, buildPythonPackage
, certifi
, future
, python-dateutil
, python-dotenv
, requests
, six
, urllib3
}:

buildPythonPackage rec {
  pname = "mbed_cloud_sdk";
  version = "2.0.4";
  format = "wheel";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1z7gf7n2sczczs2d2pkic15320p50nd8xhvjb8i3dp7fbkc08d7n";
    format = "wheel";
  };

  propagatedBuildInputs = [
    certifi
    future
    python-dateutil
    python-dotenv
    requests
    six
    urllib3
  ];
}
