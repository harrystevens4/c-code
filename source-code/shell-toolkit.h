#ifndef SHELL_TOOLKIT
#define SHELL_TOOLKIT
char *get_calling_process_executable();
int connected_to_tty();
int connected_pipe_input();
int connected_pipe_output();
char *get_pipe_input_bash_script_location();
#endif
