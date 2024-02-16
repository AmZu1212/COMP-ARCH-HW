/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 0
#define ADDRESS 30
#define MAXUNSIGNED 4294967295

int num_updates = 0, num_flushes = 0;


struct BP {
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;//
	bool isGlobalTable;//
	int Shared;//
	unsigned** BPtable;
	unsigned** LHISTtable;
	unsigned* GHISTtable;
	unsigned** LSM;
    unsigned* GSM;
} ;

struct BP *BP;

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
    
    BP = (struct BP*)malloc(sizeof(*BP));
    if(!BP) {
       	return -1;
    }
    BP->btbSize = btbSize;
	BP->historySize = historySize;
	BP->tagSize = tagSize;
	BP->fsmState = fsmState;
	BP->isGlobalHist = isGlobalHist;//
	BP->isGlobalTable = isGlobalTable;//
	BP->Shared = Shared;//

	//btb table initialization
	BP->BPtable = (unsigned**)malloc(btbSize*sizeof(unsigned*));
	for(uint32_t i=0; i<btbSize; i++){
		BP->BPtable[i] = (unsigned*)malloc(2*sizeof(unsigned));
	}
	if(BP->BPtable==NULL){
		return -1;
	}
	for(uint32_t i=0; i<BP->btbSize; i++){
		BP->BPtable[i][0] = MAXUNSIGNED;
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

