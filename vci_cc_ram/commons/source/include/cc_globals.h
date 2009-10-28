#ifndef CC_GLOBALS_H
#define CC_GLOBALS_H


//#define DEBUG_M9
//#define DEBUG_M5

#if DEBUG_W
#define CWOUT cout
#else
#define CWOUT if(0) cout
#endif
//#define DEBUG_RAM
#define DEBUG_RAM_LEVEL 3
#define CTOR_OUT\
	cout << "constructor  " << __func__ <<  endl; 
#define DTOR_OUT\
	cout << "destructor  " << __func__ <<   endl;

// mcc_ram
#ifdef PAGE_SIZE
#error vci_mcc_ram PAGE_SIZE defined by macro
#else
const unsigned int PAGE_SIZE = 512;
#endif
#endif
