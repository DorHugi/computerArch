#include "cache.h"

typedef struct LINE {
	bool valid;
	bool dirty;
	time_t lastUsed;
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

double log2( double n )
{
    // log(n)/log(2) is log2.
    return log( n ) / log( 2 );
}


void updateTime(LINE* line){
	line->lastUsed = time(NULL);
}

int getSet(int adr, CACHE* c){

	adr = adr >> blockSize;
	int linesInWay = c->waysArr->totalLines;
	int setBits = (int) log2(linesInWay);
	printf("linesInWay: %d , setBits: %d\n",linesInWay,setBits);
	printf("pow(2,setBits): %f\n", pow(2,setBits));
	return (adr % (int)pow(2,setBits));
}


int getTag (int adr,CACHE* c){

	adr = adr >> blockSize;
	int linesInWay = c->waysArr->totalLines;
	int setBits = (int) log2(linesInWay);

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
	printf("number of ways: %d, number of lines: %d\n",waysNum,waysArr[0].totalLines);
	return waysArr;

}

void zeroStats(CACHE_STATS* st){
	st->hits = 0;
	st->miss = 0;
}

void initalize(int _memCyc, int _blockSize, bool _writeAlloc, int l1Size, int l1Cyc, int l1Ways,
		int l2Size, int l2Cyc, int l2Ways){

	printf("args are: memCyc: %d, bsize:%d, l1size:%d l1cyc: %d , l1way:%d\n",_memCyc, _blockSize, l1Size, l1Cyc, l1Ways);
	//save data:
	writeAlloc = _writeAlloc;
	memCyc = _memCyc;
	blockSize = _blockSize;

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
		time_t minTime = c->waysArr[0].linesArr[set].lastUsed;
		availableWayNum = 0;

		for (int i = 1; i < c->waysNum ; i++){
			 time_t tmpTime = c->waysArr[i].linesArr[set].lastUsed;
			 if (tmpTime < minTime){
				 minTime = tmpTime;
				 availableWayNum = i;
			 }
		}
	}

	c->waysArr[availableWayNum].linesArr[set].tag = tag;
	updateTime(&c->waysArr[availableWayNum].linesArr[set]);
	c->waysArr[availableWayNum].linesArr[set].valid = true;
	c->waysArr[availableWayNum].linesArr[set].dirty = false;

	return &c->waysArr[availableWayNum].linesArr[set];

}


void updateCache(int adr, bool isWrite){

	globalStats.totalInsts++;

	int set1 = getSet(adr,l1);
	int set2 = getSet(adr,l2);
	int tag1 = getTag(adr,l1);
	int tag2 = getTag(adr,l2);

	LINE* l1Line = findInCache(l1,set1,tag1);
	LINE* l2Line = findInCache(l2,set2,tag2);

	if (l1Line){
		l1->stats.hits++;
		updateTime(l1Line);
		if (isWrite)
			l1Line->dirty =1;
	}

	else if(l2Line){
		l2->stats.hits++;
		l1->stats.miss++;
		updateTime(l2Line);
		LINE* newLine = addToCache(l1,set1,tag1);
		if (isWrite)
			newLine->dirty = true;
	}

	else {
		if (!isWrite || (isWrite && writeAlloc)){
			 LINE* tmpL1 = addToCache(l1,set1,tag1);
			 LINE* tmpL2 = addToCache(l2,set2,tag2);
			 if (isWrite){
				tmpL1->dirty = true;
				tmpL2->dirty = true;
			 }
		}

		l1->stats.miss++;
		l2->stats.miss++;
	}
}

void printStats(){
	float l1Miss = l1->stats.miss/(l1->stats.hits + l1->stats.miss);
	float l2Miss = l2->stats.miss/(l2->stats.hits + l2->stats.miss);

	printf("L1miss=%f L2miss=%f AccTimeAvg=%d\n",l1Miss,l2Miss,0);

}















