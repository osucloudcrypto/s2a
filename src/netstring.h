#include <stdio.h>
extern "C" char* read_netstring(FILE* r, int* lenp);
extern "C" int write_netstring(FILE* w, const char* buf, int len);
