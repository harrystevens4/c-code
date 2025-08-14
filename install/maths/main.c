#include "../../source-code/maths.h"
#include <time.h>
#include <stdio.h>

void calc_square_roots_cpu(uint64_t max){
	for (uint64_t i = 0; i < max; i++){
		m_sqrt(i);
	}
}

int main(int argc,char **argv){
	printf("INFINITY = %f\n",INFINITY);
	printf("NAN = %f\n",NAN);
	printf("ln(0.1) = %f\n",m_ln(0.1));
	printf("sqrt(2) = %f\n",m_sqrt(2));
	printf("ln(56) = %f\n",m_ln(56));
	printf("log(2,64) = %f\n",m_logab(2,64));
	printf("ln(-1) = %f\n",m_ln(-1));
	printf("sqrt(0) = %f\n",m_sqrt(0));
	printf("sqrt(-1) = %f\n",m_sqrt(-1));
	printf("calculating 1,000,000,000 square roots...\n");
	struct timespec start, end;
	uint64_t square_roots = 1000000000;
	clock_gettime(CLOCK_REALTIME,&start);
	calc_square_roots_cpu(square_roots);
	clock_gettime(CLOCK_REALTIME,&end);
	printf("operation took %f seconds\n",(double)end.tv_sec-(double)start.tv_sec+((double)end.tv_nsec-(double)start.tv_nsec)/1000000000);
}
