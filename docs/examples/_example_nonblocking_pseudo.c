/* Start command in non-blocking mode */
if (cmd1(param1, param2, 0) == espOK) {
    /* Cmd 1 was created and sent to message queue for processing */
}

/* Callback function for command */
callback_fn(evt) {
    if (evt == cmd1_evt) {
        /* Command was executed */
        /* Send another, must always be sent non-blocking way! */
        cmd2(param2, param3, 0);
    }
}