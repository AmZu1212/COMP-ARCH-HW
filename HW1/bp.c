/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#define DEBUG 1
#define ADDRESS 30
#define MAXUNSIGNED 4294967295

//	Definitions:
//		SM - State Machine ~  מכונת מצבים
//		HS - History ~ היסטוריה
//		G - Global ~ גלובלי
//		L - local ~ לוקאלי
//		Shared ~ רלוונטי רק כאשר SMG

// type definitions
enum state_t {SNT, WNT, WT, ST};
enum predictor_t {SML_HSL, SMG_HSG, SML_HSG, SMG_HSL};
struct bp_t {
	unsigned** BPTable;
	unsigned** HSLtable;
	unsigned*  HSGtable;
	unsigned** SML;
    unsigned*  SMG;
};


// our branch predictor
struct bp_t *this_bp;
enum predictor_t bp_type;

// our variables and sizes
unsigned this_btbSize;		// BTB Size ~ amount of lines in btb
unsigned this_histSize;		// History Size
unsigned this_tagSize;		// Tag Size
enum state_t initState;		// Starting SM state
int this_Shared;			// is the SMG Shared? 1 if yes 0 if no

// for statistics:
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


	// bp initialization
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

	// 2 for each line?
	for(uint32_t i = 0; i < this_btbSize; i++) {
		this_bp->BPTable[i] = (unsigned*)malloc(2 * sizeof(unsigned));
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
			if(!this_bp->HSLtable[i]) {
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
			if (DEBUG) printf("malloc(): SMG malloc failed\n");
			free(this_bp->BPTable);
			free(this_bp);
			// not freeing history here because its too complicated
			return -1;
		}

        for(uint32_t i = 0; i < this_btbSize; i++) {
        	this_bp->SML[i] = (unsigned*)malloc(pow(2, this_histSize) * sizeof(unsigned));
			if (DEBUG) printf("malloc(): HSLtable[%d] malloc failed\n", i);
			for(uint32_t j = 0 ; j < i; j++) {
				free(this_bp->SML[j]);
			}
			// not freeing history here because its too complicated
        }

		// initializes state machines to initial state
        for(uint32_t i = 0; i < this_btbSize; i++) {
            for( uint32_t j = 0; j < pow(2, this_histSize); j++) {
            	this_bp->SML[i][j] = (unsigned)initState;
            }
        }
	}

	return 0;
}


/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}


/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}


/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *curStats){
	return;
}

