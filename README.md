# Robot Arm Controller

ATmega328p-based robot arm controller with 16-channel servo control. Features dual control modes: manual button control via LCD Keyboard Shield and serial command interface over UART.

## Features

- **6-Servo Control**: Controls 6 servos (0-180°) via PCA9685 16-channel PWM driver (expandable to 16)
- **Dual Control Modes**:
  - **Button Mode**: Manual control using LCD Keyboard Shield (UP/DOWN/LEFT/RIGHT/SELECT buttons)
  - **Serial Mode**: Command-line interface over UART for programmatic control
- **LCD Feedback**: Real-time display of selected servo and angle on 16x2 LCD
- **I2C Communication**: Interfaces with PCA9685 PWM driver
- **UART Serial**: 115200 baud serial communication on /dev/ttyUSB0

## Hardware Requirements

- **MCU**: ATmega328p (Arduino Uno/Mini)
- **Clock**: 16 MHz crystal
- **PWM Driver**: PCA9685 16-channel PWM/Servo driver (I2C)
- **Display**: LCD Keyboard Shield (16x2 LCD with 5 buttons)
- **Servos**: Up to 16 standard servos (50Hz PWM)
- **Serial**: USB-to-serial adapter on /dev/ttyUSB0

## Build & Flash

```bash
make              # Compile all modules and generate main.hex
make clean        # Remove compiled binaries
./flash.sh        # Flash to ATmega328p via avrdude
```

## Serial Commands

Connect to `/dev/ttyUSB0` at 115200 baud:

```
START            - Enter serial control mode (blocking)
STOP             - Exit serial control mode
S<n>:<angle>     - Set servo n to angle (0-180)
                   n = 0-5 (hex: 0-9,A-F for up to 16 servos)
                   Examples: S0:90, S1:45, S5:180, SA:120
POSE <angles>    - Set multiple servos at once (coordinated movement)
                   Examples: POSE 90,45,120,90,60,30
                   Sets servos 0,1,2,3,4,5 to specified angles
GET <n>          - Query current position of servo n (hex)
                   Examples: GET 0, GET 5, GET A
HELP             - Display command help
```

**Note:** Serial mode is **blocking by design**. Once you type any command in button mode, the system waits for a complete line (newline character). After entering serial mode with START, button controls are disabled until you exit with STOP.

## Button Controls

- **LEFT/RIGHT**: Select servo (0-5)
- **UP/DOWN**: Increase/decrease servo angle (±5°)
- **SELECT**: Reset selected servo to center (90°)

## Code Structure

- `main.c` - Main program loop and mode switching
- `lcd.c/h` - LCD Keyboard Shield driver
- `buttons.c/h` - Button input handling
- `pca9685.c/h` - PCA9685 PWM driver (I2C)
- `i2c.c/h` - I2C communication layer
- `uart.c/h` - UART serial communication
- `serial_commands.c/h` - Serial command parser and handler
