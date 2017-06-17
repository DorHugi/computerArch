#include "cache.h"
#include <unistd.h>

int main(int argc, const char *argv[]) {

	if (argc < 19){
		printf("Expecting more arguments. got only %d\n ",argc);
		return -1;
	}

	const char* inputFile = argv[1];
	
	
	int MemCyc, blockSize, L1Size, L1Ways, L1CyclesNum, L2Size, L2Ways, L2CyclesNum;
	bool writeAlloc;

	for (int i = 2; i < 19; i += 2) {
			const char* s = argv[i];
			if (!strcmp(s, "--mem-cyc")) {
				MemCyc = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--bsize")) {
				blockSize = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--l1-size")) {
				L1Size = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--l2-size")) {
				L2Size = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--l1-cyc")) {
				L1CyclesNum = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--l2-cyc")) {
				L2CyclesNum = atoi(argv[i + 1]);
			} else if (!strcmp(s, "--l1-assoc")) {
				L1Ways = (int) pow(2,atoi(argv[i + 1]));
			} else if (!strcmp(s, "--l2-assoc")) {
				L2Ways =(int) pow(2,atoi(argv[i + 1]));
			} else if (!strcmp(s, "--wr-alloc")) {
				writeAlloc = atoi(argv[i + 1])==1 ? true : false;
			} else {
				printf("Error in arguments\n");
				return 0;
			}
		}
	
	initalize(MemCyc,blockSize,writeAlloc,L1Size,L1CyclesNum,L1Ways,L2Size,L2CyclesNum,L2Ways);

	//Read program
	char buf[256];
	FILE* fh;
	if ((fh = fopen(inputFile,"r")) == NULL){
		printf("specified file is: %s \n",inputFile);
		printf("cwd is: %s\n",getcwd(buf,sizeof(buf)));
		printf("Error, could not open file: %s\n",inputFile);
		printf("errno is: %s \n",strerror(errno));
		return -1;
	}

	while (fgets(buf,sizeof(buf),fh)){
		bool isWrite = (buf[0]=='w');
		char tmp[300];
		int i = 4;

		while (buf[i]!= '\n'){
			tmp[i-4]=buf[i];
			i++;
		}

		while (i-4 < 300){
			tmp[i-4]=0;
			i++;
		}

		int adr = (int)strtol(tmp,NULL,16);
		updateCache(adr,isWrite);

	}

	printStats();

	fclose(fh);
}
