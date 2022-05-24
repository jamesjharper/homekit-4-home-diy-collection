#!/bin/bash
set -e
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 main