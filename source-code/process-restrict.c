#include <dlfcn.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int (*old_execve)(const char *,char *const *argv,char *const *envp);
char banned_commands[][256] = {"java","node"};

void __attribute__((__constructor__)) init(){
	//store the old execve
	old_execve = dlsym(RTLD_NEXT,"execve");
}

static int check_if_allowed(const char *path, char * const* argv, char * const* envp){
	//====== resolve symboliclinks ======
	char resolved_link[4097] = {0};
	memset(resolved_link,0,sizeof(resolved_link));
	if (readlink(path,resolved_link,sizeof(resolved_link)) < 0){
		strcpy(resolved_link,path);
	}
	//====== deny setuid and setgid ======
	struct stat statbuf = {0};
	if (stat(resolved_link,&statbuf) < 0) return -1;
	if ((statbuf.st_mode & (S_ISGID | S_ISUID)) != 0){
		errno = EPERM;
		return -1;
	}
	//====== check for banned commands ======
	for (size_t i = 0; i < sizeof(banned_commands)/256; i++){
		if (strcmp(basename(resolved_link),banned_commands[i]) == 0){
			errno = EPERM;
			return -1;
		}
	}
	//printf("executing %s\n",resolved_link);
	return 0;
}

int execve(const char *path, char *const *argv, char *const *envp){
	if (check_if_allowed(path,argv,envp) != 0) return -1;
	return old_execve(path,argv,envp);
}
