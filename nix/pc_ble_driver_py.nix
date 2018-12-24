{ fetchPypi, buildPythonPackage, nose, behave, enum34, wrapt, future }:

buildPythonPackage rec {
  pname = "pc_ble_driver_py";
  version = "0.11.4";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0sxldf5rgxsry328jv01s77640qh1v3bas98chjacf0xc88w8i44";
  };
  
  checkInputs = [ nose behave ];
  
  propagatedBuildInputs = [ enum34 wrapt future ];
}
