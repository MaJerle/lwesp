//Execute command 1 in blocking mode
if (cmd1(param1, param2, 1) == espOK) {         // Execute cmd 1 in blocking mode and read response immediatelly
    if (cmd2(param3, param4, 1) == espOK) {     // Execute cmd 2 in blocking mode and read response immediatelly
        // Do some job after 2 commands were successfully executed
    }
}