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
	//====== render ======
	//        squish y due to terminal characters being tall
	for (int y = 0; y < term_height*2; y+=2){
		for (int x = 0; x < term_width; x++){
			//calculate colour of coordinate
			uint8_t red = 0;
			uint8_t green = 0;
			uint8_t blue = 0;
			for (int i = 0; i < point_count; i++){
				//perpendicular distance to the tangent at point points[i] of circle with radius "radius"
 				//pythagoras
				double point_distance = sqrt(
					pow((double)(x-points[i][0]),2) + 
					pow((double)(y-points[i][1]),2)
				);
				double centre_distance = sqrt(
					pow((double)((term_width/2)-points[i][0]),2) + 
					pow((double)((term_height)-points[i][1]),2)
				);
				//cosine rule
				double cos_angle = (
					(pow(centre_distance,2)-pow(point_distance,2)-pow(radius,2))
					                /
					((-2)*radius*point_distance)
				);
				double perpendicular_distance = cos_angle*point_distance;
				double proportion = (perpendicular_distance/radius);
				if (proportion > 1) proportion = 1;
				//set the colour
				if (i == 0) red = UINT8_MAX*proportion;
				if (i == 1) green = UINT8_MAX*proportion;
				if (i == 2) blue = UINT8_MAX*proportion;
			}
			for (int i = 0; i < point_count; i++){
				if (points[i][0] == x && points[i][1] == y){
					printf("x");
					goto next_loop;
				}
			}
			//not reached if a point found here
			printf("\033[48;2;%u;%u;%um ",red,green,blue);
			fflush(stdout);
			next_loop:
		}
		if (y < term_height-1) printf("\n");
	}
	sleep(5);
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
