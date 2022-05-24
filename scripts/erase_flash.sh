#!/bin/bash
set -e
esptool.py --chip esp32 --port /dev/cu.usbserial-0001 erase_flash