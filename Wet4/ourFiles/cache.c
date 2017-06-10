#include "cache.h"

typedef struct LINE {
	bool valid;
	bool dirty;
	int lastUsed;
	int tag;

} LINE;

typedef struct WAY{
	int totalLines;
	LINE* linesArr;

} WAY;


typedef struct CACHE_STATS {
	int miss;
	int hits;
} CACHE_STATS;



typedef struct CACHE{
	int size;
	int waysNum;
	int cyc;
	WAY* waysArr;
	CACHE_STATS stats;
} CACHE;



typedef struct GLOBAL_STATS{
	int cyc;
	int totalInsts;
}GLOBAL_STATS;


//globals:
CACHE* l1;
CACHE* l2;
bool writeAlloc;
int memCyc;
int blockSize;

GLOBAL_STATS globalStats;
int timeStamp;

int Log2( double n )
{
    // log(n)/log(2) is log2.

	double res = log( n ) / log( 2 ) ;

	if ((res - (int)res) >0)
		return (int)res+1;
	else
		return (int)res;

}


void updateTime(LINE* line){
	line->lastUsed = timeStamp;
	timeStamp++;
}

int getSet(int adr, CACHE* c){

	adr = adr >> blockSize;
	int linesInWay = c->waysArr->totalLines;
	int setBits = Log2(linesInWay);


	return (adr % (int)pow(2,setBits));
}


int getTag (int adr,CACHE* c){

	adr = adr >> blockSize;
	int linesInWay = c->waysArr->totalLines;
	int setBits =  Log2(linesInWay);

	return (adr >> setBits);

}



WAY* alloacteWays(int waysNum, int totalSize, int blockSize){

	WAY* waysArr = (WAY*)malloc(sizeof(WAY)*waysNum);
	int linesInWay = pow(2,totalSize)/(pow(2,blockSize) * waysNum);


	for (int i = 0; i < waysNum; i++){
		waysArr[i].totalLines = linesInWay;
		waysArr[i].linesArr = (LINE*)malloc(sizeof(LINE)*linesInWay);

		for (int j =0 ; j < linesInWay;j++){
			waysArr[i].linesArr[j].valid = false;
		}

	}
	return waysArr;

}

void zeroStats(CACHE_STATS* st){
	st->hits = 0;
	st->miss = 0;
}

void initalize(int _memCyc, int _blockSize, bool _writeAlloc, int l1Size, int l1Cyc, int l1Ways,
		int l2Size, int l2Cyc, int l2Ways){

//	//printf("args are: memCyc: %d, bsize:%d, l1size:%d l1cyc: %d , l1way:%d\n",_memCyc, _blockSize, l1Size, l1Cyc, l1Ways);
	//save data:
	writeAlloc = _writeAlloc;
	memCyc = _memCyc;
	blockSize = _blockSize;
	timeStamp = 1;

	//allocate memory to caches:
	l1 = (CACHE*) malloc(sizeof(l1));
	l1->size = l1Size;
	l1->cyc = l1Cyc;
	l1->waysArr = alloacteWays(l1Ways,l1Size,blockSize);
	l1->waysNum = l1Ways;
	zeroStats(&l1->stats);


	l2 = (CACHE*) malloc(sizeof(l2));
	l2->size = l2Size;
	l2->cyc = l2Cyc;
	l2->waysArr = alloacteWays(l2Ways,l2Size,blockSize);
	l2->waysNum = l2Ways;
	zeroStats(&l2->stats);


	globalStats.cyc =0;
	globalStats.totalInsts = 0;


	//printf("*******************PRINTING VALUES OF ARGUMENTS*******************\n");
	//printf("global - block_size: %d, mem cyc: %d, write alloc %d\n", blockSize, memCyc, writeAlloc);
	//printf("cache 1 - size : %d , ways:%d, cyc : %d, waysline: %d \n", l1->size, l1->waysNum, l1->cyc, l1->waysArr[0].totalLines);
	//printf("cache 2 - size : %d , ways:%d, cyc : %d waysline: %d\n", l2->size, l2->waysNum, l2->cyc, l2->waysArr[0].totalLines);

}

LINE* findInCache(CACHE* cache, int set, int tag ){

	for (int i = 0 ; i < cache->waysNum; i++){
		WAY curWay = cache->waysArr[i];
		LINE curLine = curWay.linesArr[set];
		if (curLine.tag == tag && curLine.valid)
				return &curWay.linesArr[set];

		}

	return NULL;

}

unsigned concatenate(unsigned x, unsigned y) {
    unsigned pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y;
}

LINE* addToCache(CACHE* c, int set,int tag){

	//find LRU and remove it.
//	time_t minTime = (c->waysArr[0])->linesArr[set].lastUsed;

	int availableWayNum = -1;
	for (int i = 0; i < c->waysNum ; i++){
		WAY curWay = c->waysArr[i];
		if (!curWay.linesArr[set].valid){
			availableWayNum = i;
			break;
		}
	}

	if (availableWayNum == -1){
		int minTime = c->waysArr[0].linesArr[set].lastUsed;
		availableWayNum = 0;

		for (int i = 1; i < c->waysNum ; i++){
			 int tmpTime = c->waysArr[i].linesArr[set].lastUsed;
			 if (tmpTime < minTime){
				 minTime = tmpTime;
				 availableWayNum = i;
			 }
		}

		//if evicting an element from l2, check if it's in L1, and if so evict it.

		if (c==l1){
			LINE* evictedLine = &c->waysArr[availableWayNum].linesArr[set];

			if (evictedLine->dirty){
//				globalStats.cyc+=l2->cyc-l1->cyc;
				int adr = concatenate(evictedLine->tag,set);
				adr = adr << blockSize;

				LINE* lineInL2 = findInCache(l2,getSet(adr,l2),getTag(adr,l2));
				if (lineInL2 != NULL){
					//evict.
					updateTime(lineInL2);
					lineInL2->dirty= true;

				}
			}
		}

		else { //(c==l2)
			LINE* evictedLine = &c->waysArr[availableWayNum].linesArr[set];

//			if (evictedLine->dirty){
//				globalStats.cyc+=memCyc - l2->cyc;
//			}

			int adr = concatenate(c->waysArr[availableWayNum].linesArr[set].tag,set);
			adr = adr << blockSize;

			LINE* lineInL1 = findInCache(l1,getSet(adr,l1),getTag(adr,l1));
			if (lineInL1 != NULL){
				//evict.
				lineInL1->valid = false;

			}

		}
	}



	c->waysArr[availableWayNum].linesArr[set].tag = tag;
	updateTime(&c->waysArr[availableWayNum].linesArr[set]);
	c->waysArr[availableWayNum].linesArr[set].valid = true;
	c->waysArr[availableWayNum].linesArr[set].dirty = false;

	return &c->waysArr[availableWayNum].linesArr[set];

}



void printCacheContent(){
	//printf("*****printing content of l1:************* \n");
	for (int i =0 ; i < l1->waysNum; i++){
		for (int j =0 ; j < l1->waysArr[i].totalLines; j++){
			//printf("Way:%d Line:%d tag: %d ,timestamp:%d \n",i,j,l1->waysArr[i].linesArr[j].tag,l1->waysArr[i].linesArr[j].lastUsed);
		}
	}
	//printf("*****printing content of l2:************* \n");
	for (int i =0 ; i < l2->waysNum; i++){
		for (int j =0 ; j < l2->waysArr[i].totalLines; j++){
			//printf("Way:%d Line:%d tag: %d ,timestamp:%d \n",i,j,l2->waysArr[i].linesArr[j].tag,l2->waysArr[i].linesArr[j].lastUsed);
		}
	}
}

void updateCache(int adr, bool isWrite){

	globalStats.totalInsts++;

	int set1 = getSet(adr,l1);
	int set2 = getSet(adr,l2);
	int tag1 = getTag(adr,l1);
	int tag2 = getTag(adr,l2);
	//printf("adr : %d, set1: %d, set2: %d, tag1: %d, tag2: %d\n", adr, set1, set2, tag1, tag2);
	//printf("isWrite: %d\n",isWrite);
	LINE* l1Line = findInCache(l1,set1,tag1);
	LINE* l2Line = findInCache(l2,set2,tag2);

	if (l1Line){
		//printf("found in l1\n");
		l1->stats.hits++;
		updateTime(l1Line);
		if (isWrite)
			l1Line->dirty =1;
		globalStats.cyc+=l1->cyc;
	}

	else if(l2Line){
		//printf("found in l2\n");
		l2->stats.hits++;
		l1->stats.miss++;
		updateTime(l2Line);
		LINE* newLine = addToCache(l1,set1,tag1);
		if (isWrite)
			newLine->dirty = true;

		globalStats.cyc+=l2->cyc;
	}

	else {
		//printf("wasnt found in any of the caches \n");
		if (!isWrite || (isWrite && writeAlloc)){
			 LINE* tmpL2 = addToCache(l2,set2,tag2);
			 LINE* tmpL1 = addToCache(l1,set1,tag1);
			 if (isWrite){
				tmpL1->dirty = true;
			 }
		}

		l1->stats.miss++;
		l2->stats.miss++;

		globalStats.cyc+=memCyc;
	}


	printCacheContent();

}

void printStats(){
	float l1Miss = (float)l1->stats.miss/(float)((float)l1->stats.hits + (float)l1->stats.miss);
	float l2Miss = (float)l2->stats.miss/(float)((float)l2->stats.hits + (float)l2->stats.miss);
	float avgCyc = (float)globalStats.cyc/(float)globalStats.totalInsts;
	printf("L1miss=%.3f L2miss=%.3f AccTimeAvg=%.3f\n",l1Miss, l2Miss, avgCyc);

}












