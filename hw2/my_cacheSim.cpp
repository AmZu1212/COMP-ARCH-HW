#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

using std::cerr;
using std::cout;
using std::endl;
using std::FILE;
using std::ifstream;
using std::string;
using std::stringstream;

// CHANGE THIS LATER VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
#define NUM_COL 5 // 0-valid bit, 1-dirty bit, 2-LRU, 3-tag, 4-address
#define N_DIRTY 0
#define DEBUG 0
#define MAX_VALUE 4294967295
// ===============also change this vvv==============================
enum write_protocol
{
	NO_ALLOC = 0,
	ALLOC = 1
};
enum col_cache
{
	VALID = 0,
	DIRTY = 1,
	LRU = 2,
	TAG = 3,
	ADDRESS = 4
};
//==================================================================

enum cache_type
{
	L1_cache,
	L2_cache
};

struct Cache
{
	unsigned **cache;
	unsigned *least_used;
	unsigned size;
	unsigned num_of_cycles;
	unsigned num_blocks;
	unsigned num_lines;
	unsigned assoc;
	unsigned set_size;
	unsigned num_ways;
};

//===================================================================
void Cache_init(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
			unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc);
void Insert_1(unsigned long int tag, unsigned long int set);
void Insert_2(unsigned long int tag, unsigned long int set);
void UPDATE_LRU_1(unsigned set);
void UPDATE_LRU_2(unsigned set);
//void UPDATE_LRU_Combine(unsigned set,cache_type type);
//bool Search_1(unsigned long int tag, unsigned long int set);
//bool Search_2(unsigned long int tag, unsigned long int set);
bool Search_combine(unsigned long int tag, unsigned long int set,cache_type type);
void Calculate_Set_Tag(unsigned long int address);
void Cache_Write(unsigned long int tag, unsigned long int set);
void Cache_Read(unsigned long int tag, unsigned long int set);
void Print_L1();
void Print_L2();
/* Statistics Variables */
//
double L1_hits = 0;
double L1_accesses = 0;
//
double L2_hits = 0;
double L2_accesses = 0;
//
double Total_Access_Time = 0;
double Memory_Total_Access = 0;
double Total_Accesses = 0;
//

//???
bool flag_write_mem = false;
bool flag_write_L2 = false;

//???
unsigned location_found_1 = 0; // idx of found way in L1
unsigned location_found_2 = 0; // idx of found way in L2

//???
unsigned long int tag_L1 = 0;
unsigned long int set_L1 = 0;

unsigned long int tag_L2 = 0;
unsigned long int set_L2 = 0;

unsigned long int current_address;
unsigned long int current_tag_L1;
unsigned long int current_set_L1;
unsigned long int current_tag_L2;
unsigned long int current_set_L2;

unsigned Block_Size;
unsigned Memory_Cycles;
unsigned Write_Alloc;

int switcher = 0;
struct Cache L1;
struct Cache L2;

int main(int argc, char **argv)
{
	//if(DEBUG) printf("entered main\n");
	// QUICK ARG CHECK
	if (argc < 19)
	{
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char *fileString = argv[1];
	ifstream file(fileString); // input file stream
	string line;
	if (!file || !file.good())
	{
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	// PARAMETER
	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			 L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2)
	{
		string s(argv[i]);
		if (s == "--mem-cyc")
		{
			MemCyc = atoi(argv[i + 1]);
		}
		else if (s == "--bsize")
		{
			BSize = atoi(argv[i + 1]);
		}
		else if (s == "--l1-size")
		{
			L1Size = atoi(argv[i + 1]);
		}
		else if (s == "--l2-size")
		{
			L2Size = atoi(argv[i + 1]);
		}
		else if (s == "--l1-cyc")
		{
			L1Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-cyc")
		{
			L2Cyc = atoi(argv[i + 1]);
		}
		else if (s == "--l1-assoc")
		{
			L1Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--l2-assoc")
		{
			L2Assoc = atoi(argv[i + 1]);
		}
		else if (s == "--wr-alloc")
		{
			WrAlloc = atoi(argv[i + 1]);
		}
		else
		{
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	//if(DEBUG) printf("passed the arg grabs\n");
	// CACHE INITIALIZER LINE <-----------------------------------------------------------------------
	Block_Size = BSize;
	Memory_Cycles = MemCyc;
	Write_Alloc = WrAlloc;
	

	Cache_init(MemCyc, BSize,L1Size  ,L2Size, L1Assoc,
			L2Assoc, L1Cyc, L2Cyc, WrAlloc);
	//if(DEBUG) printf("done with cache inits\n");
	//printf("L1 set_size is %d.\n", L1.set_size);
	//printf("L2 set_size is %d.\n", L2.set_size);
	int k = 1;
	while (getline(file, line))
	{
		if(DEBUG) printf("===================== reading line %d ==================\n", k);
		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address))
		{
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
	//	if(DEBUG) printf("done with num check\n");
		// ========================== TAG CALCS???
		tag_L1 = (num >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L1.set_size)));
		//if(DEBUG) printf("1 -> Block_Size is: %d, L1.set_size is: %d.\n", Block_Size, L1.set_size);
		set_L1 = (num >> ((unsigned long int)(Block_Size))) % ((unsigned long int)(L1.set_size)); // set size is 0 for some reason
	//	if(DEBUG) printf("2\n");
		tag_L2 = (num >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L2.set_size)));
		//if(DEBUG) printf("3\n");
		set_L2 = (num >> (unsigned long int)(Block_Size)) % (unsigned long int)(L2.set_size);
		//if(DEBUG) printf("4\n");
		//if(DEBUG) printf("before calc tags\n");
		current_address = num;
		Calculate_Set_Tag(num);
		switch(operation){
			case 'r':
				Cache_Read(tag_L1,set_L1);
		    	break;
			case 'w':
				Cache_Write(tag_L1,set_L1);
				break;
		}
		k++;
	}

	// statistics calculations:
	double L1MissRate = (L1_accesses - L1_hits) / L1_accesses;
	double L2MissRate = (L2_accesses - L2_hits) / L2_accesses;
	double avgAccTime = Total_Access_Time / Total_Accesses; /*<--------------------- this line needs to be changed*/
	// ================================================================
	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	// YOU NEED TO FREE MEMORY DOWN HERE <-----------------------------------------------------
	free(L1.least_used);
	free(L2.least_used);
	free(L1.cache);
	free(L2.cache);
	if(DEBUG)printf("sanity check 88\n");
	return 0;
}


void Cache_init(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
			unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc){
	Block_Size=BSize; //bits
	L1.size=L1Size; //bits
	L2.size=L2Size; //bits
	L1.assoc=L1Assoc; //bits
	L2.assoc=L2Assoc; //bits
	L1.num_of_cycles=L1Cyc;
	L2.num_of_cycles=L2Cyc;
	Memory_Cycles=MemCyc;
	Write_Alloc=WrAlloc;

	L1.num_ways=pow(2,L1Assoc); //num
	L2.num_ways=pow(2,L2Assoc); //num

	L1.num_blocks=(unsigned)(pow(2,L1Size))/(pow(2,BSize)); //num
	L2.num_blocks=(unsigned)(pow(2,L2Size))/(pow(2,BSize)); //num

	L1.set_size=(L1.num_blocks/L1.num_ways); //bits
	L2.set_size=(L2.num_blocks/L2.num_ways); //bits
	/*
	cache->num_lines_1=pow(2,cache->set_size_1); //num
	cache->num_lines_2=pow(2,cache->set_size_2); //num
	*/
	L1.num_lines=L1.set_size;
	L2.num_lines=L2.set_size;
	/* initializing the cache_1 table/s */
	L1.cache=(unsigned**)malloc(sizeof(unsigned*)*L1.num_lines);
	for(unsigned i=0;i<L1.num_lines;i++){
		L1.cache[i]=(unsigned*)malloc(sizeof(unsigned)*L1.num_ways*NUM_COL);
	}
	if(!L1.cache){
		free(L1.cache);
		return;
	}
	for(unsigned i=0;i<L1.num_lines;i++){
		for(unsigned j=0;j<L1.num_ways*NUM_COL;j++){
			L1.cache[i][j]=0;
		}
	}
	for(unsigned i=0;i<L1.num_lines;i++){
		for(unsigned j=TAG;j<L1.num_ways*NUM_COL;j+=NUM_COL){
			L1.cache[i][j]=MAX_VALUE;
		}
	}

	/* initializing the cache_2 table/s */
	L2.cache=(unsigned**)malloc(sizeof(unsigned*)*L2.num_lines);
	for(unsigned i=0;i<L2.num_lines;i++){
		L2.cache[i]=(unsigned*)malloc(sizeof(unsigned)*L2.num_ways*NUM_COL);
	}
	if(!L2.cache){
		free(L2.cache);
		return ;
	}
	for(unsigned i=0;i<L2.num_lines;i++){
		for(unsigned j=0;j<L2.num_ways*2 /*<-this should be 2*/;j++){
			L2.cache[i][j]=0;
		}
	}
	for(unsigned i=0;i<L2.num_lines;i++){
		for(unsigned j=TAG;j<L2.num_ways*NUM_COL;j+=NUM_COL){
			L2.cache[i][j]=MAX_VALUE;
		}
	}
	
	/* initializing the least recently used tables */
	L1.least_used=(unsigned*)malloc(sizeof(unsigned)*L1.num_lines);
	if(!L1.least_used){
		free(L1.least_used);
		return;
	}
	for(unsigned i=0; i<L1.num_lines; i++){
		L1.least_used[i]=0;
	}	
	L2.least_used=(unsigned*)malloc(sizeof(unsigned)*L2.num_lines);
	if(!L2.least_used){
		free(L2.least_used);
		return;
	}	
	for(unsigned i=0; i<L2.num_lines; i++){
		L2.least_used[i]=0;
	}
	if(DEBUG) Print_L1();		
}
void Cache_Write(unsigned long int tag, unsigned long int set){
	//searching the tag according to the set value in all 'ways'
	if(DEBUG) printf("entered cache write\n");
	if (Write_Alloc)
	{
		if (DEBUG)printf("chose allocate\n");
	}
	else
	{
		if (DEBUG)printf("chose no allocate\n");
	}
	
	Total_Accesses++;
	bool found_1 = Search_combine(tag_L1, set_L1, L1_cache);//Search_1(tag, set);
	int found_1_int= (found_1)? 1 : 0;
	if(DEBUG) printf("write search result is: %d\n", found_1);
	switch(Write_Alloc){
		case ALLOC:
			switch(found_1_int)
			{
				//found in L1, WRITE HIT
				case 1:
					if(DEBUG) printf("L1 WRITE HIT(ALLOC)\n");
					//if(DEBUG) printf("L1 hit \n");
					L1_accesses++;
					L1_hits++;
					Total_Access_Time+=L1.num_of_cycles;
					UPDATE_LRU_1(set);
					L1.cache[set][location_found_1+DIRTY]=1;
					L1.cache[set][location_found_1+ADDRESS]=current_address;
					//return;
					break;
				//didn't find in L1, WRITE MISS, go to search in L2
				case 0: 
					if(DEBUG) printf("L1 WRITE MISS(ALLOC)\n");
					L1_accesses++;
					L2_accesses++;
					bool found_2= Search_combine(tag_L2, set_L2, L2_cache);//Search_2(tag_L2, set_L2);
					if(found_2){
						if(DEBUG) printf("L2 WRITE HIT(ALLOC)\n");
						flag_write_L2 = true;
						Total_Access_Time+=L1.num_of_cycles + L2.num_of_cycles;
						L2_hits++;
						//L1_total_access++;
						//Total_Access_time+=cache->L1Cyc;
						UPDATE_LRU_2(set_L2);//update on the block we want to insert in L1
						//cache->Cache_L2[set_2][loc_found_2+DIRTY]=1;
						Insert_1(tag_L1, set_L1);//not sure were supposed to insert in L1 needs checking
						flag_write_L2 = false;
					}
					else{
						if(DEBUG) printf("L2 WRITE MISS(ALLOC)\n");
						Memory_Total_Access++;
						Total_Access_Time+=L2.num_of_cycles + L1.num_of_cycles + Memory_Cycles;
						Calculate_Set_Tag(current_address);
						flag_write_mem=true;
						Insert_2(current_tag_L2, current_set_L2);
						Insert_1(current_tag_L1, current_set_L1);
						flag_write_mem=false;
					}
			}
			break;
		case NO_ALLOC:
			switch(found_1_int)
			{
				//found in L1, WRITE HIT
				case 1:
					if(DEBUG) printf("L1 WRITE HIT(NO ALLOC)\n");
					L1_accesses++;
					L1_hits++;
					Total_Access_Time+=L1.num_of_cycles;
					UPDATE_LRU_1(set_L1);
					L1.cache[set_L1][location_found_1+DIRTY]=1;
					//return;
					break;
				//didn't find in L1, WRITE MISS, go to serach in L2
				case 0: 
					if(DEBUG) printf("L1 WRITE MISS(NO ALLOC)\n");
					L1_accesses++;
					L2_accesses++;
					bool found_2=Search_combine(tag_L2, set_L2, L2_cache);//Search_2(tag_L2, set_L2);
					if(found_2){
						if(DEBUG) printf("L2 WRITE HIT(NO ALLOC)\n");
						L2_hits++;
						Total_Access_Time += L1.num_of_cycles+L2.num_of_cycles;
						UPDATE_LRU_2(set_L2);//update on the block we want to insert in L1
						L2.cache[set_L2][location_found_2+DIRTY]=1;
						//Insert_1(tag_1, set_1);//not sure were supposed to insert in L1 needs checking
					}
					else{
						if(DEBUG) printf("L2 WRITE MISS(NO ALLOC)\n");
						Memory_Total_Access++;
						Total_Access_Time+=L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
					}
			}
			break;
	}
	if(DEBUG){
		Print_L1();
		Print_L2();
	} 	
	if(DEBUG) printf("left write\n");
}
void Cache_Read(unsigned long int tag, unsigned long int set){
	if(DEBUG) printf("entered cache read\n");
	Total_Accesses++;
	bool found_1=Search_combine(tag_L1, set_L1, L1_cache);//Search_1(tag, set);
	int found_1_int= (found_1)? 1 : 0;
	switch(found_1_int)
	{
		//found in L1, READ HIT
		case 1:
			if(DEBUG) printf("L1 READ HIT\n");
			L1_accesses++;
			L1_hits++;
			Total_Access_Time+=L1.num_of_cycles;
			UPDATE_LRU_1(set_L1);
			//return;
			break;
		//didnt find in L1, READ MISS, go to serach in L2
		case 0: 
			if(DEBUG) printf("L1 READ MISS\n");
			L1_accesses++;
			L2_accesses++;
			bool found_2=Search_combine(tag_L2, set_L2, L2_cache);//Search_2(tag_L2, set_L2);
			if(found_2){
				if(DEBUG) printf("L2 READ HIT\n");
				Total_Access_Time+=L1.num_of_cycles+L2.num_of_cycles;
				L2_hits++;
				UPDATE_LRU_2(set_L2);//update on the block we want to insert in L1
				Insert_1(tag_L1, set_L1);//not sure were supposed to insert in L1 needs checking
			}
			else{
				if(DEBUG) printf("L2 READ MISS\n");
				Memory_Total_Access++;
				Total_Access_Time+=L1.num_of_cycles+L2.num_of_cycles+Memory_Cycles;
				Calculate_Set_Tag(current_address);
				Insert_2(current_tag_L2, current_set_L2);
				Insert_1(current_tag_L1, current_set_L1);

				//need to write also insertL2 and just call both of them
				//make sure both funcs update the LRU's accordingly
				//add access to memory time aight girl? is this fine by u? do u agree? metzuyan.
			}
			break;
	}
	if(DEBUG){
		Print_L1();
		Print_L2();
	} 
	if(DEBUG) printf("left cache read\n");
}


//===============================================================================
bool Search_1(unsigned long int tag, unsigned long int set){
	unsigned way=0;
	if(DEBUG) printf("L1 SEARCH\n");
	while(way<(L1.num_ways)*NUM_COL){
		if(L1.cache[set][way+TAG] == tag && L1.cache[set][way+VALID]){
			location_found_1 = way;
			return true;
		}
		way+=NUM_COL;
	}
	return false;
}
bool Search_2(unsigned long int tag, unsigned long int set){
	unsigned way=0;
	if(DEBUG) printf("L2 SEARCH\n");
	while(way<(L2.num_ways)*NUM_COL){
		if(L2.cache[set][way+TAG] == tag && L2.cache[set][way+VALID]){
			location_found_2 = way;
			return true;
		}
		way+=NUM_COL;
	}
	return false;
}

bool Search_combine(unsigned long int tag, unsigned long int set, cache_type type)
{
	unsigned way = 0;
	switch (type)
	{
	case L1_cache:
		if (DEBUG)printf("L1 SEARCH\n");
		while (way < (L1.num_ways) * NUM_COL)
		{
			if (L1.cache[set][way + TAG] == tag && L1.cache[set][way + VALID])
			{
				location_found_1 = way;
				return true;
			}
			way += NUM_COL;
		}
		return false;

	case L2_cache:
		if (DEBUG)printf("L2 SEARCH\n");
		while (way < (L2.num_ways) * NUM_COL)
		{
			if (L2.cache[set][way + TAG] == tag && L2.cache[set][way + VALID])
			{
				location_found_2 = way;
				return true;
			}
			way += NUM_COL;
		}
		return false;
	}
}
//===============================================================================
void Insert_1(unsigned long int tag, unsigned long int set){
	//if(DEBUG) printf("entered insert (for cache 1)\n");
	unsigned i=0;
	bool found_empty = false;
	//looking for non valid bit in specific set
	if(DEBUG) printf("L1 INSERT\n");
	while(!found_empty && i<(L1.num_ways)*NUM_COL){
		if(L1.cache[set][i+VALID]==1){
			i+=NUM_COL;
		}
		else{
			found_empty=true;
			//if(DEBUG) printf("check if you are here test 1009 \n");
		}
		//L1_total_access++;
	}
	//found an available set in one of the ways, eviction wasn't needed
	if(found_empty){
		L1.cache[set][i+VALID]=1;
		L1.cache[set][i+TAG]=tag;
		L1.cache[set][i+DIRTY]=0;
		L1.cache[set][i+ADDRESS]=current_address;
		location_found_1=i;
		UPDATE_LRU_1(set);
		//if(DEBUG) printf("check if enters \n");
		if(flag_write_mem || flag_write_L2){
			L1.cache[set][i+DIRTY]=1;
		}
	}
	//didn't find an available set, eviction needed
	else{
		//if(DEBUG) printf("did u get inside here by any chance \n");
		//DIRTY CHECK. checks if there's a need to copy from L1 to L2 before placing new INFO
		//UPDATE_LRU_1(set);
		switch(L1.cache[set][L1.least_used[set]+DIRTY]){ 
			case N_DIRTY:
				L1.cache[set][L1.least_used[set]+TAG] = tag;
				L1.cache[set][L1.least_used[set]+DIRTY]=0;
				L1.cache[set][L1.least_used[set]+VALID]=1;
				L1.cache[set][L1.least_used[set]+ADDRESS]=current_address;
				location_found_1=L1.least_used[set];
				UPDATE_LRU_1(set);
				if(flag_write_mem || flag_write_L2){
					L1.cache[set][L1.least_used[set]+DIRTY]=1;
				}
				break;
			case DIRTY://gonna be at L2 for sure, only needs to update L2 and insert tag to L1 in the right place+update_LRU1
				Calculate_Set_Tag(L1.cache[set][L1.least_used[set]+ADDRESS]);
				if(Search_combine(tag_L2, set_L2, L2_cache)/*Search_2(current_tag_L2, current_set_L2)*/){
					//loc_found_2 = cache->Least_used_2[addresses->current_set_2];
					UPDATE_LRU_2(current_set_L2);//needs to update in L2 the block which is being evictes in L1
					//eviction and saving
					L1.cache[set][L1.least_used[set]+VALID]=1;
					L1.cache[set][L1.least_used[set]+TAG]=tag;
					L1.cache[set][L1.least_used[set]+ADDRESS]=current_address;
					L1.cache[set][L1.least_used[set]+DIRTY]=0;
					location_found_1=L1.least_used[current_set_L1];
					UPDATE_LRU_1(set);
					if(flag_write_mem || flag_write_L2){
						L1.cache[set][L1.least_used[set]+DIRTY]=1;
					}
					//Total_Access_time+=cache->L1Cyc + cache->L2Cyc;
					//L1_total_access++;
					//update_LRU1 on the newest tag that weve inserted
				}
				//didn't find in L2, have to keep it there
				else{
					//needs to find least_used_L2 and least_used_L1 and then update both
					//changes the tag and then update to L2 
				}
				break;
		}
	}

}
void Insert_2(unsigned long int tag, unsigned long int set){

	if(DEBUG) printf("L2 INSERT\n");
	unsigned i=0;
	bool found_empty = false;
	//looking for non valid bit in specific set
	while(!found_empty && i<(L2.num_ways)*NUM_COL){
		if(L2.cache[set][i+VALID]){
			i+=NUM_COL;
		}
		else{
			found_empty=true;
		}
		//L2_total_access++;
	}
	//found an available way in L2, putting data
	if(found_empty){
		L2.cache[set][i+VALID]=1;
		L2.cache[set][i+TAG]=tag;
		L2.cache[set][i+DIRTY]=0;
		L2.cache[set][i+ADDRESS]=current_address;
		location_found_2=i;
		UPDATE_LRU_2(set);
	}
	else
	{//didn't find available way in L2, making eviction and putting data
		if(L2.cache[set][L2.least_used[set]+DIRTY]){
			Memory_Total_Access++;
			Total_Access_Time+=Memory_Cycles;
		}		
		L2.cache[set][L2.least_used[set]+TAG] = tag;
		L2.cache[set][L2.least_used[set]+DIRTY]=0;
		L2.cache[set][L2.least_used[set]+VALID]=1;
		unsigned long int address_evicted = L2.cache[set][L2.least_used[set]+ADDRESS];
		L2.cache[set][L2.least_used[set]+ADDRESS]=current_address;	
		unsigned long int set_evicted_1= (address_evicted>>((unsigned long int)(Block_Size)))%((unsigned long int)(L1.set_size));
		unsigned long int tag_evicted_1= (address_evicted>>((unsigned long int)(Block_Size)+(unsigned long int)log2((L1.set_size))));	
		location_found_2=L2.least_used[set];
		UPDATE_LRU_2(set);
		if(Search_combine(tag_evicted_1, set_evicted_1, L1_cache)/*Search_1(tag_evicted_1, set_evicted_1)*/){
			L1.cache[set_evicted_1][location_found_1+VALID]=0;
			L1.cache[set_evicted_1][location_found_1+DIRTY]=0;
			L1.cache[set_evicted_1][location_found_1+TAG] = MAX_VALUE;
			L1.cache[set_evicted_1][location_found_1+ADDRESS]=0;
			L1.cache[set_evicted_1][location_found_1+LRU]=0;
		}
	}
}

//====================================================================================================================
void UPDATE_LRU_1(unsigned set){
	if(DEBUG) printf("L1 UPDATE\n");
	bool found=false;
	unsigned last_access = L1.cache[set][location_found_1+LRU];
	//if(DEBUG) printf("last access %d \n",last_access);
	L1.cache[set][location_found_1+LRU] = L1.num_ways-1;//last way to get accessed
	//if(DEBUG) printf("LRU is %d \n", L1.cache[set][location_found_1+LRU]);
	for(unsigned i=LRU; i < L1.num_ways*NUM_COL; i+=NUM_COL){//updates i to each beginning of a new WAY
		if((i!=location_found_1+LRU) && (L1.cache[set][i] > last_access)){
			L1.cache[set][i]--;
			//if(DEBUG) printf("your'e not supposed to be here \n");
		}
		if(L1.cache[set][i] == 0 && !found){
			L1.least_used[set]=i-LRU;//-LRU
			found=true;
		}
	}
}
void UPDATE_LRU_2(unsigned set){
	if(DEBUG) printf("L2 UPDATE\n");
	bool found=false;
	unsigned last_access = L2.cache[set][location_found_2+LRU];
	L2.cache[set][location_found_2+LRU] = L2.num_ways-1;
	for(unsigned i=LRU; i < L2.num_ways*NUM_COL; i+=NUM_COL){
		if((i!=location_found_2+LRU) && (L2.cache[set][i] > last_access)){
			L2.cache[set][i]--;
		}
		if(L2.cache[set][i] == 0 && !found){
			L2.least_used[set]=i-LRU;//-LRU
			found=true;
		}
	}	
}
//===========================================================================================================================
//void UPDATE_LRU_Combine(unsigned set,cache_type type){
	//if(type == L1_cache){
	//if(DEBUG) printf("L1 UPDATE\n");
	//bool found1=false;
	//unsigned last_access = L1.cache[set][location_found_1+LRU];
	//if(DEBUG) printf("last access %d \n",last_access);
	//L1.cache[set][location_found_1+LRU] = L1.num_ways-1;//last way to get accessed
	//if(DEBUG) printf("LRU is %d \n", L1.cache[set][location_found_1+LRU]);
	//for(unsigned i=LRU; i < L1.num_ways*NUM_COL; i+=NUM_COL){//updates i to each beginning of a new WAY
		//if((i!=location_found_1+LRU) && (L1.cache[set][i] > last_access)){
			//L1.cache[set][i]--;
			//if(DEBUG) printf("your'e not supposed to be here \n");
		//}
		//if(L1.cache[set][i] == 0 && !found1){
		//	L1.least_used[set]=i-LRU;//-LRU
			//found1=true;
		//}
	//}
//}
       //else
	 //  if(DEBUG) printf("L2 UPDATE\n");
//	bool found2=false;
	//unsigned last_access = L2.cache[set][location_found_2+LRU];
//	L2.cache[set][location_found_2+LRU] = L2.num_ways-1;
	//for(unsigned i=LRU; i < L2.num_ways*NUM_COL; i+=NUM_COL){
	//	if((i!=location_found_2+LRU) && (L2.cache[set][i] > last_access)){
		//	L2.cache[set][i]--;
		//}
		//if(L2.cache[set][i] == 0 && !found2){
			//L2.least_used[set]=i-LRU;//-LRU
			//found2=true;
		//}
	//}	
	      

//}



void Calculate_Set_Tag(unsigned long int address){
	current_tag_L1 = (address>>((unsigned long int)(Block_Size)+(unsigned long int)log2(L1.set_size)));
	current_set_L1 = (address>>((unsigned long int)(Block_Size)))%(unsigned long int)(L1.set_size);

	current_tag_L2 = (address>>((unsigned long int)(Block_Size)+(unsigned long int)log2(L2.set_size)));
	current_set_L2 = (address>>((unsigned long int)(Block_Size)))%(unsigned long int)(L2.set_size);
}
void Print_L1(){
	unsigned j=0;
	int count=5;
	for(unsigned i =0; i<L1.num_lines;i++){
		printf("cache L1 set:%d info is- \n", i);
		for( j =0; j<(L1.num_ways*NUM_COL);j++){
			if(count==5){
				count = 0;
				printf("\n");
			}
			switch(j%5){
				case 0:
					printf("valid ");
					break;
				case 1:
					printf("dirty ");
					break;
				case 2:
					printf("LRU ");
					break;
				case 3:
					printf("tag ");
					break;
				case 4:
					printf("address ");
					break;
			}
			printf("%d  ", L1.cache[i][j]);
			count++;
		}
		printf("\n");
	}
	printf("\n");
}
void Print_L2(){
	unsigned j=0;
	int count = 0;
	for(unsigned i =0; i<L2.num_lines;i++){
		printf("cache L2 set:%d info is- \n", i);
		for( j =0; j<(L2.num_ways*NUM_COL);j++){
			if(count==5){
				count = 0;
				printf("\n");
			}
			switch(j%5){
				case 0:
					printf("valid ");
					break;
				case 1:
					printf("dirty ");
					break;
				case 2:
					printf("LRU ");
					break;
				case 3:
					printf("tag ");
					break;
				case 4:
					printf("address ");
					break;
			}
			printf("%d  ", L2.cache[i][j]);
			count ++;
		}
		printf("\n");
	}
	printf("\n");
}

