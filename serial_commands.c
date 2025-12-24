#include "serial_commands.h"
#include "uart.h"
#include "lcd.h"
#include "commands.h"
#include <string.h>
#include <util/delay.h>

/* Command buffer */
static char cmd_buffer[CMD_BUFFER_SIZE];
static uint8_t cmd_index = 0;

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

        // Echo character back (disabled for fast burst sends)
        // uart_putc(c);

        // Handle newline/carriage return
        if (c == '\n' || c == '\r') {
            // uart_putc('\n');  // Disabled - no echo for fast sends
            cmd_buffer[cmd_index] = '\0';
            return cmd_buffer;
        }

        // Handle backspace
        if (c == '\b' || c == 127) {
            if (cmd_index > 0) {
                cmd_index--;
                // uart_puts("\b \b");  // Disabled - no echo for fast sends
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
 * Skip whitespace characters (space and tab)
 * Advances pointer past any whitespace
 */
static void skip_whitespace(const char **p) {
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

/*
 * Parse unsigned 16-bit integer from string
 * Returns 1 on success, 0 on error (no digits found)
 * Updates pointer to first non-digit character
 */
static uint8_t parse_uint16(const char **p, uint16_t *result) {
    // Check if first character is a digit
    if (**p < '0' || **p > '9') {
        return 0;  // Error: no digit found
    }

    *result = 0;
    while (**p >= '0' && **p <= '9') {
        *result = *result * 10 + (**p - '0');
        (*p)++;
    }

    return 1;  // Success
}

/*
 * Skip comma separator with optional surrounding whitespace
 * Returns 1 if at end of string (valid), 0 if unexpected character
 */
static uint8_t skip_comma(const char **p) {
    skip_whitespace(p);

    if (**p == ',') {
        (*p)++;
        return 1;
    } else if (**p == '\0') {
        return 1;  // End of string is valid
    } else {
        return 0;  // Unexpected character
    }
}

/*
 * Parse comma-separated list of angles
 * Returns number of angles parsed (0 on error)
 */
static uint8_t parse_angle_list(const char **p, uint8_t *angles, uint8_t max_count) {
    uint8_t count = 0;

    while (**p != '\0' && count < max_count) {
        skip_whitespace(p);
        if (**p == '\0') break;

        // Parse angle
        uint16_t angle;
        if (!parse_uint16(p, &angle)) {
            return 0;  // Parse error
        }

        // Validate angle range
        if (angle > 180) {
            return 0;  // Invalid angle
        }

        angles[count++] = (uint8_t)angle;

        // Skip comma or check for end
        if (!skip_comma(p)) {
            return 0;  // Unexpected character
        }
    }

    return count;
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

    // Parse angle
    const char *p = colon + 1;
    uint16_t angle;
    if (!parse_uint16(&p, &angle) || angle > 180) {
        return CMD_INVALID_ANGLE;
    }

    // Execute command
    cmd_set_servo_angle(channel, (uint8_t)angle);

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
    uint8_t angle = cmd_get_servo_angle(channel);
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
    uint8_t angles[NUM_SERVOS];

    // Parse angle list
    uint8_t num_servos = parse_angle_list(&p, angles, NUM_SERVOS);
    if (num_servos == 0) {
        return CMD_ERROR;  // Parse error or no angles
    }

    // Execute POSE command
    cmd_execute_pose(angles, num_servos);

    return CMD_OK;
}

/*
 * Execute MOVE command (smooth interpolated movement)
 * Format: MOVE <duration_ms> <angle1>,<angle2>,...
 * Smoothly moves servos to target positions over specified duration
 * All servos finish at the same time (synchronized linear interpolation)
 */
static uint8_t execute_move_command(const char *cmd) {
    // Expected format: MOVE <duration> <angle1>,<angle2>,...
    // Skip "MOVE " (5 characters)
    if (strlen(cmd) < 7) {  // Minimum: "MOVE 0 0"
        return CMD_ERROR;
    }

    const char *p = cmd + 5;  // Skip "MOVE "

    // Parse duration (milliseconds)
    skip_whitespace(&p);
    uint16_t duration_ms;
    if (!parse_uint16(&p, &duration_ms)) {
        return CMD_ERROR;
    }

    // Skip whitespace before angles
    skip_whitespace(&p);

    // Parse target angles
    uint8_t target_angles[NUM_SERVOS];
    uint8_t num_servos = parse_angle_list(&p, target_angles, NUM_SERVOS);
    if (num_servos == 0) {
        return CMD_ERROR;
    }

    // Execute MOVE command
    cmd_execute_move(duration_ms, target_angles, num_servos);

    return CMD_OK;
}

/*
 * Send help message
 */
void serial_send_help(void) {
    uart_puts("\n=== Robot Arm Serial Commands ===\n");
    uart_puts("START              - Enter serial control mode\n");
    uart_puts("STOP               - Exit serial control mode\n");
    uart_puts("S<n>:<angle>       - Set servo n to angle (0-180)\n");
    uart_puts("                     n = 0-9,A-F (hex)\n");
    uart_puts("                     Example: S0:90, S5:45, SA:120\n");
    uart_puts("POSE <angles>      - Set multiple servos instantly\n");
    uart_puts("                     Example: POSE 90,45,120,90,60,30\n");
    uart_puts("                     Sets servos 0,1,2,3,4,5\n");
    uart_puts("MOVE <ms> <angles> - Smooth move over duration (ms)\n");
    uart_puts("                     Example: MOVE 2000 90,45,120,90,60,30\n");
    uart_puts("                     Moves to angles over 2 seconds\n");
    uart_puts("                     All servos finish simultaneously\n");
    uart_puts("GET <n>            - Query servo n position\n");
    uart_puts("                     Example: GET 0, GET A\n");
    uart_puts("HELP               - Show this help message\n");
    uart_puts("=====================================\n");
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

    // MOVE command
    if (strncmp(cmd, "MOVE ", 5) == 0 || strncmp(cmd, "move ", 5) == 0) {
        uint8_t result = execute_move_command(cmd);
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
                uart_puts("ERROR: Invalid MOVE format\n");
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
