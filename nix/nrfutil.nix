{ fetchPypi, buildPythonApplication, nose, six, pyserial, enum34, click, ecdsa,
  behave, protobuf, pc_ble_driver_py, tqdm, piccata, pyspinel, intelhex,
  pyyaml, crcmod }:

buildPythonApplication rec {
  pname = "nrfutil";
  version = "4.0.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0cnmx8vnz1adwiba4xjnqdmh7am87rry51a4w6p99xm61k2pi88b";
  };
  
  checkInputs = [ nose ];
  
  propagatedBuildInputs = [
    six
    pyserial
    enum34
    click
    ecdsa
    behave
    protobuf
    pc_ble_driver_py
    tqdm
    piccata
    pyspinel
    intelhex
    pyyaml
    crcmod
  ];
  
  doCheck = false;
}
