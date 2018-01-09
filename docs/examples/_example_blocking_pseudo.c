/* Execute command 1 in blocking mode */
if (cmd1(param1, param2, 1) == espOK) {         /* Execute cmd 1 in blocking mode and read response after command executed */
    if (cmd2(param3, param4, 1) == espOK) {     /* Execute cmd 2 in blocking mode and read response after command executed */
        /* Do some job after 2 commands were successfully executed */
    }
}