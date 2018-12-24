#!/bin/sh

arm-none-eabi-gdb -batch \
	-ex "target extended-remote :3333" \
	-ex "load" \
	-ex "monitor reset" \
	"$@"
