{ lib, fetchFromGitHub, buildPythonPackage, click, arrow, sh, git }:

buildPythonPackage rec {
  pname = "gitlint";
  version = "0.10.0-20181107";
  
  src = fetchFromGitHub {
    owner = "jorisroovers";
    repo = "gitlint";
    rev = "20738f92ec8e8670c4e89643f4d185981b74decc";
    sha256 = "10qdhyrsd2j1zs3yw11whaigdqwvy95mfsghzlpkx76rgwqwvhgv";
  };
  
  doCheck = false;
  
  propagatedBuildInputs = [ click arrow sh ];
  
  checkInputs = [ git ];
}
