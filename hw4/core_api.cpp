/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <vector>
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
	int instructionCounter;
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
	std::vector<Thread> threads; // Queue of threads
};


// we define 2 instances for the multi-threading.
struct MT Blocked;
struct MT FineGrained;


// current instruction for each multi-threading.
Instruction BLK_Instruction;
Instruction FG_Instruction;


// ========= Helper Functions Declarations ==========
void UpdateThreadCounters();




// the MTs get initialized in this function.
void CORE_BlockedMT() {

	// First we initiate global constants.
	Blocked.contextSwitchCycles = SIM_GetSwitchCycles();
	Blocked.storeLatency		= SIM_GetStoreLat();
	Blocked.loadLatency			= SIM_GetLoadLat();
	Blocked.numOfThreads		= SIM_GetThreadsNum();
	Blocked.totalCycleCount		= 0;

	// this makes the slots to write the info into.
    Blocked.threads.resize(Blocked.numOfThreads);

	// Now we initialize each thread, then insert them to the queue 1 by 1.
	for (size_t i = 0; i < Blocked.numOfThreads; i++)
	{

		// Create a alias, and fill it with data
		Thread& newThread = Blocked.threads[i];

		// Initialize the thread with proper values
		newThread.id 				 = i;
		newThread.halted 			 = 0;
		newThread.available 		 = 0;
		newThread.loadCounter 		 = 0;
		newThread.storeCounter 		 = 0;
		newThread.instructionCounter = 0;

		// same with the register, all set to 0.
		for(size_t index = 0; index < REG_NUM; index++)
		{
			newThread.reg[index] = 0;
		}
	}


	// sources and destination for the "assembly" command.
	int src1, src2, dst;




	// we'll count how many of the current 
	// threads are idle due to stores/loads.
	int idleCount = 0;		

	// and we'll use the priotity thread id
	int threadID = 0;
	int priorityThreadID;




	// When a thread finishes it increments the halts_count variable by 1.
	int haltsCount = 0;


	// We run until all threads finish. 
	while (haltsCount != Blocked.numOfThreads)
	{
		priorityThreadID = threadID;
		idleCount = 0;

		// (*)  - i think this might get stuck in an infinite loop. 	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		// we enter this if the current picked thread is stopped 
		// for some reason (halt / load-store idle).
		while(Blocked.threads[threadID].halted || !Blocked.threads[threadID].available)
		{
			if(threadID == (Blocked.numOfThreads - 1))
			{
				// We loop until someone stops idling, 
				// we get here only if everyone is busy.
				threadID = 0; 
			}
			else
			{
				// this thread is also delayed, move to the next one
				threadID++;
			}

			if(!Blocked.threads[threadID].available)
			{
				// if the current thread is idling
				// we increment the count
				idleCount++;

			}


			// and if all the threads are blocked, we just update the load/store counters.
			if(idleCount == Blocked.numOfThreads)
			{
				Blocked.totalCycleCount++;
				// this decrements the load/store counters for all the threads
				// as if a cycle passed
				UpdateThreadCounters();
				idleCount = 0;
			}

			// we exit this while loop when a thread is released from idling.

			// CODE NOTICE - can we get stuck here if everyone is halted? seems dangerous
		}


		// checks if we switch thread
		if(threadID != priorityThreadID)
		{
			// we "pay" the context switch cost, meanwhile we 
			// dont forget to decrement the load/store counters
			for(size_t i = 0; i < Blocked.contextSwitchCycles; i++)
			{
				UpdateThreadCounters();
			}
			
			// and ofcourse we count the penalty to the total cycles.
			Blocked.totalCycleCount += Blocked.contextSwitchCycles;
		}




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
