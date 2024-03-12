
/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <stdbool.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

//make sure set and tag are for each cache level and arent interfering each other
//array of size set_size1
//holds idx of ways of least recently used
/*
	!!!!!!!!PLEASEEE DO NOT FORGET TO ADD LRU UPDATES WITHIN EACH ACCESS!!!!!!!!
*/

#define NUM_COL 5 // 0-valid bit, 1-dirty bit, 2-LRU, 3-tag, 4-address
#define N_DIRTY 0
#define DEBUG 0
#define MAX_VALUE 4294967295

//enum Operation{READ = "R", WRITE = "W"};
enum Alloc_cache{N_ALLOC = 0, ALLOC = 1};
enum col_cache{VALID = 0, DIRTY = 1,LRU = 2,TAG = 3,ADDRESS = 4};

void Cache_init(unsigned MemCyc, unsigned BSize , unsigned L1Size , unsigned L2Size , unsigned L1Assoc,
			unsigned L2Assoc ,unsigned L1Cyc ,unsigned L2Cyc ,unsigned WrAlloc );
bool Search_1(unsigned long int tag, unsigned long int set);//looking for tag in L1
bool Search_2(unsigned long int tag, unsigned long int set);//looking for tag in L2
void Insert_1(unsigned long int tag, unsigned long int set);//inserting tag in available space
void Cache_Write(unsigned long int tag, unsigned long int set);
void Insert_2(unsigned long int tag, unsigned long int set);
void Cache_Read(unsigned long int tag, unsigned long int set);
//void Calculate_tag_set(unsigned long int tag, unsigned long int set);// calculating for each address its set and tag
void UPDATE_LRU_1(unsigned set);//checking and updating Least Recently Used Way in L1+
                                //saving idx of way of LRU
void UPDATE_LRU_2(unsigned set);//checking and updating Least Recently Used Way in L2
								//saving idx of way of LRU
void Calculate_Set_Tag(unsigned long int address);
void Print_L1();
void Print_L2();

////////////////// important paramaters for the excercise //////////////////
/* 
set size = number of blocks / number of ways
number of ways = 2^L(1/2)Assoc
number of blocks = 2^cache size / 2^block size
number of lines = 2^set size
*/
////////////////////////////////////////////////////////////////////////////

/* Global parametrs*/
double L1_total_hit = 0;
//double L1_total_miss = 0;
double L1_total_access = 0;
double L2_total_hit = 0;
//double L2_total_miss = 0;
double L2_total_access = 0;
double Total_Access_time = 0;
double memory_total_Access = 0;
double Total_Access = 0;

bool flag_write_mem = false;
bool flag_write_L2 = false;

unsigned loc_found_1 = 0;//idx of found way in L1
unsigned loc_found_2 = 0;//idx of found way in L2

unsigned long int tag_1 = 0;
unsigned long int set_1 = 0;

unsigned long int tag_2 = 0;
unsigned long int set_2 = 0;

//maybe we should add another struct which the only thing it holds is the set and size of each  address given
struct Addresses{
	unsigned long int current_address ;
	unsigned long int current_tag_1 ;
	unsigned long int current_set_1 ;
	unsigned long int current_tag_2 ;
	unsigned long int current_set_2 ;	
};

struct Cache{
	unsigned Block_size;
	unsigned MemCyc;
	unsigned WrAlloc;

	
	unsigned** Cache_L1;
	unsigned* Least_used_1;
	unsigned set_size_1;
	unsigned num_blocks_1;
	unsigned num_lines_1;
	unsigned num_ways_1;
	unsigned L1Cyc;
	unsigned L1Size;
	unsigned L1Assoc;


	unsigned** Cache_L2;
	unsigned num_ways_2;
	unsigned L2Size;
	unsigned L2Cyc;
	unsigned L2Assoc;
	unsigned num_blocks_2;
	unsigned set_size_2;
	unsigned num_lines_2;
	unsigned* Least_used_2;
};

struct Cache *cache;
struct Addresses *addresses;

int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}
	//========================================================================
	Cache_init(MemCyc, BSize,L1Size  ,L2Size, L1Assoc,
			L2Assoc, L1Cyc, L2Cyc, WrAlloc);
	//========================================================================
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}
		/*
		// DEBUG - remove this line
		cout << "operation: " << operation;
		*/
		string cutAddress = address.substr(2); // Removing the "0x" part of the address
		/*
		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;
		*/

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		tag_1 = (num >> ((unsigned long int)(cache->Block_size) + (unsigned long int)log2((cache->set_size_1))));
		printf("1 -> Block_Size is: %d, L1.set_size is: %d.\n", cache->Block_size, cache->set_size_1);
		set_1 = (num >> ((unsigned long int)(cache->Block_size))) % ((unsigned long int)(cache->set_size_1));

		tag_2 = (num >> ((unsigned long int)(cache->Block_size) + (unsigned long int)log2(cache->set_size_2)));
		set_2 = (num >> ((unsigned long int)(cache->Block_size))) % (unsigned long int)(cache->set_size_2);

		addresses->current_address = num;
		Calculate_Set_Tag(num);

		switch(operation){
			case 'r':
				Cache_Read(tag_1,set_1);
		    	break;
			case 'w':
				Cache_Write(tag_1,set_1);
				break;
		}
		
		// DEBUG - remove this line
		//cout << " (dec) " << num << endl;
		

	}

	double L1MissRate=(L1_total_access-L1_total_hit)/L1_total_access;
	double L2MissRate=(L2_total_access-L2_total_hit)/L2_total_access;
	//double avgAccTime=Total_Access_time/(L1_total_access+L2_total_access+memory_total_Access);
	double avgAccTime=Total_Access_time/Total_Access;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	//releasing memory
	free(cache->Cache_L1);
	free(cache->Cache_L2);
	free(cache->Least_used_1);
	free(cache->Least_used_2);
	free(cache);
	free(addresses);

	return 0;
}
/*
void Calculate_tag_set(unsigned address){
	unsigned tag = (address>>(log2(cache->Block_size)+log2(cache->set_size_1)));
	unsigned set = (address>>(log2(cache->Block_size)))%( cache->set_size_1);
}
*/

void Cache_init(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc,
			unsigned L2Assoc, unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc){
				cache = (struct Cache*)malloc(sizeof(Cache));
				if(!cache){
					return;
				}
				cache->Block_size=BSize; //bits
				cache->L1Size=L1Size; //bits
				cache->L2Size=L2Size; //bits
				cache->L1Assoc=L1Assoc; //bits
				cache->L2Assoc=L2Assoc; //bits
				cache->L1Cyc=L1Cyc;
				cache->L2Cyc=L2Cyc;
				cache->MemCyc=MemCyc;
				cache->WrAlloc=WrAlloc;

				cache->num_ways_1=pow(2,L1Assoc); //num
				cache->num_ways_2=pow(2,L2Assoc); //num

				cache->num_blocks_1=(unsigned)(pow(2,L1Size))/(pow(2,BSize)); //num
				cache->num_blocks_2=(unsigned)(pow(2,L2Size))/(pow(2,BSize)); //num

				cache->set_size_1=(cache->num_blocks_1/cache->num_ways_1); //bits
				cache->set_size_2=(cache->num_blocks_2/cache->num_ways_2); //bits
				/*
				cache->num_lines_1=pow(2,cache->set_size_1); //num
				cache->num_lines_2=pow(2,cache->set_size_2); //num
				*/
				cache->num_lines_1=cache->set_size_1;
				cache->num_lines_2=cache->set_size_2;
				/* initializing the cache_1 table/s */
				cache->Cache_L1=(unsigned**)malloc(sizeof(unsigned*)*cache->num_lines_1);
				for(unsigned i=0;i<cache->num_lines_1;i++){
					cache->Cache_L1[i]=(unsigned*)malloc(sizeof(unsigned)*cache->num_ways_1*NUM_COL);
				}
				if(!cache->Cache_L1){
					free(cache->Cache_L1);
					return;
				}
				for(unsigned i=0;i<cache->num_lines_1;i++){
					for(unsigned j=0;j<cache->num_ways_1*NUM_COL;j++){
						cache->Cache_L1[i][j]=0;
					}
				}
				for(unsigned i=0;i<cache->num_lines_1;i++){
					for(unsigned j=TAG;j<cache->num_ways_1*NUM_COL;j+=NUM_COL){
						cache->Cache_L1[i][j]=MAX_VALUE;
					}
				}

				/* initializing the cache_2 table/s */
				cache->Cache_L2=(unsigned**)malloc(sizeof(unsigned*)*cache->num_lines_2);
				for(unsigned i=0;i<cache->num_lines_2;i++){
					cache->Cache_L2[i]=(unsigned*)malloc(sizeof(unsigned)*cache->num_ways_2*NUM_COL);
				}
				if(!cache->Cache_L2){
					free(cache->Cache_L2);
					return ;
				}
				for(unsigned i=0;i<cache->num_lines_2;i++){
					for(unsigned j=0;j<cache->num_ways_2*2;j++){
						cache->Cache_L2[i][j]=0;
					}
				}
				for(unsigned i=0;i<cache->num_lines_2;i++){
					for(unsigned j=TAG;j<cache->num_ways_2*NUM_COL;j+=NUM_COL){
						cache->Cache_L2[i][j]=MAX_VALUE;
					}
				}
				
				/* initializing the least recently used tables */
				cache->Least_used_1=(unsigned*)malloc(sizeof(unsigned)*cache->num_lines_1);
				if(!cache->Least_used_1){
					free(cache->Least_used_1);
					return;
				}
				for(unsigned i=0; i<cache->num_lines_1; i++){
					cache->Least_used_1[i]=0;
				}	
				cache->Least_used_2=(unsigned*)malloc(sizeof(unsigned)*cache->num_lines_2);
				if(!cache->Least_used_2){
					free(cache->Least_used_2);
					return;
				}	
				for(unsigned i=0; i<cache->num_lines_2; i++){
					cache->Least_used_2[i]=0;
				}		
				addresses=(struct Addresses*)malloc(sizeof(Addresses));
				if(!addresses){
					return;
				}	
				//if(DEBUG) Print_L1();		
			}
void Cache_Write(unsigned long int tag, unsigned long int set){
	//searching the tag according to the set value in all 'ways'
	Total_Access++;
	bool found_1=Search_1(tag, set);
	int found_1_int= (found_1)? 1 : 0;
	switch(cache->WrAlloc){
		case ALLOC:
			switch(found_1_int)
			{
				//found in L1, WRITE HIT
				case 1:
					if(DEBUG) printf("L1 hit \n");
					L1_total_access++;
					L1_total_hit++;
					Total_Access_time+=cache->L1Cyc;
					UPDATE_LRU_1(set);
					cache->Cache_L1[set][loc_found_1+DIRTY]=1;
					cache->Cache_L1[set][loc_found_1+ADDRESS]=addresses->current_address;
					//return;
					break;
				//didn't find in L1, WRITE MISS, go to search in L2
				case 0: 
					L1_total_access++;
					L2_total_access++;
					bool found_2=Search_2(tag_2, set_2);
					if(found_2){
						flag_write_L2 = true;
						Total_Access_time+=cache->L1Cyc + cache->L2Cyc;
						L2_total_hit++;
						//L1_total_access++;
						//Total_Access_time+=cache->L1Cyc;
						UPDATE_LRU_2(set_2);//update on the block we want to insert in L1
						//cache->Cache_L2[set_2][loc_found_2+DIRTY]=1;
						Insert_1(tag_1, set_1);//not sure were supposed to insert in L1 needs checking
						flag_write_L2 = false;
					}
					else{
						memory_total_Access++;
						Total_Access_time+=cache->L2Cyc + cache->L1Cyc + cache->MemCyc;
						Calculate_Set_Tag(addresses->current_address);
						flag_write_mem=true;
						Insert_2(addresses->current_tag_2, addresses->current_set_2);
						Insert_1(addresses->current_tag_1, addresses->current_set_1);
						flag_write_mem=false;
					}
			}
			break;
		case N_ALLOC:	
			switch(found_1_int)
			{
				//found in L1, WRITE HIT
				case 1:
					L1_total_access++;
					L1_total_hit++;
					Total_Access_time+=cache->L1Cyc;
					UPDATE_LRU_1(set_1);
					cache->Cache_L1[set_1][loc_found_1+DIRTY]=1;
					//return;
					break;
				//didn't find in L1, WRITE MISS, go to serach in L2
				case 0: 
					L1_total_access++;
					L2_total_access++;
					bool found_2=Search_2(tag_2, set_2);
					if(found_2){
						L2_total_hit++;
						Total_Access_time+=cache->L1Cyc+cache->L2Cyc;
						UPDATE_LRU_2(set_2);//update on the block we want to insert in L1
						cache->Cache_L2[set_2][loc_found_2+DIRTY]=1;
						//Insert_1(tag_1, set_1);//not sure were supposed to insert in L1 needs checking
					}
					else{
						memory_total_Access++;
						Total_Access_time+=cache->L1Cyc + cache->L2Cyc + cache->MemCyc;
					}
			}
			break;
	}
	if(DEBUG){
		Print_L1();
		Print_L2();
	} 	
}
void Cache_Read(unsigned long int tag, unsigned long int set){
	Total_Access++;
	bool found_1=Search_1(tag, set);
	int found_1_int= (found_1)? 1 : 0;
	switch(found_1_int)
	{
		//found in L1, READ HIT
		case 1:
			L1_total_access++;
			L1_total_hit++;
			Total_Access_time+=cache->L1Cyc;
			UPDATE_LRU_1(set_1);
			//return;
			break;
		//didnt find in L1, READ MISS, go to serach in L2
		case 0: 
			L1_total_access++;
			L2_total_access++;
			bool found_2=Search_2(tag_2, set_2);
			if(found_2){
				Total_Access_time+=cache->L1Cyc+cache->L2Cyc;
				L2_total_hit++;
				UPDATE_LRU_2(set_2);//update on the block we want to insert in L1
				Insert_1(tag_1, set_1);//not sure were supposed to insert in L1 needs checking
			}
			else{
				memory_total_Access++;
				Total_Access_time+=cache->L1Cyc+cache->L2Cyc+cache->MemCyc;
				Calculate_Set_Tag(addresses->current_address);
				Insert_2(addresses->current_tag_2, addresses->current_set_2);
				Insert_1(addresses->current_tag_1, addresses->current_set_1);

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
}


//===============================================================================
bool Search_1(unsigned long int tag, unsigned long int set){
	//if(DEBUG) Print_L1();
	//searching the tag according to the set value in all 'ways'
	unsigned way=0;
	//L1_total_access++;
	while(way<(cache->num_ways_1)*NUM_COL){
		//found in current way
		if(cache->Cache_L1[set][way+TAG] == tag && cache->Cache_L1[set][way+VALID]){
			loc_found_1 = way;
			//L1_total_hit++;
			return true;
		}
		//next way
		//L1_total_miss++;
		way+=NUM_COL;
	}
	return false;
}
bool Search_2(unsigned long int tag, unsigned long int set){

	//searching the tag according to the set value in all 'ways'
	unsigned way=0;
	//L2_total_access++;
	while(way<(cache->num_ways_2)*NUM_COL){
		//found in current way
		if(cache->Cache_L2[set][way+TAG] == tag && cache->Cache_L2[set][way+VALID]){
			loc_found_2 = way;
			//L2_total_hit++;
			return true;
		}
		//next way
		//L2_total_miss++;
		way+=NUM_COL;
	}
	return false;
}
//===============================================================================
void Insert_1(unsigned long int tag, unsigned long int set){
	unsigned i=0;
	bool found_empty = false;
	//looking for non valid bit in specific set
	while(!found_empty && i<(cache->num_ways_1)*NUM_COL){
		if(cache->Cache_L1[set][i+VALID]==1){
			i+=NUM_COL;
		}
		else{
			found_empty=true;
			if(DEBUG) printf("check if you are here test 1009 \n");
		}
		//L1_total_access++;
	}
	//found an available set in one of the ways, eviction wasn't needed
	if(found_empty){
		cache->Cache_L1[set][i+VALID]=1;
		cache->Cache_L1[set][i+TAG]=tag;
		cache->Cache_L1[set][i+DIRTY]=0;
		cache->Cache_L1[set][i+ADDRESS]=addresses->current_address;
		loc_found_1=i;
		UPDATE_LRU_1(set);
		if(DEBUG) printf("check if enters \n");
		if(flag_write_mem || flag_write_L2){
			cache->Cache_L1[set][i+DIRTY]=1;
		}
	}
	//didn't find an available set, eviction needed
	else{
		if(DEBUG) printf("did u get inside here by any chance \n");
		//DIRTY CHECK. checks if there's a need to copy from L1 to L2 before placing new INFO
		//UPDATE_LRU_1(set);
		switch(cache->Cache_L1[set][cache->Least_used_1[set]+DIRTY]){ 
			case N_DIRTY:
				cache->Cache_L1[set][cache->Least_used_1[set]+TAG] = tag;
				cache->Cache_L1[set][cache->Least_used_1[set]+DIRTY]=0;
				cache->Cache_L1[set][cache->Least_used_1[set]+VALID]=1;
				cache->Cache_L1[set][cache->Least_used_1[set]+ADDRESS]=addresses->current_address;
				loc_found_1=cache->Least_used_1[set];
				UPDATE_LRU_1(set);
				if(flag_write_mem || flag_write_L2){
					cache->Cache_L1[set][cache->Least_used_1[set]+DIRTY]=1;
				}
				break;
			case DIRTY://gonna be at L2 for sure, only needs to update L2 and insert tag to L1 in the right place+update_LRU1
				Calculate_Set_Tag(cache->Cache_L1[set][cache->Least_used_1[set]+ADDRESS]);
				if(Search_2(addresses->current_tag_2, addresses->current_set_2)){
					//loc_found_2 = cache->Least_used_2[addresses->current_set_2];
					UPDATE_LRU_2(addresses->current_set_2);//needs to update in L2 the block which is being evictes in L1
					//eviction and saving
					cache->Cache_L1[set][cache->Least_used_1[set]+VALID]=1;
					cache->Cache_L1[set][cache->Least_used_1[set]+TAG]=tag;
					cache->Cache_L1[set][cache->Least_used_1[set]+ADDRESS]=addresses->current_address;
					cache->Cache_L1[set][cache->Least_used_1[set]+DIRTY]=0;
					loc_found_1=cache->Least_used_1[addresses->current_set_1];
					UPDATE_LRU_1(set);
					if(flag_write_mem || flag_write_L2){
						cache->Cache_L1[set][cache->Least_used_1[set]+DIRTY]=1;
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
	unsigned i=0;
	bool found_empty = false;
	//looking for non valid bit in specific set
	while(!found_empty && i<(cache->num_ways_2)*NUM_COL){
		if(cache->Cache_L2[set][i+VALID]){
			i+=NUM_COL;
		}
		else{
			found_empty=true;
		}
		//L2_total_access++;
	}
	//found an available way in L2, putting data
	if(found_empty){
		cache->Cache_L2[set][i+VALID]=1;
		cache->Cache_L2[set][i+TAG]=tag;
		cache->Cache_L2[set][i+DIRTY]=0;
		cache->Cache_L2[set][i+ADDRESS]=addresses->current_address;
		loc_found_2=i;
		UPDATE_LRU_2(set);
	}
	//didn't find available way in L2, making eviction and putting data
	else{
		if(cache->Cache_L2[set][cache->Least_used_2[set]+DIRTY]){
			memory_total_Access++;
			Total_Access_time+=cache->MemCyc;
		}		
		cache->Cache_L2[set][cache->Least_used_2[set]+TAG] = tag;
		cache->Cache_L2[set][cache->Least_used_2[set]+DIRTY]=0;
		cache->Cache_L2[set][cache->Least_used_2[set]+VALID]=1;
		unsigned long int address_evicted = cache->Cache_L2[set][cache->Least_used_2[set]+ADDRESS];
		cache->Cache_L2[set][cache->Least_used_2[set]+ADDRESS]=addresses->current_address;	
		unsigned long int set_evicted_1= (address_evicted>>((unsigned long int)(cache->Block_size)))%((unsigned long int)(cache->set_size_1));
		unsigned long int tag_evicted_1= (address_evicted>>((unsigned long int)(cache->Block_size)+(unsigned long int)log2((cache->set_size_1))));	
		loc_found_2=cache->Least_used_2[set];
		UPDATE_LRU_2(set);
		if(Search_1(tag_evicted_1, set_evicted_1)){
			cache->Cache_L1[set_evicted_1][loc_found_1+VALID]=0;
			cache->Cache_L1[set_evicted_1][loc_found_1+DIRTY]=0;
			cache->Cache_L1[set_evicted_1][loc_found_1+TAG]=MAX_VALUE;
			cache->Cache_L1[set_evicted_1][loc_found_1+ADDRESS]=0;
			cache->Cache_L1[set_evicted_1][loc_found_1+LRU]=0;
		}
	}
}

//====================================================================================================================
void UPDATE_LRU_1(unsigned set){
	bool found=false;
	unsigned last_access = cache->Cache_L1[set][loc_found_1+LRU];
	if(DEBUG) printf("last access %d \n",last_access);
	cache->Cache_L1[set][loc_found_1+LRU] = cache->num_ways_1-1;//last way to get accessed
	if(DEBUG) printf("LRU is %d \n", cache->Cache_L1[set][loc_found_1+LRU]);
	for(unsigned i=LRU; i < cache->num_ways_1*NUM_COL; i+=NUM_COL){//updates i to each beginning of a new WAY
		if((i!=loc_found_1+LRU) && (cache->Cache_L1[set][i] > last_access)){
			cache->Cache_L1[set][i]--;
			if(DEBUG) printf("your'e not supposed to be here \n");
		}
		if(cache->Cache_L1[set][i] == 0 && !found){
			cache->Least_used_1[set]=i-LRU;//-LRU
			found=true;
		}
	}
}
void UPDATE_LRU_2(unsigned set){
	bool found=false;
	unsigned last_access = cache->Cache_L2[set][loc_found_2+LRU];
	cache->Cache_L2[set][loc_found_2+LRU] = cache->num_ways_2-1;
	for(unsigned i=LRU; i < cache->num_ways_2*NUM_COL; i+=NUM_COL){
		if((i!=loc_found_2+LRU) && (cache->Cache_L2[set][i] > last_access)){
			cache->Cache_L2[set][i]--;
		}
		if(cache->Cache_L2[set][i] == 0 && !found){
			cache->Least_used_2[set]=i-LRU;//-LRU
			found=true;
		}
	}	
}
//===========================================================================================================================




void Calculate_Set_Tag(unsigned long int address){
	addresses->current_tag_1 = (address>>((unsigned long int)(cache->Block_size)+(unsigned long int)log2(cache->set_size_1)));
	addresses->current_set_1 = (address>>((unsigned long int)(cache->Block_size)))%(unsigned long int)( cache->set_size_1);

	addresses->current_tag_2 = (address>>((unsigned long int)(cache->Block_size)+(unsigned long int)log2(cache->set_size_2)));
	addresses->current_set_2 = (address>>((unsigned long int)(cache->Block_size)))%(unsigned long int)(cache->set_size_2);
}
void Print_L1(){
	unsigned j=0;
	int count=5;
	for(unsigned i =0; i<cache->num_lines_1;i++){
		printf("cache L1 set:%d info is- \n", i);
		for( j =0; j<(cache->num_ways_1*NUM_COL);j++){
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
			printf("%d  ", cache->Cache_L1[i][j]);
			count++;
		}
		printf("\n");
	}
	printf("\n");
}
void Print_L2(){
	unsigned j=0;
	int count = 0;
	for(unsigned i =0; i<cache->num_lines_2;i++){
		printf("cache L2 set:%d info is- \n", i);
		for( j =0; j<(cache->num_ways_2*NUM_COL);j++){
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
			printf("%d  ", cache->Cache_L2[i][j]);
			count ++;
		}
		printf("\n");
	}
	printf("\n");
}

