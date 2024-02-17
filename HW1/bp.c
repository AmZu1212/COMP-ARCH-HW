/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// Parameters
#define DEBUG 0
#define ADDRESS 30
#define MAXUNSIGNED 4294967295

// Prediction Functions
bool SML_HSL_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag);
bool SMG_HSG_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag);
bool SML_HSG_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag);
bool SMG_HSL_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag);

// Update Functions
void SML_HSL_update(bool taken, unsigned tag_idx, uint32_t tag);
void SMG_HSG_update(uint32_t pc, bool taken, unsigned tag_idx, uint32_t tag);
void SML_HSG_update(bool taken, unsigned tag_idx, uint32_t tag);
void SMG_HSL_update(uint32_t pc, bool taken, unsigned tag_idx, uint32_t tag);



//	Definitions:
//		SM - State Machine ~  מכונת מצבים
//		HS - History ~ היסטוריה
//		G - Global ~ גלובלי
//		L - local ~ לוקאלי
//		this_Shared ~ רלוונטי רק כאשר SMG

// type definitions
// SM States
enum state_t {SNT, WNT, WT, ST};

// Predictor Types
enum predictor_t {SML_HSL, SMG_HSG, SML_HSG, SMG_HSL};

// Branch Predictor Structure, SML/SMG - HSL/HSG
struct bp_t {
	unsigned** BPTable;
	unsigned** HSLtable;
	unsigned*  HSGtable;
	unsigned** SML;
    unsigned*  SMG;
};


//					Globals

// bp
struct bp_t *this_bp;
enum predictor_t bp_type;

// sizes and variables
unsigned this_btbSize;		// BTB Size ~ amount of lines in btb
unsigned this_histSize;		// History Size
unsigned this_tagSize;		// Tag Size
enum state_t initState;		// Starting SM state
int this_Shared;			// is the SMG this_Shared? 1 if yes 0 if no

// statistics
unsigned flush_num = 0;		// Machine flushes
unsigned br_num = 0;      	// Number of branch instructions


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
	}
	else if((isGlobalTable) && (isGlobalHist)){
		bp_type = SMG_HSG;
	}
	else if((!isGlobalTable) && (isGlobalHist)){
		bp_type = SML_HSG;
	}
	else if((isGlobalTable) && (!isGlobalHist)){
		bp_type = SMG_HSL;
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
		if (DEBUG) printf("malloc(): this_bp malloc failed\n");
       	return -1;
    }


	// table initialization
	this_bp->BPTable = (unsigned**)malloc(this_btbSize * sizeof(unsigned*));

	if(!this_bp->BPTable) {
		if (DEBUG) printf("malloc(): BPTable malloc failed\n");
		free(this_bp);
       	return -1;
    }


	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->BPTable[i] = (unsigned*)malloc(2 * sizeof(unsigned)); // one for tag, one for prev destination
		if(!this_bp->BPTable[i]) {
			if (DEBUG) printf("malloc(): BPTable[%d] malloc failed\n", i);
			//cleanup
			for(uint32_t j = 0 ; j < i; j++) {
				free(this_bp->BPTable[j]);
			}
			free(this_bp->BPTable);
			free(this_bp);
			return -1;
		}
	}

	// place holder value for each lines
	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->BPTable[i][0] = MAXUNSIGNED;
	}


	// history initizalization this_bp->HSGtable
	if(bp_type == SMG_HSG || bp_type == SML_HSG) {

        this_bp->HSGtable = (unsigned*)malloc(this_histSize *  sizeof(unsigned));

        if(!this_bp->HSGtable) {
			if (DEBUG) printf("malloc(): HSGtable malloc failed\n");
			free(this_bp->BPTable);
			free(this_bp);
			return -1;
		}

		// init history to 0
        for(uint32_t i = 0; i < this_histSize; i++) {
        	this_bp->HSGtable[i] = 0;
        }
	}
	else { // SMG_HSL or SML_HSL

		this_bp->HSLtable = (unsigned**)malloc(this_btbSize * sizeof(unsigned*)); //rows

		if(!this_bp->HSLtable){
			printf("malloc(): HSLtable malloc failed\n");
			free(this_bp->BPTable);
			free(this_bp);
			return -1;
		}

        for(uint32_t i = 0; i < this_btbSize; i++) {
        	this_bp->HSLtable[i] = (unsigned*)malloc(this_histSize * sizeof(unsigned)); //columns
			if(this_bp->HSLtable[i] == NULL) {
				if (DEBUG) printf("malloc(): HSLtable[%d] malloc failed\n", i);
				//cleanup
				
				for(uint32_t j = 0 ; j < i; j++) {
					free(this_bp->HSLtable[j]);
				}
				free(this_bp->HSLtable);
				free(this_bp->BPTable);
				free(this_bp);
				return -1;
			}
		}

		// init histories to 0
        for(uint32_t i = 0; i < this_btbSize; i++) {
            for( uint32_t j = 0; j < this_histSize; j++) {
            	this_bp->HSLtable[i][j] = 0;
            }
        }
	}


	// state machines initizalization (global then local)
	if(bp_type == SMG_HSG || bp_type == SMG_HSL) {

        this_bp->SMG = (unsigned*)malloc((pow(2, this_histSize))*sizeof(unsigned));
		
		if(!this_bp->SMG) {
			if (DEBUG) printf("malloc(): SMG malloc failed\n");
			free(this_bp->BPTable);
			free(this_bp);
			// not freeing history here because its too complicated
			return -1;
		}

        for(uint32_t i = 0; i < (pow(2, this_histSize)); i++) {
        	this_bp->SMG[i] = (unsigned)initState;
        }
	} else {

		this_bp->SML = (unsigned**)malloc(this_btbSize * sizeof(unsigned*));
		if(!this_bp->SML) {
			if (DEBUG) printf("malloc(): SML malloc failed\n");
			free(this_bp->BPTable);
			free(this_bp);
			// not freeing history here because its too complicated
			return -1;
		}

        for(uint32_t i = 0; i < this_btbSize; i++) {
        	this_bp->SML[i] = (unsigned*)malloc(pow(2, this_histSize) * sizeof(unsigned));
			if(this_bp->SML == NULL) {
				if (DEBUG) printf("malloc(): SML[%d] malloc failed\n", i);
				for(uint32_t j = 0 ; j < i; j++) {
					free(this_bp->SML[j]);
				}
				return -1;
				// not freeing history here because its too complicated
			}
			
        }

		// initializes state machines to initial state
        for(uint32_t i = 0; i < this_btbSize; i++) {
            for( uint32_t j = 0; j < pow(2, this_histSize); j++) {
            	this_bp->SML[i][j] = (unsigned)initState;
            }
        }
	}

	if (DEBUG) printf("BP_init(): left init successfully\n");
	return 0;
}


/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst) {
	uint32_t tag = ((pc / 4) / this_btbSize) % ((unsigned)pow(2, this_tagSize));
	unsigned tag_idx = (pc / 4) % (this_btbSize);
	bool result;
	if (DEBUG) printf("BP_predict(): entered predict\n");
	switch (bp_type) {

		case SML_HSL:
			result = SML_HSL_predict(pc, dst, tag_idx, tag);
			break;

		case SMG_HSG:
			result = SMG_HSG_predict(pc, dst, tag_idx, tag);
			break;

		case SML_HSG:
			result = SML_HSG_predict(pc, dst, tag_idx, tag);
			break;

		case SMG_HSL:
			result = SMG_HSL_predict(pc, dst, tag_idx, tag);
			break;

		default:
			if (DEBUG) {
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
	br_num++;
	uint32_t tag = ((pc / 4) / this_btbSize) % ((unsigned)pow(2, this_tagSize));
	unsigned tag_idx = (pc / 4) % (this_btbSize);
	if (DEBUG) printf("BP_update(): entered update\n");
	switch (bp_type) {

		case SML_HSL:
			SML_HSL_update(taken, tag_idx, tag);
			break;

		case SMG_HSG:
			SMG_HSG_update(pc,  taken, tag_idx, tag);
			break;

		case SML_HSG:
			SML_HSG_update(taken, tag_idx, tag);
			break;

		case SMG_HSL:
			SMG_HSL_update(pc, taken, tag_idx, tag);
			break;

		default:
			if (DEBUG) 
			{
				printf("BP_update(): switch failed, ");
				printf("invalid type or non-branch command");
				printf("(not supposed to get here)\n");
			}		
	}

	this_bp->BPTable[tag_idx][1] = targetPc;//?

    if ((( pred_dst != pc + 4) && !taken) || ((targetPc != pred_dst) && taken)) {
		if(DEBUG) printf("FLUSHED!\n");
        flush_num++;
    }
	return;
}


/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *currStats) {
	currStats->br_num = br_num;
    currStats->flush_num = flush_num;
	if (DEBUG) printf("BP_GetStats(): entered stats\n");
	switch (bp_type) {

		case SML_HSL:
			currStats->size = this_btbSize * (ADDRESS + this_tagSize + 1 + this_histSize + 2 * pow(2.0, (double)this_histSize));
			free(this_bp->SML);
			free(this_bp->HSLtable);	
			break;

		case SMG_HSG:
			currStats->size = this_btbSize * (ADDRESS + this_tagSize + 1)  + this_histSize + 2 * pow(2.0, (double)this_histSize);
			free(this_bp->SMG);
			free(this_bp->HSGtable);
			break;

		case SML_HSG:
			currStats->size = this_btbSize * (ADDRESS + this_tagSize + 1 + 2 * pow(2.0, (double)this_histSize)) + this_histSize;
			free(this_bp->SML);
			free(this_bp->HSGtable);
			break;

		case SMG_HSL:
			currStats->size = this_btbSize * (ADDRESS + this_tagSize + 1 + this_histSize) + 2 * pow(2.0, (double)this_histSize);
			free(this_bp->SMG);
			free(this_bp->HSLtable);
			break;

		default:
			if (DEBUG) {
				printf("BP_GetStats(): switch failed, ");
				printf("invalid type or non-branch command");
				printf("(not supposed to get here)\n");
			}		
	}
	
	free(this_bp->BPTable);
	free(this_bp);
	return;
}


// PREDICTION FUNCTIONS
/* return true when prediction is taken,
 * return false when prediction is not taken) 
 */
bool SML_HSL_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag) {
 //found the command
	if(this_bp->BPTable[tag_idx][0] == tag) {
		int hist_val = 0;
		for(uint32_t j = 0; j < this_histSize; j++) {
			hist_val += (this_bp->HSLtable[tag_idx][j]) * (pow(2, (this_histSize-j-1)));
			// this translates from binary to decimal for array accessing
		}
		int predict_val = this_bp->SML[tag_idx][hist_val];
		bool prediction = (predict_val == 0 || predict_val == 1)? false : true;
		if(prediction) {
			*dst = this_bp->BPTable[tag_idx][1];
			
		}
		return prediction;
	} 
	return false;
}

bool SMG_HSG_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag) {
	if(this_bp->BPTable[tag_idx][0] == tag) {
		int hist_val = 0;
		for(uint32_t j = 0; j < this_histSize; j++) {
			hist_val += (this_bp->HSGtable[j])*(pow(2, ((this_histSize)-j-1)));
		}
		int machine_address;
		//LSB share
		if(this_Shared == 1) {
			machine_address = ((pc/((unsigned)(pow(2,2))) % ((unsigned)pow(2, this_histSize)))^(hist_val));
		}
		//MSB share
		else if(this_Shared == 2) {
			machine_address = ((pc/((unsigned)(pow(2,16))) % ((unsigned)pow(2, this_histSize)))^(hist_val));
		}
		//No share
		else {
			machine_address = hist_val;
		}
		int predict_val = this_bp->SMG[machine_address];
		bool prediction = (predict_val < WT) ? false : true;
		if(prediction) {
			*dst = this_bp->BPTable[tag_idx][1];
		}
		return prediction;
	}
	return false;
}

bool SML_HSG_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx , uint32_t tag) {
	if(this_bp->BPTable[tag_idx][0] == tag){
		int hist_val = 0;
		for(uint32_t i = 0; i < this_histSize; i++) {
			hist_val += (this_bp->HSGtable[i]) * (pow(2, (this_histSize-i-1)));
			// this translates from binary to decimal for array accessing
		}
		int predict_val = this_bp->SML[tag_idx][hist_val];
		bool prediction = (predict_val == 0 || predict_val == 1) ? false : true;
		if(prediction) {
			*dst = this_bp->BPTable[tag_idx][1];
		}
		return prediction;
	}
	return false;
}

bool SMG_HSL_predict(uint32_t pc, uint32_t *dst, unsigned tag_idx, uint32_t tag) {
	if(this_bp->BPTable[tag_idx][0] == tag) {
		int hist_val = 0;
		for(uint32_t j = 0; j < this_histSize; j++) {
			hist_val += (this_bp->HSLtable[tag_idx][j]) * (pow(2, (this_histSize-j-1)));
		}
		int machine_address;
		//LSB share
		if(this_Shared == 1) {
			machine_address = ((pc/((unsigned)(pow(2,2))) % ((unsigned)pow(2, this_histSize))) ^ (hist_val));
		}
		//MSB share
		else if(this_Shared == 2) {
			machine_address = ((pc/((unsigned)(pow(2,16))) % ((unsigned)pow(2, this_histSize))) ^ (hist_val));               
		}
		//No share
		else {
			machine_address = hist_val;
		}
		int predict_val = this_bp->SMG[machine_address];
		bool prediction = (predict_val == 0 || predict_val == 1) ? false : true;
		if(prediction) {
			*dst = this_bp->BPTable[tag_idx][1];
		}
		return prediction;
	}
	return false;
}


// UPDATE FUNCTIONS
// updates the predictor with actual decision (taken / not taken)
void SML_HSL_update(bool taken, unsigned tag_idx, uint32_t tag){
	//found the command
	if (this_bp->BPTable[tag_idx][0] == tag) {
		int hist_val = 0;
		for (uint32_t i = 0; i < this_histSize; i++) {
			hist_val += (this_bp->HSLtable[tag_idx][i]) * (pow(2, (this_histSize - i - 1)));
		}
		//SM update
		if (this_bp->SML[tag_idx][hist_val] != ST && taken) {
			(this_bp->SML[tag_idx][hist_val])++;
		}
		if (this_bp->SML[tag_idx][hist_val] != SNT && !taken) {
			(this_bp->SML[tag_idx][hist_val])--;
		}
		//History table update
		for (uint32_t i = 0; i < this_histSize - 1; i++) {
			this_bp->HSLtable[tag_idx][i] = this_bp->HSLtable[tag_idx][i + 1];
		}
		this_bp->HSLtable[tag_idx][(this_histSize) - 1] = (taken) ? 1 : 0;
	}
	
	//New command or same address for different command
	else {
		this_bp->BPTable[tag_idx][0] = tag;
		for (uint32_t i = 0; i < pow(2, this_histSize); i++) {
			this_bp->SML[tag_idx][i] = initState;
		}
		for (uint32_t i = 0; i < this_histSize; i++) {
			this_bp->HSLtable[tag_idx][i]=0;
		}
		//SM update
		if (this_bp->SML[tag_idx][0] != ST && taken) {
			(this_bp->SML[tag_idx][0])++;
		}
		if (this_bp->SML[tag_idx][0] != SNT && !taken) {
			(this_bp->SML[tag_idx][0])--;
		}
		this_bp->HSLtable[tag_idx][this_histSize -1] = (taken)? 1 : 0;
	}
}

void SMG_HSG_update(uint32_t pc, bool taken, unsigned tag_idx, uint32_t tag){
	this_bp->BPTable[tag_idx][0] = tag;
	int hist_val = 0;
	int machine_address;
	for (uint32_t i = 0; i < this_histSize; i++) {
		hist_val += (this_bp->HSGtable[i]) * (pow(2, ((this_histSize) - i - 1)));
	}
	//LSB share
	if(this_Shared == 1){
		machine_address = (((pc/4)%((unsigned)pow(2,this_histSize)))^(hist_val));
	}
	//MSB share
	else if(this_Shared == 2){
		machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,this_histSize)))^(hist_val));
	}
	else {
		machine_address = hist_val;
	}

	if (taken && this_bp->SMG[machine_address] != ST) {
		this_bp->SMG[machine_address]++;
	}
	if (!taken && this_bp->SMG[machine_address] != SNT) {
		this_bp->SMG[machine_address]--;
	}
	
	//shift register
	for (uint32_t i = 0; i < this_histSize - 1; i++) {
		this_bp->HSGtable[i] = this_bp->HSGtable[i + 1];
	}
	this_bp->HSGtable[this_histSize - 1] = (taken) ? 1 : 0;
}

void SML_HSG_update(bool taken, unsigned tag_idx, uint32_t tag){
	int hist_val = 0;
	if (this_bp->BPTable[tag_idx][0] == tag) {
		for (uint32_t i = 0; i < this_histSize; i++) {
			hist_val += (this_bp->HSGtable[i]) * (pow(2, ((this_histSize) - i - 1)));
		}
		//SM update
		if (this_bp->SML[tag_idx][hist_val] != ST && taken) {
			this_bp->SML[tag_idx][hist_val] ++;
		}
		if (this_bp->SML[tag_idx][hist_val] != SNT && !taken) {
			this_bp->SML[tag_idx][hist_val] --;
		}
		//History table update
		for (uint32_t i = 0; i < this_histSize - 1; i++) {
			this_bp->HSGtable[i] = this_bp->HSGtable[i + 1];
		}
		this_bp->HSGtable[this_histSize - 1] = (taken) ? 1 : 0;
	}
	//New command or same address for different command
	else {
		hist_val = 0;
		this_bp->BPTable[tag_idx][0] = tag;
		for (uint32_t j = 0; j < this_histSize; j++) {
			hist_val += (this_bp->HSGtable[j]) * (pow(2, ((this_histSize) - j - 1)));
		}
		for (uint32_t j = 0; j < pow(2, this_histSize); j++) {
			this_bp->SML[tag_idx][j] = initState;
		}
		//SM update
		if (this_bp->SML[tag_idx][hist_val] != ST && taken) {
			(this_bp->SML[tag_idx][hist_val])++;
		}
		if (this_bp->SML[tag_idx][hist_val] != SNT && !taken) {
			(this_bp->SML[tag_idx][hist_val])--;
		}

		for (uint32_t j = 0; j < this_histSize - 1; j++) {
			this_bp->HSGtable[j] = this_bp->HSGtable[j + 1];
		}
		this_bp->HSGtable[this_histSize - 1] = (taken) ? 1 : 0;
	}
}

void SMG_HSL_update(uint32_t pc, bool taken, unsigned tag_idx, uint32_t tag){
	//found the command
	if (this_bp->BPTable[tag_idx][0] == tag) {;
		int hist_val = 0;
		for (uint32_t i = 0; i < this_histSize; i++) {
			hist_val += (this_bp->HSLtable[tag_idx][i]) * (pow(2, ((this_histSize) - i - 1)));
		}
		int machine_address;
		//LSB share
		if(this_Shared == 1){
			machine_address = (((pc/4)%((unsigned)pow(2,this_histSize)))^(hist_val));
		}
		//MSB share
		else if(this_Shared == 2){
			machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,this_histSize)))^(hist_val));
		}
		//No share
		else{
			machine_address = hist_val;
		}
		//SM update
		if (this_bp->SMG[machine_address] != ST && taken) {
			this_bp->SMG[machine_address] ++;
		}
		if (this_bp->SMG[machine_address] != SNT && !taken) {
			this_bp->SMG[machine_address] --;
		}
		//History table update           
		for (uint32_t i = 0; i < (this_histSize) - 1; i++) {
			this_bp->HSLtable[tag_idx][i] = this_bp->HSLtable[tag_idx][i + 1];
		}
		this_bp->HSLtable[tag_idx][(this_histSize) - 1] = (taken) ? 1 : 0;
	}
	//New command or same address for different command
	else {
		this_bp->BPTable[tag_idx][0] = tag;           
		int hist_val = 0;
		for (uint32_t i = 0; i < this_histSize; i++) {
			this_bp->HSLtable[tag_idx][i] = 0;
		}
		int machine_address;
		//LSB share
		if(this_Shared == 1){
			machine_address = (((pc/4)%((unsigned)pow(2,this_histSize))) ^ (hist_val));
		}
		//MSB share
		else if(this_Shared == 2){
			machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2, this_histSize))) ^ (hist_val));
		}
		//No share
		else{
			machine_address = hist_val;
		}
		
		//SM update
		if (this_bp->SMG[machine_address] != ST && taken) {
			this_bp->SMG[machine_address] ++;
		}
		if (this_bp->SMG[machine_address] != SNT && !taken) {
			this_bp->SMG[machine_address] --;
		}

		this_bp->HSLtable[tag_idx][this_histSize - 1] = (taken) ? 1 : 0;
	}
}