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

// CHANGE THIS LATER 
// 0-valid bit, 1-dirty bit, 2-LRU, 3-tag, 4-address
#define DEBUG 0
#define N_DIRTY 0
#define NUMBERS_COLOUMN 5 
#define MAX_VALUE 4294967295

// =============== also change this ==========================
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
//============================================================

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

//======================= FUNCTIONS ============================
void Cache_init(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
			unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc);

void Insert(unsigned long int tag, unsigned long int set, cache_type L);
void Insert_L1(unsigned long int tag, unsigned long int set);
void Insert_L2(unsigned long int tag, unsigned long int set);
void UPDATE_LRU(unsigned set, unsigned location, Cache* cache_ptr);
bool Search(unsigned long int tag, unsigned long int set, Cache* cache_ptr);
void Calculate_Set_Tag(unsigned long int address);
void Cache_Write();
void Cache_Read();
void Write_With_Allocate(bool found);
void Write_No_Allocate(bool found);
void L1_init(unsigned L1Size, unsigned L1Assoc, unsigned L1Cyc);
void L2_init(unsigned L2Size, unsigned L2Assoc, unsigned L2Cyc);
void Cache_Feed(char operation);



/* ============= Statistics Variables ============ */
double L1_hits = 0;
double L1_accesses = 0;

double L2_hits = 0;
double L2_accesses = 0;

double Total_Access_Time = 0;
double Memory_Total_Access = 0;
double Total_Accesses = 0;


// flags that tells us if we need to write to meme orL2
bool write_mem = false;
bool write_L2 = false;

// this is the place where the tag sits if found
unsigned location_found_1 = 0; // in L1
unsigned location_found_2 = 0; // in L2


unsigned long int tag_L1 = 0;
unsigned long int set_L1 = 0;
unsigned long int tag_L2 = 0;
unsigned long int set_L2 = 0;


// Global tags for each cache for the current address
unsigned long int current_address;
unsigned long int current_tag_L1;
unsigned long int current_set_L1;
unsigned long int current_tag_L2;
unsigned long int current_set_L2;

// Global Variables
unsigned Block_Size;
unsigned Memory_Cycles;
unsigned Write_Alloc;

struct Cache L1;
struct Cache L2;

int main(int argc, char **argv)
{
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
	if(DEBUG) printf("passed the arg grabs\n");



	// CACHE INITIALIZER LINE
	Cache_init( 
				L1Size,  L1Assoc,  L1Cyc, 
				L2Size,  L2Assoc,  L2Cyc,
				WrAlloc,  MemCyc,  BSize
			);


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


		// ==================== TAG CALCS =====================
		tag_L1 = (num >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L1.set_size)));
		set_L1 = (num >> ((unsigned long int)(Block_Size))) % ((unsigned long int)(L1.set_size));
		tag_L2 = (num >> ((unsigned long int)(Block_Size) + (unsigned long int)log2(L2.set_size)));
		set_L2 = (num >> (unsigned long int)(Block_Size)) % (unsigned long int)(L2.set_size);
		current_address = num;
		Calculate_Set_Tag(num);
		Cache_Feed(operation);
		k++;
	}


	//  STATS CALC
	double L1MissRate = (L1_accesses - L1_hits) / L1_accesses;
	double L2MissRate = (L2_accesses - L2_hits) / L2_accesses;
	double avgAccTime = Total_Access_Time / Total_Accesses;


	// STATS PRINT
	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);


	// MEMORY RELEASE (COPIUM)
	free(L1.least_used);
	free(L2.least_used);
	free(L1.cache);
	free(L2.cache);
	if(DEBUG)printf("sanity check 88\n");
	return 0;
}


void Cache_init(
			unsigned L1Size,  unsigned L1Assoc, unsigned L1Cyc, 
			unsigned L2Size,  unsigned L2Assoc, unsigned L2Cyc,
			unsigned WrAlloc, unsigned MemCyc,  unsigned BSize
			){
	Block_Size = BSize;
	Memory_Cycles = MemCyc;
	Write_Alloc = WrAlloc;
	L1_init(L1Size, L1Assoc, L1Cyc);
	L2_init(L2Size, L2Assoc, L2Cyc);
}


void L1_init(unsigned L1Size, unsigned L1Assoc, unsigned L1Cyc){

	L1.size = L1Size;	
	L1.assoc = L1Assoc; 
	L1.num_of_cycles = L1Cyc;
	L1.num_ways = pow(2, L1Assoc);								 
	L1.num_blocks = (unsigned)(pow(2, L1Size)) / (pow(2, Block_Size));
	L1.set_size = (L1.num_blocks / L1.num_ways);
	L1.num_lines = L1.set_size;

	/* INIT L1 TABLES */
	L1.cache = (unsigned **)malloc(sizeof(unsigned *) * L1.num_lines);
	for (unsigned i = 0; i < L1.num_lines; i++)
	{
		L1.cache[i] = (unsigned *)malloc(sizeof(unsigned) * L1.num_ways * NUMBERS_COLOUMN);
	}
	if (!L1.cache)
	{
		free(L1.cache);
		return;
	}
	for (unsigned i = 0; i < L1.num_lines; i++)
	{
		for (unsigned j = 0; j < L1.num_ways * NUMBERS_COLOUMN; j++)
		{
			L1.cache[i][j] = 0;
		}
	}
	for (unsigned i = 0; i < L1.num_lines; i++)
	{
		for (unsigned j = TAG; j < L1.num_ways * NUMBERS_COLOUMN; j += NUMBERS_COLOUMN)
		{
			L1.cache[i][j] = MAX_VALUE;
		}
	}

	L1.least_used = (unsigned *)malloc(sizeof(unsigned) * L1.num_lines);
	if (!L1.least_used)
	{
		free(L1.least_used);
		return;
	}
	for (unsigned i = 0; i < L1.num_lines; i++)
	{
		L1.least_used[i] = 0;
	}
}


void L2_init(unsigned L2Size, unsigned L2Assoc, unsigned L2Cyc){

	L2.size = L2Size;
	L2.assoc = L2Assoc;
	L2.num_of_cycles = L2Cyc;
	L2.num_ways = pow(2, L2Assoc);						  		
	L2.num_blocks = (unsigned)(pow(2, L2Size)) / (pow(2, Block_Size));
	L2.set_size = (L2.num_blocks / L2.num_ways);
	L2.num_lines = L2.set_size;

	/* INIT L2 TABLES */
	L2.cache = (unsigned **)malloc(sizeof(unsigned *) * L2.num_lines);

	for (unsigned i = 0; i < L2.num_lines; i++)
	{
		L2.cache[i] = (unsigned *)malloc(sizeof(unsigned) * L2.num_ways * NUMBERS_COLOUMN);
	}

	if (!L2.cache)
	{
		free(L2.cache);
		return;
	}

	for (unsigned i = 0; i < L2.num_lines; i++)
	{
		for (unsigned j = 0; j < L2.num_ways * 2; j++)
		{
			L2.cache[i][j] = 0;
		}
	}

	for (unsigned i = 0; i < L2.num_lines; i++)
	{
		for (unsigned j = TAG; j < L2.num_ways * NUMBERS_COLOUMN; j += NUMBERS_COLOUMN)
		{
			L2.cache[i][j] = MAX_VALUE;
		}
	}

	L2.least_used = (unsigned *)malloc(sizeof(unsigned) * L2.num_lines);

	if (!L2.least_used)
	{
		free(L2.least_used);
		return;
	}

	for (unsigned i = 0; i < L2.num_lines; i++)
	{
		L2.least_used[i] = 0;
	}
}


void Cache_Feed(char operation)
{
	switch (operation)
	{
	case 'r':
		Cache_Read();
		break;
	case 'w':
		Cache_Write();
		break;
	}
}


void Cache_Read()
{
	if (DEBUG) printf("entered cache read\n");
	Total_Accesses++;
	if (Search(tag_L1, set_L1, &L1))
	{
		// L1 READ HIT
		if (DEBUG) printf("L1 READ HIT\n");
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		UPDATE_LRU(set_L1, location_found_1, &L1);
	}
	else
	{
		// L1 READ MISS
		if (DEBUG) printf("L1 READ MISS\n");
		L1_accesses++;
		L2_accesses++;
		if (Search(tag_L2, set_L2, &L2))
		{
			// L2 READ HIT
			if (DEBUG) printf("L2 READ HIT\n");
			L2_hits++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			UPDATE_LRU(set_L2, location_found_2, &L2);
			Insert(tag_L1, set_L1, L1_cache);
		}
		else
		{
			// L2 READ MISS
			if (DEBUG)printf("L2 READ MISS\n");
			Memory_Total_Access++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
			Calculate_Set_Tag(current_address);
			Insert(current_tag_L2, current_set_L2, L2_cache);
			Insert(current_tag_L1, current_set_L1, L1_cache);
		}
	}
	if (DEBUG)printf("left cache read\n");
}


void Cache_Write(){
	Total_Accesses++;
	bool found = Search(tag_L1, set_L1, &L1);
	if (DEBUG)printf("write search result is: %d\n", found);
	if (DEBUG)printf("entered cache write\n");
	if (Write_Alloc)
	{
		if (DEBUG)printf("chose allocate\n");
		Write_With_Allocate(found);
	}
	else
	{
		if (DEBUG)printf("chose no allocate\n");
		Write_No_Allocate(found);
	}
	if(DEBUG) printf("left write\n");
}


void Write_With_Allocate(bool found)
{
	if (found)
	{
		// L1 WRITE HIT
		if (DEBUG)printf("L1 WRITE HIT(ALLOC)\n");
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		UPDATE_LRU(set_L1, location_found_1, &L1);
		L1.cache[set_L1][location_found_1 + DIRTY] = 1;
		L1.cache[set_L1][location_found_1 + ADDRESS] = current_address;
		// return;
	}
	else
	{
		// L1 WRITE MISS
		if (DEBUG)printf("L1 WRITE MISS(ALLOC)\n");
		L1_accesses++;
		L2_accesses++;
		bool found_2 = Search(tag_L2, set_L2, &L2);
		if (found_2)
		{
			// L2 HIT
			if (DEBUG)printf("L2 WRITE HIT(ALLOC)\n");
			write_L2 = true;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			L2_hits++;
			UPDATE_LRU(set_L2, location_found_2, &L2);
			Insert(tag_L1, set_L1, L1_cache);
			write_L2 = false;
		}
		else
		{
			// L2 MISS
			if (DEBUG)printf("L2 WRITE MISS(ALLOC)\n");
			Memory_Total_Access++;
			Total_Access_Time += L2.num_of_cycles + L1.num_of_cycles + Memory_Cycles;
			Calculate_Set_Tag(current_address);
			write_mem = true;
			Insert(current_tag_L2, current_set_L2, L2_cache);
			Insert(current_tag_L1, current_set_L1, L1_cache);
			write_mem = false;
		}
	}
}


void Write_No_Allocate(bool found)
{
	if (found)
	{
		// L1 WRITE HIT
		if (DEBUG)printf("L1 WRITE HIT(NO ALLOC)\n");
		L1_accesses++;
		L1_hits++;
		Total_Access_Time += L1.num_of_cycles;
		UPDATE_LRU(set_L1, location_found_1, &L1);
		L1.cache[set_L1][location_found_1 + DIRTY] = 1;
		// return;
	}
	else
	{
		// L1 WRITE MISS
		if (DEBUG)printf("L1 WRITE MISS(NO ALLOC)\n");
		L1_accesses++;
		L2_accesses++;
		bool found_2 = Search(tag_L2, set_L2, &L2);
		if (found_2)
		{
			// L2 WRITE HIT
			if (DEBUG)printf("L2 WRITE HIT(NO ALLOC)\n");
			L2_hits++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles;
			UPDATE_LRU(set_L2, location_found_2, &L2);
			L2.cache[set_L2][location_found_2 + DIRTY] = 1;
		}
		else
		{
			// L2 WRITE MISS
			if (DEBUG)printf("L2 WRITE MISS(NO ALLOC)\n");
			Memory_Total_Access++;
			Total_Access_Time += L1.num_of_cycles + L2.num_of_cycles + Memory_Cycles;
		}
	}
}


bool Search(unsigned long int tag, unsigned long int set, Cache *cache_ptr)
{
	unsigned way = 0;
	while (way < (cache_ptr->num_ways) * NUMBERS_COLOUMN)
	{
		if (cache_ptr->cache[set][way + TAG] == tag && cache_ptr->cache[set][way + VALID])
		{
			if (cache_ptr == &L1)
			{
				location_found_1 = way;
			}
			else
			{
				location_found_2 = way;
			}
			return true;
		}
		way += NUMBERS_COLOUMN;
	}
	return false;
}


void UPDATE_LRU(unsigned set, unsigned location, Cache* cache_ptr)
{
	bool found = false;
	unsigned last_access = cache_ptr->cache[set][location + LRU];
	cache_ptr->cache[set][location + LRU] = cache_ptr->num_ways - 1;
	for (unsigned i = LRU; i < cache_ptr->num_ways * NUMBERS_COLOUMN; i += NUMBERS_COLOUMN)
	{ 	
		if ((i != location + LRU) && (cache_ptr->cache[set][i] > last_access))
		{
			cache_ptr->cache[set][i]--;
		}
		if (cache_ptr->cache[set][i] == 0 && !found)
		{
			cache_ptr->least_used[set] = i - LRU;
			found = true;
		}
	}
}

void Insert(unsigned long int tag, unsigned long int set, cache_type L){

	if(L == L1_cache){
		Insert_L1(tag, set);
	}
	else
	{
		Insert_L2(tag, set);
	}
}


void Insert_L1(unsigned long int tag, unsigned long int set)
{
	unsigned i = 0;
	bool found_empty = false;
	// LOOKING FOR EMPTY SLOT
	if (DEBUG)
		printf("L1 INSERT\n");
	while (!found_empty && i < (L1.num_ways) * NUMBERS_COLOUMN)
	{
		if (L1.cache[set][i + VALID] == 1)
		{
			i += NUMBERS_COLOUMN;
		}
		else
		{
			found_empty = true;
		}
	}
	// FOUND SLOT
	if (found_empty)
	{
		L1.cache[set][i + VALID] = 1;
		L1.cache[set][i + DIRTY] = 0;
		L1.cache[set][i + TAG] = tag;
		L1.cache[set][i + ADDRESS] = current_address;


		location_found_1 = i;
		UPDATE_LRU(set, location_found_1, &L1);
		if (write_mem || write_L2)
		{
			L1.cache[set][i + DIRTY] = 1;
		}
	}
	else
	{
		// WRITEBACK CHECKING (NEED KICKING)
		switch (L1.cache[set][L1.least_used[set] + DIRTY])
		{
		case N_DIRTY:
			L1.cache[set][L1.least_used[set] + DIRTY] = 0;
			L1.cache[set][L1.least_used[set] + VALID] = 1;
			L1.cache[set][L1.least_used[set] + TAG] = tag;
			L1.cache[set][L1.least_used[set] + ADDRESS] = current_address;

			location_found_1 = L1.least_used[set];
			UPDATE_LRU(set, location_found_1, &L1);
			if (write_mem || write_L2)
			{
				L1.cache[set][L1.least_used[set] + DIRTY] = 1;
			}

			break;
		case DIRTY:
			Calculate_Set_Tag(L1.cache[set][L1.least_used[set] + ADDRESS]);
			if (Search(current_tag_L2, current_set_L2, &L2))
			{
				UPDATE_LRU(current_set_L2, location_found_2, &L2);
				// KICKING AND FILLING
				L1.cache[set][L1.least_used[set] + DIRTY] = 0;
				L1.cache[set][L1.least_used[set] + VALID] = 1;
				L1.cache[set][L1.least_used[set] + TAG] = tag;
				L1.cache[set][L1.least_used[set] + ADDRESS] = current_address;
				
				location_found_1 = L1.least_used[current_set_L1];
				UPDATE_LRU(set, location_found_1, &L1);
				if (write_mem || write_L2)
				{
					L1.cache[set][L1.least_used[set] + DIRTY] = 1;
				}
			}
			break;
		}
	}
}


void Insert_L2(unsigned long int tag, unsigned long int set)
{

	if (DEBUG)printf("L2 INSERT\n");
	unsigned i = 0;
	bool found_empty = false;


	// LOOKING FOR AN INVALID SLOT
	while (!found_empty && i < (L2.num_ways) * NUMBERS_COLOUMN)
	{
		if (L2.cache[set][i + VALID])
		{
			i += NUMBERS_COLOUMN;
		}
		else
		{
			found_empty = true;
		}
	}
	
	if (found_empty)
	{
		// FOUND SLOT
		L2.cache[set][i + VALID] = 1;
		L2.cache[set][i + TAG] = tag;
		L2.cache[set][i + DIRTY] = 0;
		L2.cache[set][i + ADDRESS] = current_address;
		location_found_2 = i;
		UPDATE_LRU(set, location_found_2, &L2);
	}
	else
	{ 
		// NO SPACE NEED TO KICK SOMEONE
		if (L2.cache[set][L2.least_used[set] + DIRTY])
		{
			Memory_Total_Access++;
			Total_Access_Time += Memory_Cycles;
		}


		L2.cache[set][L2.least_used[set] + VALID] = 1;
		L2.cache[set][L2.least_used[set] + DIRTY] = 0;
		L2.cache[set][L2.least_used[set] + TAG] = tag;
		unsigned long int address_evicted = L2.cache[set][L2.least_used[set] + ADDRESS];
		L2.cache[set][L2.least_used[set] + ADDRESS] = current_address;
		
		unsigned long int tag_victim = (address_evicted >> ((unsigned long int)(Block_Size) + (unsigned long int)log2((L1.set_size))));
		unsigned long int set_victim = (address_evicted >> ((unsigned long int)(Block_Size))) % ((unsigned long int)(L1.set_size));
		


		location_found_2 = L2.least_used[set];
		UPDATE_LRU(set, location_found_2, &L2);
		if (Search(tag_victim, set_victim, &L1))
		{
			L1.cache[set_victim][location_found_1 + DIRTY] = 0;
			L1.cache[set_victim][location_found_1 + VALID] = 0;
			L1.cache[set_victim][location_found_1 + TAG] = MAX_VALUE;
			L1.cache[set_victim][location_found_1 + ADDRESS] = 0;
			L1.cache[set_victim][location_found_1 + LRU] = 0;
		}
	}
}

// Updates current address's global L1 and L2 tags and sets. (DONE)
void Calculate_Set_Tag(unsigned long int address){
	// TAGS
	unsigned long int shift_amount_L1 = (unsigned long int)(Block_Size) + (unsigned long int)log2(L1.set_size);
	unsigned long int shift_amount_L2 = (unsigned long int)(Block_Size) + (unsigned long int)log2(L2.set_size);
	current_tag_L1 = (address >> shift_amount_L1);
	current_tag_L2 = (address >> shift_amount_L2);

	// SETS
	unsigned long int remainder_L1 = (unsigned long int)(L1.set_size);
	unsigned long int remainder_L2 = (unsigned long int)(L2.set_size);
	current_set_L1 = (address >> ((unsigned long int)(Block_Size))) % remainder_L1;
	current_set_L2 = (address >> ((unsigned long int)(Block_Size))) % remainder_L2;
}

