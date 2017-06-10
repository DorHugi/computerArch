/* 046267 Computer Architecture - Spring 2017 - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include <stdio.h>
#define MAX_DEPENDANCIES  2



typedef struct _node {
	InstInfo inst;
	struct _node* pNodesArr[2]; //Pointer to the dependent inst nodes
	unsigned int time;
	unsigned int idx;
} node ;

int total = 0;

node* createNewNode(InstInfo inst, unsigned int time, int idx){
	node* pNode = (node*)malloc(sizeof(node));

	for (int i =0 ; i < MAX_DEPENDANCIES; i++)
		pNode->pNodesArr[i]=NULL;

	pNode->inst = inst;
	pNode->time = time;
	pNode->idx = idx;
	return pNode;
}

node* findFirstDependancy(node** nodesArr,unsigned int idx, unsigned int src ){

	for (int i = idx-1 ; i >= 0; i--){
		if (nodesArr[i]->inst.dstIdx == src)
			return nodesArr[i];
	}

	return NULL;
}

int* cache;

int getMaxDepthInner (node* curNode){
	static int booli = 0;
	if (booli == 0){
		cache = (int*)malloc(sizeof(int)*total);
		for (int i = 0; i< total; i++)
			cache[i]=-1;
	}
	booli = 1;
	int idx = curNode->idx;

	if (cache[idx]!=-1)
		return cache[idx];

	node* dep1 = curNode-> pNodesArr[0];
	node* dep2 = curNode-> pNodesArr[1];

	int sum1 = 0 ,sum2 = 0;

	if (dep1 != NULL){
		sum1 = getMaxDepthInner(dep1);
		cache[dep1->idx] = sum1;
	}

	if (dep2 != NULL){
		sum2 = getMaxDepthInner(dep2);
		cache[dep2->idx] = sum2;
	}

	int maxSum = sum1 > sum2 ? sum1 : sum2;

	return maxSum+curNode->time;



}

int getMaxDepth (node* curNode){
	return getMaxDepthInner(curNode) - curNode->time;
}


ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
	total = numOfInsts;
	//Create new array of pointers to nodes.
	node** nodesArr = (node**)malloc(sizeof(node*)*numOfInsts);
	for (int i =0; i < numOfInsts; i++)
		nodesArr[i]=NULL;

	//topologically sort the nodesArr.

	nodesArr[0] = createNewNode(progTrace[0],opsLatency[progTrace[0].opcode],0);


	for (int i = 1 ; i < numOfInsts;i++){
		nodesArr[i] = createNewNode(progTrace[i],opsLatency[progTrace[i].opcode],i);

		//Update dependencies.
		nodesArr[i]->pNodesArr[0]= findFirstDependancy(nodesArr,i,nodesArr[i]->inst.src1Idx);
		nodesArr[i]->pNodesArr[1]= findFirstDependancy(nodesArr,i,nodesArr[i]->inst.src2Idx);
	}
	return (ProgCtx)nodesArr;

}

void freeProgCtx(ProgCtx ctx) {

	for (int i = 0 ; i < total; i ++)
		free(((node**)ctx)[i]);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
	node** nodesArr = (node**)ctx;
	return getMaxDepth(nodesArr[theInst]);

}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
	node** nodesArr = (node**)ctx;

	if (theInst < 0 ||  theInst > total-1)
		return -1;

	node* curNode  = nodesArr[theInst];
	(*src1DepInst) = curNode->pNodesArr[0] == NULL ? -1 : curNode->pNodesArr[0]->idx;
	(*src2DepInst) = curNode->pNodesArr[1] == NULL ? -1 : curNode->pNodesArr[1]->idx;

	return 0;

}

int getProgDepth(ProgCtx ctx) {
	int sum = 0 ;
	node** nodesArr = (node**)ctx;

	for (int i = 0 ; i < total; i++){
		int tmpSum = getMaxDepthInner(nodesArr[i]);

		sum = sum > tmpSum ? sum : tmpSum;
	}

	return sum;

}


