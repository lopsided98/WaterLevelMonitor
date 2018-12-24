{ fetchPypi, buildPythonPackage, click, arrow, sh, git }:

buildPythonPackage rec {
  pname = "gitlint";
  version = "0.10.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "0mb09j8q91y7s640kq757ykxavm7g2lbgfg9z7czg8g1ky1ydfcf";
  };
  
  propagatedBuildInputs = [ click arrow sh ];
  
  checkInputs = [ git ];
}
