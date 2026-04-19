{
  description = "Water level sensor firmware and base station";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    with flake-utils.lib;
    eachSystem defaultSystems (
      system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages = {
          base-station = pkgs.callPackage ./base_station { };
          sensor = pkgs.callPackage ./sensor { };
        };

        apps.default = mkApp {
          drv = self.packages.${system}.base-station;
          exePath = "/bin/water_level_base_station";
        };

        devShells = {
          base-station = pkgs.callPackage ./base_station/shell.nix { };
          sensor = pkgs.callPackage ./sensor/shell.nix { };
        };
      }
    )
    // eachSystem [ "x86_64-linux" ] (system: {
      hydraJobs = self.packages.${system};
    })
    // {
      nixosModule = import ./base_station/module.nix;
    };
}
