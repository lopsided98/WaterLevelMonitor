{
  description = "Water level sensor firmware and base station";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    crate2nix = { url = "github:kolloch/crate2nix"; flake = false; };
  };

  outputs = { self, nixpkgs, flake-utils, crate2nix }:
    with flake-utils.lib;
    eachSystem defaultSystems (system: let
      crate2nixOverlay = final: prev: {
        crate2nix = import crate2nix { pkgs = final; };
      };
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ crate2nixOverlay ];
      };
    in rec {
      packages = {
        base-station = pkgs.callPackage ./base_station { };
        firmware = pkgs.callPackage ./sensor { };
      };

      defaultPackage = packages.base-station;

      defaultApp = mkApp {
        drv = defaultPackage;
        exePath = "/bin/water_level_base_station";
      };
    }) //
    eachSystem [ "x86_64-linux" ] (system: {
      hydraJobs = self.packages.${system};
    }) //
    {
      nixosModule = import ./base_station/module.nix;
    };
}
