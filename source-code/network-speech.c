#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../lib/external/speechd/libspeechd.h"

#define SOCKET_PATH "speech-dispatcher/speechd.sock"

int main(int argc, char**argv){
	//get user
	char *user = getenv("USER");
	if (user == NULL){
		fprintf(stderr,"FATAL: Could not get user.\n");
		return 1;
	}
	
	//connect to the speech dispatcher
	SPDConnection *dispatcher;
	//                    client_name        connection_name user
	dispatcher = spd_open("dispatch-speech", "main",         (const char *)user,SPD_MODE_SINGLE);

	//send a message
	SPDPriority priority = SPD_TEXT;
	spd_say(dispatcher,priority,"what if, hear me out here, this program worked first try");

	//cleanup
	spd_close(dispatcher);
	return 0;
}
