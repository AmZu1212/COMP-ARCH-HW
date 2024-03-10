/* 046267 Computer Architecture - HW #1 */

/* BP - Final Version 1.2.18
 * 
 *  Written by:
 *   _______________________
 *	|						|
 *	|		Amir Zuabi		|
 *	|	  & Liz Dushkin		|
 * 	|_______________________|
 */

#include "bp_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// Parameters
#define DBG 0					// toggle for debug prints (1). leave on 0.
#define TYPE_DBG 1
#define ADDRESS_SIZE 30 		// bottom 2 bits are always 0

// Prediction Functions
bool SML_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
bool SMG_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
bool SML_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
bool SMG_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);

// Update Functions
void SML_HSL_update(bool taken, unsigned lineNum, uint32_t tag);
void SMG_HSG_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag);
void SML_HSG_update(bool taken, unsigned lineNum, uint32_t tag);
void SMG_HSL_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag);

// Init Functions
int SMG_init();
int SML_init();
int HSG_init();
int HSL_init();

// Sharing Functions
int GetLSBShare(uint32_t pc, int history, int bits);
int GetMSBShare(uint32_t pc, int history, int bits);


// PC Parsing Functions
uint32_t GetTag(uint32_t pc);
unsigned GetLineNum(uint32_t pc);


// Calcs stats at run end
unsigned CalcStats();


//	Legend:
//		  _____________________________________
//		||									   
//		||  SM - State Machine ~ מכונת מצבים  
//		||  HS - History ~ היסטוריה			  
//		||  G - Global  ~ גלובלי			  
//		||  L - local   ~ לוקאלי			  
//		||  this_Shared ~ SMG במצב רק כאשר 	  
//		||_____________________________________
//

// type definitions
// SM States
enum state_t {SNT, WNT, WT, ST};

// Predictor Types
enum predictor_t {SML_HSL, SMG_HSG, SML_HSG, SMG_HSL};

// Branch Predictor Structure, SML/SMG - HSL/HSG
struct bp_t {
	unsigned** BPtable;
	unsigned** HSLtable;
	unsigned*  HSGtable;
	unsigned** SML;
    unsigned*  SMG;
};


// Global Variables

// our branch predictor
struct bp_t *this_bp;
enum predictor_t bp_type;
enum state_t initState;		// initial SM state
int this_Shared;			// is the SMG this_Shared?  0 - no, 1 - LSB Share, 2 - MSB Share


// size variables
unsigned this_btbSize;		// BTB Size ~ amount of lines in btb
unsigned this_histSize;		// History Size
unsigned this_tagSize;		// Tag Size


// statistics variables
unsigned flush_num = 0;		// Machine flushes
unsigned br_num = 0;      	// Number of branch instructions



// Header Functions Implementations

/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return < 0
 */
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
	
	// type classification for signature functions.
	if((!isGlobalTable) && (!isGlobalHist)) {
		bp_type = SML_HSL;
		if (TYPE_DBG) printf("type is SML_HSL\n");
	}
	else if((isGlobalTable) && (isGlobalHist)){
		bp_type = SMG_HSG;
		if (TYPE_DBG) printf("type is SMG_HSG\n");
	}
	else if((!isGlobalTable) && (isGlobalHist)){
		bp_type = SML_HSG;
		if (TYPE_DBG) printf("type is SML_HSG\n");
	}
	else if((isGlobalTable) && (!isGlobalHist)){
		bp_type = SMG_HSL;
		if (TYPE_DBG) printf("type is SMG_HSL\n");
	}
	
	// Tells whether or not sm is shared. relevant only for SMG.
	this_Shared = Shared;				// SMG Shared? 1 if yes 0 if no
	this_btbSize = btbSize;				// BTB Size ~ amount of lines in btb
	this_histSize = historySize;		// History Size
	this_tagSize = tagSize;				// Tag Size
	initState = (enum state_t)fsmState;	// Starting SM state


	// this_bp initialization
	this_bp = (struct bp_t*)malloc(sizeof(struct bp_t));
	if(!this_bp) {
		if (DBG) printf("malloc(): this_bp malloc failed\n");
       	return -1;
    }


	// table initialization
	this_bp->BPtable = (unsigned**)malloc(this_btbSize * sizeof(unsigned*));

	if(!this_bp->BPtable) {
		if (DBG) printf("malloc(): BPtable malloc failed\n");
		free(this_bp);
       	return -1;
    }


	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->BPtable[i] = (unsigned*)malloc(2 * sizeof(unsigned)); // one for tag, one for prev destination
		if(!this_bp->BPtable[i]) {
			if (DBG) printf("malloc(): BPtable[%d] malloc failed\n", i);
			//cleanup
			for(uint32_t j = 0 ; j < i; j++) {
				free(this_bp->BPtable[j]);
			}
			free(this_bp->BPtable);
			free(this_bp);
			return -1;
		}
	}

	// place holder value for each lines
	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->BPtable[i][0] = 0;
	}

	int sm = -2;
	int hst = -2;
	switch (bp_type) {

		case SML_HSL:
			sm =  SML_init();
			hst = HSL_init();
			if(sm < 0 || hst < 0) {
				if (DBG) printf("BP_init(): SML_HSL allocation failed failed\n");
				free(this_bp->BPtable);
				free(this_bp);
				return -1;
			}
			
			break;

		case SMG_HSG:
			sm =  SMG_init();
			hst = HSG_init();
			if(sm < 0 || hst < 0) {
				if (DBG) printf("BP_init(): SMG_HSG allocation failed failed\n");
				free(this_bp->BPtable);
				free(this_bp);
				return -1;
			}
			
			break;

		case SML_HSG:
			sm =  SML_init();
			hst = HSG_init();
			if(sm < 0 || hst < 0) {
				if (DBG) printf("BP_init(): SML_HSG allocation failed failed\n");
				free(this_bp->BPtable);
				free(this_bp);
				return -1;
			}
			
			break;

		case SMG_HSL:
			sm =  SMG_init();
			hst = HSL_init();
			if(sm < 0 || hst < 0) {
				if (DBG) printf("BP_init(): SMG_HSL allocation failed failed\n");
				free(this_bp->BPtable);
				free(this_bp);
				return -1;
			}
			
			break;

		default:
			if (DBG) 
			{
				printf("BP_init(): switch failed, ");
				printf("invalid type");
				printf("(not supposed to get here)\n");
			}
			free(this_bp->BPtable);
			free(this_bp);
			return -1;
	}

	if (DBG) printf("BP_init(): left init successfully\n");
	return 0;
}


/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst) {
	uint32_t tag = GetTag(pc);
	unsigned lineNum = GetLineNum(pc);
	bool result;
	if (DBG) printf("BP_predict(): entered predict\n");
	switch (bp_type) {

		case SML_HSL:
			result = SML_HSL_predict(pc, dst, lineNum, tag);
			break;

		case SMG_HSG:
			result = SMG_HSG_predict(pc, dst, lineNum, tag);
			break;

		case SML_HSG:
			result = SML_HSG_predict(pc, dst, lineNum, tag);
			break;

		case SMG_HSL:
			result = SMG_HSL_predict(pc, dst, lineNum, tag);
			break;

		default:
			if (DBG) {
				printf("BP_predict(): switch failed, ");
				printf("invalid type or non-branch command");
				printf("(not supposed to get here)\n");
			}
			result = false;
	}


	if(!result) {
		//Didn't find the command or not taken
		*dst = pc + 4;
	}
	
	return result;
}


/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
	uint32_t tag = GetTag(pc);
	unsigned lineNum = GetLineNum(pc);
	if (DBG) printf("BP_update(): entered update\n");
	switch (bp_type) {

		case SML_HSL:
			SML_HSL_update(taken, lineNum, tag);
			break;

		case SMG_HSG:
			SMG_HSG_update(pc,  taken, lineNum, tag);
			break;

		case SML_HSG:
			SML_HSG_update(taken, lineNum, tag);
			break;

		case SMG_HSL:
			SMG_HSL_update(pc, taken, lineNum, tag);
			break;

		default:
			if (DBG) 
			{
				printf("BP_update(): switch failed, ");
				printf("invalid type or non-branch command");
				printf("(not supposed to get here)\n");
			}		
	}

	this_bp->BPtable[lineNum][1] = targetPc;
	br_num++;
    if ((( pred_dst != pc + 4) && !taken) || ((targetPc != pred_dst) && taken)) {
		if(DBG) printf("FLUSHED!\n");
        flush_num++;
    }
	return;
}


/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *currStats) {
	if (DBG) printf("BP_GetStats(): entered stats\n");
	unsigned size = CalcStats();
	currStats->size = size;
	currStats->br_num = br_num;
    currStats->flush_num = flush_num;

	free(this_bp->SML);
	free(this_bp->SMG);
	free(this_bp->HSGtable);
	free(this_bp->HSLtable);
	free(this_bp->BPtable);
	free(this_bp);
	return;
}

// INITIALIZATION FUNCTIONS
/* returns 0 when successful memory allocations
 * returns 1 when some malloc fails, frees everything that was allocated
 */

int SMG_init() {
	this_bp->SMG = (unsigned*)malloc((pow(2, this_histSize)) * sizeof(unsigned));	
	if(!this_bp->SMG) {
		// clear memory if malloc fails
		if (DBG) printf("malloc(): SMG malloc failed\n");
		return -1;
	}

	for(uint32_t i = 0; i < (pow(2, this_histSize)); i++) {
		this_bp->SMG[i] = (unsigned)initState;
	}

	if (DBG) printf("malloc(): SMG malloc successful\n");
	return 0;
}

int SML_init(){
	this_bp->SML = (unsigned**)malloc(this_btbSize * sizeof(unsigned*));
	if(!this_bp->SML) {
		// clear memory if malloc fails
		if (DBG) printf("malloc(): SML malloc failed\n");
		return -1;
	}

	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->SML[i] = (unsigned*)malloc(pow(2, this_histSize) * sizeof(unsigned));
		if(this_bp->SML[i] == NULL) {
			// clear memory if some malloc fails.
			if (DBG) printf("malloc(): SML[%d] malloc failed\n", i);
			for(uint32_t j = 0 ; j < i; j++) {
				free(this_bp->SML[j]);
			}
			free(this_bp->SML);
			return -1;
		}
		
	}

	// initializes state machines to initial state
	for(uint32_t i = 0; i < this_btbSize; i++) {
		for( uint32_t j = 0; j < pow(2, this_histSize); j++) {
			this_bp->SML[i][j] = (unsigned)initState;
		}
	}

	if (DBG) printf("malloc(): SML malloc successful\n");
	return 0;
}

int HSG_init(){
	this_bp->HSGtable = (unsigned*)malloc(this_histSize *  sizeof(unsigned));

	if(!this_bp->HSGtable) {
		if (DBG) printf("malloc(): HSGtable malloc failed\n");
		return -1;
	}

	// init history to 0
	for(uint32_t i = 0; i < this_histSize; i++) {
		this_bp->HSGtable[i] = 0;
	}

	if (DBG) printf("malloc(): HSGtable malloc successful\n");
	return 0;
}

int HSL_init(){
	this_bp->HSLtable = (unsigned**)malloc(this_btbSize * sizeof(unsigned*)); //rows

	if(!this_bp->HSLtable){
		printf("malloc(): HSLtable malloc failed\n");
		return -1;
	}

	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->HSLtable[i] = (unsigned*)malloc(this_histSize * sizeof(unsigned));
		if(this_bp->HSLtable[i] == NULL) {
			// clear memory
			if (DBG) printf("malloc(): HSLtable[%d] malloc failed\n", i);			
			for(uint32_t j = 0 ; j < i; j++) {
				free(this_bp->HSLtable[j]);
			}
			free(this_bp->HSLtable);
			return -1;
		}
	}

	// init histories to 0
	for(uint32_t i = 0; i < this_btbSize; i++) {
		for( uint32_t j = 0; j < this_histSize; j++) {
			this_bp->HSLtable[i][j] = 0;
		}
	}
	if (DBG) printf("malloc(): HSLtable malloc successful\n");
	return 0;
}


// PREDICTION FUNCTIONS
/* return true when prediction is taken,
 * return false when prediction is not taken
 */
bool SML_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag) {
 //found the command
	if(this_bp->BPtable[lineNum][0] == tag) {
		int historyValue = 0;
		for(uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSLtable[lineNum][i] * bitWeight;
			// this translates from binary to decimal for array accessing
		}

		int sm_val = this_bp->SML[lineNum][historyValue];

		if(sm_val >= WT) { // taken
			*dst = this_bp->BPtable[lineNum][1];
			return true;
		}
	} 
	// not taken, pc updated in bp_predict
	return false;
}

bool SMG_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag) {
	if(this_bp->BPtable[lineNum][0] == tag) {
		int historyValue = 0;
		for(uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSGtable[i] * bitWeight;
		}
		int smIndex;

		switch (this_Shared) {
			case 1://LSB share
				smIndex = GetLSBShare(pc, historyValue, this_histSize); 
				break;

			case 2://MSB share
				smIndex = GetMSBShare(pc, historyValue, this_histSize);
				break;
			
			default://No share
				smIndex = historyValue;
				break;
		}

		int sm_val = this_bp->SMG[smIndex];

		if(sm_val >= WT) { // taken
			*dst = this_bp->BPtable[lineNum][1];
			return true;
		}
	}

	// not taken, pc updated in bp_predict
	return false;
}

bool SML_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum , uint32_t tag) {
	if(this_bp->BPtable[lineNum][0] == tag){
		int historyValue = 0;
		for(uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSGtable[i] * bitWeight;
			// this translates from binary to decimal for array accessing
		}

		int sm_val = this_bp->SML[lineNum][historyValue];

		if(sm_val >= WT) { // taken
			*dst = this_bp->BPtable[lineNum][1];
			return true;
		}
	}

	// not taken, pc updated in bp_predict
	return false;
}

bool SMG_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag) {
	
	if(this_bp->BPtable[lineNum][0] == tag) {
		int historyValue = 0;
		for(uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSLtable[lineNum][i] * bitWeight;
		}

		int smIndex;
		switch (this_Shared) {
			case 1://LSB share
				smIndex = GetLSBShare(pc, historyValue, this_histSize); 
				break;

			case 2://MSB share
				smIndex = GetMSBShare(pc, historyValue, this_histSize);
				break;
			
			default://No share
				smIndex = historyValue;
				break;
		}

		int sm_val = this_bp->SMG[smIndex];
		
		if(sm_val >= WT) { // taken
			*dst = this_bp->BPtable[lineNum][1];
			return true;
		}
	}
	// not taken, pc updated in bp_predict
	return false;
}


// UPDATE FUNCTIONS
// updates the predictor with actual decision (taken / not taken)
void SML_HSL_update(bool taken, unsigned lineNum, uint32_t tag){
	//found the command
	if (this_bp->BPtable[lineNum][0] == tag) {
		int historyValue = 0;
		// get history for sm index
		for (uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSLtable[lineNum][i] * bitWeight;
		}
		
		// SM update
		if(taken) { 
			// was taken
			if (this_bp->SML[lineNum][historyValue] != ST) { // isnt maximal
				this_bp->SML[lineNum][historyValue]++;
			}
		}
		else {
			// was not taken
			if (this_bp->SML[lineNum][historyValue] != SNT) { // isnt minimal
				this_bp->SML[lineNum][historyValue]--;
			}		
		}

		//History table update
		for (uint32_t i = 0; i < this_histSize - 1; i++) {
			this_bp->HSLtable[lineNum][i] = this_bp->HSLtable[lineNum][i + 1];
		}	
	}
	
	//New command or same address for different command
	else {
		this_bp->BPtable[lineNum][0] = tag;
		for (uint32_t i = 0; i < pow(2, this_histSize); i++) {
			this_bp->SML[lineNum][i] = initState;
		}
		for (uint32_t i = 0; i < this_histSize; i++) {
			this_bp->HSLtable[lineNum][i] = 0;
		}

		// SM update
		if(taken) { 
			// was taken
			if (this_bp->SML[lineNum][0] != ST) { // isnt maximal
				this_bp->SML[lineNum][0]++;
			}
		}
		else {
			// was not taken
			if (this_bp->SML[lineNum][0] != SNT) { // isnt minimal
				this_bp->SML[lineNum][0]--;
			}		
		}
	}
	this_bp->HSLtable[lineNum][this_histSize - 1] = (int)taken;
}

void SMG_HSG_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag){
	this_bp->BPtable[lineNum][0] = tag;
	int historyValue = 0;
	// get history for sm index
	for (uint32_t i = 0; i < this_histSize; i++) {
		int bitWeight = pow(2, this_histSize - i - 1);
		historyValue += this_bp->HSGtable[i] * bitWeight;
	}

	// find sm index
	int smIndex;
	switch (this_Shared) {
		case 1://LSB share
			smIndex = GetLSBShare(pc, historyValue, this_histSize); 
			break;

		case 2://MSB share
			smIndex = GetMSBShare(pc, historyValue, this_histSize);
			break;
		
		default://No share
			smIndex = historyValue;
			break;
	}


	// SM update
	if(taken) { 
		// was taken
		if (this_bp->SMG[smIndex] != ST) { // isnt maximal
			this_bp->SMG[smIndex]++;
		}
	}
	else {
		// was not taken
		if (this_bp->SMG[smIndex] != SNT) { // isnt minimal
			this_bp->SMG[smIndex]--;
		}		
	}

	// History update
	// history bit shift
	for (uint32_t i = 0; i < this_histSize - 1; i++) {
		this_bp->HSGtable[i] = this_bp->HSGtable[i + 1];
	}

	//history bit fill
	this_bp->HSGtable[this_histSize - 1] = (int)taken;
}

void SML_HSG_update(bool taken, unsigned lineNum, uint32_t tag){
	int historyValue = 0;
	for (uint32_t i = 0; i < this_histSize; i++) {
		int bitWeight = pow(2, this_histSize - i - 1);
		historyValue += this_bp->HSGtable[i] * bitWeight;
	}

	// if new command, reset entry
	if (this_bp->BPtable[lineNum][0] != tag) {
		// tag update
		this_bp->BPtable[lineNum][0] = tag;
		
		// SM reset
		for (uint32_t j = 0; j < pow(2, this_histSize); j++) {
			this_bp->SML[lineNum][j] = initState;
		}
	}

	// SM update
	if(taken) { 
		// was taken
		if (this_bp->SML[lineNum][historyValue] != ST) { // isnt maximal
			this_bp->SML[lineNum][historyValue]++;
		}
	}
	else {
		// was not taken
		if (this_bp->SML[lineNum][historyValue] != SNT) { // isnt minimal
			this_bp->SML[lineNum][historyValue]--;
		}		
	}
	

	// History update
	// history bit shift
	for (uint32_t i = 0; i < this_histSize - 1; i++) {
		this_bp->HSGtable[i] = this_bp->HSGtable[i + 1];
	}

	// history bit fill
	this_bp->HSGtable[this_histSize - 1] = (int)taken;
}

void SMG_HSL_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag){
	// command found
	int historyValue = 0;
	if (this_bp->BPtable[lineNum][0] == tag) {
		// get history for sm index
		for (uint32_t i = 0; i < this_histSize; i++) {
			int bitWeight = pow(2, this_histSize - i - 1);
			historyValue += this_bp->HSLtable[lineNum][i] * bitWeight;
		}
		
		//History table update           
		for (uint32_t j = 0; j < this_histSize - 1; j++) {
			this_bp->HSLtable[lineNum][j] = this_bp->HSLtable[lineNum][j + 1];
		}
	}
	else {
		//same mapping for different command
		this_bp->BPtable[lineNum][0] = tag;           
		for (uint32_t j = 0; j < this_histSize; j++) {
			this_bp->HSLtable[lineNum][j] = 0;
		}
	}

	// find SM index
	int smIndex;
	switch (this_Shared) {
		case 1://LSB share
			smIndex = GetLSBShare(pc, historyValue, this_histSize); 
			break;

		case 2://MSB share
			smIndex = GetMSBShare(pc, historyValue, this_histSize);
			break;
		
		default://No share
			smIndex = historyValue;
			break;
	}
	
	// SM update
	if(taken) { 
		// was taken
		if (this_bp->SMG[smIndex] != ST) { // isnt maximal
			this_bp->SMG[smIndex]++;
		}
	}
	else {
		// was not taken
		if (this_bp->SMG[smIndex] != SNT) { // isnt minimal
			this_bp->SMG[smIndex]--;
		}
	}

	this_bp->HSLtable[lineNum][this_histSize - 1] = (int)taken;
}


// Sharing Functions
// used for getting the sm index for a SMG, using pc/4 and size.(share = 1)
int GetLSBShare(uint32_t pc, int history, int size) {
	int index = pc / (unsigned)pow(2, 2); 			// remove 2 bottom bits
	int LSB_size = (unsigned)pow(2, size); 			// number of bits we want
	index = index % LSB_size;						// the share bits
	index = index ^ history; 						// xor operation with history
	return index;
}

// used for getting the sm index for a SMG, using pc/16 and size.(share = 2)
int GetMSBShare(uint32_t pc, int history, int size) {
	int index = pc / (unsigned)pow(2, 16);			// remove 16 bottom bits
	int MSB_size = (unsigned)pow(2, size); 			// number of bits we want
	index = index % MSB_size;						// the share bits
	index = index ^ history; 						// xor operation with history
	return index;
}


// PC Parsing Functions
// parses tag out of the pc
uint32_t GetTag(uint32_t pc) {
	uint32_t tag = (pc / 4) / this_btbSize;
	tag = tag % ((unsigned)pow(2, this_tagSize));
	return tag;
}

// parses line out of the pc
unsigned GetLineNum(uint32_t pc){
	unsigned line = (pc / 4) % this_btbSize;
	return line;
}

// statistic formulas, returns BP size (depends on bp_type)
unsigned CalcStats(){
	if(DBG) printf("CalcStats(): entered CalcStats\n");
	unsigned size;
	switch (bp_type) {
		case SML_HSL:
			size = this_btbSize * (ADDRESS_SIZE + this_tagSize + 1 + this_histSize + 2 * pow(2.0, (double)this_histSize));	
			break;

		case SMG_HSG:
			size = this_btbSize * (ADDRESS_SIZE + this_tagSize + 1)  + this_histSize + 2 * pow(2.0, (double)this_histSize);
			break;

		case SML_HSG:
			size = this_btbSize * (ADDRESS_SIZE + this_tagSize + 1 + 2 * pow(2.0, (double)this_histSize)) + this_histSize;
			break;

		case SMG_HSL:
			size = this_btbSize * (ADDRESS_SIZE + this_tagSize + 1 + this_histSize) + 2 * pow(2.0, (double)this_histSize);
			break;	
	}
	if(DBG) printf("CalcStats(): left CalcStats\n");
	return size;
}
