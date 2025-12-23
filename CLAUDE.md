# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an AVR microcontroller project targeting the ATmega328p (Arduino Uno/Mini) for a robot arm controller. The project uses bare-metal C programming with direct hardware register manipulation rather than the Arduino framework.
The servo motors are controlled using PWM driver PCA9685

**Target Hardware:**
- MCU: ATmega328p
- Clock: 16 MHz
- Serial: /dev/ttyUSB0 at 115200 baud

PWM driver PCA9685
LCD Keyboard Shield

## Build System

**Build commands:**
```bash
make              # Build main.hex from main.c
make clean        # Remove compiled binaries
./flash.sh        # Flash main.hex to the microcontroller via avrdude
```

**Build chain:**
- `main.c` → `avr-gcc` → `main.elf` → `avr-objcopy` → `main.hex`
- Compiler flags: `-Wall -Os -DF_CPU=16000000UL -mmcu=atmega328p`

**Note:** Currently the Makefile only compiles `main.c`.

## Code Architecture

### Hardware Abstraction

The code uses direct register manipulation via macros for hardware control.

### Code Organization

- `main.c`: Simple LED blink demo (currently built)

### Programming Model

