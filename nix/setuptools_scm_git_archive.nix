{ lib, fetchPypi, buildPythonPackage, setuptools_scm, pytest }:

buildPythonPackage rec {
  pname = "setuptools_scm_git_archive";
  version = "1.0";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1nii1sz5jq75ilf18bjnr11l9rz1lvdmyk66bxl7q90qan85yhjj";
  };

  nativeBuildInputs = [ setuptools_scm ];
  
  checkInputs = [ pytest ];
  
  # Work around the most broken build system I have ever seen
  preBuild = ''
    mkdir -p build/bdist.linux-x86_64/wheel/setuptools_scm_git_archive
  '';
  
  postBuild = ''
    rm dist/setuptools_scm_git_archive-0-py2.py3-none-any.whl
  '';
}
