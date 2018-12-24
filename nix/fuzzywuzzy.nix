{ lib, buildPythonPackage, fetchPypi, python-Levenshtein
, pytest, hypothesis, enum34, pycodestyle }:

buildPythonPackage rec {
  pname = "fuzzywuzzy";
  version = "0.17.0";

  src = fetchPypi {
    inherit pname version;
    sha256 = "0qid283ysgzn3pm6pdm9dy9mghsa511dl5md80fwgq80vd3xwjbg";
  };

  propagatedBuildInputs = [ python-Levenshtein ];

  checkInputs = [ pytest hypothesis enum34 pycodestyle ];

  meta = with lib; {
    description = "Fuzzy string matching in python";
    homepage = https://github.com/seatgeek/fuzzywuzzy;
    license = licenses.gpl2;
  };
}
