#include "serial_commands.h"
#include "uart.h"
#include "lcd.h"
#include "pca9685.h"
#include <string.h>
#include <util/delay.h>

/* Command buffer */
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_index = 0;

/* Current servo angles (for GET command) */
static uint8_t servo_angles[16] = {90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90, 90};  // Default 90° for all 16 channels

/*
 * Initialize serial command system
 */
void serial_commands_init(void) {
    cmd_index = 0;
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);
}

/*
 * Read a complete command line
 * Blocks until newline received or buffer full
 * Returns pointer to command string
 */
static char* read_command_line(void) {
    cmd_index = 0;
    memset(cmd_buffer, 0, CMD_BUFFER_SIZE);

    while (1) {
        // Wait for character (blocking)
        char c = uart_getc();

        // Echo character back
        uart_putc(c);

        // Handle newline/carriage return
        if (c == '\n' || c == '\r') {
            uart_putc('\n');  // Ensure newline
            cmd_buffer[cmd_index] = '\0';
            return cmd_buffer;
        }

        // Handle backspace
        if (c == '\b' || c == 127) {
            if (cmd_index > 0) {
                cmd_index--;
                uart_puts("\b \b");  // Erase character on terminal
            }
            continue;
        }

        // Add to buffer if space available
        if (cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
        }
    }
}

/*
 * Parse hex digit (0-9, A-F, a-f) to value
 * Returns 0-15 on success, 0xFF on error
 */
static uint8_t parse_hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0xFF;  // Invalid
}

/*
 * Convert value (0-15) to hex character
 * Returns '0'-'9', 'A'-'F'
 */
static char value_to_hex(uint8_t value) {
    if (value < 10) {
        return '0' + value;
    }
    return 'A' + (value - 10);
}

/*
 * Parse and execute a servo command (S0:90 format)
 * Returns CMD_OK on success, error code otherwise
 */
static uint8_t execute_servo_command(const char *cmd) {
    // Expected format: S<channel>:<angle>
    // Example: S0:90, S1:45, SA:180 (hex: 0-9, A-F for channels 0-15)

    // Check minimum length and format
    if (strlen(cmd) < 4) {  // Minimum: "S0:0"
        return CMD_ERROR;
    }

    if (cmd[0] != 'S' && cmd[0] != 's') {
        return CMD_UNKNOWN;
    }

    // Parse servo channel (hex digit: 0-9, A-F)
    uint8_t channel = parse_hex_digit(cmd[1]);
    if (channel == 0xFF || channel >= NUM_SERVOS) {
        return CMD_INVALID_SERVO;
    }

    // Find colon separator
    const char *colon = strchr(cmd, ':');
    if (!colon) {
        return CMD_ERROR;
    }

    // Parse angle (simple atoi)
    uint16_t angle = 0;
    const char *p = colon + 1;
    while (*p >= '0' && *p <= '9') {
        angle = angle * 10 + (*p - '0');
        p++;
    }

    // Validate angle
    if (angle > 180) {
        return CMD_INVALID_ANGLE;
    }

    // Execute command
    pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, channel, (uint8_t)angle);

    // Store angle for GET command
    servo_angles[channel] = (uint8_t)angle;

    return CMD_OK;
}

/*
 * Execute GET command (query servo position)
 */
static uint8_t execute_get_command(const char *cmd) {
    // Expected format: GET <channel>
    // Example: GET 0, GET A (hex: 0-9, A-F for channels 0-15)

    // Skip "GET " (4 characters)
    if (strlen(cmd) < 5) {
        return CMD_ERROR;
    }

    // Parse servo channel (hex digit: 0-9, A-F)
    uint8_t channel = parse_hex_digit(cmd[4]);
    if (channel == 0xFF || channel >= NUM_SERVOS) {
        return CMD_INVALID_SERVO;
    }

    // Send response
    uart_puts("SERVO ");
    uart_putc(value_to_hex(channel));
    uart_puts(": ");

    // Send angle (simple integer to string)
    uint8_t angle = servo_angles[channel];
    if (angle >= 100) {
        uart_putc((angle / 100) + '0');
    }
    if (angle >= 10) {
        uart_putc(((angle / 10) % 10) + '0');
    }
    uart_putc((angle % 10) + '0');
    uart_puts(" degrees\n");

    return CMD_OK;
}

/*
 * Execute POSE command (set multiple servos sequentially)
 * Format: POSE 90,45,120,90,60,30
 * Sets servos 0,1,2,3,4,5 to the specified angles
 * Number of values determines how many servos are set
 */
static uint8_t execute_pose_command(const char *cmd) {
    // Expected format: POSE <angle1>,<angle2>,...
    // Skip "POSE " (5 characters)
    if (strlen(cmd) < 6) {
        return CMD_ERROR;
    }

    const char *p = cmd + 5;  // Skip "POSE "
    uint8_t servo_index = 0;

    while (*p != '\0' && servo_index < NUM_SERVOS) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') break;

        // Parse angle
        uint16_t angle = 0;
        if (*p < '0' || *p > '9') {
            return CMD_ERROR;  // Expected digit
        }
        while (*p >= '0' && *p <= '9') {
            angle = angle * 10 + (*p - '0');
            p++;
        }

        // Validate angle
        if (angle > 180) {
            return CMD_INVALID_ANGLE;
        }

        // Set servo position
        pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, servo_index, (uint8_t)angle);
        servo_angles[servo_index] = (uint8_t)angle;
        servo_index++;

        // Skip comma
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == ',') {
            p++;
        } else if (*p != '\0') {
            return CMD_ERROR;  // Unexpected character
        }
    }

    // Must have set at least one servo
    if (servo_index == 0) {
        return CMD_ERROR;
    }

    return CMD_OK;
}

/*
 * Send help message
 */
void serial_send_help(void) {
    uart_puts("\n=== Robot Arm Serial Commands ===\n");
    uart_puts("START          - Enter serial control mode\n");
    uart_puts("STOP           - Exit serial control mode\n");
    uart_puts("S<n>:<angle>   - Set servo n to angle (0-180)\n");
    uart_puts("                 n = 0-9,A-F (hex)\n");
    uart_puts("                 Example: S0:90, S5:45, SA:120\n");
    uart_puts("POSE <angles>  - Set multiple servos at once\n");
    uart_puts("                 Example: POSE 90,45,120,90,60,30\n");
    uart_puts("                 Sets servos 0,1,2,3,4,5\n");
    uart_puts("GET <n>        - Query servo n position\n");
    uart_puts("                 Example: GET 0, GET A\n");
    uart_puts("HELP           - Show this help message\n");
    uart_puts("===================================\n");
}

/*
 * Process a single command
 * Returns CMD_EXIT to exit serial mode, CMD_OK to continue
 */
static uint8_t process_command(const char *cmd) {
    // Skip empty commands
    if (strlen(cmd) == 0) {
        return CMD_OK;
    }

    // STOP command - exit serial mode
    if (strcmp(cmd, "STOP") == 0 || strcmp(cmd, "stop") == 0) {
        uart_puts("Exiting serial mode\n");
        return CMD_EXIT;
    }

    // HELP command
    if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "help") == 0) {
        serial_send_help();
        return CMD_OK;
    }

    // Servo command (S0:90 format)
    if (cmd[0] == 'S' || cmd[0] == 's') {
        uint8_t result = execute_servo_command(cmd);
        switch (result) {
            case CMD_OK:
                uart_puts("OK\n");
                break;
            case CMD_INVALID_SERVO:
                uart_puts("ERROR: Invalid servo (must be 0-");
                uart_putc(value_to_hex(NUM_SERVOS - 1));
                uart_puts(" hex)\n");
                break;
            case CMD_INVALID_ANGLE:
                uart_puts("ERROR: Invalid angle (must be 0-180)\n");
                break;
            default:
                uart_puts("ERROR: Invalid command format\n");
                break;
        }
        return CMD_OK;
    }

    // GET command
    if (strncmp(cmd, "GET ", 4) == 0 || strncmp(cmd, "get ", 4) == 0) {
        uint8_t result = execute_get_command(cmd);
        if (result != CMD_OK) {
            uart_puts("ERROR: Invalid GET command\n");
        }
        return CMD_OK;
    }

    // POSE command
    if (strncmp(cmd, "POSE ", 5) == 0 || strncmp(cmd, "pose ", 5) == 0) {
        uint8_t result = execute_pose_command(cmd);
        switch (result) {
            case CMD_OK:
                uart_puts("OK\n");
                break;
            case CMD_INVALID_SERVO:
                uart_puts("ERROR: Too many servos (max ");
                uart_putc(value_to_hex(NUM_SERVOS - 1));
                uart_puts(")\n");
                break;
            case CMD_INVALID_ANGLE:
                uart_puts("ERROR: Invalid angle (must be 0-180)\n");
                break;
            default:
                uart_puts("ERROR: Invalid POSE format\n");
                break;
        }
        return CMD_OK;
    }

    // Unknown command
    uart_puts("ERROR: Unknown command (type HELP for list)\n");
    return CMD_OK;
}

/*
 * Enter serial control mode
 * Blocking function that processes commands until STOP is received
 */
void serial_mode(void) {
    // Send welcome message
    uart_puts("\n=== SERIAL MODE ACTIVE ===\n");
    uart_puts("Type HELP for command list\n");
    uart_puts("Type STOP to exit\n\n");

    // Update LCD
    lcd_clear();
    lcd_print("Serial Mode");

    // Main command loop
    while (1) {
        uart_puts("> ");  // Prompt

        // Read command
        char *cmd = read_command_line();

        // Process command
        uint8_t result = process_command(cmd);

        // Exit if STOP command
        if (result == CMD_EXIT) {
            break;
        }
    }

    // Exiting serial mode
    uart_puts("\n=== BUTTON MODE ACTIVE ===\n");
    lcd_clear();
    lcd_print("Button Mode");
    _delay_ms(500);
}

/*
 * Check if START command was received
 * Non-blocking check for entering serial mode
 */
uint8_t serial_check_start(void) {
    if (!uart_available()) {
        return 0;
    }

    // Read the command
    char *cmd = read_command_line();

    // Check if it's START
    if (strcmp(cmd, "START") == 0 || strcmp(cmd, "start") == 0) {
        return 1;
    }

    // Not START, could be HELP
    if (strcmp(cmd, "HELP") == 0 || strcmp(cmd, "help") == 0) {
        serial_send_help();
        uart_puts("\nType START to enter serial mode\n");
    } else {
        uart_puts("Type START to enter serial mode\n");
    }

    return 0;
}
