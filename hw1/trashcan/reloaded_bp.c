/* 046267 Computer Architecture - HW #1 */

/* BP - V2
 * 
 *  Written by:
 *   _______________________
 *	|						|
 *	|		Amir Zuabi		|
 *	|	        &           | 
 *  |       Liz Dushkin		|
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
#define TYPE_DBG 0
#define ADDRESS_SIZE 30 		// bottom 2 bits are always 0
#define MAXUNSIGNED 4294967295

// Prediction Functions
//bool SML_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
//bool SMG_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
//bool SML_HSG_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);
//bool SMG_HSL_predict(uint32_t pc, uint32_t *dst, unsigned lineNum, uint32_t tag);

// Update Functions
//void SML_HSL_update(bool taken, unsigned lineNum, uint32_t tag);
//void SMG_HSG_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag);
//void SML_HSG_update(bool taken, unsigned lineNum, uint32_t tag);
//void SMG_HSL_update(uint32_t pc, bool taken, unsigned lineNum, uint32_t tag);

// Init Functions
//int SMG_init();
//int SML_init();
//int HSG_init();
//int HSL_init();

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

	//history table initialization
	if(isGlobalHist){
        BP->GHISTtable = (unsigned*)malloc(historySize*sizeof(unsigned));
        if(BP->GHISTtable==NULL){
			free(BP->BPtable);
			return -1;
		}
        for(uint32_t i=0; i<historySize; i++){
        	BP->GHISTtable[i] = 0;
        }
	}

	else{
       	BP->LHISTtable = (unsigned**)malloc(btbSize*sizeof(unsigned*));//rows
        for(uint32_t i=0; i<btbSize; i++){
        	BP->LHISTtable[i] = (unsigned*)malloc(historySize*sizeof(unsigned));//columns
        }
        if(BP->LHISTtable==NULL){
			free(BP->BPtable);
			return -1;
		}
        for(uint32_t i=0; i<btbSize; i++){
            for( uint32_t j=0; j<historySize; j++){
            	BP->LHISTtable[i][j] = 0;
            }
        }
	}
	//state machine table initialization
	if(isGlobalTable){
        BP->GSM = (unsigned*)malloc((pow(2,historySize))*sizeof(unsigned));
        for(uint32_t i=0; i<(pow(2,historySize)); i++){
        	BP->GSM[i] = fsmState;
        }
	}
	else{
       	BP->LSM = (unsigned**)malloc(btbSize*sizeof(unsigned*));
        for(uint32_t i=0; i<btbSize; i++){
        	BP->LSM[i] = (unsigned*)malloc(pow(2,historySize)*sizeof(unsigned));
        }
        for(uint32_t i=0; i<btbSize; i++){
            for( uint32_t j=0; j<(pow(2,historySize)); j++){
            	BP->LSM[i][j] = fsmState;
            }
        }
	}
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	uint32_t tag = ((pc / 4)/BP->btbSize) % ((unsigned)pow(2, BP->tagSize));
	unsigned tag_idx = (pc / 4) % (BP->btbSize);
    
    //Both SM and hist are local
	if(!(BP->isGlobalHist) && !(BP->isGlobalTable)){
            //found the command
            if(BP->BPtable[tag_idx][0] == tag){
                int hist_val = 0;
                for(uint32_t j=0; j<BP->historySize; j++){
                    hist_val += (BP->LHISTtable[tag_idx][j])*(pow(2,((BP->historySize)-j-1)));
                }
                int predict_val = BP->LSM[tag_idx][hist_val];
                bool prediction = (predict_val == 0 || predict_val == 1)? false : true;
                if(prediction){
                    *dst = BP->BPtable[tag_idx][1];
                   
                }
                else{
                    *dst = pc + 4;
                }
                return prediction;
            }
            
        
    }

	//Both SM and hist are global
	if(BP->isGlobalHist && BP->isGlobalTable){    
		if(BP->BPtable[tag_idx][0] == tag){
            	int hist_val = 0;
                for(uint32_t j=0; j<BP->historySize; j++){
                    hist_val += (BP->GHISTtable[j])*(pow(2,((BP->historySize)-j-1)));
                }
                int machine_address;
                //LSB share
                if(BP->Shared == 1){
                    machine_address = (((pc/4)%((int)pow(2,BP->historySize)))^(hist_val));
                }
                //MSB share
                else if(BP->Shared == 2){
                    machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,BP->historySize)))^(hist_val));
                }
                //No share
                else{
                    machine_address = hist_val;
                }
                int predict_val = BP->GSM[machine_address];
                bool prediction = (predict_val == 0 || predict_val == 1)? false : true;
                
                if(prediction){
                    *dst = BP->BPtable[tag_idx][1];
                }
                else{
                    *dst = pc + 4;
                }
                return prediction;
            }
	}

    //Local SM and global hist
	if(BP->isGlobalHist && !BP->isGlobalTable){
            //found the command
            if(BP->BPtable[tag_idx][0] == tag){
                int hist_val = 0;
                for(uint32_t j=0; j<BP->historySize; j++){
                    hist_val += (BP->GHISTtable[j])*(pow(2,((BP->historySize)-j-1)));
                }
                int predict_val = BP->LSM[tag_idx][hist_val];
                bool prediction = (predict_val == 0 || predict_val == 1)? false : true;
                if(prediction){
                    *dst = BP->BPtable[tag_idx][1];
                }
                else{
                    *dst = pc + 4;
                }
                return prediction;
            }
	}

	//Global SM and local hist
	if(!BP->isGlobalHist && BP->isGlobalTable){
            if(BP->BPtable[tag_idx][0] == tag){
                int hist_val = 0;
                for(uint32_t j=0; j<BP->historySize; j++){
                	hist_val += (BP->LHISTtable[tag_idx][j])*(pow(2,((BP->historySize)-j-1)));
                }
                int machine_address;
                //LSB share
                if(BP->Shared == 1){
                    machine_address = ((pc/4)%((unsigned)pow(2,BP->historySize)))^(hist_val);
                }
                //MSB share
                else if(BP->Shared == 2){
                	machine_address = ((pc/((unsigned)pow(2,16)))%((unsigned)pow(2,BP->historySize)))^(hist_val);               
                }
                //No share
                else{
                    machine_address = hist_val;
                }
                int predict_val = BP->GSM[machine_address];
                bool prediction = (predict_val == 0 || predict_val == 1)? false : true;
                if(prediction){
                    *dst = BP->BPtable[tag_idx][1];
                }
                else{
                    *dst = pc + 4;
                }
                return prediction;
            }
	}

    //Didn't find the command
	*dst = pc + 4;
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	num_updates++;
	
	uint32_t tag = ((pc / 4)/BP->btbSize) % ((unsigned)pow(2, BP->tagSize));
    int tag_idx = (pc / 4) % (BP->btbSize);
    
    //Both SM and History table are local
    if (!BP->isGlobalHist && !BP->isGlobalTable) {
        //found the command
        if (BP->BPtable[tag_idx][0] == tag) {
        	int hist_val = 0;
            for (uint32_t j = 0; j < BP->historySize; j++) {
                hist_val += (BP->LHISTtable[tag_idx][j]) * (pow(2, ((BP->historySize) - j - 1)));
            }
            //SM update
            if (BP->LSM[tag_idx][hist_val] != 3 && taken) {
                (BP->LSM[tag_idx][hist_val])++;
            }
            if (BP->LSM[tag_idx][hist_val] != 0 && !taken) {
                (BP->LSM[tag_idx][hist_val])--;
            }
            //History table update
            for (uint32_t j = 0; j < BP->historySize - 1; j++) {
                BP->LHISTtable[tag_idx][j] = BP->LHISTtable[tag_idx][j + 1];
            }
            BP->LHISTtable[tag_idx][(BP->historySize) - 1] = (taken) ? 1 : 0;
        }
      
        //New command or same address for different command
        else {
            BP->BPtable[tag_idx][0] = tag;
            for (uint32_t j = 0; j < pow(2, BP->historySize); j++) {
                BP->LSM[tag_idx][j] = BP->fsmState;
            }
            for (uint32_t j = 0; j < BP->historySize; j++) {
                BP->LHISTtable[tag_idx][j]=0;
            }
            //SM update
            if (BP->LSM[tag_idx][0] != 3 && taken) {
                (BP->LSM[tag_idx][0])++;
            }
            if (BP->LSM[tag_idx][0] != 0 && !taken) {
                (BP->LSM[tag_idx][0])--;
            }
            BP->LHISTtable[tag_idx][BP->historySize -1] = (taken)? 1 : 0;
        }
    }

    //Both SM and History table are Global
    if (BP->isGlobalHist && BP->isGlobalTable) {
        BP->BPtable[tag_idx][0] = tag;
        int hist_val = 0;
        int machine_address;
        for (uint32_t j = 0; j < BP->historySize; j++) {
            hist_val += (BP->GHISTtable[j]) * (pow(2, ((BP->historySize) - j - 1)));
        }
        //LSB share
        if(BP->Shared == 1){
            machine_address = (((pc/4)%((unsigned)pow(2,BP->historySize)))^(hist_val));
        }
        //MSB share
        else if(BP->Shared == 2){
            machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,BP->historySize)))^(hist_val));
        }
        else {
            machine_address = hist_val;
        }

        if (taken && BP->GSM[machine_address] != 3) {
            BP->GSM[machine_address]++;
        }
        if (!taken && BP->GSM[machine_address] != 0) {
            BP->GSM[machine_address]--;
        }

        for (uint32_t j = 0; j < BP->historySize - 1; j++) {
            BP->GHISTtable[j] = BP->GHISTtable[j + 1];
        }
        BP->GHISTtable[BP->historySize - 1] = (taken) ? 1 : 0;

    }

    //Local SM and Global History
    if (BP->isGlobalHist && !BP->isGlobalTable) {
        //found the command
    	int hist_val = 0;
    	if (BP->BPtable[tag_idx][0] == tag) {
            for (uint32_t j = 0; j < BP->historySize; j++) {
                hist_val += (BP->GHISTtable[j]) * (pow(2, ((BP->historySize) - j - 1)));
            }
            //SM update
            if (BP->LSM[tag_idx][hist_val] != 3 && taken) {
                BP->LSM[tag_idx][hist_val] ++;
            }
            if (BP->LSM[tag_idx][hist_val] != 0 && !taken) {
                BP->LSM[tag_idx][hist_val] --;
            }
            //History table update
            for (uint32_t j = 0; j < BP->historySize - 1; j++) {
                BP->GHISTtable[j] = BP->GHISTtable[j + 1];
            }
            BP->GHISTtable[BP->historySize - 1] = (taken) ? 1 : 0;
        }
        //New command or same address for different command
        else {
            hist_val = 0;
        	BP->BPtable[tag_idx][0] = tag;
            for (uint32_t j = 0; j < BP->historySize; j++) {
                hist_val += (BP->GHISTtable[j]) * (pow(2, ((BP->historySize) - j - 1)));
            }
            for (uint32_t j = 0; j < pow(2, BP->historySize); j++) {
                BP->LSM[tag_idx][j] = BP->fsmState;
            }
            //SM update
            if (BP->LSM[tag_idx][hist_val] != 3 && taken) {
                (BP->LSM[tag_idx][hist_val])++;
            }
            if (BP->LSM[tag_idx][hist_val] != 0 && !taken) {
                (BP->LSM[tag_idx][hist_val])--;
            }

            for (uint32_t j = 0; j < BP->historySize - 1; j++) {
            	BP->GHISTtable[j] = BP->GHISTtable[j + 1];
            }
			BP->GHISTtable[BP->historySize - 1] = (taken) ? 1 : 0;
        }
    }

    //Local History and Global SM
    if (!BP->isGlobalHist && BP->isGlobalTable) {
        //found the command
    	if (BP->BPtable[tag_idx][0] == tag) {;
    		int hist_val = 0;
            for (uint32_t j = 0; j < BP->historySize; j++) {
                hist_val += (BP->LHISTtable[tag_idx][j]) * (pow(2, ((BP->historySize) - j - 1)));
            }
            int machine_address;
           	//LSB share
			if(BP->Shared == 1){
			   machine_address = (((pc/4)%((unsigned)pow(2,BP->historySize)))^(hist_val));
			}
            //MSB share
		    else if(BP->Shared == 2){
			   machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,BP->historySize)))^(hist_val));
            }
		    //No share
		    else{
			    machine_address = hist_val;
		    }
            //SM update
            if (BP->GSM[machine_address] != 3 && taken) {
                BP->GSM[machine_address] ++;
            }
            if (BP->GSM[machine_address] != 0 && !taken) {
                BP->GSM[machine_address] --;
            }
            //History table update           
            for (uint32_t j = 0; j < (BP->historySize) - 1; j++) {
                BP->LHISTtable[tag_idx][j] = BP->LHISTtable[tag_idx][j + 1];
            }
            BP->LHISTtable[tag_idx][(BP->historySize) - 1] = (taken) ? 1 : 0;
    	}
        //New command or same address for different command
        else {
            BP->BPtable[tag_idx][0] = tag;           
            int hist_val = 0;
            for (uint32_t j = 0; j < BP->historySize; j++) {
                BP->LHISTtable[tag_idx][j] = 0;
            }
            int machine_address;
		    //LSB share
		    if(BP->Shared == 1){
			   machine_address = (((pc/4)%((unsigned)pow(2,BP->historySize)))^(hist_val));
		    }
			//MSB share
		    else if(BP->Shared == 2){
			   machine_address = ((pc/((unsigned)(pow(2,16)))%((unsigned)pow(2,BP->historySize)))^(hist_val));
            }
			//No share
		    else{
			   machine_address = hist_val;
		    }
            
            //SM update
            if (BP->GSM[machine_address] != 3 && taken) {
                BP->GSM[machine_address] ++;
            }
            if (BP->GSM[machine_address] != 0 && !taken) {
                BP->GSM[machine_address] --;
            }

            BP->LHISTtable[tag_idx][BP->historySize - 1] = (taken) ? 1 : 0;
        }
    }
    
    BP->BPtable[tag_idx][1] = targetPc;

    if ((( pred_dst != pc+4) && !taken)||((targetPc != pred_dst) && taken)) {
        num_flushes++;
    }
    return;
}

void BP_GetStats(SIM_stats *curStats){
    curStats->br_num =num_updates;
    curStats->flush_num=num_flushes;
    
    //Both SM and hist are local
  	if(!(BP->isGlobalHist) && !(BP->isGlobalTable)){
  		 curStats->size=BP->btbSize*(ADDRESS+BP->tagSize+1+BP->historySize+2*pow(2.0,(double)BP->historySize));	
  	}

	//Both SM and hist are global
	if(BP->isGlobalHist && BP->isGlobalTable){
		 curStats->size=BP->btbSize*(ADDRESS+BP->tagSize+1)+BP->historySize+2*pow(2.0,(double)BP->historySize);
	}
	
	//Local SM and global hist
	if(BP->isGlobalHist && !BP->isGlobalTable){
		 curStats->size=BP->btbSize*(ADDRESS+BP->tagSize+1+2*pow(2.0,(double)BP->historySize))+BP->historySize;
	}

	//Global SM and local hist
	if(!BP->isGlobalHist && BP->isGlobalTable){
		 curStats->size=BP->btbSize*(ADDRESS+BP->tagSize+1+BP->historySize)+2*pow(2.0,(double)BP->historySize);
	}

    free(BP->BPtable);
    if (BP->isGlobalHist)
    {
        free(BP->GHISTtable);
    }
    else {
        free(BP->LHISTtable);
    }
    if (BP->isGlobalTable) {
        free(BP->GSM);
    }
    else {
        free(BP->LSM);
    }
	return;
}

