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
 * Copyright (c) TIMA
 *         Perre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr> 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#ifndef MCC_RAM_CORE_H
#define MCC_RAM_CORE_H
#define NOCTRL

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include <fstream>

#include "caba_base_module.h"
#include "vci_target_fsm_nlock_tlb.h" // Special target_fsm that supports extended requests (TLB MISS's)
#include "vci_initiator.h"
#include "types_sls.h"

#include "generic_fifo.h" 
#include "mapping_table.h"
#include "cc_id_table.h"
#include "phys_page_table.h"
#include "soclib_directory.h" 
#include <string>

#include "mig_control.h"

#if 0
#define DEBUG_SRAM
#endif
#ifdef DEBUG_SRAM
#include "loader.h"
#endif

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
	template <typename vci_param, typename sram_param>
		class MccRamCore : BaseModule {

			private :
				enum ram_fsm_state_e{   // RAM_FSM states
					// Standard requests : 
					RAM_IDLE,
					RAM_DIRUPD,					// On read, update directory
					RAM_DATA_INVAL,			// On write, send invalidations
					RAM_DATA_INVAL_WAIT,// On write, wait for invalidation ack's

					// Migration related requests :
					RAM_POISON,								// Poison a page
					RAM_UNPOISON,							// Unpoison a page

					RAM_PAGE_TABLE_DIR_SAVE,  // a) Save current page translation 
					RAM_PAGE_TABLE_DIR_UPDT,  // b) Update to new page translation
					RAM_TLB_INVAL,						// c) Send invalidations of current page translation 
					RAM_TLB_INV_OK,						// d) Received an ACK request for and invalidation, check if all ack's were received

					RAM_WIAM_UPDT,						// x) Update translated page adress (Who-I-am)
				};

				enum inv_fsm_state_e{		// INV_FSM states
					RAM_INV_IDLE,         // a) Wait for invalidation request (DATA or TLB) 
					RAM_INV_REQ,					// b) Send an invalidation according to presence bit-vector in directory
					RAM_INV_RSP						// c) Wait for response (Ack), and go to INV_REQ if presence bit-vector is not empty.
				};

				// IO PORTS
			public :
				sc_in<bool>                     	p_clk;
				sc_in<bool>                     	p_resetn;
				soclib::caba::VciTarget<vci_param>			p_t_vci;  // Target interface
				soclib::caba::VciInitiator<vci_param>  	p_i_vci;  // Initiator interface (used to send invalidations)

				// S-RAM interface
				sc_out<typename sram_param::bk_t>   p_sram_bk;
				sc_out<bool>   p_sram_ce;
				sc_out<bool>   p_sram_oe;
				sc_out<bool>   p_sram_we;
				sc_out<typename sram_param::be_t>    p_sram_be;
				sc_out<typename sram_param::addr_t>  p_sram_addr;
				sc_out<typename sram_param::data_t>  p_sram_dout; // wdata
				sc_in<typename sram_param::data_t>   p_sram_din;  // rdata
				sc_in<bool>   p_sram_ack;  // rdata

				// Manometer interface
				sc_out<bool>   p_manometer_req;

				// Counters interface
				sc_out< bool >			p_counters_enable;
				sc_out< sc_uint<32> >	p_counters_page_sel;
				sc_out< sc_uint<32> >	p_counters_node_id; // TODO width can be reduced
				sc_out< sc_uint<32> >	p_counters_cost;    // TODO set types correctly

				// Migration related ports
#ifndef NOCTRL
#if 0
				sc_in<bool>   	  p_in_ctrl_req;
				sc_in<uint32_t>   p_in_ctrl_cmd;
				sc_in<uint32_t>   p_in_ctrl_data_0;
				sc_in<uint32_t>   p_in_ctrl_data_1;
				sc_in<bool>       p_in_ctrl_rsp_ack; 

				sc_out<bool>  	  p_out_ctrl_req_ack; 
				sc_out<uint32_t>  p_out_ctrl_cmd;
				sc_out<bool>  	  p_out_ctrl_rsp; 
				sc_out<uint32_t>  p_out_ctrl_data_0; 
#endif
#endif


			public :
				//  REGISTERS

				// 2 FSM's
				sc_signal<unsigned int>		r_RAM_FSM;
				sc_signal<unsigned int>		r_INV_FSM;

				// Control signals
				sc_signal<bool> 		r_IN_TRANSACTION;
				sc_signal<bool>			r_PROCESSING_TLB_INV;

				// Directory entry copy for invalidation and control purpose
				SOCLIB_DIRECTORY    	r_SAVE_PT_DIRECTORY; // On a TLB_ACK request, check if all were received 
				SOCLIB_DIRECTORY			r_INV_BLOCKSTATE;    // Used to send invalidations to all the owner of a copy (DATA or TLB).

				// Vci save request
				sc_signal<typename vci_param::addr_t>		r_SAV_ADDRESS;
				sc_signal<unsigned int>									r_SAV_SEGNUM;
				sc_signal<typename vci_param::srcid_t>	r_SAV_SCRID; 
				sc_signal<unsigned int>									r_SAV_BLOCKNUM;
				// Vci save request additional information to access S-RAM	
				typename vci_param::addr_t		m_sram_addr;
				typename vci_param::data_t		m_sram_wdata;
				typename vci_param::be_t			m_sram_be;
				bool													m_sram_oe;
				bool													m_sram_we;
				bool													m_sram_ce;
				typename sram_param::bk_t			m_sram_bk;

				uint32_t m_counters_page_sel;
				uint32_t m_counters_cost; 
				bool     m_counters_enable;	
				uint32_t m_counters_node_id;
			

				// Vci invalidation request
				sc_signal<typename vci_param::addr_t>	r_INV_BLOCKADDRESS;
				sc_signal<typename vci_param::addr_t>	r_INV_TARGETADDRESS;
				sc_signal<typename vci_param::addr_t>	r_INV_ADDR_OFFSET;

				// Data , page table , directories (data, page table, poison flag) implemented in S-RAM
#ifdef DEBUG_SRAM
				typename  vci_param::data_t	* s_RAM; // this doesnt work on a 64bit machine due to "m_loader.load"
									 // wich merely cast on void * the s_RAM and memcpy the data (32bits elf
									 // format in a mips32 platform)
#endif
				PHYS_PAGE_TABLE							* s_PAGE_TABLE;
				SOCLIB_DIRECTORY						* s_DIRECTORY;	
				SOCLIB_DIRECTORY						* s_PT_DIRECTORY; 
				sc_signal<bool>							* s_POISONNED; 


				// Control module, save request 
				sc_signal<uint32_t>   r_TARGET_PAGE_ADDR; // PT update, poison, unpoison
				sc_signal<uint32_t>   r_NEW_PAGE_ADDR;    // PT update
				sc_signal<uint32_t>   r_VIRT_POISON_PAGE; // current virtual address of a poisoned page (sent as a response).

				//  STRUCTURAL PARAMETERS
			private :

#ifdef DEBUG_SRAM
				typename vci_param::data_t m_should_get; // pointer to the data that will sent in response by the target fsm on a read,
#endif
				typename vci_param::data_t * m_fsm_data; // pointer to the data that will sent in response by the target fsm on a read,
																								 // this pointer is saved because the data is provided in the mealy function.

				soclib::caba::VciTargetFsmNlockTlb<vci_param,true,true> m_vci_fsm;

#ifdef DEBUG_SRAM
				soclib::common::Loader					m_loader;
#endif
				soclib::common::MappingTable		m_MapTab;			// Request/response network
				soclib::common::MappingTable		m_MapTab_inv; // invalidation network
				soclib::common::CcIdTable *			m_cct;				// used for invalidations, used to convert source_id to Target address 
				soclib::common::Segment					m_segment;    // Only 1 segment on this version (page migration allowed).

				unsigned int	m_PAGE_SHIFT;				// page_index = address >> m_PAGET_SHIFT
				unsigned int	m_ADDR_WORD_SHIFT;	// data_index = address >> m_ADDR_WORD_SHIFT (index of data in r_RAM) 
				unsigned int	m_ADDR_BLOCK_SHIFT; // directory_index = address >> m_ADDR_BLOCK_SHIFT 
				unsigned int	m_BLOCKMASK;				// mask used to convert data_address into block_address (ie. set to 0 lsb).

				unsigned int	m_I_IDENT;		// Source_id of this component
				unsigned int	m_NB_PROCS;  // Number of processors, coherence is only mantained for processors (ie. not for fd_access, etc).



				bool						m_node_zero;				// used to identify the first memory node, it will contains the token of the ring at reset time
				uint64_t				m_nbcycles;
				bool						m_last_write_nack;  // used to detect on read/write modification of a page
																						// status (poison/unpoison) meanwhile processing a packet. Only used for checking, can be removed

			public :
				MccRamCore (
						sc_module_name insname,
						bool node_zero,
						const soclib::common::IntTab &i_ident,			// Source_id
						const soclib::common::IntTab &t_ident,			// Target_id
						const soclib::common::MappingTable * mt,			// Mapping Table for read/write requets
						const soclib::common::MappingTable * mt_inv, // Mapping Table for invalidation requests (alternative NoC). 
						soclib::common::CcIdTable  * cct,						// CacheCoherence Id Table to convert Source_id <-> Invalidation Target_address
						const unsigned int nb_p,										// Number of processors in the system -> size of directory entry
#ifdef DEBUG_SRAM
						const soclib::common::Loader &loader,				// Code loader
#endif
						const unsigned int line_size,								// Configured line size. TODO : should/can be changed by vci_param::B ?
						unsigned int * table_cost,									// Used to compute node distance to processors (nb. hops)
						addr_to_homeid_entry_t * home_addr_table,		// Used to convert node_id <-> memory_segment_base_address
						unsigned int nb_m);													// Number of memory nodes (used in migration)

				~MccRamCore();  

			protected:

				SC_HAS_PROCESS(MccRamCore);

			private:
				bool check_virtual_access(typename vci_param::addr_t addr, unsigned int node_id);
				bool check_page_access(typename vci_param::addr_t addr);

				bool on_write(size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t data, int be, bool eop );
				bool on_read( size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t &data, bool eop );
				bool is_busy(void);
				bool on_tlb_miss( size_t seg, typename vci_param::addr_t addr, typename vci_param::data_t &data );
				bool on_tlb_ack( size_t seg, typename vci_param::addr_t full_addr);

				void transition();
				void genMoore();
#ifndef DEBUG_SRAM
				void genMealy(); // no mealy when using the embeded s-ram for debug
#endif

				// TODO : obsolete methods
				unsigned int rw_seg(
						typename vci_param::data_t* tab, unsigned int index,
						typename vci_param::data_t wdata, typename vci_param::be_t be,
						typename vci_param::cmd_t cmd
						);
#ifdef DEBUG_SRAM
				void reload();
#endif
				unsigned int read_seg(unsigned int page_index,unsigned int word_page_offset);
				void write_seg(unsigned int page_index,unsigned int word_page_offset, unsigned int wdata);

				bool	is_access_dir_possible(void);
				bool	is_access_sram_possible(void);

				SOCLIB_DIRECTORY * read_dir(unsigned int page_index,unsigned int word_page_offset);
				void write_dir(unsigned int page_index,unsigned int word_page_offset, SOCLIB_DIRECTORY *  wdata);
				// end TODO

				// TODO : types temporaires a mettre dans le bon composant.
				typedef enum {CTRL_VIRT_PP, CTRL_INV_OK } ctrl_out_cmd_t;
		};  
}}
#endif


