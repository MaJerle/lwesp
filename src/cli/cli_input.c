#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "cli/cli.h"
#include "cli/cli_input.h"
#include "cli/cli_config.h"

/* Statically allocate CLI buffer to eliminate overhead of using */
static char cmd_buffer[CLI_MAX_CMD_LENGTH] = {{0}};
static uint32_t cmd_pos = 0;

/**
 * \brief           Special key sequence check
 * \param[in]       ch: input char from CLI
 * \return          true when special key sequence is active, else false
 */
static bool cli_special_key_check(char ch) {
    static uint32_t key_sequence = 0;

    if (key_sequence == 0 && ch == 27) {
        key_sequence = 1;
        return true;
    }
    else if (key_sequence == 1 && ch == '[') {
        key_sequence = 2;
        return true;
    }
    else if (key_sequence == 2 && ch >= 'A' && ch <= 'D') {
        key_sequence = 0;
        //char * histBuf;
        switch (ch) {
            case 'A': /* up */
                // TODO for now don't do anything (need history)
                break;
            case 'B': /* down */
                // TODO for now don't do anything (need history)
                break;
            case 'C': /* right */
                // TODO for now don't do anything (need courser location)
                break;
            case 'D': /* left */
                // TODO for now don't do anything (need courser location)
                break;
        }
        return true;
    }

    return false;
}

/**
 * \brief           parse and execute the given command
 * \param[in]       cliprintf: Pointer CLI printf function
 * \param[in]       input: input string to parse
 * \return          true when command is found and parsed, else false
 */
static bool cli_parse_and_execute_command(cli_printf cliprintf, char *input) {
    const cli_command_t *command;
    char * argv[CLI_MAX_NUM_OF_ARGS];
    uint32_t argc = 0;

    argv[argc] = strtok(input, " ");
    while (argv[argc] != NULL) {
        argv[++argc] = strtok (NULL, " ");
    }

    if ((command = cli_lookup_command(argv[0])) == NULL) {
        cliprintf("Unknown command: %s\n", argv[0]);
        return false;
    } else {
        command->func(cliprintf, argc, argv);
    }

    return true;
}

/**
 * \brief           clear the command buffer and reset the position
 */
static void clear_cmd_buffer( void ) {
    memset(cmd_buffer, 0x0, sizeof(cmd_buffer));
    cmd_pos = 0;
}

/**
 * \brief           parse new characters to the CLI
 * \param[in]       cliprintf: Pointer CLI printf function
 * \param[in]       ch: new character to CLI
 */
void cli_in_data(cli_printf cliprintf, char ch) {
    static char last_key = 0;

    if (!cli_special_key_check(ch)) {
        /* Parse the characters only if they are not part of the special key sequence */
        switch (ch) {
            /* Backspace */
            case '\b':
            case 127:
                // TODO
                break;
            /* Tab for autocomplete */
            case '\t':
                cli_tab_auto_complete(cliprintf, cmd_buffer, &cmd_pos, (last_key == '\t'));
                break;
            /* New line -> new command */
            case '\n':
            case '\r':
                //TODO store history
                if (strlen(cmd_buffer) == 0) {
                    clear_cmd_buffer();
                    cliprintf(CLI_NL CLI_PROMPT);
                    return;
                }

                cliprintf(CLI_NL);
                cli_parse_and_execute_command(cliprintf, cmd_buffer);

                clear_cmd_buffer();
                cliprintf(CLI_NL CLI_PROMPT);
                break;
            /* All other chars */
            default:
                if (cmd_pos < CLI_MAX_CMD_LENGTH) {
                    cmd_buffer[cmd_pos++] = ch;
                }
                else {
                    clear_cmd_buffer();
                    cliprintf(CLI_NL"\aERR: Command to long"CLI_NL CLI_PROMPT);
                    return;
                }
                cliprintf("%c", ch);
        }
    }

    /* Store last key for double key detection */
    last_key = ch;
}

