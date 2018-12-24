{ fetchFromGitHub, buildPythonPackage, github3_py, whelk, docopt, six }:

buildPythonPackage rec {
  pname = "git-spindle";
  version = "3.4.4-20180422";
  
  src = fetchFromGitHub {
    owner = "seveas";
    repo = pname;
    rev = "e174b4ad26965ab8d0f7a959fb0d40db4e7f427e";
    sha256 = "00n9h7drjflyb8ix6k508l3pwjj1rvqbficdx828w4d2a21q5f1y";
  };

  propagatedBuildInputs = [ github3_py whelk docopt six ];
}
