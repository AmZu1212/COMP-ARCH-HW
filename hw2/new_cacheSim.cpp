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
	if(DEBUG)printf("sanity check 88\n");
	return 0;
}
