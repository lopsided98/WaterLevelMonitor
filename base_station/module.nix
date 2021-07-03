{ config, lib, pkgs, ... }:

with lib;

let
  cfg = config.services.waterLevelMonitor;
in {
  options.services.waterLevelMonitor = {
    enable = mkEnableOption "Water Level Monitor";

    influxdb = mkOption {
      type = types.submodule {
        options = {
          url = mkOption {
            type = types.str;
            default = "http://localhost:8086";
            description = ''
              URL used to connect to InfluxDB.
            '';
          };

          database = mkOption {
            type = types.str;
            default = "water-level";
            description = ''
              InfluxDB database name.
            '';
          };

          certificateFile = mkOption {
            type = types.path;
            description = ''
              File containing the PKCS #12 certificate and private key used to
              authenticate with InfluxDB. The private key must have no password.
            '';
          };
        };
      };
      default = {};
    };

    address = mkOption {
      type = types.str;
      description = ''
        BLE address of the sensor.
      '';
    };
  };

  config = mkIf cfg.enable {
    users = {
      users.water-level = {
        isSystemUser = true;
        group = "water-level";
      };
      groups.water-level = {};
    };

    hardware.bluetooth.enable = true;

    services.dbus.packages = singleton (pkgs.writeTextFile {
      name = "dbus-water-level-bluetooth.conf";
      destination = "/etc/dbus-1/system.d/water-level-bluetooth.conf";
      text = ''
        <!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
         "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
        <busconfig>
          <policy user="water-level">
            <allow send_destination="org.bluez"/>
          </policy>
        </busconfig>
      '';
    });

    systemd.services.water-level-base-station = let
      water-level-base-station = pkgs.callPackage ./. { };
    
      settings = pkgs.writeText "water-level-settings.yaml" (builtins.toJSON {
        influxdb = {
          inherit (cfg.influxdb) url database;
          certificate.file = cfg.influxdb.certificateFile;
        };
        inherit (cfg) address;
      });
    in {
      wantedBy = [ "multi-user.target" ];
      after = [ "bluetooth.target" ];
      serviceConfig = {
        User = "water-level";
        Group = "water-level";
        Restart = "always";
        RestartSec = 10;
        ExecStart = escapeShellArgs [
          "${water-level-base-station}/bin/water_level_base_station" 
          settings
        ];
      };
      unitConfig = {
        StartLimitBurst = 3;
        StartLimitIntervalSec = 60;
      };
    };
  };
}
