{ fetchPypi, buildPythonPackage
, mercurial
, git
, pip
, colorama
, pyserial
, prettytable
, jinja2
, intelhex
, junit-xml
, pyyaml
, urllib3
, requests
, mbed-ls
, mbed-host-tests
, mbed-greentea
, beautifulsoup4
, fuzzywuzzy
, pyelftools
, jsonschema
, future
, six
, manifest-tool
, mbed-cloud-sdk
, icetea
}:

buildPythonPackage rec {
  pname = "mbed-cli";
  version = "1.8.3";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "04vn2v0d7y3vmm8cswzvn2z85balgp3095n5flvgf3r60fdlhlmp";
  };
  
  propagatedBuildInputs = [
    mercurial
    git
    pip
    colorama
    pyserial
    prettytable
    jinja2
    intelhex
    junit-xml
    pyyaml
    urllib3
    requests
    mbed-ls
    mbed-host-tests
    mbed-greentea
    beautifulsoup4
    fuzzywuzzy
    pyelftools
    jsonschema
    future
    six
    manifest-tool
    mbed-cloud-sdk
    icetea
  ];
  
  doCheck = false;
}
