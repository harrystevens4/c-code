#define _GNU_SOURCE
#include <getopt.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

struct colour_grid {
	uint8_t *grid;
};

enum mouse_events {
	SCROLL_DOWN = UINT8_MAX,
	SCROLL_UP,
	MOUSE_UP,
	MOUSE_DOWN,
};

struct termios old_term_settings;
int mouse_position[2];

int get_term_size(int *width, int *height);
void init_terminal(void);
void restore_terminal(void);
int generate_colour_grid(struct colour_grid *colour_grid);
void free_colour_grid(struct colour_grid *colour_grid);
int render_colour_grid(struct colour_grid *colour_grid);
int get_input(void);
void print_help(char *program_name);

int main(int argc, char **argv){
	//====== process command line ======
	struct option options[] = {
		{"help",no_argument,0,'h'},
		{0,0,0,0},
	};
	for (;;){
		int option_index = 0;
		int option = getopt_long(argc,argv,"h",options,&option_index);
		if (option == -1) break;
		switch (option){
		case 'h':
			print_help(argv[0]);
			return 0;
		}
	}
	//====== get the terminal size ======
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return 1;
	//====== prep terminal ======
	init_terminal();
	atexit(restore_terminal);
	//====== block signals ======
	sigset_t blocked_sigset;
	sigemptyset(&blocked_sigset);
	sigaddset(&blocked_sigset,SIGWINCH);
	sigaddset(&blocked_sigset,SIGINT);
	sigaddset(&blocked_sigset,SIGIO);
	sigprocmask(SIG_BLOCK,&blocked_sigset,NULL);
	//====== generate grid ======
	struct colour_grid colour_grid;
	generate_colour_grid(&colour_grid);
	//====== show it ======
	render_colour_grid(&colour_grid);
	//====== mainloop ======
	int brightness = 0;
	int x = term_width/2, y = term_height/2;
	//completely signal driven
	for (;;){
		//====== display the status bar ======
		//clear line
		printf("\033[2K\r");
		//set colour to match the one selected
		int red =   MAX(MIN(colour_grid.grid[y*term_width*3 + x*3 + 0]+brightness,0xff),0);
		int green = MAX(MIN(colour_grid.grid[y*term_width*3 + x*3 + 1]+brightness,0xff),0);
		int blue =  MAX(MIN(colour_grid.grid[y*term_width*3 + x*3 + 2]+brightness,0xff),0);
		printf("\033[48;2;%d;%d;%dm",red,green,blue);
		//show coordinates
		printf("(%d,%d)",x,y);
		//show hex colour value
		printf(" #%.2x%.2x%.2x",red,green,blue);
		//show brightness
		printf(" | brightness %+d",brightness);
		//continue the line with the normal colour
		printf("\033[49m");
		//flush stdout
		fflush(stdout);
		//====== wait for signal ======
		int signal;
		if (sigwait(&blocked_sigset,&signal) < 0){
			perror("sigwait");
			break;
		}
		switch (signal){
		case SIGINT:
			//cleanly exit
			goto main_cleanup;
		case SIGWINCH:
			//try to maintain coordinates
			int old_width = term_width;
			int old_height = term_height;
			get_term_size(&term_width,&term_height);
			x = ((double)x/(double)old_width)*term_width;
			y = ((double)y/(double)old_height)*term_height;
			//====== rerender screen ======
			//regenerate and redisplay grid
			free_colour_grid(&colour_grid);
			generate_colour_grid(&colour_grid);
			//clear screen and rerender
			printf("\033[2J\033[49m\r");
			render_colour_grid(&colour_grid);
			fflush(stdout);
			break;
		case SIGIO:
			for (;;){
				//====== read input ======
				int input = get_input();
				if (input < 0) break;
				//====== handle inputs ======
				if (input == MOUSE_UP){
					//get mouse position
					x = mouse_position[0];
					y = mouse_position[1];
				}
				if (input == SCROLL_DOWN){
					brightness = MAX(brightness-2,-0xff);
					//snap to 0
					if (brightness == -1 || brightness == 1) brightness = 0;
				}
				if (input == SCROLL_UP){
					brightness = MIN(brightness+2,0xff);
					//snap to 0
					if (brightness == -1 || brightness == 1) brightness = 0;
				}
			}
			break;
		}
	}
	//====== cleanup ======
	main_cleanup:
	free_colour_grid(&colour_grid);
}

int get_term_size(int *width, int *height){
	//ioctl makes for non portable but theres not much I can do
	struct winsize window_size = {0};
	if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&window_size) < 0){
		perror("ioctl(TIOCGWINSZ)");
		return -1;
	}
	*width = window_size.ws_col;
	*height = window_size.ws_row;
	return 0;
}

void init_terminal(void){
	//enable raw mode etc.
	tcgetattr(STDIN_FILENO,&old_term_settings);
	struct termios new_settings;
	memcpy(&new_settings,&old_term_settings,sizeof(struct termios));
	new_settings.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO,TCSANOW,&new_settings);
	//enable alternative buffer
	printf("\033[?1049h");
	//save screen
	printf("\033[?47h");
	//enable mouse reporting
	printf("\033[?1002h");
	//otherwise these wont take effect untill the next newline
	fflush(stdout);
	//turn on nonblocking mode on stdin
	int flags = fcntl(STDIN_FILENO,F_GETFL);
	fcntl(STDIN_FILENO,F_SETFL,flags | O_NONBLOCK | O_ASYNC);
	//take ownership of stdin signals
	fcntl(STDIN_FILENO,F_SETOWN,getpid());
}
void restore_terminal(void){
	tcsetattr(STDIN_FILENO,TCSANOW,&old_term_settings);
	//disable alternative buffer
	printf("\033[?1049l");
	//restore screen
	printf("\033[?47l");
	//enable mouse reporting
	printf("\033[?1002l");
	//otherwise these wont take effect untill the program exits
	fflush(stdout);
}

int generate_colour_grid(struct colour_grid *colour_grid){
	//====== get the terminal size ======
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return -1;
	//====== find the 3 corners of the triangle ======
	const int point_count = 3;
	//radius from the centre to each of the points
	double radius = (term_width+term_height)/8;
	//        x,y
	int points[point_count][2];
	//evenly space points around circle with radius "radius"
	for (int i = 0; i < point_count; i++){
		points[i][0] = rint( (radius)*sin((2*i*M_PI)/point_count)+(term_width/2) );
		//                    squish y due to terminal characters being tall
		points[i][1] = rint( (radius)*cos((2*i*M_PI)/point_count)+(term_height) );
	}
	//====== allocate colour grid ======
	//[ (red,green,blue), ... ]
	colour_grid->grid = malloc(term_width*term_height*3);
	//====== generate colour grid ======
	//        squish y due to terminal characters being tall
	for (int y = 0; y < term_height*2; y+=2){
		for (int x = 0; x < term_width; x++){
			//calculate "magic value" of each colour
			double red = 0;
			double green = 0;
			double blue = 0;
			for (int i = 0; i < point_count; i++){
				const double max_perpendicular_distance = cos((M_PI)/point_count)*radius+radius*2;
				//perpendicular distance to the tangent at point points[i] of circle with radius "radius"
 				//pythagoras
				double point_distance = sqrt(
					pow((double)(x-points[i][0]),2) + 
					pow((double)(y-points[i][1]),2)
				);
				//short ciruit (otherwise we end up dividing by 0 later)
				if (point_distance == 0){
					if (i == 0) red   = max_perpendicular_distance;
					if (i == 1) green = max_perpendicular_distance;
					if (i == 2) blue  = max_perpendicular_distance;
					continue;
				}
				double centre_distance = sqrt(
					pow((double)((term_width/2)-x),2) + 
					pow((double)((term_height)-y),2)
				);
				//cosine rule
				double cos_angle = (
					(pow(centre_distance,2)-pow(point_distance,2)-pow(radius,2))
					                /
					(-2*radius*point_distance)
				);
				//what proportion of colour?
				double perpendicular_distance = cos_angle*point_distance;
				double magic_distance = max_perpendicular_distance-perpendicular_distance;
				if (magic_distance < 0) magic_distance = 0;
				//set the colour
				if (i == 0) red   = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
				if (i == 1) green = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
				if (i == 2) blue  = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
			}
			//use a proportional value for each component
			double rgb_total = red + green + blue;
			colour_grid->grid[(y/2)*term_width*3 + x*3 + 0] = (red   /rgb_total)*UINT8_MAX;
			colour_grid->grid[(y/2)*term_width*3 + x*3 + 1] = (green /rgb_total)*UINT8_MAX;
			colour_grid->grid[(y/2)*term_width*3 + x*3 + 2] = (blue  /rgb_total)*UINT8_MAX;
		}
	}
	return 0;
}

void free_colour_grid(struct colour_grid *grid){
	free(grid->grid);
}

int render_colour_grid(struct colour_grid *colour_grid){
	//====== get the terminal size ======
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return -1;
	//====== render each colour from the grid to the terminal ======
	for (int y = 0; y < term_height-1; y++){
		for (int x = 0; x < term_width; x++){
			uint8_t red   = colour_grid->grid[y*term_width*3 + x*3 + 0];
			uint8_t green = colour_grid->grid[y*term_width*3 + x*3 + 1];
			uint8_t blue  = colour_grid->grid[y*term_width*3 + x*3 + 2];
			printf("\033[48;2;%u;%u;%um ",red,green,blue);
			fflush(stdout);
		}
		if (y < term_height-1) printf("\n");
	}
	//stop colour overspill
	printf("\033[49m");
	fflush(stdout);
	return 0;
}

int get_input(void){
	//wait 1ms for input to show up
	usleep(1000);
	//term height and such
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return 1;
	//read stdin
	char stdin_buffer[1];
	if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
	//"escape" character
	switch (stdin_buffer[0]){
	case 033:
		//check for a '[' to see if it is a CSI
		if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
		if (stdin_buffer[0] == '['){
			//what kind of CSI?
			if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
			switch (stdin_buffer[0]){
			case 'M':
				//====== mouse event ======
				if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
				if (stdin_buffer[0] == 97) return SCROLL_DOWN;
				if (stdin_buffer[0] == 96) return SCROLL_UP;
				if (stdin_buffer[0] == 32) return MOUSE_DOWN;
				if (stdin_buffer[0] == 35){
					//read mouse position
					if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
					//im not sure why but (0,0) is (33,33)
					//char is signed but we dont want that
					mouse_position[0] = MIN((uint8_t)stdin_buffer[0] - 33,term_width);
					if (read(STDIN_FILENO,stdin_buffer,1) < 0) return -1;
					mouse_position[1] = MIN((uint8_t)stdin_buffer[0] - 33,term_height);
					return MOUSE_UP;
				}
				break;
			}
		}else {
			return 033;
		}
		break;
	default:
		return stdin_buffer[0];
	}
	return -1;
}

void print_help(char *program_name){
	printf("Usage: %s [options]\n",program_name);
	printf("Options:\n");
	printf("	-h, --help : show help text\n");
	printf("Controls:\n");
	printf("	This program is entirely mouse based (ironic for a terminal application)\n");
	printf("	Click on a colour to select it\n");
	printf("	Scroll up and down to change brightness\n");
	printf("	Ctrl-c to quit\n");
	printf("	The bottom status bar shows the coordinates, hex colour and brightness\n");
}
