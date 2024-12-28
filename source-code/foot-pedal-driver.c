#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
int simulate_keypress(char *key, int pressed);
int get_pedal_state(int pedal_fd);
int main(int argc, char **argv){
	char *device = "/dev/ttyACM0";
	int pedal_fd = open(device,O_RDONLY);
	if (pedal_fd < 0){
		perror("open");
		return 1;
	}
	for (;;){
		static int previous_pedal_state = 0;
		static int new_pedal_state = 0;
		new_pedal_state = get_pedal_state(pedal_fd);
		if (new_pedal_state < 0){
			fprintf(stderr,"main(): get_pedal_state error\n");
			close(pedal_fd);
			return 1;	
		}
		//lets not waste system resources
		//usleep(100000); //100,000us = 0.1s
		if (previous_pedal_state != new_pedal_state){
			printf("%d\n",new_pedal_state);
			simulate_keypress("Control_L",new_pedal_state);
		}
		previous_pedal_state = new_pedal_state;
	}
	close(pedal_fd);
	return 0;
}
int get_pedal_state(int pedal_fd){
	char state[2] = "0";

	int result = read(pedal_fd,&state,sizeof(char));
	if (result < 0){
		perror("read");
		return -1;
	}
	return strtol(state,NULL,10);
}
int simulate_keypress(char *key, int pressed){
	Display *display = XOpenDisplay(NULL);
	if (display == NULL){
		fprintf(stderr,"Could not connect to X display.\n");
		return -1;
	}
	Window root_window = XDefaultRootWindow(display);
	int revert; //we dont use this
	Window focused_window;
	int result = XGetInputFocus(display,&focused_window,&revert);
	
	//im manualy packing the struct
	XKeyEvent key_event = {
		.display = display,
		.window = focused_window,
		.root = root_window,
		.subwindow = None,
		.time = CurrentTime,
		.x = 1,
		.y = 1,
		.x_root = 1,
		.y_root = 1,
		.same_screen = True,
		.keycode = XKeysymToKeycode(display,XStringToKeysym(key)),
		.state = 0,
		.type = (pressed == 1) ? KeyPress : KeyRelease,
	};
	XSendEvent(display,focused_window,True,KeyPressMask,(XEvent *)&key_event);
	XCloseDisplay(display);
}
