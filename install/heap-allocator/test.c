#include "my-heap-allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){
	char *string_1 = mha_alloc(6);
	memcpy(string_1,"12345",6);
	printf("%s\n",string_1);
	return 0;
}
