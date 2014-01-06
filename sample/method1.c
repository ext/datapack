#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "files.h"

int main(int argc, const char* argv[]){
	char* d1; unpack(&DATA1, &d1);
	char* d2; unpack(&DATA2, &d2);
	char* d3; unpack(&DATA3, &d3);

	printf("data1: %s\n", d1);
	printf("data2: %s\n", d2);
	printf("data3: %s\n", d3);

	free(d1);
	free(d2);
	free(d3);

	return 0;
}
