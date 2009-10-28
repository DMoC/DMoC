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
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#ifndef SOCLIB_CABA_VCI_CC_CACHE_H
#define SOCLIB_CABA_VCI_CC_CACHE_H

#ifdef COHERENT_XCACHE_DEBUG
#include <string>
#include <fstream>
#endif

#include <inttypes.h>
#include <systemc>
#include <fstream>
#include "caba_base_module.h"
#include "types_sls.h" // TODO inclure ceci dans la cc-id-table, réfléchir à une solution propre
#include "generic_buffer.h"
#include "vci_initiator.h"
#include "vci_target.h"
#include "mapping_table.h"

#define USE_STATS
#ifdef USE_STATS
#include "vci_cc_cache_stats.h"
#endif

namespace soclib {
using namespace common;
using namespace std;
namespace caba {
		
	using namespace sc_core;

	template<typename    vci_param, typename iss_t>
	class VciCcCache : public soclib::caba::BaseModule
	{
		
		// DCACHE_FSM STATES : manages the data cache interface with the processor
		enum dcache_fsm_state_e{
			DCACHE_INIT,              // Reset TAG (at reset time ;) )
			DCACHE_IDLE,
			DCACHE_WUPDT,							// Update data in cache (on a write request)
			DCACHE_MISSWAIT,					// Data miss, wait for signal from request fsm
			DCACHE_MISSUPDT,					// Data miss data serviced, write cache line
			DCACHE_UNC_LL_SC_WAIT,		// LL/SC/UNC request wait for response
			DCACHE_UNC_LL_SC_GO,			// LL/SC/UNC response received, invalidate local copy to avoid cached read of an uncached data.
			DCACHE_CPUINVAL,					// CPU issues a Line invalidate command, reset TAG entry for this line
			DCACHE_RAMINVAL,					// Invalidation received on target fsm, invalidate the requested cache line
		};

		// ICACHE_FSM STATES : manages the instruction cache
		enum icache_fsm_state_e{
			ICACHE_INIT,						// Reset TAG (at reset time)
			ICACHE_IDLE,
			ICACHE_WAIT,						// Instruction Miss, wait for signal from request fsm
			ICACHE_UPDT,						// Instruction miss serviced, update instruction cache
		};

		// REQ_FSM STATES : build and send requiered requests on the interconnect (read, write misses, etc..)
		enum req_fsm_state_e{
			REQ_IDLE,					// Idle state, synchronise with RSP_FSM
			REQ_IMISS,				// Send Instruction Miss request
			REQ_IWAIT,				// Wait for Instruction Miss response (signal r_IRSP_OK set by RSP_FSM)
			REQ_DMISS,				// Send Data Miss request
			REQ_UNC	,					// Send Uncached data request
			REQ_LL	,					// Send Load-Linked data request
			REQ_SC	,					// Send Store-Conditional data request
			REQ_WRITE,				// Send Write request
			REQ_DWAIT,				// Wait for a data/ack response (DMISS/UNC/LL/SC/WRITE)

		};

		// RSP_FSM STATES : works together with REQ_FSM, manages the responses to the requests.
		enum rsp_fsm_state_e{
			RSP_IDLE,					
			RSP_IMISS,				// Process response for Instruction Miss
			RSP_DMISS,				// Process response for Data Miss
			RSP_UNC,					// Process response for an uncached request (LL/SC/UNC)
			RSP_WRITE,				// Process response for a write request
		};

		// INV_FSM STATES : manages the target interface, receive and process invalidations
		enum inv_fsm_state_e{
			INV_IDLE,
			INV_CMD,				// Process a data invalidation (set invalidation signal)
			INV_WAIT,				// Wait for invalidation to be processed by DCACHE_FSM (DCACHE_RAMINVAL)
			INV_RSP					// Send response (ack) for the invalidation
		};

		public:

		// PORTS
		sc_in<bool> 				p_clk;
		sc_in<bool> 				p_resetn;
		sc_in<bool>                             p_irq[iss_t::n_irq];
		soclib::caba::VciInitiator<vci_param>   p_i_vci; // Initator interface (send Inst./Data/TLB requests)
		soclib::caba::VciTarget<vci_param>      p_t_vci; // Target interface (receive Invalidations (DATA/TLB))


		private :
		// STRUCTURAL PARAMETERS
		soclib::common::AddressDecodingTable<uint32_t, bool> m_cacheability_table; // each memory segment is taged as cacheable/uncacheable
		iss_t m_iss;	// processor instruction set simulator
		soclib::common::Segment m_segment;	// memory segment allocated for the vci target interface
		unsigned int m_i_ident; // initiator id
		unsigned int m_t_ident; // target id 
		static const char * m_model;  

		// Size configuration of the cache
		const size_t  s_dcache_lines;
		const size_t  s_dcache_words;
		const size_t  s_icache_lines;
		const size_t  s_icache_words;

		const size_t  s_write_buffer_size;

		// Mask's and Shifters to address line, word, tag
		const size_t  m_plen_shift;

#if 0 // TODO : fix this, correct way is to use "addr_t" types but it falls at link time
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_i_x;
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_i_y;
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_i_z;
#else
		const soclib::common::AddressMaskingTable<uint32_t> m_i_x;
		const soclib::common::AddressMaskingTable<uint32_t> m_i_y;
		const soclib::common::AddressMaskingTable<uint32_t> m_i_z;
#endif
		const size_t  m_icache_yzmask;

#if 0
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_d_x;
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_d_y;
		const soclib::common::AddressMaskingTable<typename vci_param::addr_t> m_d_z;
#else
		const soclib::common::AddressMaskingTable<uint32_t> m_d_x;
		const soclib::common::AddressMaskingTable<uint32_t> m_d_y;
		const soclib::common::AddressMaskingTable<uint32_t> m_d_z;
#endif

		const size_t  m_dcache_yzmask;
		const size_t  m_dcache_zmask;

		const typename vci_param::be_t m_full_be;


		// Structures : data/inst. cache, data/inst. tag, TLB.
		sc_signal<typename vci_param::data_t>	**s_DCACHE_DATA;
		sc_signal<typename vci_param::addr_t>	*s_DCACHE_TAG;
		sc_signal<typename vci_param::data_t>	**s_ICACHE_DATA;
		sc_signal<typename vci_param::addr_t>	*s_ICACHE_TAG;


		// REGISTERS
		sc_signal<unsigned int>		r_DCACHE_FSM;					// DCACHE controler state
		sc_signal<unsigned int>		r_PREV_DCACHE_FSM;		// Used to return from the DCACHE_RAMINVAL state	
		sc_signal<unsigned int>		r_ICACHE_FSM;					// ICACHE controler state
		sc_signal<unsigned int> 	r_VCI_REQ_FSM;				// VCI_REQ controler state
		sc_signal<unsigned int> 	r_VCI_RSP_FSM;				// VCI_RSP controler state 
		sc_signal<unsigned int>		r_VCI_INV_FSM;				// VCI_INV controler state

		// processor data request save
		sc_signal<typename vci_param::addr_t>	r_DCACHE_ADDR_SAVE;
		sc_signal<typename vci_param::data_t>	r_DCACHE_DATA_SAVE;
		sc_signal<typename vci_param::be_t>		r_DCACHE_BE_SAVE;
		sc_signal<bool>		r_DCACHE_UNCACHED_SAVE;
		sc_signal<bool>		r_DCACHE_LL_SAVE;
		sc_signal<bool>		r_DCACHE_SC_SAVE;
		// processor instruction save request
		sc_signal<typename vci_param::addr_t>	r_ICACHE_MISS_ADDR_SAVE;	// Processor missed address in instruction cache

		// save related to data write commands
		sc_signal<typename vci_param::addr_t>	r_DCACHE_ENQUEUED_ADDR_SAVE;   // last address enqueued in write-buffer (build burst request)
		sc_signal<typename vci_param::data_t>	r_DCACHE_WUPDT_DATA_SAVE; // Data read, modified and written back to cache (DCACHE_WUPDT)

		sc_signal<typename vci_param::be_t>		r_DCACHE_WF_BE_SAVE; // BE of first word (cell) of a write burst packet 
		sc_signal<typename vci_param::be_t>		r_DCACHE_WL_BE_SAVE; // BE of last (currently processed) cell of a write burst packet.

		// control signals for DCACHE_FSM
		sc_signal<bool>		r_DCACHE_PENDING_WRITE; // There is a waiting  write request in the _SAVE register
		sc_signal<bool>		r_DCACHE_FIRST_CELL;		// Control signal to tag a "first entry in write-buffer"

		// control signals from DCACHE_FSM to REQ_FSM
		sc_signal<bool>		r_DCACHE_REQ;				 		// There is a data cache request wich requieres a memory request
		sc_signal<bool>		r_DCACHE_FLUSH_WB_REQ;  // There is a write-buffer flush request 
		sc_signal<size_t>	r_WRITE_BURST_SIZE;     // Number of elements in the write-buffer, used to control size of burst paquet

		// control signals from ICACHE_FSM to REQ_FSM
		sc_signal<bool>  	r_ICACHE_MISS_REQ; 			// There is a inst. cache resquest wich requieres a memory request (ie. miss)

		// control signals from REQ_FSM to DCACHE_FSM (used also in REQ_FSM)
		sc_signal<bool> 	r_DRSP_OK;		// Data response packet received (unlock waiting state like DCACHE_MISSWAIT)
		sc_signal<bool> 	r_IRSP_OK;		// Instruction Miss response packet received
		sc_signal<bool> 	r_DCACHE_RAM_INVAL_REQ;	// Synchronize fsm's on invalidations
		sc_signal<bool>		r_DRETRY_REQ;		// flag set to 1 if the previous request doesnt ensures a correct TLB (for data)
		sc_signal<bool>		r_IRETRY_REQ;		// flag set to 1 if the previous request doesnt ensures a correct TLB (for instructions)
		sc_signal<bool>		r_RETRY_ACK_REQ;	// flag set to 1 if the previous request doesnt ensures a correct TLB (for instructions)
																				// transalation (ie, TLB invalidation has been received during the

		// Write-Buffer
		GenericBuffer<typename vci_param::data_t>	m_WRITE_BUFFER_DATA; // Write-buffer DATA
		GenericBuffer<typename vci_param::addr_t>	m_WRITE_BUFFER_ADDR; // Write-buffer Address (avoid using a "+" to increment address in REQ_FSM)
		sc_signal<typename vci_param::data_t>	r_DATA_FIFO_INPUT;	// Write-buffer next input (data)
		sc_signal<typename vci_param::addr_t>	r_ADDR_FIFO_INPUT;	// Write-buffer next input (addr)


		// REQ_FSM internal registers
		sc_signal<typename vci_param::addr_t>	r_REQ_ICACHE_ADDR_PHYS; // miss address translated through the TLB
		sc_signal<typename vci_param::data_t> r_REQ_DCACHE_DATA;			 // data for next VCI request
		sc_signal<typename vci_param::addr_t>	r_REQ_DCACHE_ADDR_PHYS;	// address for VCI request 
		sc_signal<size_t>		r_REQ_CPT;		// counter for VCI request packet

		// RSP_FSM internal registers, (mainly responses for DCACHE/ICACHE_FSM)
		sc_signal<typename vci_param::data_t> 	r_RSP_DCACHE_UNC_BUF;	
		sc_signal<typename vci_param::data_t> 	*r_RSP_ICACHE_MISS_BUF;	
		sc_signal<typename vci_param::data_t> 	*r_RSP_DCACHE_MISS_BUF;	
		sc_signal<size_t> 	r_RSP_CPT;		// counter for VCI response packet

		// INV_FSM internal registers
		sc_signal<typename vci_param::srcid_t>	r_SAV_SCRID;
		sc_signal<typename vci_param::pktid_t>	r_SAV_PKTID;
		sc_signal<typename vci_param::trdid_t>	r_SAV_TRDID;

		// Signals from target FSM 
		
		// Regarding cache invalidation
		sc_signal<typename vci_param::addr_t>	r_RAM_INV_ADDR;		// address of bloc that must be invalidated

		// Misc
		sc_signal<size_t> 	r_DCACHE_CPT_INIT;	// Counter for DCACHE initialisation
		sc_signal<size_t> 	r_ICACHE_CPT_INIT;	// Counter for ICACHE initialisation

		private :
		// structural parameters
		unsigned int m_id;
		uint64_t ncycles;
		unsigned int * m_table_cost;
		addr_to_homeid_entry_t * m_home_addr_table;
		unsigned int m_nb_memory_nodes;

		// for debug
#ifdef USE_STATS
		cache_info_t stats;
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
#ifdef COHERENT_XCACHE_DEBUG
		std::ofstream file_chrono;
		std::string chemin; 	// set it to : ./insname.debug
#endif



		protected:

		SC_HAS_PROCESS(VciCcCache);

		public:


		VciCcCache(
				sc_module_name insname,
				const soclib::common::MappingTable &mt,
				const soclib::common::MappingTable &mt_inv,
				const soclib::common::IntTab &i_index,
				const soclib::common::IntTab &t_index,
				size_t icache_lines,
				size_t icache_words,
				size_t dcache_lines,
				size_t dcache_words, 
				unsigned int procid,	
				unsigned int * table_cost,
				addr_to_homeid_entry_t * home_addr_table,
				unsigned int nb_memory_nodes
				);

		//struct XCacheInfo getCacheInfo() const; Unimplemented yet

		~VciCcCache();

#ifdef CDB_COMPONENT_IF_H
		//     CDB_user_if member overload
		const char* GetModel();
		int PrintResource(modelResource *res, char **p);
		int TestResource(modelResource *res, char **p);
		int Resource(char** args);
#endif

		struct XCacheInfo getCacheInfo() const;

		private:
#ifdef USE_STATS
		void gen_output_stats( void );
#endif

		int address_to_id(typename vci_param::addr_t addr);
		typename vci_param::data_t be_to_mask(typename iss_t::be_t be);
		void transition();
		void genMoore();

#ifdef CDB_COMPONENT_IF_H
		//  CDB related private utilities
		void printdcachelines(struct modelResource *d);
		void printdcachedata(struct modelResource *d);
		void printicachelines(struct modelResource *d);
		void printicachedata(struct modelResource *d);
#endif

	}; 
}}
#endif
