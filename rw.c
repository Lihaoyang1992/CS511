#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#define BUFFER_SIZE	65536

int main(int argc, char *argv[])
{
	FILE	*istream, *ostream;
	char	*line = NULL;
	size_t	len = 0;
	ssize_t	read, size = 0;
	char	buffer[BUFFER_SIZE];

	if (argc != 3) {
		fprintf(stderr, "<usage>: %s filename(for read) filename(for write)\n", 
				getprogname());
		return EXIT_FAILURE;
	}

	/* read section */
	if ((istream = fopen(argv[1], "r")) == NULL) {
		perror("foepn");
		return EXIT_FAILURE;
	}
	while ((read = getline(&line, &len, istream)) != -1) {
		size += read;
		strcat(buffer, line);
	}
	fclose(istream);
	free(line);

	/* write section */
	if ((ostream = fopen(argv[2], "w")) == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}
	fwrite(buffer, sizeof(char), size, ostream);

	return 0;
}
