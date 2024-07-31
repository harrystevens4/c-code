#include <stdio.h>
#include "shell-toolkit.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define MAXBUF      (BUFSIZ * 2)

int pgetppid(int pid);
int main(){
	char buffer[1024];
	char script_location[1024];
	get_pipe_input_bash_script_location(script_location,1024);
	while (1){
		fgets(buffer,1024,stdin);
		printf("%s\n",buffer);
	}
	return 0;
}
int pgetppid(int pid) {
    int ppid;
    char buf[MAXBUF];
    char procname[32];  // Holds /proc/4294967296/status\0
    FILE *fp;

    snprintf(procname, sizeof(procname), "/proc/%u/status", pid);
    fp = fopen(procname, "r");
    if (fp != NULL) {
        size_t ret = fread(buf, sizeof(char), MAXBUF-1, fp);
        if (!ret) {
            return 0;
        } else {
            buf[ret++] = '\0';  // Terminate it.
        }
    }
    fclose(fp);
    char *ppid_loc = strstr(buf, "\nPPid:");
    if (ppid_loc) {
        int ret = sscanf(ppid_loc, "\nPPid:%d", &ppid);
        if (!ret || ret == EOF) {
            return 0;
        }
        return ppid;
    } else {
        return 0;
    }

}
