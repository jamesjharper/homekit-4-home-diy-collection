#!/bin/bash
set -e


get_abs_filename() {
  # $1 : relative filename
  echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
}


LIBRARIES="\
    --library $(get_abs_filename "./libraries/gpio_pwm") \
    --library $(get_abs_filename "./libraries/controllers") \
    --library $(get_abs_filename "./libraries/homespan_services")"

arduino-cli compile -v $LIBRARIES --fqbn esp32:esp32:esp32 lightswitch
arduino-cli upload -p /dev/cu.usbserial-0001 --fqbn esp32:esp32:esp32 lightswitch