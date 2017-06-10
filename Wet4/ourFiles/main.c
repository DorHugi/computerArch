#include "cache.h"

int main(int argc, const char *argv[]) {


	if (argc < 20){
		//printf("Expecting more arguments. got only %d\n ",argc);
		return -1;
	}

	const char* inputFile = argv[1];
	const char* memCycStr = argv[3];
	const char* blockSizeStr = argv[5];
	const char* writeAllocStr = argv[7];
	const char* L1SizeStr = argv[9];
	const char* L1WaysStr = argv[11];
	const char* L1CycStr = argv[13];
	const char* L2SizeStr = argv[15];
	const char* L2WaysStr = argv[17];
	const char* L2CycStr = argv[19];

//	for (int i = 0; i < argc ; i++){
//		//printf("argv[%d] is: %s , atoi of argv is: %d: \n",i,argv[i],atoi(argv[i]));
//		//printf("length is: %d \n", strlen(argv[i]));
//
//
//	}


	int MemCyc = atoi(memCycStr);
	int blockSize= atoi(blockSizeStr);
	bool writeAlloc = atoi(writeAllocStr)==1 ? true : false;
	int L1Size = atoi(L1SizeStr);
	int L1Ways =(int) pow(2,atoi(L1WaysStr));
	int L1CyclesNum = atoi(L1CycStr);
	int L2Size = atoi(L2SizeStr);
	int L2Ways =(int) pow(2,atoi(L2WaysStr));
	int L2CyclesNum = atoi(L2CycStr);


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
//		//printf("line is %s \n",buf);
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
		//printf("adrstring: %s, adr: %d\n",tmp,adr);
		updateCache(adr,isWrite);

	}

	printStats();

	fclose(fh);
}
