/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define MAX_INST 100
#define COL 5
#define REG_NUM 8
#define DEBUG 0
#define DEBUG2 0

enum BITS{HALT = 0, AVAILABLE = 1, INST_NUM = 2, LOAD_CTR = 3, STORE_CTR = 4}; //HALT=0 - means hlt hasnt been pressed yet
												  // AVAILABLE = 1 - means the thread is currenltly available 
												  // INST_NUM = self explantory 

double global_cycle_count_blocked = 0; // global cycle counter will be updated each time we pass an instruction
double global_cycle_count_finegrained = 0;

struct B_MT{
	int** arr;
	int num_threads;
	int switch_cycles;
	int store_latency;
	int load_latency;
};

struct FG_MT{
	int** arr;
	int num_threads;
	int switch_cycles;
	int store_latency;
	int load_latency;
};

struct B_MT *b_mt;
struct FG_MT *fg_mt;
Instruction *instrcution_b;
Instruction *instrcution_fg;
int **reg_arr_blocked;
int **reg_arr_finegrained;

void Updates_BLOCKED(struct B_MT *b_mt);
void Updates_FINEGRAINED(struct FG_MT *fg_mt);
void print_reg_file(int **reg_arr_blocked);
int return_thread_num_available(struct B_MT *b_mt,int crnt_thd);


// ===================== BLOCKED SECTION ===============================

void CORE_BlockedMT() {
	// AMZU - We initiate the blocked_MT variant.
	b_mt = (struct B_MT *)malloc(sizeof(struct B_MT));
	b_mt->num_threads = SIM_GetThreadsNum();
	b_mt->switch_cycles = SIM_GetSwitchCycles();
	b_mt->store_latency = SIM_GetStoreLat();
	b_mt->load_latency = SIM_GetLoadLat();

	b_mt->arr = (int **)malloc(sizeof(int *) * b_mt->num_threads);

	// AMZU - i guess he had 5 
	// allocating 5 colomns per line
	for(int i =0;i< b_mt->num_threads; i++){
		b_mt->arr[i]=(int*)malloc(sizeof(int)*COL);
	}


	// AMZU - zeroing some array not 
	// initializing values for array
	for(int i =0; i< b_mt->num_threads; i++){
		b_mt->arr[i][HALT]= 0;
		b_mt->arr[i][AVAILABLE]= 1;
		b_mt->arr[i][INST_NUM]= 0;	
	    b_mt->arr[i][LOAD_CTR]= 0;	
		b_mt->arr[i][STORE_CTR]= 0;
	}

	//come back and check
	instrcution_b=(struct _inst*)malloc(sizeof(struct _inst));

	//local register file per thread
	reg_arr_blocked=(int**)malloc(sizeof(int*)*b_mt->num_threads);
	for(int i =0; i< b_mt->num_threads; i++){
		reg_arr_blocked[i]=(int*)malloc(sizeof(int)*REG_NUM);
	}

	// initializing local register file
	for (int i = 0; i < b_mt->num_threads; i++)
	{
		for (int j = 0; j < REG_NUM; j++)
		{
			reg_arr_blocked[i][j] = 0;
		}
	}

	int num_halts = 0, thd_idx = 0, src_1, src_2 ,dst , count_idle = 0, prior_thd_idx, count = 0;

	//the simulation of all of the instructions
	while(num_halts != b_mt->num_threads){
		prior_thd_idx =thd_idx;
		count_idle = 0;

		//finds the next thread which is available according to round robin algorithm (SWITCH)
		while(b_mt->arr[thd_idx][HALT]== 1 || b_mt->arr[thd_idx][AVAILABLE]==0){
			if(thd_idx == b_mt->num_threads -1){
				thd_idx = 0;
			}
			else {
				thd_idx++;
				if(DEBUG) printf("\ncurrent thread = %d\n",thd_idx );
			}	
				
			if(b_mt->arr[thd_idx][AVAILABLE]!=1){
				count_idle++;
			}
			
			//making sure we aint stuck in an infinite loop by updating the cycle+availibality status of each thread
			if(count_idle==(b_mt->num_threads)){
				global_cycle_count_blocked++;
				Updates_BLOCKED(b_mt);
				count_idle = 0;
			}
		}



		if(DEBUG2){
			if(count != 0 && prior_thd_idx != thd_idx) printf("\n switching from thread- %d to thread %d\n",prior_thd_idx,thd_idx );
		} 

		if(DEBUG2) printf("\n\n current num of cycles is - %f after idles\n\n", global_cycle_count_blocked);
		

		if(thd_idx!=prior_thd_idx){
			for(int i =0; i<b_mt->switch_cycles; i++){
				Updates_BLOCKED(b_mt);
			}
			global_cycle_count_blocked+=b_mt->switch_cycles;
		}



		SIM_MemInstRead(b_mt->arr[thd_idx][INST_NUM] ,instrcution_b, thd_idx);
		src_1=instrcution_b->src1_index;
		src_2=instrcution_b->src2_index_imm;
		dst=instrcution_b->dst_index;




		//search for all threads which arent available and update their store/load counters
		Updates_BLOCKED(b_mt);


		// amzu - next command
		switch(instrcution_b->opcode){
			case CMD_HALT:
				b_mt->arr[thd_idx][HALT] = 1;
				b_mt->arr[thd_idx][AVAILABLE] = 0;
				b_mt->arr[thd_idx][INST_NUM]++;
				num_halts++;
				break;
			case CMD_ADD:
				b_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_blocked[thd_idx][dst]=reg_arr_blocked[thd_idx][src_1]+reg_arr_blocked[thd_idx][src_2];
				break;
			case CMD_SUB:
				b_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_blocked[thd_idx][dst]=reg_arr_blocked[thd_idx][src_1]-reg_arr_blocked[thd_idx][src_2];
				break;					
			case CMD_ADDI:
				b_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_blocked[thd_idx][dst]=reg_arr_blocked[thd_idx][src_1]+src_2;
				break;					
			case CMD_SUBI:
				b_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_blocked[thd_idx][dst]=reg_arr_blocked[thd_idx][src_1]-src_2;
				break;		
			case CMD_LOAD:
				b_mt->arr[thd_idx][INST_NUM] ++;
				b_mt->arr[thd_idx][LOAD_CTR]= b_mt->load_latency;
				b_mt->arr[thd_idx][AVAILABLE] = 0;
				if(instrcution_b->isSrc2Imm == true){
					SIM_MemDataRead(reg_arr_blocked[thd_idx][src_1]+src_2,&(reg_arr_blocked[thd_idx][dst]));
				}
				else{
					SIM_MemDataRead(reg_arr_blocked[thd_idx][src_1]+reg_arr_blocked[thd_idx][src_2],&(reg_arr_blocked[thd_idx][dst]));
				}
				break;
			case CMD_STORE:
					b_mt->arr[thd_idx][INST_NUM] ++;
					b_mt->arr[thd_idx][STORE_CTR]= b_mt->store_latency;
					b_mt->arr[thd_idx][AVAILABLE] = 0;
				if(instrcution_b->isSrc2Imm == true){
					SIM_MemDataWrite(reg_arr_blocked[thd_idx][dst]+src_2 , reg_arr_blocked[thd_idx][src_1]);
				}
				else {
					SIM_MemDataWrite(reg_arr_blocked[thd_idx][dst]+reg_arr_blocked[thd_idx][src_2] , reg_arr_blocked[thd_idx][src_1]);						
				}
				break;
			case CMD_NOP:
				break;
		}
		global_cycle_count_blocked++;
		if(DEBUG2) printf("\n\n current num of cycles is - %f after doing inst\n\n", global_cycle_count_blocked);

	}
	if(DEBUG) print_reg_file(reg_arr_blocked);
}

double CORE_BlockedMT_CPI(){
	int num_inst = 0;
	for(int i =0; i<b_mt->num_threads;i++){
		num_inst+=b_mt->arr[i][INST_NUM];
	}
	free(b_mt->arr);
	free(b_mt);
	free(reg_arr_blocked);
	free(instrcution_b);
	if (DEBUG2) printf("\nnum inst = %d, cyclesb = %f\n", num_inst, global_cycle_count_blocked);
	return (double)(global_cycle_count_blocked/num_inst);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0;i<REG_NUM;i++){
		context[threadid].reg[i]=reg_arr_blocked[threadid][i];
	}
}

void Updates_BLOCKED(struct B_MT *b_mt){
	for(int i =0; i<b_mt->num_threads;i++){
		if(b_mt->arr[i][AVAILABLE] == 0){
			if(b_mt->arr[i][LOAD_CTR]!=0){
				b_mt->arr[i][LOAD_CTR]--;
				if(b_mt->arr[i][LOAD_CTR]==0){
					b_mt->arr[i][AVAILABLE] = 1;
				}
			}
			if(b_mt->arr[i][STORE_CTR]!=0){
				b_mt->arr[i][STORE_CTR]--;
				if(b_mt->arr[i][STORE_CTR]==0){
					b_mt->arr[i][AVAILABLE] = 1;
				}
			}
		}
	}
}


// ========================== HELPER FUNCTIONS =================================
void print_reg_file(int **reg_arr_blocked){
	for(int i=0; i<b_mt->num_threads;i++){
		printf("thread num %d:\n", i);
		for (int j = 0; j<REG_NUM; j++){
			printf("reg %d= %d  ",j,reg_arr_blocked[i][j]);
		}
		printf("\n");
	}
}

int return_thread_num_available(struct B_MT *b_mt, int crnt_thd)
{
	int min_, flag = 0, num_thd = crnt_thd, cnt = 0, max;
	while (cnt < b_mt->num_threads)
	{
		if (b_mt->arr[crnt_thd][HALT] != 1 && b_mt->arr[crnt_thd][AVAILABLE] != 1)
		{
			if (!flag)
			{
				flag = 1;
				max = (b_mt->arr[crnt_thd][STORE_CTR] > b_mt->arr[crnt_thd][LOAD_CTR]) ? b_mt->arr[crnt_thd][STORE_CTR] : b_mt->arr[crnt_thd][LOAD_CTR];
				min_ = max;
				num_thd = crnt_thd;
			}
			max = (b_mt->arr[crnt_thd][STORE_CTR] > b_mt->arr[crnt_thd][LOAD_CTR]) ? b_mt->arr[crnt_thd][STORE_CTR] : b_mt->arr[crnt_thd][LOAD_CTR];
			if (max < min_)
			{
				min_ = max;
				num_thd = crnt_thd;
			}
		}
		cnt++;
		if (crnt_thd == b_mt->num_threads - 1)
			crnt_thd = 0;
		else
			crnt_thd++;
	}
	return num_thd;
}

// ====================== FINE GRAINED SECTION =====================================
void CORE_FinegrainedMT() {
	fg_mt=(struct FG_MT*)malloc(sizeof(struct FG_MT));
	fg_mt->num_threads = SIM_GetThreadsNum();
	fg_mt->switch_cycles = SIM_GetSwitchCycles();
	fg_mt->store_latency = SIM_GetStoreLat();
	fg_mt->load_latency =SIM_GetLoadLat();

	fg_mt->arr = (int**)malloc(sizeof(int*)*fg_mt->num_threads);

	// allocating 5 colomns per line
	for(int i =0;i< fg_mt->num_threads; i++){
		fg_mt->arr[i]=(int*)malloc(sizeof(int)*COL);
	}
	// initializing values for array
	for(int i =0; i< fg_mt->num_threads; i++){
		fg_mt->arr[i][HALT]= 0;
		fg_mt->arr[i][AVAILABLE]= 1;
		fg_mt->arr[i][INST_NUM]= 0;	
	    fg_mt->arr[i][LOAD_CTR]= 0;	
		fg_mt->arr[i][STORE_CTR]= 0;
	}

	//come back and check
	instrcution_fg=(struct _inst*)malloc(sizeof(struct _inst));
	//local register file per thread
	reg_arr_finegrained=(int**)malloc(sizeof(int*)*fg_mt->num_threads);
	for(int i =0; i< fg_mt->num_threads; i++){
		reg_arr_finegrained[i]=(int*)malloc(sizeof(int)*REG_NUM);
	}
	//initializing local register file
	for(int i = 0; i<fg_mt->num_threads;i++){
		for(int j=0; j<REG_NUM;j++){
			reg_arr_finegrained[i][j] = 0;
		}
	}
	int num_halts = 0, thd_idx = 0, src_1, src_2 ,dst , count_idle = 0;

	//the simulation of all of the instructions
	while(num_halts != fg_mt->num_threads){

		//finds the next thread which is available according to round robin algorithm (SWITCH)
		while(fg_mt->arr[thd_idx][HALT]!=0 || fg_mt->arr[thd_idx][AVAILABLE]!=1){
			if(thd_idx == fg_mt->num_threads -1){
				thd_idx = 0;
			}
			else {
				thd_idx++;	
			}		
			if(fg_mt->arr[thd_idx][AVAILABLE]!=1){
				count_idle++;
			}
			//making sure we aint stuck in an infinite loop by updating the cycle+availibality status of each thread
			if(count_idle==(fg_mt->num_threads)){
				global_cycle_count_finegrained++;
				Updates_FINEGRAINED(fg_mt);
				count_idle = 0;
			}
		}
		count_idle = 0;

		SIM_MemInstRead(fg_mt->arr[thd_idx][INST_NUM] ,instrcution_fg, thd_idx);
		src_1=instrcution_fg->src1_index;
		src_2=instrcution_fg->src2_index_imm;
		dst=instrcution_fg->dst_index;
		//search for all threads which arent available and update their store/load counters
		Updates_FINEGRAINED(fg_mt);

		switch(instrcution_fg->opcode){
			case CMD_HALT:
				fg_mt->arr[thd_idx][HALT] = 1;
				fg_mt->arr[thd_idx][AVAILABLE] = 0;
				fg_mt->arr[thd_idx][INST_NUM]++;
				num_halts++;
				break;
			case CMD_ADD:
				fg_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_finegrained[thd_idx][dst]=reg_arr_finegrained[thd_idx][src_1]+reg_arr_finegrained[thd_idx][src_2];
				break;
			case CMD_SUB:
				fg_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_finegrained[thd_idx][dst]=reg_arr_finegrained[thd_idx][src_1]-reg_arr_finegrained[thd_idx][src_2];
				break;					
			case CMD_ADDI:
				fg_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_finegrained[thd_idx][dst]=reg_arr_finegrained[thd_idx][src_1]+src_2;
				break;					
			case CMD_SUBI:
				fg_mt->arr[thd_idx][INST_NUM]++;
				reg_arr_finegrained[thd_idx][dst]=reg_arr_finegrained[thd_idx][src_1]-src_2;
				break;		
			case CMD_LOAD:
				fg_mt->arr[thd_idx][INST_NUM] ++;
				fg_mt->arr[thd_idx][LOAD_CTR]= fg_mt->load_latency;
				fg_mt->arr[thd_idx][AVAILABLE] = 0;
				if(instrcution_fg->isSrc2Imm == true){
					SIM_MemDataRead(reg_arr_finegrained[thd_idx][src_1]+src_2,&(reg_arr_finegrained[thd_idx][dst]));
				}
				else{
					SIM_MemDataRead(reg_arr_finegrained[thd_idx][src_1]+reg_arr_finegrained[thd_idx][src_2],&(reg_arr_finegrained[thd_idx][dst]));
				}
				break;
				case CMD_STORE:
					fg_mt->arr[thd_idx][INST_NUM] ++;
					fg_mt->arr[thd_idx][STORE_CTR]= fg_mt->store_latency;
					fg_mt->arr[thd_idx][AVAILABLE] = 0;
				if(instrcution_fg->isSrc2Imm == true){
					SIM_MemDataWrite(reg_arr_finegrained[thd_idx][dst]+src_2 , reg_arr_finegrained[thd_idx][src_1]);
				}
				else {
					SIM_MemDataWrite(reg_arr_finegrained[thd_idx][dst]+reg_arr_finegrained[thd_idx][src_2] , reg_arr_finegrained[thd_idx][src_1]);						
				}
				break;
			case CMD_NOP:
				break;
		}
		global_cycle_count_finegrained++;
		if(thd_idx == fg_mt->num_threads -1){
			thd_idx = 0;
		}
		else {
			thd_idx++;	
		}
	}
}

double CORE_FinegrainedMT_CPI(){
	int num_inst = 0;
	for(int i =0; i<fg_mt->num_threads;i++){
		num_inst+=fg_mt->arr[i][INST_NUM];
	}
	free(fg_mt->arr);
	free(fg_mt);
	free(reg_arr_finegrained);
	free(instrcution_fg);
	if (DEBUG2) printf("\nnum inst = %d, cyclesfg = %f\n", num_inst, global_cycle_count_finegrained);
	double cpi = global_cycle_count_finegrained/num_inst;
	return cpi;
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	for(int i = 0;i<REG_NUM;i++){
		context[threadid].reg[i]=reg_arr_finegrained[threadid][i];
	}
}

void Updates_FINEGRAINED(struct FG_MT *fg_mt){
	for(int i =0; i<fg_mt->num_threads;i++){
		if(fg_mt->arr[i][AVAILABLE] == 0){
			if(fg_mt->arr[i][LOAD_CTR]!=0){
				fg_mt->arr[i][LOAD_CTR]--;
				if(fg_mt->arr[i][LOAD_CTR]==0){
					fg_mt->arr[i][AVAILABLE] = 1;
				}
			}
			if(fg_mt->arr[i][STORE_CTR]!=0){
				fg_mt->arr[i][STORE_CTR]--;
				if(fg_mt->arr[i][STORE_CTR]==0){
					fg_mt->arr[i][AVAILABLE] = 1;
				}
			}
		}
	}
}




