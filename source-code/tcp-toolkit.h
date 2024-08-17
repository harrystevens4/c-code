#ifndef TCP_TOOLKIT_H //include guard
#define TCP_TOOLKIT_H

int make_server_socket(const char * port, int backlog);//sets up a listening server socket

extern int verbose_tcp_toolkit; //debugging info from tcp-toolkit functions

#endif
