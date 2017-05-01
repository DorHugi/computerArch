/* 046267 Computer Architecture - Spring 2017 - HW #1 */
/* This file should hold your implementation of the CPU pipeline core simulator */

#include "sim_api.h"

#define NO_DEPENDANCY -1
// Global variables
static SIM_coreState curState;
static PipeStageState nextPipe[SIM_PIPELINE_DEPTH];
static int curRes[SIM_PIPELINE_DEPTH];
static int nextRes[SIM_PIPELINE_DEPTH];
static int enable_write;
static int32_t pcCurPipe[SIM_PIPELINE_DEPTH];
static int newPc = -1;
static int32_t pcNextPipe[SIM_PIPELINE_DEPTH];



//Funcs
bool checkDataHazard(SIM_cmd cur_cmd);
void cpyPipes (PipeStageState* src , PipeStageState* dst);
void cpyRegFile(int32_t* src, int32_t* dst);
void updatePc();
void initializePipe(PipeStageState* p);
void handleFetch();
void handleDecode(bool cpy);
void handleExec();
int handleMem();
void handleWb();
void flushPipe();

/*! SIM_CoreReset: Reset the processor core simulator machine to start new simulation
  Use this API to initialize the processor core simulator's data structures.
  The simulator machine must complete this call with these requirements met:
  - PC = 0  (entry point for a program is at address 0)
  - All the register file is cleared (all registers hold 0)
  - The value of IF is the instuction in address 0x0
  \returns 0 on success. <0 in case of initialization failure.
*/
int SIM_CoreReset(void) {
	enable_write = 0;	
	curState.pc = 0;
	for (int i=0; i< SIM_REGFILE_SIZE; i++)
		curState.regFile[i] = 0;
	
	initializePipe(curState.pipeStageState);
	initializePipe(nextPipe);

	for (int i=0; i< SIM_PIPELINE_DEPTH;i++){
		curRes[i]=0;
		nextRes[i]=0;
	}
	SIM_MemInstRead(curState.pc,&curState.pipeStageState[FETCH].cmd);
	return 0;
}

/*! SIM_CoreClkTick: Update the core simulator's state given one clock cycle.
  This function is expected to update the core pipeline given a clock cycle event.
*/
void SIM_CoreClkTick() {
	bool doFlush  = false;
	handleWb();

	if (enable_write == 0){
		bool add_nop = false;
		SIM_cmd tmp_cmd;
		tmp_cmd = curState.pipeStageState[DECODE].cmd;

		if (newPc ==-1)
			add_nop = checkDataHazard(tmp_cmd);

		if (add_nop){
			nextPipe[EXECUTE].cmd.opcode = CMD_NOP;
			nextPipe[EXECUTE].cmd.src1 = 0;
			nextPipe[EXECUTE].cmd.src2 = 0;
			nextPipe[EXECUTE].cmd.isSrc2Imm = 0;
			nextPipe[EXECUTE].cmd.dst = 0;
			nextPipe[EXECUTE].src1Val = 0;
			nextPipe[EXECUTE].src2Val = 0;
			nextRes[EXECUTE]= 0;		
			handleDecode(false);
		}
		else{
			updatePc();
			handleFetch();
			if (newPc != -1){
				doFlush = true;
				newPc = -1;
			}

			if (!doFlush){ //if there's no branch.
				handleDecode(true);
				handleExec();
			}
			else{
				flushPipe();
			}
		}
	}

	if (enable_write != 0)
		handleDecode(false);

	if (!doFlush){ //if there's no branch.
		enable_write = handleMem();
	}

	for (int i =0 ; i < SIM_PIPELINE_DEPTH; i++){

		if (enable_write == 0 || i != EXECUTE)
			curRes[i] = nextRes[i];

		curState.pipeStageState[i] = nextPipe[i];
		pcCurPipe[i] = pcNextPipe[i];
	}


}

/*! SIM_CoreGetState: Return the current core (pipeline) internal state
    curState: The returned current pipeline state
    The function will return the state of the pipe at the end of a cycle
*/
void SIM_CoreGetState(SIM_coreState *theirCurState) {
	theirCurState->pc = curState.pc;
	cpyRegFile(curState.regFile,theirCurState->regFile);
	cpyPipes(curState.pipeStageState,theirCurState->pipeStageState);
}

void cpyPipes (PipeStageState* src , PipeStageState* dst){
	for (int i =0 ; i < SIM_PIPELINE_DEPTH; i++)
		dst[i] = src[i];
}

void cpyRegFile(int32_t* src, int32_t* dst){
	for (int i =0 ; i <SIM_REGFILE_SIZE; i++){
		dst[i] = src[i];
	}
}

void updatePc(){
	if (newPc !=-1){ //jump to newPc - there's a branch in mem
		curState.pc = newPc;
	}
	else //no branch
		curState.pc+=4;

}

void initializePipe(PipeStageState* p ){

	for (int i =0 ; i < SIM_PIPELINE_DEPTH; i++){
		p[i].src1Val = 0;
		p[i].src2Val = 0;
		p[i].cmd.opcode = CMD_NOP;
		p[i].cmd.src1 = 0;
		p[i].cmd.src2 = 0;
		p[i].cmd.isSrc2Imm = 0;
		p[i].cmd.dst = 0;
	}
}

bool checkDataHazard(SIM_cmd cur_cmd){


	bool comesAfterLoad = (curState.pipeStageState[EXECUTE].cmd.opcode == CMD_LOAD);
	bool src1EqualsDstExec = (cur_cmd.src1 == curState.pipeStageState[EXECUTE].cmd.dst)  && (cur_cmd.src1 != 0) ;
	bool src2EqualsDstExec  = (!cur_cmd.isSrc2Imm && cur_cmd.src2 == curState.pipeStageState[EXECUTE].cmd.dst) && (cur_cmd.src2 != 0) ;
	bool dstEqualsDstExec  = (cur_cmd.dst == curState.pipeStageState[EXECUTE].cmd.dst) && (cur_cmd.dst != 0) ;
	bool instWrites = (cur_cmd.opcode == CMD_ADD || cur_cmd.opcode == CMD_SUB || cur_cmd.opcode == CMD_LOAD);

	SIM_cmd_opcode curExecCmd = curState.pipeStageState[EXECUTE].cmd.opcode ;
	SIM_cmd_opcode curMemCmd = curState.pipeStageState[MEMORY].cmd.opcode ;
	SIM_cmd_opcode curWbCmd = curState.pipeStageState[WRITEBACK].cmd.opcode ;

	bool comesAfterWriteExec = (curExecCmd == CMD_ADD || curExecCmd == CMD_SUB || curExecCmd == CMD_LOAD);
	bool comesAfterWriteMem = (curMemCmd == CMD_ADD || curMemCmd == CMD_SUB || curMemCmd == CMD_LOAD);
	bool comesAfterWriteWb = (curWbCmd == CMD_ADD || curWbCmd == CMD_SUB || curWbCmd == CMD_LOAD);

	bool isNotNop = (cur_cmd.opcode != CMD_NOP);
	bool src1EqualsDstMem = (cur_cmd.src1 == curState.pipeStageState[MEMORY].cmd.dst) && (cur_cmd.src1 != 0) ;
	bool src2EqualsDstMem  = (!cur_cmd.isSrc2Imm && cur_cmd.src2 == curState.pipeStageState[MEMORY].cmd.dst) && (cur_cmd.src2 != 0) ;
	bool src1EqualsDstWb = (cur_cmd.src1 == curState.pipeStageState[WRITEBACK].cmd.dst)  && (cur_cmd.src1 != 0) ;
	bool src2EqualsDstWb  = (!cur_cmd.isSrc2Imm && cur_cmd.src2 == curState.pipeStageState[WRITEBACK].cmd.dst) && (cur_cmd.src2 != 0) ;

	bool dstEqualsDstWb  = (cur_cmd.dst == curState.pipeStageState[WRITEBACK].cmd.dst) && ( cur_cmd.dst !=0) ;
	bool dstEqualsDstMem  = (cur_cmd.dst == curState.pipeStageState[MEMORY].cmd.dst) && ( cur_cmd.dst !=0);

//	printf("GOT TO HAZARD!\n");
	if (forwarding){

		if( comesAfterLoad && ( src1EqualsDstExec|| src2EqualsDstExec || (!instWrites && dstEqualsDstExec) )               ){
			return true;
//			printf("1111111\n");
		}
	}
	else{

//		printf("isnotnop: %d , comesAfterWRiteExec %d , src1equalsDst %d, src2equalsDst %d",isNotNop, comesAfterWriteExec, src1EqualsDstExec, src2EqualsDstExec);
		if( (isNotNop && comesAfterWriteExec &&
				( src1EqualsDstExec|| src2EqualsDstExec || (!instWrites && dstEqualsDstExec) ))){
//			printf("2222222\n");
			return true;
		}

		if( (isNotNop && comesAfterWriteMem &&
				( src1EqualsDstMem|| src2EqualsDstMem || (!instWrites && dstEqualsDstMem) ))){
//			printf("3333333\n");
			return true;
		}

		if(!split_regfile && (isNotNop && comesAfterWriteWb &&
				( src1EqualsDstWb|| src2EqualsDstWb || (!instWrites && dstEqualsDstWb) ))){
//			printf("4444444\n");
			return true;
		}
	}
//	printf("55555555\n");
	return false;
}


void handleFetch(){
	SIM_MemInstRead(curState.pc,&nextPipe[FETCH].cmd);
	pcNextPipe[FETCH] = curState.pc;
}

void handleDecode(bool cpy){

	if(cpy){
		nextPipe[DECODE].cmd = curState.pipeStageState[FETCH].cmd;	
		pcNextPipe[DECODE] = pcCurPipe[FETCH];
	}
	int reg1Num = nextPipe[DECODE].cmd.src1;
	nextPipe[DECODE].src1Val = curState.regFile[reg1Num];

	if (nextPipe[DECODE].cmd.isSrc2Imm)
		nextPipe[DECODE].src2Val = nextPipe[DECODE].cmd.src2;
	else{
		int reg2Num = nextPipe[DECODE].cmd.src2;
		nextPipe[DECODE].src2Val = curState.regFile[reg2Num];
	}
}

//TODO: handle the case of halt!
void handleExec(){
	
	int op1,op2,op3RegNum,op3,res;
	SIM_cmd cur_cmd = curState.pipeStageState[DECODE].cmd;
	bool isNotNop = (cur_cmd.opcode != CMD_NOP);

	op1 = curState.pipeStageState[DECODE].src1Val;
	op2 = curState.pipeStageState[DECODE].src2Val;
	op3RegNum = curState.pipeStageState[DECODE].cmd.dst;
	op3 = curState.regFile[op3RegNum];

	nextPipe[EXECUTE] = curState.pipeStageState[DECODE];

	if (forwarding && isNotNop){
		//Check for hazards, and handle accordingly.

//		printf("FORRRRRWARDING\n");
//		printf("curResMem is: %d",curRes[MEMORY]);
		bool src1EqualsDstWb = (cur_cmd.src1 == curState.pipeStageState[MEMORY].cmd.dst)  && (cur_cmd.src1 != 0) ;
		bool src2EqualsDstWb  = (!cur_cmd.isSrc2Imm && cur_cmd.src2 == curState.pipeStageState[MEMORY].cmd.dst) && (cur_cmd.src2 != 0) ;

		bool src1EqualsDstMem = (cur_cmd.src1 == curState.pipeStageState[EXECUTE].cmd.dst)  && (cur_cmd.src1 != 0) ;
		bool src2EqualsDstMem  = (!cur_cmd.isSrc2Imm && cur_cmd.src2 == curState.pipeStageState[EXECUTE].cmd.dst) && (cur_cmd.src2 != 0) ;

		bool dstEqualsDstWb  = (cur_cmd.dst == curState.pipeStageState[MEMORY].cmd.dst) && ( cur_cmd.dst !=0) ;
		bool dstEqualsDstMem  = (cur_cmd.dst == curState.pipeStageState[EXECUTE].cmd.dst) && ( cur_cmd.dst !=0);

		bool instWrites = (cur_cmd.opcode == CMD_ADD || cur_cmd.opcode == CMD_SUB || cur_cmd.opcode == CMD_LOAD);

		bool memInstWrites = (curState.pipeStageState[EXECUTE].cmd.opcode == CMD_ADD || curState.pipeStageState[EXECUTE].cmd.opcode == CMD_SUB || curState.pipeStageState[EXECUTE].cmd.opcode == CMD_LOAD);
		bool wbInstWrites = (curState.pipeStageState[MEMORY].cmd.opcode == CMD_ADD || curState.pipeStageState[MEMORY].cmd.opcode == CMD_SUB || curState.pipeStageState[MEMORY].cmd.opcode == CMD_LOAD);

//		printf("memInstWrites: %d , wbInstWrites: %d\n",memInstWrites, wbInstWrites);
		if (src1EqualsDstWb && wbInstWrites ){
			op1 = curRes[MEMORY];
//			printf("curRes[MEMORY] = %d \n",curRes[MEMORY]);
//			printf("Got to 1 \n");

			nextPipe[EXECUTE].src1Val = op1;
		}

		if (src2EqualsDstWb && wbInstWrites){
			op2 = curRes[MEMORY];
//			printf("Got to 2 \n");
			nextPipe[EXECUTE].src2Val = op2;
		}

		if (dstEqualsDstWb && cur_cmd.opcode == CMD_STORE){
			op3 = curRes[MEMORY];
//			printf("Got to 3 \n");
//			nextPipe[EXECUTE].cmd.dst = op3;
		}

		if (src1EqualsDstMem && memInstWrites){
			op1 = curRes[EXECUTE];
//			printf("Got to 4 \n");
			nextPipe[EXECUTE].src1Val = op1;
//			printf("curRes[EXECUTE] = %d \n",curRes[EXECUTE]);
		}

		if (src2EqualsDstMem && memInstWrites){
			op2 = curRes[EXECUTE];
//			printf("Got to 5 \n");
			nextPipe[EXECUTE].src2Val = op2;
		}

		if (dstEqualsDstMem && (cur_cmd.opcode == CMD_STORE)){
			op3 = curRes[EXECUTE];
//			printf("Got to 6 \n");
//			nextPipe[EXECUTE].cmd.dst = op3;
		}


	}

	switch(curState.pipeStageState[DECODE].cmd.opcode){
		case CMD_ADD:
			res = op1+op2;
			break;
		case CMD_SUB :
			res = op1-op2;
			break;
		case CMD_LOAD :
			res = op1+op2;
			break;
		case CMD_STORE :
			res = op2+op3;
			break;
		case CMD_BREQ:
			res = (op1==op2);
			break;
		case CMD_BRNEQ:
			res = (op1!=op2);
			break;
		default:
			break;
	}

//	printf("op1 is %d, op2 is %d\n",op1,op2);
	nextRes[EXECUTE]= res;
	pcNextPipe[EXECUTE] = pcCurPipe[DECODE];
}

int handleMem(){

	SIM_cmd_opcode curCmd;
	uint32_t adr = curRes[EXECUTE];
	int32_t buf = 0;
	int dstReg;
	int32_t dstValue;
	SIM_cmd cur_cmd;

	if (enable_write == 0){
		curCmd = curState.pipeStageState[EXECUTE].cmd.opcode;
		nextPipe[MEMORY] = curState.pipeStageState[EXECUTE];
		cur_cmd = curState.pipeStageState[EXECUTE].cmd;
	}
	else{
		curCmd = nextPipe[MEMORY].cmd.opcode;
		cur_cmd = nextPipe[MEMORY].cmd;
	}

	dstReg = nextPipe[MEMORY].cmd.dst;

	//NEW CHANGES:
	bool dstEqualsDstWb  = (cur_cmd.dst == curState.pipeStageState[WRITEBACK].cmd.dst) && ( cur_cmd.dst !=0) ;
	bool dstEqualsDstMem  = (cur_cmd.dst == curState.pipeStageState[MEMORY].cmd.dst) && ( cur_cmd.dst !=0);

	bool instWrites = (cur_cmd.opcode == CMD_ADD || cur_cmd.opcode == CMD_SUB || cur_cmd.opcode == CMD_LOAD ||  cur_cmd.opcode == CMD_STORE);

	if (dstEqualsDstWb && !instWrites){
		dstValue = curRes[WRITEBACK];
//		printf("Got to 33 \n");
	}

	if (dstEqualsDstMem && !instWrites){
		dstValue = curRes[MEMORY];
//		printf("Got to 44 \n");
	}
	dstValue = curState.regFile[dstReg];


	if (curCmd == CMD_LOAD){
//		printf("GAAAT HERE\n");
//		printf("adr is: %X\n",adr);
//		printf("buf is: %d\n",buf);
		enable_write = SIM_MemDataRead(adr,&buf);
		if (enable_write == 0){
			nextRes[MEMORY] = buf;
//			printf("buf is: %d, adr is: %X \n",buf,adr);

		}
//		printf("GOT HERE\n");
	}
	else if (curCmd == CMD_STORE){
		SIM_MemDataWrite(adr,curState.pipeStageState[EXECUTE].src1Val);
//		printf("srcVal1 is: %d, adr is: %X\n",curState.pipeStageState[EXECUTE].src1Val,adr);
	}
	else if (((curCmd == CMD_BREQ || curCmd == CMD_BRNEQ)
			&& curRes[EXECUTE])|| curCmd == CMD_BR){
		//FLUSH, Update pc.

		int32_t tempPc = pcCurPipe[EXECUTE];
		int32_t jmpAdr = tempPc+4+dstValue;
		newPc = jmpAdr;
	}

	else
		nextRes[MEMORY] = adr;
	return enable_write;
}

void handleWb(){

//	printf("Enbale write is: %d\n",enable_write);
	if (split_regfile ){
		if (enable_write == 0){
			SIM_cmd_opcode curCmd = curState.pipeStageState[MEMORY].cmd.opcode;
			if (curCmd == CMD_ADD || curCmd == CMD_SUB || curCmd == CMD_LOAD){
				int targetReg = curState.pipeStageState[MEMORY].cmd.dst;
				if (targetReg!=0)
					curState.regFile[targetReg] = curRes[MEMORY];
//				printf("regsplit curRes is: %d\n",curRes[MEMORY]);
			}
		}
	}
	else{
		SIM_cmd_opcode curCmd = curState.pipeStageState[WRITEBACK].cmd.opcode;
		if (curCmd == CMD_ADD || curCmd == CMD_SUB || curCmd == CMD_LOAD){
//			printf("GOT TO BAZINGA\n");
			int targetReg = curState.pipeStageState[WRITEBACK].cmd.dst;
			if (targetReg!=0)
				curState.regFile[targetReg] = curRes[WRITEBACK];
		}
		nextRes[WRITEBACK] = curRes[MEMORY];
//		printf("GOT TO ELSE!\n");
	}
	
	if(enable_write == 0){
		nextPipe[WRITEBACK] = curState.pipeStageState[MEMORY];
	}
	else {
		nextPipe[WRITEBACK].cmd.opcode = CMD_NOP;
		nextPipe[WRITEBACK].cmd.src1 = 0;
		nextPipe[WRITEBACK].cmd.src2 = 0;
		nextPipe[WRITEBACK].cmd.isSrc2Imm = 0;
		nextPipe[WRITEBACK].cmd.dst = 0;
		nextPipe[WRITEBACK].src1Val = 0;
		nextPipe[WRITEBACK].src2Val = 0;
		nextRes[WRITEBACK]= 0;
	}
}

void flushPipe(){
	//flushes all stages before mem.
	for(int i = DECODE ; i<=MEMORY;i++){

		nextPipe[i].cmd.opcode = CMD_NOP;
		nextPipe[i].cmd.src1 = 0;
		nextPipe[i].cmd.src2 = 0;
		nextPipe[i].cmd.isSrc2Imm = 0;
		nextPipe[i].cmd.dst = 0;
		nextPipe[i].src1Val = 0;
		nextPipe[i].src2Val = 0;
		nextRes[i]= 0;
	}

}

