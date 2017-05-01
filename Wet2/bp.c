/* 046267 Computer Architecture - Spring 2016 - HW #2 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include "stdlib.h"
//*******Typedefs**********

#define EMPTY -1
typedef enum {SNT,WNT,WT,ST} PREDICTION;


typedef struct {

	uint32_t target;
	int tag;
	PREDICTION* predictionArr;
	bool* histArr;

} BTB_ENTRY;




typedef struct{
	BTB_ENTRY* entryArr;
	unsigned histSize;
	unsigned size;
	unsigned tagMaxSize;
	bool isGlobalHist;
	bool isGlobalTable;
	bool isShare;
	SIM_stats stats;

	//global values:
	PREDICTION* globalPredictionArr;
	bool* globalHistArr;
} BTB;


static BTB btb;

int power (int x,int y){

	int res = 1;
	for (int i =0; i <y ; i++){
		res= res*x;
	}

	return res;
}


void zeroHist(bool* histArr,unsigned size){
	for (int i =0; i < size ; i++)
		histArr[i] = false;
}
void zeroPredictionArray(PREDICTION* predictionArr, unsigned size){
	for (int i =0 ; i < size ; i++){
		predictionArr[i] = WNT;
	}

}

void zeroStats(SIM_stats* s){
	s->flush_num= 0;
	s->br_num= 0;
	s->size= 0;


	//TODO: need to update size correctly!!!
}


void updateHistory(bool* hist, unsigned size , bool taken){

	for (int i = size -1 ; i >= 1; i--){
		hist[i]=hist[i-1];
	}
	hist[0] = taken;

}



unsigned getBtbIndex(uint32_t pc){

	//remove last two bits.
	pc =  (pc >> 2);
	return (pc % btb.size);
}

unsigned getBtbTag(uint32_t pc){

	//remove last two bits.
	pc =  (pc >> 2);
	return (pc % btb.tagMaxSize);
}


unsigned getHistVal(bool* hist, unsigned size){

	unsigned sum = 0;
	for (int i = 0 ; i < size ; i++)
		if (hist[i] ==1)
			sum+= power(2,i);

	return sum;
}

bool getPrediction (PREDICTION p ){
	if (p == WT || p == ST)
		return true;
	else
		return false;
}

void updatePredTaken(PREDICTION* p){

	if (*p == ST)
		return;
	else {
		*p = (*p+1);
		return;
	}
}

void updatePredNotTaken(PREDICTION* p){

	if (*p == SNT)
		return;
	else {
		*p = (*p-1);
		return;
	}
}

void updatePrediction(PREDICTION* p, bool taken){
	//printf("predict before is: %d\n", *p);
	if (taken)
		updatePredTaken(p);
	else
		updatePredNotTaken(p);
	//printf("predict after is: %d\n", *p);
	return;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize,
             bool isGlobalHist, bool isGlobalTable, bool isShare){

	//Validate input:
	if (btbSize > 32 || btbSize < 1 || historySize > 8 || historySize< 1 || tagSize>30)
		return -1;
	else
	{
		btb.histSize = historySize;
		btb.tagMaxSize = (unsigned)power(2,tagSize);
		btb.size = btbSize;
		btb.isGlobalHist = isGlobalHist;
		btb.isGlobalTable = isGlobalTable;
		btb.isShare = isShare;

		btb.entryArr = (BTB_ENTRY*)malloc(sizeof(BTB_ENTRY)*btb.size);

		//initialize entry array
		for (int i =0; i < btb.size ; i++){
			//initialize hist
			btb.entryArr[i].histArr = (bool*)malloc(sizeof(bool)*btb.histSize);
			zeroHist(btb.entryArr[i].histArr, btb.histSize);

			//initialize prediction array
			unsigned predictionArrSize = power(2,btb.histSize);
			btb.entryArr[i].predictionArr = (PREDICTION*)malloc(sizeof(PREDICTION)*predictionArrSize);
			zeroPredictionArray(btb.entryArr[i].predictionArr , predictionArrSize);

			//initialize tag
			btb.entryArr[i].tag = EMPTY;


			//initialize target
			btb.entryArr[i].target = 0;
		}


		zeroStats(&btb.stats);
		return 1;
	}

}

bool BP_predict(uint32_t pc, uint32_t *dst){

	unsigned btbIndex = getBtbIndex(pc);
	unsigned tag = getBtbTag(pc);
	if (btb.entryArr[btbIndex].tag == tag){
		unsigned histVal = getHistVal(btb.entryArr[btbIndex].histArr, btb.histSize);
		PREDICTION pr = btb.entryArr[btbIndex].predictionArr[histVal];
		bool predVal = getPrediction(pr);
		if (predVal == true){
			*dst = btb.entryArr[btbIndex].target;
			return predVal;
		}
	}

	*dst = (pc+4);
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){

	unsigned btbIndex = getBtbIndex(pc);
	unsigned tag = getBtbTag(pc);
	int wasFlush = 0;
	//printf("btb index is: %d , tag is: %d \n", btbIndex, tag);
	if (btb.entryArr[btbIndex].tag != tag  ){ //or tag is empty (the if includes this case as well)
		//initialize entry
		btb.entryArr[btbIndex].tag = tag;
		zeroHist(btb.entryArr[btbIndex].histArr, btb.histSize);
		unsigned predictionArrSize = power(2,btb.histSize);
		zeroPredictionArray(btb.entryArr[btbIndex].predictionArr,predictionArrSize);
	}

	btb.entryArr[btbIndex].target = targetPc;
	//update history
	unsigned oldHistVal =getHistVal(btb.entryArr[btbIndex].histArr, btb.histSize);
	//printf("Old history val is: %d \n" , oldHistVal);

	updateHistory(btb.entryArr[btbIndex].histArr,btb.histSize,taken);
	updatePrediction(&btb.entryArr[btbIndex].predictionArr[oldHistVal], taken);
	//printf("update taken is: %d\n" , taken);

	//TODO: update stats
	btb.stats.br_num+=1;
	if (((targetPc != pred_dst) && taken) || ((targetPc == pred_dst) && !taken))
		wasFlush = 1;
	btb.stats.flush_num = btb.stats.flush_num + wasFlush;


	return;
}

void BP_GetStats(SIM_stats *curStats) {

	(*curStats) = btb.stats;

	return;
}

