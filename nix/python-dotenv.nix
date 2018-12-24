{ lib, fetchPypi, buildPythonPackage, click, ipython }:

buildPythonPackage rec {
  pname = "python-dotenv";
  version = "0.10.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "01vk4i6hb01x32kgsp2qbsiwjmp0gjqlr94s9jmwbnh89n884dag";
  };
  
  propagatedBuildInputs = [ click ];
  
  checkInputs = [ ipython ];
}
