#include "luci.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>

int *version;

int main(int argc, char** argvp)
{	
	slp_file_t *slpfilep;
	version = (int *)calloc(VERSION_LENGTH-1, sizeof(int));
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "%s <filename>\n", argvp[0]);
		exit(1);
	}

	if (argc == 2) {
		printf("No version number specified. Setting version to 0.0.0. This is not reccomended, read the README.\n");
	} else {
		int count = 0;
		char* token = strtok(argvp[2], "."); 
   		while (token != NULL) { 
        		version[count] = atoi(token);
			count++; 
        		token = strtok(NULL, ".");
			if (count > 3) break;
    		} 
		if (count != 3) {
			printf("Invalid version number specified. Setting version to 0.0.0. This is not reccomended, read the README.\n");
			version = (int [3]) {0, 0, 0};
		}
	}

	slpfilep = map_and_process(argvp[1], version);
	printf("x position of player on frame 302: %f \n", slpfilep->gamep->framearrayp[302].ports[0].char_frames[0].preframe.x_position);
	exit(0);
}
