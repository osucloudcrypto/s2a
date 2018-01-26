#include <stdio.h>
char* read_netstring(FILE* r, int* lenp);
int write_netstring(FILE* w, char* buf, int len);
