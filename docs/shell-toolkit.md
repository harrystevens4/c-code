
# How to use

## `int connected_to_tty`

This function checks if both standard input and standard output are both a tty.
It returns 1 for true and 0 for false (designed to be used within an if statement).

## `char *get_calling_process_executable`

This function returns a string containing the calling process executable. If called when the program is connected to a pipe output, will find the executable location of the input to the pipe.

## `int connected_pipe_output` and `int connected_pipe_input`

If the program is connected to the input of a pipe `connected_pipe_input` will return 1.
Same for pipe output and `connected_pipe_output`.
