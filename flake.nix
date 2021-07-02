{
  description = "Water level sensor firmware and base station";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    crate2nix = { url = "github:kolloch/crate2nix"; flake = false; };
  };

  outputs = { self, nixpkgs, flake-utils, crate2nix }:
    with flake-utils.lib;
    eachSystem allSystems (system: let
      crate2nixOverlay = final: prev: {
        crate2nix = import crate2nix { pkgs = final; };
      };
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ self.overlay crate2nixOverlay ];
      };
    in {
      defaultPackage = pkgs.water-level-base-station;

      defaultApp = mkApp {
        drv = self.defaultPackage.${system};
        exePath = "/bin/water_level_base_station";
      };
    }) //
    eachSystem [ "x86_64-linux" "aarch64-linux" ] (system: {
      hydraJobs.build = self.defaultPackage.${system};
    }) //
    {
      overlay = final: prev: {
        water-level-base-station = final.callPackage ./base_station { };
      };
    };
}
