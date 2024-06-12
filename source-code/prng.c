#include <stdio.h>
#include <sys/time.h>
#include <time.h>

void copy_number_array(int array1[], int array2[]);
void display_int_list(int list[]);
int *next_number(int seed[]);
int flip_bit(int bit);
int convert_to_decimal(int bit_array[]);
int digit_n(long number,int n);

int main(){
	int seed[] = {0,1,1,0,1,1,0,0};
	time_t t = time(NULL);
	struct timeval start;
	gettimeofday(&start,NULL);
	struct tm time = *localtime(&t);
	//printf("milisecond time:%lu\n",start.tv_usec);
	int i;
	for (i=0;i<8;i++){
		//printf("pt%d:%d\n",i,digit_n(start.tv_usec,i));
		seed[i] = digit_n(start.tv_usec,i)%2;
	}
	//printf("reference seconds for itteration count:%d\n",time.tm_sec);
	//printf("generating random 8 bit number...\n");
	//printf("first pass over array:");
	//printf("using seed:\n");
	//display_int_list(seed);
	//printf("as a decimal:%d\n",convert_to_decimal(seed));
	for (i=0;i<time.tm_sec;i++){
		//printf("pass %d:",i);
		copy_number_array(next_number(seed),seed);
		//printf("new array of:");
		//display_int_list(seed);
	}
	printf("%d\n",convert_to_decimal(seed));
}

int *next_number(int seed[]){
	int i;
	int lsb = seed[7];
	//printf("using seed of:");
	//display_int_list(seed);
	for (i=7;i>0;i--){
		seed[i] = seed[i-1];
	}
	seed[0] = 0;
	//printf("performed right shift, resulting in:");
	//display_int_list(seed);
	if (lsb == 1){
		seed[0] = 1;
		seed[2] = flip_bit(seed[2]);
		seed[5] = flip_bit(seed[5]);
	}
	//printf("result after possible bit flips:");
	//display_int_list(seed);
	return seed;
}
void display_int_list(int list[]){
	printf("{");
	int i;
	for (i=0;i<8;i++){
		printf("%d,",list[i]);
	}
	printf("}\n");
}
int flip_bit(int bit){
	if (bit == 1){
		return 0;
	}
	else{
		return 1;
	}
}
int convert_to_decimal(int bit_array[]){
	//printf("converting to decimal...\n");
	int i;
	int total = 0;
	int values[] = {128,64,32,16,8,4,2,1};
	for (i=0;i<8;i++){
		if (bit_array[i] == 1){
			//printf("added %d to count\n",values[i]);
			total += values[i];
		}
	}
	//printf("total conversion of:%d\n",total);
	return total;
}
void copy_number_array(int array1[], int array2[]){
	int i;
	for (i=0;i<8;i++){
		array2[i] = array1[i];
	}
}
int digit_n(long number,int n){
	while (n--) {
        	number /= 10;
    	}
    	return (int)((number % 10));
}
