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

#ifndef VCI_MCC_RAM_H
#define VCI_MCC_RAM_H
#define NOCTRL

#include <inttypes.h>
#include <systemc>
#include <cassert>
#include <fstream>

#include "caba_base_module.h"
#include "types_sls.h"

#include "mcc_ram_core.h"
#include "sram.h"
#include "sram_param.h"
#include "manometer.h"
#include "counters.h"

#include "mapping_table.h"
#include "loader.h"
#include "cc_id_table.h"
#include "loader.h"
#include <string>

namespace soclib{
namespace caba{
	using namespace sc_core;

	////////////////////////////////////////        
	//    structure definition
	////////////////////////////////////////
	template <typename vci_param>
		class VciMccRam : BaseModule {

			// this class is a wrapper or shell that contains several 
			// interconnected modules : 
			// c_core : the memory controller
			// c_sram : the S-Ram memory bank 

			private :
				typedef SramParam<4,32> sram_param ;
#ifndef DEBUG_SRAM
				soclib::caba::SRam<sram_param> * c_sram_32; // 32bits Sram
#endif
				soclib::caba::MccRamCore<vci_param,sram_param> * c_core;
				soclib::caba::Manometer * c_manometer; // a manometer to detect contention
				soclib::caba::Counters * c_counters;  // counters to compute page access

				soclib::common::Loader					m_loader;
				soclib::common::MappingTable		m_MapTab;			
				std::list<soclib::common::Segment>					* m_segment_list;    // A segment list 

				// Interconnection signals : c_core -> c_sram 
				sc_signal<typename sram_param::bk_t>   s_bk_core2sram;
				sc_signal<bool>   s_ce_core2sram;
				sc_signal<bool>   s_oe_core2sram;
				sc_signal<bool>   s_we_core2sram;
				sc_signal<typename sram_param::be_t>   s_be_core2sram;
				sc_signal<typename sram_param::addr_t>   s_addr_core2sram;
				sc_signal<typename sram_param::data_t>   s_din_core2sram;
				sc_signal<typename sram_param::data_t>   s_dout_core2sram;
				sc_signal<bool>   s_ack_core2sram;

				// Interconnection signals : c_core -> c_manometer
				sc_signal<bool>	s_core_req_core2manometer;
				
				sc_signal<bool>	s_contention_ack_x2manometer;
				sc_signal< Manometer::mnter_cmd_t >	s_cmd_x2manometer;
				sc_signal<bool>	s_req_x2manometer;

				sc_signal<bool>	s_valid_manometer2x;
				sc_signal<bool>	s_contention_manometer2x;
				sc_signal<bool>	s_ack_manometer2x;
				sc_signal< Manometer::mnter_pressure_t>	s_pressure_manometer2x;

				// Interconnection signals : c_core -> c_counters
				sc_signal< bool >			s_enable_core2counters;
				sc_signal< sc_uint<32> >	s_page_sel_core2counters;
				sc_signal< sc_uint<32> >	s_node_id_core2counters;
				sc_signal< sc_uint<32> >	s_cost_core2counters;

				sc_signal< bool >	s_contention_counters2ctrl;

				
			


				// IO PORTS
			public :
				sc_in<bool>                     	p_clk;
				sc_in<bool>                     	p_resetn;
				soclib::caba::VciTarget<vci_param>			p_t_vci;  // Target interface
				soclib::caba::VciInitiator<vci_param>  	p_i_vci;  // Initiator interface (used to send invalidations)

				VciMccRam (
						sc_module_name insname,
						bool node_zero,
						const soclib::common::IntTab &i_ident,			// Source_id
						const soclib::common::IntTab &t_ident,			// Target_id
						soclib::common::CcIdTable  * cct,						// CacheCoherence Id Table to convert Source_id <-> Invalidation Target_address
						const unsigned int nb_p,										// Number of processors in the system -> size of directory entry
						const soclib::common::Loader &loader,				// Code loader
						const unsigned int line_size,								// Configured line size. TODO : should/can be changed by vci_param::B ?
						unsigned int * table_cost,									// Used to compute node distance to processors (nb. hops)
						addr_to_homeid_entry_t * home_addr_table,		// Used to convert node_id <-> memory_segment_base_address
						unsigned int nb_m,													// Number of memory nodes (used in migration)
						const soclib::common::MappingTable * mt,			// Mapping Table for read/write requets
						const soclib::common::MappingTable * mt_inv = NULL // Mapping Table for invalidation requests (alternative NoC). 
						);

				~VciMccRam();  

			protected :

				SC_HAS_PROCESS(VciMccRam);
			private : 
					void Transition();

		};  
}}
#endif


