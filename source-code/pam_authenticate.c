#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
char *get_user();
int pam_conversation_func();
int main(int argc, char **argv){
	//variables
	int return_status = EXIT_SUCCESS;
	int result; //generic
	char *user;
	char *password = "a"; //get from user input

	//prep
	user = get_user();
	if (user == NULL){
		fprintf(stderr,"main() error: could not get user.\n");
		return EXIT_FAILURE;
	}

	//pam related
	int pam_result; //needs to be passed to pam_end at the end
	const char *conversation_data[2] = {user,password};
	struct pam_conv conversation = {pam_conversation_func,conversation_data};
	pam_handle_t *pam_handle;


	//start pam
	pam_result = pam_start("other",user,&conversation,&pam_handle);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"pam_start() error: %s\n",pam_strerror(pam_handle,pam_result));
		return_status = EXIT_FAILURE;
		goto end;
	}

	//end pam
	end:
	result = pam_end(pam_handle,result);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"pam_end() error: %s\n",pam_strerror(pam_handle,pam_result));
		return EXIT_FAILURE;
	}
	return return_status;
}
char *get_user(){
	//check env variables first
	char *user = getenv("USER");
	if (user != NULL) return user;

	//try extrapolate it from their uid
	uid_t uid = getuid(); //always successfull (apparently)
	//get passwd struct which contains user
	struct passwd *password = getpwuid(uid);
	return password->pw_name;
}
int pam_conversation_func(){
}
