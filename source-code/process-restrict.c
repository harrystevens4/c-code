#include <dlfcn.h>
#include <stdlib.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int (*old_execve)(const char *,char *const *argv,char *const *envp);
char banned_commands[][256] = {"java","node","ssh"};
char *ld_preload_value;

void __attribute__((__constructor__)) init(){
	//store the old execve
	old_execve = dlsym(RTLD_NEXT,"execve");
	ld_preload_value = strdup(getenv("LD_PRELOAD"));
}

static int check_if_allowed(const char *path, char * const* argv, char * const* envp){
	//====== resolve symbolic links ======
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
	//====== check LD_PRELOAD is untampered with ======
	int ld_preload_found = 0;
	for (int i = 0; envp[i] != NULL; i++){
		char *env_var = envp[i];
		//compare everything before the '=' to LD_PRELOAD
		if (strncmp("LD_PRELOAD",env_var,strchr(env_var,'=')-env_var) == 0){
			ld_preload_found = 1;
			if (strcmp(strchr(env_var,'=')+1,ld_preload_value) != 0){
				//LD_PRELOAD has been tampered with
				errno = EPERM;
				return -1;
			}
			printf("%s == %s\n",strchr(env_var,'=')+1,ld_preload_value);
		}
	}
	if (!ld_preload_found){
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
