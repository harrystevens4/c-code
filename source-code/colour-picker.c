#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

int get_term_size(int *width, int *height);

int main(int argc, char **argv){
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
	uint8_t *colour_grid = malloc(term_width*term_height*3);
	//====== generate colour grid ======
	//        squish y due to terminal characters being tall
	for (int y = 0; y < term_height*2; y+=2){
		for (int x = 0; x < term_width; x++){
			//calculate "magic value" of each colour
			double red = 0;
			double green = 0;
			double blue = 0;
			for (int i = 0; i < point_count; i++){
				//perpendicular distance to the tangent at point points[i] of circle with radius "radius"
 				//pythagoras
				double point_distance = sqrt(
					pow((double)(x-points[i][0]),2) + 
					pow((double)(y-points[i][1]),2)
				);
				//short ciruit (otherwise we end up dividing by 0 later)
				if (point_distance == 0){
					if (i == 0) red = UINT8_MAX;
					if (i == 1) green = UINT8_MAX;
					if (i == 2) blue = UINT8_MAX;
					break;
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
				double max_perpendicular_distance = cos((M_PI)/point_count)*radius+radius*2;
				//what proportion of colour?
				double perpendicular_distance = cos_angle*point_distance;
				double magic_distance = max_perpendicular_distance-perpendicular_distance;
				if (magic_distance < 0) magic_distance = 0;
				//set the colour
				if (i == 0) red   = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
				if (i == 1) green = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
				if (i == 2) blue  = magic_distance;//UINT8_MAX-(UINT8_MAX*proportion);
			}
			double rgb_total = red + green + blue;
			colour_grid[(y/2)*term_width*3 + x*3 + 0] = (red   /rgb_total)*UINT8_MAX;
			colour_grid[(y/2)*term_width*3 + x*3 + 1] = (green /rgb_total)*UINT8_MAX;
			colour_grid[(y/2)*term_width*3 + x*3 + 2] = (blue  /rgb_total)*UINT8_MAX;
		}
	}
	for (int y = 0; y < term_height-1; y++){
		for (int x = 0; x < term_width; x++){
			uint8_t red   = colour_grid[y*term_width*3 + x*3 + 0];
			uint8_t green = colour_grid[y*term_width*3 + x*3 + 1];
			uint8_t blue  = colour_grid[y*term_width*3 + x*3 + 2];
			printf("\033[48;2;%u;%u;%um ",red,green,blue);
			fflush(stdout);
		}
		if (y < term_height-1) printf("\n");
	}
	free(colour_grid);
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
