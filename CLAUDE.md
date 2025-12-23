# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an AVR microcontroller project for a robot arm controller targeting the ATmega328p (Arduino Uno/Mini). The project uses bare-metal C programming with direct hardware register manipulation rather than the Arduino framework.

**Target Hardware:**
- MCU: ATmega328p (16 MHz)
- PWM Driver: PCA9685 16-channel I2C PWM driver
- Display: LCD Keyboard Shield (16x2 LCD with 5 buttons)
- Serial: /dev/ttyUSB0 at 115200 baud
- Servos: Up to 16 servos on PCA9685 channels 0-15

**Control Modes:**
1. **Button Mode** (default): Manual servo control via LCD shield buttons
2. **Serial Mode**: UART command interface for programmatic control

## Build System

**Build commands:**
```bash
make              # Compile all modules and build main.hex
make clean        # Remove compiled binaries
./flash.sh        # Flash main.hex to the microcontroller via avrdude
```

**Build chain:**
- Source files → `avr-gcc` → object files → `main.elf` → `avr-objcopy` → `main.hex`
- Compiler flags: `-Wall -Os -DF_CPU=16000000UL -mmcu=atmega328p`
- All .c files in the root directory are compiled and linked

## Code Architecture

### Hardware Abstraction

The code uses direct AVR register manipulation for maximum control and minimal overhead. Hardware abstraction is provided through dedicated driver modules rather than HAL layers.

### Module Organization

**Core Application:**
- `main.c` - Main program loop, mode switching, button handling

**Hardware Drivers:**
- `lcd.c/h` - LCD Keyboard Shield driver (4-bit mode, direct register control)
- `buttons.c/h` - Analog button reading via ADC (5 buttons on single ADC pin)
- `pca9685.c/h` - PCA9685 PWM driver for servo control (I2C interface)
- `i2c.c/h` - I2C master implementation using TWI hardware
- `uart.c/h` - UART serial communication (polling-based)

**Application Layer:**
- `serial_commands.c/h` - Serial command parser and handler

**Note:** `mm.c` is a legacy test file with various peripheral examples (DHT11, DS1307, 74HC595, ADC) and is NOT part of the main build.

### Programming Model

**Initialization Sequence:**
1. LCD initialization (4-bit mode)
2. Button ADC configuration
3. UART initialization (115200 baud)
4. I2C initialization
5. PCA9685 initialization (50Hz for servos)
6. Set initial servos to center position (90°) - currently servos 0-5 in code

**Main Loop:**
- Continuously checks for serial data availability (non-blocking check)
- If serial data is available, reads complete command line (blocking until newline)
- Reads button state and updates selected servo
- Updates LCD display with current servo number and angle
- On START command, enters serial mode (blocking until STOP)

**Serial Mode:**
- Displays command prompt `>`
- **Blocking by design**: Waits for complete command lines (blocking on newline)
- Parses commands: START, STOP, S<n>:<angle>, POSE <angles>, GET <n>, HELP
- **Hex notation**: Servo channels use hex digits (0-9,A-F) for up to 16 servos
- **POSE command**: Sets multiple servos simultaneously for coordinated movement
- Validates servo channels (0-5 configurable via NUM_SERVOS) and angles (0-180)
- Updates PCA9685 PWM values accordingly
- Returns to button mode on STOP command

**Note:** Current configuration supports 6 servos (NUM_SERVOS=6 in serial_commands.h). The PCA9685 hardware supports up to 16 channels - to use all 16, change NUM_SERVOS and update main.c button mode limits.

### Key Design Patterns

**Serial Input Handling (Blocking by Design):**
- `uart_available()` performs non-blocking check for data
- Once data is detected, `read_command_line()` blocks waiting for newline
- This is intentional: prevents partial command processing
- In button mode, ensures complete commands are received before processing
- In serial mode, `serial_mode()` blocks in command loop until STOP received

**Direct PWM Control:**
- PCA9685 driver converts angles (0-180°) to PWM pulse widths
- Standard servo timing: 1ms (0°) to 2ms (180°) at 50Hz

### Common Development Tasks

**Adding new servo positions:**
- Modify angle in `pca9685_set_servo_angle()` calls
- Valid range: 0-180 degrees

**Changing servo update speed:**
- Adjust delays in main loop (currently 50ms LCD update, 100ms servo update)

**Adding serial commands:**
- Add command parsing in `process_command()` in serial_commands.c
- Follow existing pattern: parse, validate, execute, respond

**Modifying servo timing:**
- Adjust pulse width calculations in `pca9685_set_servo_angle()`
- Default: 150-600 (1ms-2ms pulse width at 50Hz)

