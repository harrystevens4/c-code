#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
int connect_pedal(char *device);
int simulate_keypress(char *key, int pressed);
int get_pedal_state(int pedal_fd);
int main(int argc, char **argv){
	if (argc < 3){
		fprintf(stderr,"args : <device filename e.g. /dev/ttyACM0> <key to simulate>\n");
		return 1;
	}
	char *device = argv[1];
	int pedal_fd = connect_pedal(device);//open(device,O_RDONLY);
	if (pedal_fd < 0){
		fprintf(stderr,"could not open the serial line.\n");
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
			simulate_keypress(argv[2],new_pedal_state);
			//Control_l, Shift_L and so on
		}
		previous_pedal_state = new_pedal_state;
	}
	close(pedal_fd);
	return 0;
}
int connect_pedal(char *device){
	//open device
	int pedal_fd = open(device,O_RDONLY);
	if (pedal_fd < 0){
		perror("open");
		return -1;
	}

	//set baud rate
	struct termios options;
	int result = tcgetattr(pedal_fd, &options); //fill in the current options
	if (result < 0){perror("tcgetattr");return -1;}
	
	result = cfsetispeed(&options,B9600);
	if (result < 0){perror("cfsetispeed");return -1;}

	result = cfsetospeed(&options,B9600);
	if (result < 0){perror("cfsetospeed");return -1;}
	
	//set other important attributes
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;
	// no flow control
	options.c_cflag &= ~CRTSCTS;
	options.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
	options.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	options.c_oflag &= ~OPOST; // make raw

	//timings for bytes in control sequences
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 20;

	result = tcsetattr(pedal_fd,TCSANOW,&options); //make changes immediately
	if (result < 0){perror("tcsetattr");return -1;}

	tcgetattr(pedal_fd, &options);
	printf("baud: %d in, %d out, should be %d\n",cfgetispeed(&options),cfgetospeed(&options),B9600);

	result = tcflush(pedal_fd,TCIOFLUSH); //flush any data
	if (result < 0){perror("tcflush");return -1;}

	return pedal_fd;
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
