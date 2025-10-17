#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv){
	setenv("LD_PRELOAD","/usr/lib/libpr.so",1);
	argv[0] = "-fish";
	return execv("/usr/bin/fish",argv);
}
