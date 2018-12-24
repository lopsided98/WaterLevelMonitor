{ fetchPypi, buildPythonPackage, pytest }:

buildPythonPackage rec {
  pname = "junit2html";
  version = "022";
  
  src = fetchPypi {
    inherit pname version;
    sha256 = "1pcwdmzabw63mfa0zpq700if0fmkyn3cz91x6ayvls21xixrfq11";
  };

  checkInputs = [ pytest ];

  # error: [Errno 2] No such file or directory: 'test'
  doCheck = false;
}
