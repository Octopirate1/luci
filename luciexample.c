#include "luci.h"

int main(int argc, char** argvp)
{
	if (argc != 2) {
		fprintf(stderr, "%s <filename>\n", argvp[0]);
		exit(1);
	}
	(void)map_and_process(argvp[1]);
	exit(0);
}