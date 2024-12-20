#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
char *get_user();
int pam_conversation_func(int num_msg, const struct pam_message **msg, struct pam_response **resp,void *appdata_ptr); //callback function
int login(char *username, char *password,pam_handle_t **pam_handle);
int logout(pam_handle_t *pam_handle);
int main(int argc, char **argv){
	//variables
	int result; //generic
	char *username;
	char *password = "a"; //get from user input

	//prep
	username = get_user();
	if (username == NULL){
		fprintf(stderr,"main() error: could not get user.\n");
		return EXIT_FAILURE;
	}
	username = "a";

	//pam related
	pam_handle_t *pam_handle;
	
	//login
	result = login(username,password,&pam_handle); //will clean itself up if failed
	if (result < 0) return 1;
	printf("logged in.\n");
	//logout
	result = logout(pam_handle);
	if (result < 0) return 1;
	printf("logged out.\n");
	return 0;
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
int pam_conversation_func(int num_msg, const struct pam_message **msg, struct pam_response **resp,void *appdata_ptr){
	int pam_result = PAM_SUCCESS;
	//appdata_ptr = conversation_data from previous struct
	char *conversation_data = (char*)appdata_ptr;
	*response = malloc(sizeof(struct pam_response)); //alloc into the pointer we are provided
	if (response == NULL){
		return PAM_BUF_ERR;
	}
	
	//deal with messages
	for (int i = 0; i < num_msg; i++){ //its almost like an argc and argv of messages
		switch( msg[i]->msg_style){
		case PAM_TEXT_INFO:
			printf("%s\n",msg[i]->msg);
			break;
		}
		//stop when an error has occured
		if (pam_result != PAM_SUCCESS){
			break;
		}
	}

	if (pam_result != PAM_SUCCESS){
		free(*response);
		*response = 0; //return an error
	}

	return PAM_SUCCESS;
}
int login(char *username, char *password,pam_handle_t **pam_handle){
	int return_status = -1; //returned when there is an error
	int pam_result; //needs to be passed to pam_end at the end
	int result = 0; //for other glibc functions
	char *conversation_data[2] = {username,password};
	struct pam_conv conversation = {pam_conversation_func,conversation_data};


	//start pam
	pam_result = pam_start("login",username,&conversation,pam_handle);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"login(): pam_start() error: %s\n",pam_strerror(*pam_handle,pam_result));
		return_status = EXIT_FAILURE;
		goto end;
	}

	//authenitcate
	pam_result = pam_authenticate(*pam_handle,0);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"login(): pam_authenticate() error: %s\n",pam_strerror(*pam_handle,pam_result));
		goto end;
	}
	
	//-----successfull exit-------
	return 0;
	//---only comes here on failure---
	end:
	result = pam_end(*pam_handle,result);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"logout(): pam_end() error: %s\n",pam_strerror(*pam_handle,pam_result));
	}
	return return_status;
}
int logout(pam_handle_t *pam_handle){
	int pam_result = PAM_SUCCESS; //placeholder so pam_end doesnt bug
	int result = 0;
	int return_status = 0;
	end:
	result = pam_end(pam_handle,result);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"logout(): pam_end() error: %s\n",pam_strerror(pam_handle,pam_result));
		return EXIT_FAILURE;
	}
	return return_status;
}
