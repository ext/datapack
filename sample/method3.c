#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include "files.h"

int main(int argc, char* argv[]){
	/* read datapack from file on filesystem */
	datapack_t pak = datapack_open("sample/files.pak");
	if ( !pak ){
		fprintf(stderr, "failed to open datapack: %s\n", strerror(errno));
		return 1;
	}

	/* read data */
	char* d1; unpack_filename(pak, "data1.txt", &d1);
	char* d2; unpack_filename(pak, "spam.txt", &d2);
	char* d3; unpack_filename(pak, "data3.txt", &d3);
	datapack_close(pak);

	/* show results */
	printf("data1: %s\n", d1);
	printf("data2: %s\n", d2);
	printf("data3: %s\n", d3);

	/* release resources */
	free(d1);
	free(d2);
	free(d3);

	return 0;
}
