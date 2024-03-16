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
#define DEBUG 1
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

void Insert(unsigned long int tag, unsigned long int set, cache_type type);
void Update_LRU(unsigned set, cache_type type);
int Search(unsigned long int tag, unsigned long int set, cache_type type);
void Calculate_Set_Tag(unsigned long int address);
void Write_No_Allocate();
void Write_Allocate();
void Cache_Write();
void Cache_Read();
void Cache_Feed(char operation);
void init_Caches(Cache *L, unsigned size, unsigned num_of_cycles, unsigned assoc);

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
	

	init_Caches(&L1 ,L1Size, L1Cyc, L1Assoc);
	init_Caches(&L2, L2Size, L2Cyc, L2Assoc);
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
		//==========================================
		//if(DEBUG) printf("after calc tags\n");
		Cache_Feed(operation);
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
	return 0;
}



void init_Caches(Cache *L, unsigned size, unsigned num_of_cycles, unsigned assoc)
{
	//if(DEBUG) printf("entered cache init\n");
	//printf("size is %d, cycles is: %d, assoc is: %d.\n", size, num_of_cycles, assoc);
	L->size = size;	  // bits
	L->assoc = assoc; // bits
	L->num_of_cycles = num_of_cycles;
	L->num_ways = pow(2, assoc);								// num
	L->num_blocks = (unsigned)(pow(2, size)) / (pow(2, Block_Size)); // num
	L->set_size = (L->num_blocks / L->num_ways);		// bits
	L->num_lines = L->set_size;
	L->cache = (unsigned **)malloc(sizeof(unsigned *) * L->num_lines);
	//printf("set_size is %d.\n", L->set_size);
	for (unsigned i = 0; i < L->num_lines; i++)
	{
		L->cache[i] = (unsigned *)malloc(sizeof(unsigned) * L->num_ways * NUM_COL);
	}

	if (!L->cache)
	{
		free(L->cache);
		return;
	}

	for (unsigned i = 0; i < L->num_lines; i++)
	{
		for (unsigned j = 0; j < L->num_ways * NUM_COL; j++)
		{
			L->cache[i][j] = 0;
		}
	}

	for (unsigned i = 0; i < L->num_lines; i++)
	{
		for (unsigned j = TAG; j < L->num_ways * NUM_COL; j += NUM_COL)
		{
			L->cache[i][j] = MAX_VALUE;
		}
	}

	/* initializing the least recently used tables */
	L->least_used = (unsigned *)malloc(sizeof(unsigned) * L->num_lines);
	if (!L->least_used)
	{
		free(L->least_used);
		return;
	}
	for (unsigned i = 0; i < L->num_lines; i++)
	{
		L->least_used[i] = 0;
	}
	//if(DEBUG) printf("left cache init\n");
	//printf("size is %d, cycles is: %d, assoc is: %d.\n", L->size, L->num_of_cycles, L->assoc);
	//printf("set_size is %d.\n", L->set_size);
}

void Cache_Feed(char operation)
{
	//if(DEBUG) printf("entered cache feed\n");
	if (operation == 'r')
	{
		Cache_Read();
	}
	else
	{
		Cache_Write();
	}
	//if(DEBUG) printf("left cache feed\n");
}

void Cache_Read()
{
	if(DEBUG) printf("entered cache read\n");
	Total_Accesses++;
	int found_1 = Search(tag_L1, set_L1, L1_cache);
	if (found_1 == 1)
	{
		if(DEBUG) printf("L1 READ HIT\n");
		// L1 READ HIT
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		Update_LRU(set_L1, L1_cache);
	}
	else
	{
		if(DEBUG) printf("L1 READ MISS\n");
		// L1 READ MISS
		L1_accesses++;
		L2_accesses++;
		int found_2 = Search(tag_L2, set_L2, L2_cache);
		if (found_2 == 2)
		{
			if(DEBUG) printf("L2 READ HIT\n");
			// L2 READ HIT
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			L2_hits++;
			Update_LRU(set_L2, L2_cache);
			Insert(tag_L1, set_L1, L1_cache);
		}
		else
		{
			if(DEBUG) printf("L2 READ MISS\n");
			// L2 READ MISS
			Memory_Total_Access++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
			Calculate_Set_Tag(current_address);
			Insert(current_tag_L2, current_set_L2, L2_cache);
			Insert(current_tag_L1, current_set_L1, L1_cache);
		}
	}
	if(DEBUG) printf("left cache read\n");
}

void Cache_Write()
{
	if(DEBUG) printf("entered cache write\n");
	if (Write_Alloc == 0)
	{
		if(DEBUG) printf("chose no allocate\n");
		Write_No_Allocate();
	}
	else
	{
		if(DEBUG) printf("chose allocate\n");
		Write_Allocate();
	}
	if(DEBUG) printf("left write\n");
}

void Write_Allocate()
{
	Total_Accesses++;
	int found_1 = Search(tag_L1, set_L1, L1_cache);

	if (found_1 == 1)
	{
		if(DEBUG) printf("L1 WRITE HIT(ALLOC)\n");
		// L1 WRITE HIT
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		Update_LRU(set_L1, L1_cache);
		L1.cache[set_L1][location_found_1 + DIRTY] = 1;
		L1.cache[set_L1][location_found_1 + ADDRESS] = current_address;
	}
	else
	{
		if(DEBUG) printf("L1 WRITE MISS(ALLOC)\n");
		// L1 WRITE MISS
		L1_accesses++;
		L2_accesses++;
		int found_2 = Search(tag_L2, set_L2, L2_cache);
		if (found_2 == 2)
		{
			if(DEBUG) printf("L2 WRITE HIT(ALLOC)\n");
			// L2 WRITE HIT
			flag_write_L2 = true;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			L2_hits++;
			Update_LRU(set_L2, L2_cache);
			Insert(tag_L1, set_L1, L1_cache);
			flag_write_L2 = false;
		}
		else
		{
			if(DEBUG) printf("L2 WRITE MISS(ALLOC)\n");
			// L2 WRITE MISS
			Memory_Total_Access++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
			Calculate_Set_Tag(current_address);
			flag_write_mem = true;
			Insert(current_tag_L2, current_set_L2, L2_cache);
			Insert(current_tag_L1, current_set_L1, L1_cache);
			flag_write_mem = false;
		}
	}
}

void Write_No_Allocate()
{
	Total_Accesses++;
	int found_1 = Search(tag_L1, set_L1, L1_cache);

	if (found_1 == 1)
	{
		if(DEBUG) printf("L1 WRITE HIT(NO ALLOC)\n");
		// L1 WRITE HIT
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		Update_LRU(set_L1, L1_cache);
		L1.cache[set_L1][location_found_1 + DIRTY] = 1;
	}
	// didn't find in L1, WRITE MISS, go to serach in L2
	else
	{
		if(DEBUG) printf("L1 WRITE MISS(NO ALLOC)\n");
		// L1 WRITE MISS
		L1_accesses++;
		L2_accesses++;
		int found_2 = Search(tag_L2, set_L2, L2_cache);
		if (found_2 == 2)
		{
			if(DEBUG) printf("L2 WRITE HIT(NO ALLOC)\n");
			// L2 WRITE HIT
			L2_hits++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			Update_LRU(set_L2, L2_cache); // update on the block we want to insert in L1
			L2.cache[set_L2][location_found_2 + DIRTY] = 1;
			// Insert_1(tag_1, set_1);//not sure were supposed to insert in L1 needs checking
		}
		else
		{
			if(DEBUG) printf("L2 WRITE MISS(NO ALLOC)\n");
			// L2 WRITE MISS
			Memory_Total_Access++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
		}
	}
}

void Calculate_Set_Tag(unsigned long int address)
{
	current_tag_L1 = address >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L1.set_size));
	current_set_L1 = (address >> (unsigned long int)(Block_Size)) % (unsigned long int)(L1.set_size);

	current_tag_L2 = address >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L2.set_size));
	current_set_L2 = (address >> (unsigned long int)(Block_Size)) % (unsigned long int)(L2.set_size);
}

int Search(unsigned long int tag, unsigned long int set, cache_type type)
{
	unsigned way = 0;
	switch (type)
	{
	case L1_cache:
		if(DEBUG) printf("L1 SEARCH\n");
		while (way < (L1.num_ways) * NUM_COL)
		{
			// found in current way
			if (L1.cache[set][way + TAG] == tag && L1.cache[set][way + VALID])
			{
				location_found_1 = way;
				return 1;
			}
			// next way
			way += NUM_COL;
		}
		break;

	case L2_cache:
		if(DEBUG) printf("L2 SEARCH\n");
		while (way < (L2.num_ways) * NUM_COL)
		{
			// found in current way
			if (L2.cache[set][way + TAG] == tag && L2.cache[set][way + VALID])
			{
				location_found_2 = way;
				return 2;
			}
			// next way
			way += NUM_COL;
		}
		break;
	}
	return 0;
}

void Update_LRU(unsigned set, cache_type type)
{
	bool found = false;
	unsigned last_access;
	switch (type)
	{
		
		case L1_cache:
			if(DEBUG) printf("L1 UPDATE\n");
			last_access = L1.cache[set][location_found_1 + LRU];
			L1.cache[set][location_found_1 + LRU] = L1.num_ways - 1; // last way to get accessed
			for (unsigned i = LRU; i < L1.num_ways * NUM_COL; i += NUM_COL)
			{ // updates i to each beginning of a new WAY
				if ((i != location_found_1 + LRU) && (L1.cache[set][i] > last_access))
				{
					L1.cache[set][i]--;
				}
				if (L1.cache[set][i] == 0 && !found)
				{
					L1.least_used[set] = i - LRU; //-LRU
					found = true;
				}
			}
			return;
			break;

		case L2_cache:
			if(DEBUG) printf("L2 UPDATE\n");
			last_access = L2.cache[set][location_found_2 + LRU];
			L2.cache[set][location_found_2 + LRU] = L2.num_ways - 1; // last way to get accessed
			for (unsigned i = LRU; i < L2.num_ways * NUM_COL; i += NUM_COL)
			{ // updates i to each beginning of a new WAY
				if ((i != location_found_2 + LRU) && (L2.cache[set][i] > last_access))
				{
					L2.cache[set][i]--;
				}
				if (L2.cache[set][i] == 0 && !found)
				{
					L2.least_used[set] = i - LRU; //-LRU
					found = true;
				}
			}
			return;
			break;
	}
}
void Insert(unsigned long int tag, unsigned long int set, cache_type type)
{
	//if(DEBUG) printf("entered insert\n");
	unsigned i = 0;
	bool found_empty = false;
	// looking for non valid bit in specific set
	if (type == L1_cache)
	{
		if(DEBUG) printf("L1 INSERT\n");
		while (!found_empty && i < (L1.num_ways) * NUM_COL)
		{
			if (L1.cache[set][i + VALID] == 1)
			{
				i += NUM_COL;
			}
			else
			{
				found_empty = true;
			}
			// L1_total_access++;
		}
		// found an available set in one of the ways, eviction wasn't needed
		if (found_empty)
		{
			L1.cache[set][i + VALID] = 1;
			L1.cache[set][i + TAG] = tag;
			L1.cache[set][i + DIRTY] = 0;
			L1.cache[set][i + ADDRESS] = current_address;
			location_found_1 = i;
			//if(DEBUG) printf("calling update from insert for cache 1\n");
			Update_LRU(set, L1_cache);
			//if(DEBUG) printf("update returned (for cache 1)\n");
			if (flag_write_mem || flag_write_L2)
			{
				L1.cache[set][i + DIRTY] = 1;
			}
		}
		// didn't find an available set, eviction needed
		else
		{
			switch (L1.cache[set][L1.least_used[set] + DIRTY])
			{
			case N_DIRTY:
				L1.cache[set][L1.least_used[set] + TAG] = tag;
				L1.cache[set][L1.least_used[set] + DIRTY] = 0;
				L1.cache[set][L1.least_used[set] + VALID] = 1;
				L1.cache[set][L1.least_used[set] + ADDRESS] = current_address;
				location_found_1 = L1.least_used[set];
				Update_LRU(set, L1_cache);
				if (flag_write_mem || flag_write_L2)
				{
					L1.cache[set][L1.least_used[set] + DIRTY] = 1;
				}
				break;
			case DIRTY: // gonna be at L2 for sure, only needs to update L2 and insert tag to L1 in the right place+update_LRU1
				Calculate_Set_Tag(L1.cache[set][L1.least_used[set] + ADDRESS]);
				if (Search(current_tag_L2, current_set_L2, L2_cache) == 2)
				{
					// loc_found_2 = cache->Least_used_2[addresses->current_set_2];
					Update_LRU(current_set_L2, L2_cache); // needs to update in L2 the block which is being evictes in L1
					// eviction and saving
					L1.cache[set][L1.least_used[set] + VALID] = 1;
					L1.cache[set][L1.least_used[set] + TAG] = tag;
					L1.cache[set][L1.least_used[set] + ADDRESS] = current_address;
					L1.cache[set][L1.least_used[set] + DIRTY] = 0;
					location_found_1 = L1.least_used[current_set_L1];
					Update_LRU(set, L1_cache);
					if (flag_write_mem || flag_write_L2)
					{
						L1.cache[set][L1.least_used[set] + DIRTY] = 1;
					}
					// Total_Access_time+=cache->L1Cyc + cache->L2Cyc;
					// L1_total_access++;
					// update_LRU1 on the newest tag that weve inserted
				}
				break;
			}
		}
		return;
	}


	if (type == L2_cache)
	{
		if(DEBUG) printf("L2 INSERT\n");
		while (!found_empty && i < (L2.num_ways) * NUM_COL)
		{
			if (L2.cache[set][i + VALID] == 1)
			{
				i += NUM_COL;
			}
			else
			{
				found_empty = true;
			}
			// L1_total_access++;
		}
		
		if(found_empty){
			L2.cache[set][i + VALID] = 1;
			L2.cache[set][i + TAG] = tag;
			L2.cache[set][i + DIRTY] = 0;
			L2.cache[set][i + ADDRESS] = current_address;
			location_found_2 = i;
			Update_LRU(set, L2_cache);
		}
		else
		{
			if (L2.cache[set][L2.least_used[set] + DIRTY])
			{
				Memory_Total_Access++;
				Total_Access_Time += Memory_Cycles;
			}
			L2.cache[set][L2.least_used[set] + TAG] = tag;
			L2.cache[set][L2.least_used[set] + DIRTY] = 0;
			L2.cache[set][L2.least_used[set] + VALID] = 1;
			unsigned long int address_evicted = L2.cache[set][L2.least_used[set] + ADDRESS];
			L2.cache[set][L2.least_used[set] + ADDRESS] = current_address;
			unsigned long int set_evicted_1 = (address_evicted >> ((unsigned long int)(Block_Size))) % ((unsigned long int)(L1.set_size));
			unsigned long int tag_evicted_1 = (address_evicted >> ((unsigned long int)(Block_Size) + (unsigned long int)log2((L1.set_size))));
			location_found_2 = L2.least_used[set];
			Update_LRU(set, L2_cache);
			if (Search(tag_evicted_1, set_evicted_1, L1_cache) == 1)
			{
				L2.cache[set_evicted_1][location_found_1 + VALID] = 0;
				L2.cache[set_evicted_1][location_found_1 + DIRTY] = 0;
				L2.cache[set_evicted_1][location_found_1 + TAG] = MAX_VALUE;
				L2.cache[set_evicted_1][location_found_1 + ADDRESS] = 0;
				L2.cache[set_evicted_1][location_found_1 + LRU] = 0;
			}
		}
	}
	
}