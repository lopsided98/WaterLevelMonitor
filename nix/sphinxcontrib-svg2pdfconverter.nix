{ fetchPypi, buildPythonPackage, sphinx }:

buildPythonPackage rec {
  pname = "sphinxcontrib-svg2pdfconverter";
  version = "0.1.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1abvbgkkii13q8nsb10r0gc5lm0p9iq1iwhfhakn5ifn6asa0183";
  };

  propagatedBuildInputs = [ sphinx ];
}
