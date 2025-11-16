#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct colour_grid {
	uint8_t *grid;
};

int get_term_size(int *width, int *height);
void init_terminal(void);
void restore_terminal(void);
int generate_colour_grid(struct colour_grid *colour_grid);
void free_colour_grid(struct colour_grid *colour_grid);
int render_colour_grid(struct colour_grid *colour_grid);


int main(int argc, char **argv){
	//====== get the terminal size ======
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return 1;
	//====== prep terminal ======
	init_terminal();
	atexit(restore_terminal);
	//====== generate grid ======
	struct colour_grid colour_grid;
	generate_colour_grid(&colour_grid);
	//====== show it ======
	render_colour_grid(&colour_grid);
	sleep(4);
	//====== cleanup ======
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
	//enable alternative buffer
	printf("\033[?1049h");
	//save screen
	printf("\033[?47h");
}
void restore_terminal(void){
	//disable alternative buffer
	printf("\033[?1049l");
	//restore screen
	printf("\033[?47l");
}

int generate_colour_grid(struct colour_grid *colour_grid){
	//====== get the terminal size ======
	int term_width = 0;	
	int term_height = 0;
	if (get_term_size(&term_width,&term_height) < 0) return 1;
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
	if (get_term_size(&term_width,&term_height) < 0) return 1;
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
}
