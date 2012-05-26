#include <stdio.h>
#include <stdlib.h>
#include "offsets.h"

int main(int argc, const char* argv[]){
	char* d1; unpack(&DATA1, &d1);
	char* d2; unpack(&DATA2, &d2);
	char* d3; unpack_filename("data3.txt", &d3);

	printf("data1: %s\n", d1);
	printf("data2: %s\n", d2);
	printf("data3: %s\n", d3);

	free(d1);
	free(d2);
	free(d3);

	return 0;
}
