#ifndef SERIAL_COMMANDS_H
#define SERIAL_COMMANDS_H

#include <stdint.h>

/*
 * Serial Command Interface for Robot Arm Control
 *
 * Commands:
 * - START       : Enter serial control mode
 * - STOP        : Exit serial control mode
 * - S<n>:<angle>: Set servo n to angle (e.g., S0:90)
 * - GET <n>     : Query servo n position
 * - HELP        : List available commands
 */

/* Command buffer size */
#define CMD_BUFFER_SIZE 32

/* Command result codes */
#define CMD_OK              0
#define CMD_ERROR           1
#define CMD_EXIT            2
#define CMD_UNKNOWN         3
#define CMD_INVALID_SERVO   4
#define CMD_INVALID_ANGLE   5

/**
 * Initialize serial command system
 */
void serial_commands_init(void);

/**
 * Enter serial control mode
 * Blocking function that processes commands until STOP is received
 *
 * Commands accepted:
 * - STOP        : Exit serial mode (returns CMD_EXIT)
 * - S0:90       : Set servo 0 to 90 degrees
 * - GET 0       : Query servo 0 position
 * - HELP        : Show help message
 */
void serial_mode(void);

/**
 * Check if START command was received
 * Non-blocking check for entering serial mode
 *
 * @return 1 if START command received, 0 otherwise
 */
uint8_t serial_check_start(void);

/**
 * Send help message
 */
void serial_send_help(void);

#endif /* SERIAL_COMMANDS_H */
