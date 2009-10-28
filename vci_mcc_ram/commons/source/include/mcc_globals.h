#ifndef MCC_GLOBALS_H
#define MCC_GLOBALS_H

#include "micro_ring_param.h"

//#define DEBUG_M9
//#define DEBUG_M5

#if DEBUG_W
#define CWOUT cout
#else
#define CWOUT if(0) cout
#endif
#define DEBUG_M6
#define DEBUG_M6_LEVEL 3
//#define START_TRACE_CYCLE 117300000
#define START_TRACE_CYCLE 0
//#define DEBUG_M7
//#define DEBUG_M10
#define DEBUG_M10_LEVEL 2
//#define DEBUG_M9
//#define DEBUG_M8
//#define DEBUG_RAM
#define DEBUG_RAM_LEVEL 3
#define DEBUG_CACHE
#define DEBUG_CACHE_LEVEL 3
//#define TOTO
#define CTOR_OUT\
	cout << "constructor  " << __func__ <<  endl; 
#define DTOR_OUT\
	cout << "destructor  " << __func__ <<   endl;



// cache
const unsigned int TLB_SIZE = 8;

// mcc_ram
#ifdef PAGE_SIZE
#error vci_mcc_ram PAGE_SIZE defined by macro
#else
const unsigned int PAGE_SIZE = 512;
#endif

#define MAX_NODES 128
#define MAX_PAGES 0x8000

// Module_9
typedef unsigned int counter_t;
typedef int counter_id_t;

#define NBIT_ORDER 1

// PE_PERIOD : how many requests to a page are necessary in order to make the migration worthless
// with ~2k cycles per migration , the overhead is about 1k cycles (2k/2). Thus, with a mesh topology,
// roughly 16 nodes and 2 cycles per node. Optimizing data placement will lead to a ~4 cycles latency
// reduction on each requests!. 1/5 is a store, so at the end : 
// 1000 / ( 4*(4/5 nbr))  => ~300 requests are requiered at least to make the migration worthless!
//
// For DSPin : min lat = 2*5 + 3, mean lat = sqrt(2*16)/3 * 5 * 2 + 3
// In the end, PE_PERIOD near to 100
//#define PE_PERIOD 500
//#define PE_PERIOD_CYCLES 100000
#define PE_PERIOD 128 
#define PE_PERIOD_CYCLES 100000

//#define PE_ON_GLOBAL

#define M9_COMP_OP ==
// Module_10
#define TIME_TO_WAIT 1024
//#define TIME_TO_WAIT 1
// Module_8


// SAT_THRESHOLD : 80% of how many requests per SAT_TIME_SLOT period can
// process the memory module (in our case, 1 request => 11 cycles (8 words line)
//#define SAT_THRESHOLD 877
//#define THRES_P
#define SAT_THRESHOLD 100000
#define SAT_TIME_SLOT 10000000 

#define CONTENTION_THRESHOLD 3
// Module_6
#define M6_RING_FIFO_SIZE 3
#define M6_M10_INTER_FIFO_SIZE 1
#define M6_MAX_LOC_COST 100000
//typedef unsigned int micro_ring_cmd_t;
//typedef unsigned int micro_ring_data_t;
//typedef unsigned int micro_ring_id_t;
//typedef bool micro_ring_ack_t;
struct no_sc_MicroRingSignals
{
	public:
	soclib::caba::micro_ring_cmd_t  cmd;
	soclib::caba::micro_ring_id_t   id;
	soclib::caba::micro_ring_data_t data;
	soclib::caba::micro_ring_ack_t  ack;
	soclib::caba::micro_ring_id_t   srcid;
};

#endif
