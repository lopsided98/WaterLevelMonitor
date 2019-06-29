#!/bin/sh

ssh -t Roomba \
  -L 4444:localhost:4444 \
  -L 3333:localhost:3333  \
  "nix run nixpkgs.openocd -c sudo openocd -f WaterLevelMonitor/openocd.cfg"
