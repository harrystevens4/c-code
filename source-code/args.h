#ifndef ARGS_H_
#define ARGS_H_
struct args{
	int number_single;
	int number_multi;
	int number_other; //for non - arguments
	char single[62]; //single letter arguments (array of chars, no \0)
	char multi[10][20]; //longer then one letter arguments
	char other[][100];
};
void parse_args(int argc, char *argv[],struct args *arguments);
#endif
