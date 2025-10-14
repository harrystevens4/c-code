#include <dlfcn.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int (*old_execve)(const char *,char *const *argv,char *const *envp);

void __attribute__((__constructor__)) init(){
	//load the banned word list in a shared memory object
	old_execve = dlsym(RTLD_NEXT,"execve");
}

int execve(const char *path, char *const *argv, char *const *envp){
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
	printf("executing %s\n",resolved_link);
	return old_execve(path,argv,envp);
}
