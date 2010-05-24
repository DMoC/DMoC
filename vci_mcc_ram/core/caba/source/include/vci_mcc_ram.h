/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 * 
 * This file is part of SoCLib, GNU LGPLv2.1.
 * 
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 * 
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) UPMC, Lip6, Asim
 *         Adapted from soclib_vci_multiram_wt.h by Frederic Arzel 20/09/2004
 *         Perre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr> 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#ifndef VCI_MCC_RAM_H
#define VCI_MCC_RAM_H

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include <fstream>


#include "vci_mcc_ram_interface.h"
#include "caba_base_module.h"
#include "vci_target_fsm_nlock_tlb.h"
#include "vci_initiator.h"
#include "vci_target.h"
#include "generic_fifo.h" 
#include "mapping_table.h"
#include "cc_id_table.h"
#include "loader.h"

#include "phys_page_table.h"
#include "soclib_directory.h" 
#include "module_5.h"
#include "module_6.h"
#include "micro_ring_ports.h"
#include <string>


#ifdef DEBUG_RAM
	#define DRAMCOUT(x) if (x>= DEBUG_RAM_LEVEL) cout
#else
	#define DRAMCOUT(x) if(0) cout
#endif

namespace soclib{
namespace caba{
using namespace sc_core;

////////////////////////////////////////        
//    structure definition
////////////////////////////////////////
template <typename vci_param>
class VciMccRam : BaseModule,  public soclib::caba::VciMccRam_interface {

private :

typedef uint32_t        data_t;
typedef uint32_t        addr_t;
typedef bool            eop_t;
typedef uint32_t        cmd_t;

// RAM_FSM states
enum ram_fsm_state_e{
	RAM_IDLE,
	RAM_POISON,
	RAM_UNPOISON,
	RAM_TLB_INV,
	RAM_TLB_ACK,
	RAM_TLB_DIR_UPDT,
	RAM_TLB_INV_OK,
	RAM_DIRUPD,
	RAM_WIAM_UPDT,
	RAM_INVAL,
	RAM_ERROR,
	RAM_INVAL_WAIT
};

// INV_FSM states
enum inv_fsm_state_e{
	RAM_INV_IDLE, 
	RAM_INV_REQ,
	RAM_INV_RSP
};


// IO PORTS
    
public :
sc_in<bool>                     	p_clk;
sc_in<bool>                     	p_resetn;
soclib::caba::VciTarget<vci_param>	p_t_vci;
soclib::caba::VciInitiator<vci_param>  	p_i_vci;
soclib::caba::VciTargetFsmNlockTlb<vci_param,true,true> m_vci_fsm;

public :
// Internal Modules REGISTERS and STRUCTURAL PARAMETERS

Module_5 * m_5;
std::ostringstream m5_name;
std::ostringstream m6_name;

std::list<soclib::common::Segment>	*m_SEG_5_LIST;
unsigned int		m_PAGE_SHIFT;
int			*m_alias_registers;

Module_6  * m_6;

soclib::caba::MicroRing_Out	p_ring_out;
soclib::caba::MicroRing_In 	p_ring_in;
// Used as a wrapper between non SystemC modules (ie m_6), and SystemC Vci interfaces
// In order to work, at the begenning of transition, all the input signals should be copied to the wrapper/port,
// and, at the beginning of the Moore, all the outputs signals should be copied to the port/wrapper
no_sc_MicroRingSignals		m_m6_out;
no_sc_MicroRingSignals		m_m6_in;

//  REGISTERS

sc_signal<int>		r_RAM_FSM;
sc_signal<int>		r_INV_FSM;

sc_signal<bool> 	r_IN_TRANSACTION;

sc_signal<addr_t>	r_SAV_ADDRESS;
sc_signal<int>		r_SAV_SEGNUM;
sc_signal<int>		r_SAV_SCRID;
sc_signal<int>		r_SAV_BLOCKNUM;

sc_signal<addr_t>	r_INV_BLOCKADDRESS;
sc_signal<addr_t>	r_INV_TARGETADDRESS;
sc_signal<int> 		r_INV_ADDR_OFFSET;
sc_signal<bool>	    r_WAITING_TLB_INV_ACK;

unsigned int          	**r_RAM;        // segment buffers todo 
SOCLIB_DIRECTORY    	**r_DIRECTORY;  // directory: 1 per segment        
SOCLIB_DIRECTORY    	*r_TLB_DIRECTORY;  // directory: 1 per segment        
SOCLIB_DIRECTORY    	r_SAVE_TLB_DIRECTORY; // For invalidation ack purpose 

bool		       ** r_POISONNED;        // Used to tag as poisonned pages
unsigned int              r_PAGE_TO_POISON;   // todo , should be sc_signal
unsigned int              r_VIRT_POISON_PAGE; // todo , should be sc_signal
unsigned int              r_POISON_REQ;
unsigned int              r_UNPOISON_REQ;

// HOME_PAGE is the local page offset, NEW_PAGE_ADDR is the new translation in the tlb
unsigned int              r_HOME_PAGE;   // todo , should be sc_signal
unsigned int              m_PAGE_TO_INVALIDATE;   // todo , should be sc_signal
unsigned int              r_NEW_PAGE_ADDR; // todo , should be sc_signal

//  STRUCTURAL PARAMETERS

soclib::common::Loader m_loader;
soclib::common::MappingTable		m_MapTab;
soclib::common::MappingTable		m_MapTab_inv;
soclib::common::CcIdTable *		m_cct;
std::list<soclib::common::Segment>	m_SEGLIST;

unsigned int        m_NB_SEG;              // segment number
unsigned int       *m_SIZE;        // segment sizes
unsigned int       *m_BASE;        // segment bases
unsigned int        m_BLOCK_SIZE; 
unsigned int        m_ADDR_BLOCK_SHIFT; 
unsigned int        m_ADDR_WORD_SHIFT; 


unsigned int        m_I_IDENT;
unsigned int        m_NB_PROCS;


unsigned int       *m_DIR_SIZE;     // sizes of directories (number of lines)
unsigned int        m_BLOCKMASK;            // mask used to get block address 
unsigned int        m_LSBMASK;              // mask used to align a memory address

SOCLIB_DIRECTORY    m_BLOCKSTATE;
SOCLIB_DIRECTORY    m_INV_BLOCKSTATE;

// module 3 add on page table
PHYS_PAGE_TABLE         **m_PAGE_TABLE;
bool m_node_zero;  // used to identify the first memory node, it will contains the token of the ring at reset time
unsigned int		m_nbcycles;
unsigned int		m_waiting;
bool			m_last_write_nack;
unsigned int	m_last_address;

// for debug 
uint32_t d_acces_p[TOP_N];
// end for debug

unsigned int	m_segnum_5;
unsigned int	m_node_id_5;
unsigned int	m_page_sel_5;
unsigned int	m_cost_5;
bool			m_enable_5;
bool			m_dir_accessed_idle_state;

#define USE_STATS
#ifdef USE_STATS
unsigned long long * m_nb_poisonned_nack;
unsigned long long * m_nb_busy_nack;
unsigned long long * m_nb_success;
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
////////////////////////////////////////////////////
//    constructor
////////////////////////////////////////////////////


public:

VciMccRam (
		sc_module_name insname,
		bool node_zero,
		const soclib::common::IntTab &i_ident,
		const soclib::common::IntTab &t_ident,
		const soclib::common::MappingTable &mt,
		const soclib::common::MappingTable &mt_inv,
		soclib::common::CcIdTable  * cct,
		const unsigned int nb_p,
		const soclib::common::Loader &loader,
		const unsigned int line_size,
		unsigned int * table_cost,
		addr_to_homeid_entry_t * home_addr_table,
		unsigned int nb_m);


~VciMccRam();  

// CDB overloaded virtual method's
const char* GetModel();
int PrintResource(modelResource *res, char **p);
int TestResource(modelResource *res, char **p);
int Resource(char** args);

// m_5 related
void alias(const std::string seg,const std::string  register_seg);
typedef typename vci_param::addr_t vci_addr_t;
typedef typename vci_param::data_t vci_data_t;
protected:
SC_HAS_PROCESS(VciMccRam);

private:

bool on_write(size_t seg, vci_addr_t addr, vci_data_t data, int be, bool eop );
bool on_read( size_t seg, vci_addr_t addr, vci_data_t &data, bool eop );
bool is_busy(void);
bool on_tlb_miss( size_t seg, vci_addr_t addr, vci_data_t &data );
bool on_tlb_ack( size_t seg, vci_addr_t full_addr);
void directory_init();
void transition();
void genMoore();
void reload();
unsigned int rw_seg(unsigned int* tab, unsigned int index, unsigned int wdata, unsigned int be, unsigned int cmd);


// CDB utilities method's
void printramlines(struct modelResource *d);
void printramdata(struct modelResource *d);

bool	is_access_sram_possible(void);
unsigned int read_seg(unsigned int page_index,unsigned int word_page_offset);
void write_seg(unsigned int page_index,unsigned int word_page_offset, unsigned int wdata);
bool	is_access_dir_possible(void);
SOCLIB_DIRECTORY * read_dir(unsigned int page_index,unsigned int word_page_offset);
void write_dir(unsigned int page_index,unsigned int word_page_offset, SOCLIB_DIRECTORY *  wdata);
};  

}}
#endif


