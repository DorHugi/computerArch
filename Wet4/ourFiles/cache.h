#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

void initalize(int _memCyc, int _blockSize, bool _writeAlloc, int l1Size, int l1Cyc, int l1Ways,
		int l2Size, int l2Cyc, int l2Ways);

void updateCache(int adr, bool isWrite);

void printStats();
