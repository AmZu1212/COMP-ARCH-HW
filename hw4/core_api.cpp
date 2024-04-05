/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <iostream>
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
	double totalCycleCount;
	int contextSwitchCycles;
	std::vector<Thread> threads; // Queue of threads
};


// we define 2 instances for the multi-threading.
struct MT Blocked;
struct MT FineGrained;


// current instruction for each multi-threading.
Instruction *BLK_Instruction;
Instruction *FG_Instruction;


// =================================== Helper Functions Declarations ===============================
void UpdateThreadCountersBLK();
void UpdateThreadCountersFG();


// =================================== Blocked SECTION =============================================
// the Blocked MT get initialized in this function.
void CORE_BlockedMT() 
{
	// First we initiate constants.
	Blocked.contextSwitchCycles = SIM_GetSwitchCycles();
	Blocked.storeLatency		= SIM_GetStoreLat();
	Blocked.loadLatency			= SIM_GetLoadLat();
	Blocked.numOfThreads		= SIM_GetThreadsNum();
	Blocked.totalCycleCount		= 0;

	// this makes the slots to write the info into.
    Blocked.threads.resize(Blocked.numOfThreads);

	// Now we initialize each thread, then insert them to the queue 1 by 1.
	for (int i = 0; i < Blocked.numOfThreads; i++)
	{

		// Create a alias, and fill it with data
		Thread& newThread = Blocked.threads[i];

		// Initialize the thread with proper values
		newThread.id 				 = i;
		newThread.halted 			 = 0;
		newThread.available 		 = 1;
		newThread.loadCounter 		 = 0;
		newThread.storeCounter 		 = 0;
		newThread.instructionCounter = 0;

		// same with the register, all set to 0.
		for(int index = 0; index < REG_NUM; index++)
		{
			newThread.reg[index] = 0;
		}
	}

	//inst malloc
	BLK_Instruction = (struct _inst*)malloc(sizeof(struct _inst));


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
		//std::cout << "BLOCKED: stuck in outer while" << std::endl;
		priorityThreadID = threadID;
		idleCount = 0;

		// (*)  - i think this might get stuck in an infinite loop. 	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		// we enter this if the current picked thread is stopped 
		// for some reason (halt / load-store idle).
		while(Blocked.threads[threadID].halted || !Blocked.threads[threadID].available)
		{
			//std::cout << "BLOCKED: stuck in halt/available while" << std::endl;
			//printf("number of threads is: %d, number of halted is: %d\n", Blocked.numOfThreads, haltsCount);
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
				UpdateThreadCountersBLK();
				idleCount = 0;
			}

			//if(haltsCount == Blocked.numOfThreads)
			//{
			//	printf("all halted in RR while. exiting...\n");
			//	break;
			//}
			// we exit this while loop when a thread is released from idling.

			// CODE NOTICE - can we get stuck here if everyone is halted? seems dangerous
		}

		//printf("exited the halt/available loop\n");
		// checks if we switch thread
		if(threadID != priorityThreadID)
		{
			// we "pay" the context switch cost, meanwhile we 
			// dont forget to decrement the load/store counters
			for(int i = 0; i < Blocked.contextSwitchCycles; i++)
			{
				UpdateThreadCountersBLK();
			}
			
			// and ofcourse we count the penalty to the total cycles.
			Blocked.totalCycleCount += Blocked.contextSwitchCycles;
		}

		// Load next command
		SIM_MemInstRead(Blocked.threads[threadID].instructionCounter, BLK_Instruction, threadID);
		src1 = BLK_Instruction->src1_index;
		src2 = BLK_Instruction->src2_index_imm;
		dst  = BLK_Instruction->dst_index;

		// Update the Load/Store counters
		UpdateThreadCountersBLK();

		if(DEBUG) printf("before switch\n");
		// OPCODE SWITCH
		if(DEBUG) printf("(before switch)threadID is: %d\n",threadID);
		switch(BLK_Instruction->opcode)
		{
			case CMD_HALT:
				if(DEBUG) printf("chose HALT\n");
				// when we halt, for clarity's sake we put available to 0 aswell.
				Blocked.threads[threadID].halted = 1;
				Blocked.threads[threadID].available = 0;
				Blocked.threads[threadID].instructionCounter++;
				haltsCount++; // also we increment the halt count
				break;

			case CMD_ADD:
				if(DEBUG) printf("chose ADD\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].reg[dst] = Blocked.threads[threadID].reg[src1] + Blocked.threads[threadID].reg[src2];
				break;
			
			case CMD_SUB:
				if(DEBUG) printf("chose SUB\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].reg[dst] = Blocked.threads[threadID].reg[src1] - Blocked.threads[threadID].reg[src2];
				break;

			case CMD_ADDI:
				if(DEBUG) printf("chose ADDI\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].reg[dst] = Blocked.threads[threadID].reg[src1] + src2;
				break;
			
			case CMD_SUBI:
				if(DEBUG) printf("chose SUBI\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].reg[dst] = Blocked.threads[threadID].reg[src1] - src2;
				break;
			
			case CMD_LOAD:
				if(DEBUG) printf("chose LOAD\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].loadCounter = Blocked.loadLatency;
				Blocked.threads[threadID].available = 0;

				// We need to check if src 2 is a number or an imm
				if(BLK_Instruction->isSrc2Imm == true)
				{	// its a number
					SIM_MemDataRead(Blocked.threads[threadID].reg[src1] + src2, &(Blocked.threads[threadID].reg[dst]));
				}
				else
				{	// its a variable
					SIM_MemDataRead(Blocked.threads[threadID].reg[src1] + Blocked.threads[threadID].reg[src2], &(Blocked.threads[threadID].reg[dst]));
				}
				break;
			
			case CMD_STORE:
				if(DEBUG) printf("chose STORE\n");
				Blocked.threads[threadID].instructionCounter++;
				Blocked.threads[threadID].storeCounter = Blocked.storeLatency;
				Blocked.threads[threadID].available = 0;

				// need to check if src 2 is a number or a variable
				if(BLK_Instruction->isSrc2Imm == true)
				{	// its a number
					SIM_MemDataWrite(Blocked.threads[threadID].reg[dst] + src2, Blocked.threads[threadID].reg[src1]);
				}
				else 
				{	// its a variable
					SIM_MemDataWrite(Blocked.threads[threadID].reg[dst] + Blocked.threads[threadID].reg[src2], Blocked.threads[threadID].reg[src1]);						
				}
				break;

			case CMD_NOP:
				if(DEBUG) printf("chose NOP\n");
				break;
		}
		if(DEBUG) printf("after switch\n");
		Blocked.totalCycleCount++;
	}

}



double CORE_BlockedMT_CPI()
{
	// we sum up all the instructions we did to calc the CPI
	int totalInstructionCount = 0;
	for(int i = 0; i < Blocked.numOfThreads; i++){
		//printf("\nthread %d did %d instructions.\n",i, Blocked.threads[i].instructionCounter);
		totalInstructionCount += Blocked.threads[i].instructionCounter;
	}
	//printf("total instruction count is: %d\n", totalInstructionCount);
	//printf("total counted cycles is: %d\n", Blocked.totalCycleCount);
	double cpi = Blocked.totalCycleCount / totalInstructionCount;//((double)Blocked.totalCycleCount) / totalInstructionCount;
	return cpi;
}


void CORE_BlockedMT_CTX(tcontext* context, int threadid) 
{
	for(int i = 0; i < REG_NUM; i++)
	{
		context[threadid].reg[i] = Blocked.threads[threadid].reg[i];
	}
}

// this function goes over every thread and if the thread is unavailable 
// it decrements the load/store counter and if the counters reach 0 it
// ups the available bit.
void UpdateThreadCountersBLK()
{
	for (int i = 0; i < Blocked.numOfThreads; i++)
	{
		if (Blocked.threads[i].available == 0)
		{
			if (Blocked.threads[i].loadCounter != 0)
			{
				Blocked.threads[i].loadCounter--;
				if (Blocked.threads[i].loadCounter == 0)
				{
					Blocked.threads[i].available = 1;
				}
			}
			if (Blocked.threads[i].storeCounter != 0)
			{
				Blocked.threads[i].storeCounter--;
				if (Blocked.threads[i].storeCounter == 0)
				{
					Blocked.threads[i].available = 1;
				}
			}
		}
	}
}




// ================================ FINE-GRAINED SECTION ===========================================
// the FineGrained MT get initialized in this function.
void CORE_FinegrainedMT() 
{
	// First we initiate constants.
	FineGrained.contextSwitchCycles = SIM_GetSwitchCycles();
	FineGrained.storeLatency		= SIM_GetStoreLat();
	FineGrained.loadLatency			= SIM_GetLoadLat();
	FineGrained.numOfThreads		= SIM_GetThreadsNum();
	FineGrained.totalCycleCount		= 0;

	// this makes the slots to write the info into.
    FineGrained.threads.resize(FineGrained.numOfThreads);

	// Now we initialize each thread, then insert them to the queue 1 by 1.
	for (int i = 0; i < FineGrained.numOfThreads; i++)
	{

		// Create a alias, and fill it with data
		Thread& newThread = FineGrained.threads[i];

		// Initialize the thread with proper values
		newThread.id 				 = i;
		newThread.halted 			 = 0;
		newThread.available 		 = 1;
		newThread.loadCounter 		 = 0;
		newThread.storeCounter 		 = 0;
		newThread.instructionCounter = 0;

		// same with the register, all set to 0.
		for(int index = 0; index < REG_NUM; index++)
		{
			newThread.reg[index] = 0;
		}
	}

	//inst malloc
	FG_Instruction = (struct _inst*)malloc(sizeof(struct _inst));


	// sources and destination for the "assembly" command.
	int src1, src2, dst;


	// we'll count how many of the current 
	// threads are idle due to stores/loads.
	int idleCount = 0;		


	// and we'll use the priotity thread id
	int threadID = 0;


	// When a thread finishes it increments the halts_count variable by 1.
	int haltsCount = 0;


	// We run until all threads finish. 
	while (haltsCount != FineGrained.numOfThreads)
	{

		// (*)  - i think this might get stuck in an infinite loop. 	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		// we enter this if the current picked thread is stopped 
		// for some reason (halt / load-store idle).
		while(FineGrained.threads[threadID].halted || !FineGrained.threads[threadID].available)
		{
			if(threadID == (FineGrained.numOfThreads - 1))
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

			if(!FineGrained.threads[threadID].available)
			{
				// if the current thread is idling
				// we increment the count
				idleCount++;

			}

			// and if all the threads are blocked, we just update the load/store counters.
			if(idleCount == FineGrained.numOfThreads)
			{
				FineGrained.totalCycleCount++;
				// this decrements the load/store counters for all the threads
				// as if a cycle passed
				UpdateThreadCountersFG();
				idleCount = 0;
			}

			// we exit this while loop when a thread is released from idling.

			// CODE NOTICE - can we get stuck here if everyone is halted? seems dangerous
		}




		idleCount = 0;
		
		// Load the next command
		SIM_MemInstRead(FineGrained.threads[threadID].instructionCounter, FG_Instruction, threadID);
		src1 = FG_Instruction->src1_index;
		src2 = FG_Instruction->src2_index_imm;
		dst  = FG_Instruction->dst_index;

		// Update the Load/Store counters
		UpdateThreadCountersFG();

		// OPCODE SWITCH
		switch(FG_Instruction->opcode)
		{
			case CMD_HALT:
				// when we halt, for clarity's sake we put available to 0 aswell.
				FineGrained.threads[threadID].halted = 1;
				FineGrained.threads[threadID].available = 0;
				FineGrained.threads[threadID].instructionCounter++;
				haltsCount++; // also we increment the halt count
				break;

			case CMD_ADD:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].reg[dst] = FineGrained.threads[threadID].reg[src1] + FineGrained.threads[threadID].reg[src2];
				break;
			
			case CMD_SUB:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].reg[dst] = FineGrained.threads[threadID].reg[src1] - FineGrained.threads[threadID].reg[src2];
				break;

			case CMD_ADDI:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].reg[dst] = FineGrained.threads[threadID].reg[src1] + src2;
				break;
			
			case CMD_SUBI:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].reg[dst] = FineGrained.threads[threadID].reg[src1] - src2;
				break;
			
			case CMD_LOAD:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].loadCounter = FineGrained.loadLatency;
				FineGrained.threads[threadID].available = 0;

				// need to check if src 2 is a number or a variable
				if(FG_Instruction->isSrc2Imm == true)
				{	// its a number
					SIM_MemDataRead(FineGrained.threads[threadID].reg[src1] + src2, &(FineGrained.threads[threadID].reg[dst]));
				}
				else
				{	// its a variable
					SIM_MemDataRead(FineGrained.threads[threadID].reg[src1] + FineGrained.threads[threadID].reg[src2], &(FineGrained.threads[threadID].reg[dst]));
				}
				break;
			
			case CMD_STORE:
				FineGrained.threads[threadID].instructionCounter++;
				FineGrained.threads[threadID].storeCounter = FineGrained.storeLatency;
				FineGrained.threads[threadID].available = 0;

				// need to check if src 2 is a number or a variable
				if(FG_Instruction->isSrc2Imm == true)
				{	// its a number
					SIM_MemDataWrite(FineGrained.threads[threadID].reg[dst] + src2, FineGrained.threads[threadID].reg[src1]);
				}
				else 
				{	// its a variable
					SIM_MemDataWrite(FineGrained.threads[threadID].reg[dst] + FineGrained.threads[threadID].reg[src2], FineGrained.threads[threadID].reg[src1]);						
				}
				break;

			case CMD_NOP:
				break;
		}


		FineGrained.totalCycleCount++;
		if(threadID == (FineGrained.numOfThreads - 1))
		{
			threadID = 0;
		}
		else 
		{
			threadID++;	
		}
	}
}


double CORE_FinegrainedMT_CPI()
{
		// we sum up all the instructions we did to calc the CPI
	int totalInstructionCount = 0;
	for(int i = 0; i < FineGrained.numOfThreads; i++){
		//printf("\nthread %d did %d instructions\n", i, FineGrained.threads[i].instructionCounter);
		totalInstructionCount += FineGrained.threads[i].instructionCounter;
	}
	//printf("\nnum inst = %d, cyclesfg = %f\n", totalInstructionCount, FineGrained.totalCycleCount);
	double cpi = FineGrained.totalCycleCount / totalInstructionCount;
	return cpi;
}


void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) 
{
	for (int i = 0; i < REG_NUM; i++)
	{
		context[threadid].reg[i] = FineGrained.threads[threadid].reg[i];
	}
}



// this function goes over every thread and if the thread is unavailable 
// it decrements the load/store counter and if the counters reach 0 it
// ups the available bit.
void UpdateThreadCountersFG()
{
	for (int i = 0; i < FineGrained.numOfThreads; i++)
	{
		if (FineGrained.threads[i].available == 0)
		{
			if (FineGrained.threads[i].loadCounter != 0)
			{
				FineGrained.threads[i].loadCounter--;
				if (FineGrained.threads[i].loadCounter == 0)
				{
					FineGrained.threads[i].available = 1;
				}
			}
			if (FineGrained.threads[i].storeCounter != 0)
			{
				FineGrained.threads[i].storeCounter--;
				if (FineGrained.threads[i].storeCounter == 0)
				{
					FineGrained.threads[i].available = 1;
				}
			}
		}
	}
}
