#include <stdio.h>
#include <stdlib.h>

// Netstring routines adapted from
// https://cr.yp.to/proto/netstrings.txt

// Read a netstring from a file,
// which must be opened in binary mode.
char* read_netstring(FILE* r, int* lenp) {
	char* buf = NULL;
	int len;
	int c;
	// Read length
	if (fscanf(r, "%9d", &len) < 1) {
		/* >999999999 bytes is bad */
		goto readerror;
	}
	c = fgetc(r);
	if (c != ':') {
		if (c == EOF) {
			goto readerror;
		}
		fprintf(stderr, "expected colon, found %c\n", c);
		return NULL;
	}
	if (len < 0) {
		len = 0;
	}

	// Allocate space for the string
	// and read it
	buf = malloc(len + 1);  /* malloc(0) is not portable */
	if (buf == NULL) {
		perror("malloc");
		return NULL;
	}
	if ((int)fread(buf, 1, len, r) < len) {
		goto readerror;
	}

	// Final terminator
	c = fgetc(r);
	if (c != ',') {
		if (c == EOF) {
			goto readerror;
		}
		fprintf(stderr, "expected comma, found %c\n", c);
		free(buf);
		return NULL;
	}

	buf[len] = 0; // NUL-terminate buffer
	*lenp = len;
	return buf;

readerror:
	if (ferror(r)) {
		perror("read");
	} else if (feof(r)) {
		fprintf(stderr, "read: unexpected end of file\n");
	}
	free(buf);
	return NULL;
}

// Write a string to a file as a netstring.
// The file must be opened in binary mode.
int write_netstring(FILE* w, char* buf, int len) {
	if (fprintf(w, "%d:", len) < 0) {
		goto error;
	}
	if ((int)fwrite(buf, 1, len, w) < len) {
		goto error;
	}
	if (fputc(',', w) < 0) {
		goto error;
	}
	return 0;

error:
	perror("send");
	return -1;
}
