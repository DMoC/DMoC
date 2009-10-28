#ifndef MAPPING_9_H
#define MAPPINT_9_H

// max size is MAX_PAGESxsizeof(short) = 0x8000 * 2
#define BASE_GLOBAL_COUNTERS_OFFSET 0x00000000

// max size is MAX_PAGESxTOP_N*sizeof(int) = 0x60000
#define BASE_T_ID_PERPAGE	    0x00080000

// max size is MAX_PAGESxsizeof(int) = 0x20000
#define BASE_MAX_ID_PERPAGE         0x00100000 

// max size is TOP_N*sizeof(int) + sizeof(int) = 0x10
#define BASE_T_MAX_PAGE            0x00180000 
#define MASK_T_MAX_PAGE            0xFFFFFFF0 
#define SHIFT_T_MAX_PAGE             4

#define BASE_T_PAGE                0x00000010
#define BASE_MAX_PAGE              0x00000000


// max size is MAX_PAGESxMAX_NODES*sizeof(short) = 0x800000
#define BASE_NODES_COUNTERS_OFFSET  0x00800000 

#define MASK_NCOUNT                 0xFF800000

#define SHIFT_NCOUNT                23
#define NCOUNT                      (BASE_NODES_COUNTERS_OFFSET >> SHIFT_NCOUNT) 

#define MASK_NOT_NCOUNT             0xFFF80000
#define SHIFT_NOT_NCOUNT            16

#define GCOUNT                      (BASE_GLOBAL_COUNTERS_OFFSET >> SHIFT_NOT_NCOUNT) 
#define TIDP                        (BASE_T_ID_PERPAGE >> SHIFT_NOT_NCOUNT)	
#define MAXIP                       (BASE_MAX_ID_PERPAGE >> SHIFT_NOT_NCOUNT) 
#define TMAXP                       (BASE_T_MAX_PAGE   >> SHIFT_NOT_NCOUNT) 

#define TP                          (BASE_T_PAGE   >> SHIFT_T_MAX_PAGE) 
#define MAX                         (BASE_MAX_PAGE  >> SHIFT_T_MAX_PAGE) 

#endif
