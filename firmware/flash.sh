#!/bin/bash
avrdude -c arduino -p atmega328p -P /dev/ttyUSB0 -b 115200 -U flash:w:main.hex:i
