/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <queue>
#define MAX_INST 100	// max amount of instructions per thread.
#define REG_NUM 8		// numbers of registers per thread.
#define DEBUG 0 		// this enables debug prints.



// definition for a thread unit.
struct Thread
{
	int reg[REG_NUM]; // all of the registers.
	int id;
	int halted;
	int available;
	int loadCounter;
	int storeCounter;
	int instructionNumber;
};


// we define one for blocked and one for fine-grained.
// in blocked we pop and put to the back.
// in finegrained we'll jump over, 
// and continue (using the halted/available bits).

struct MT{
	int loadLatency;
	int storeLatency;
	int numOfThreads;
	int totalCycleCount;
	int contextSwitchCycles;
	std::queue<Thread> threads; // Queue of threads
};


// we define 2 instances for the multi-threading.
struct MT Blocked;
struct MT FineGrained;


// current instruction for each multi-threading.
Instruction BLK_Instruction;
Instruction FG_Instruction;




// need to make a thread initializer.
// the MTs get initialized within the function already.

void CORE_BlockedMT() {

	// First we initiate global constants.
	Blocked.contextSwitchCycles = SIM_GetSwitchCycles();
	Blocked.storeLatency		= SIM_GetStoreLat();
	Blocked.loadLatency			= SIM_GetLoadLat();
	Blocked.numOfThreads		= SIM_GetThreadsNum();
	Blocked.totalCycleCount		= 0;


	// Now we initialize each thread, then insert them to the queue 1 by 1.
	for (size_t i = 0; i < Blocked.numOfThreads; i++)
	{

		// Create a new thread
        Thread newThread;

        // Initialize the thread with proper values
        newThread.id 				= i;
        newThread.halted 			= 0;
        newThread.available 		= 0;
        newThread.loadCounter 		= 0;
        newThread.storeCounter 		= 0;
        newThread.instructionNumber = 0;

        // Insert the thread to the queue
        Blocked.threads.push(newThread);
		
		// (*) - in cpp, when inserting to a container, 
		// 		 we create a copy of the object thats is 
		//		 inserted. so we can create and use the same name,
		//		 while avoiding pointers :)
	}



	



}



double CORE_BlockedMT_CPI(){
	return 0;
}


void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}















// ========= FINE-GRAINED SECTION ============
void CORE_FinegrainedMT() {
}


double CORE_FinegrainedMT_CPI(){
	return 0;
}


void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
