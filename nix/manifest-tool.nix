{ lib, fetchFromGitHub, buildPythonPackage
, ecdsa
, cryptography
, pyasn1
, asn1ate
, pyparsing
, future
, urllib3
, colorama
, protobuf
}:

buildPythonPackage rec {
  pname = "manifest-tool";
  version = "1.4.7";
  
  src = fetchFromGitHub {
    owner = "ARMmbed";
    repo = pname;
    rev = "v${version}";
    sha256 = "0s76g1yzc2wqdgyl1yd4w0kch0m137cc2jxv4wnczj9w7x378r2y";
  };

  propagatedBuildInputs = [
    ecdsa
    cryptography
    pyasn1
    asn1ate
    pyparsing
    future
    urllib3
    colorama
    protobuf
  ];
}
