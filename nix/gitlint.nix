{ lib, fetchPypi, buildPythonPackage, click, arrow, sh, git }:

buildPythonPackage rec {
  pname = "gitlint";
  version = "0.12.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "131pmggb5bsqmxd7rk3xg6nsi6vcmkba28vpmfcw0gkdakn0w15q";
  };
  
  # Exact versions of dependencies are pinned
  pipInstallFlags = [ "--no-deps" ];
  doCheck = false;
  
  propagatedBuildInputs = [ click arrow sh ];
  
  checkInputs = [ git ];
}
