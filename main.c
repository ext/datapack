#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "files.h"

int main(int argc, const char* argv[]){
	unpack_override(SRCDIR);

	char* d1; unpack(&DATA1, &d1);
	char* d2; unpack_filename("spam.txt", &d2);
	char* d3; unpack_filename("data3.txt", &d3);

	printf("data1: %s\n", d1);
	printf("data2: %s\n", d2);
	printf("data3: %s\n", d3);

	free(d1);
	free(d2);
	free(d3);

	return 0;
}
