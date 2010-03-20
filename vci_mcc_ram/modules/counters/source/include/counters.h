/* -*- c++ -*-
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
 * License ashort with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) TIMA	
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>
 *
 */
#ifndef COUNTERS_H
#define COUNTERS_H

#include <stdint.h>
#include <string>
#include <fstream>
#include <systemc>
#include "mcc_globals.h"
#include "caba_base_module.h"
#define DEBUG_COUNTERS
#ifdef DEBUG_COUNTERS
	#define D_COUNTERS_COUT cout
#else
	#define D_COUNTERS_COUT if(0) cout
#endif
namespace soclib {
namespace caba {

using namespace std;
using namespace sc_core;

typedef uint16_t counter_t; 
typedef uint16_t counter_id_t;

class Counters : public soclib::caba::BaseModule {

	public :
		// some public types (or commands)
		typedef sc_uint < 4 > cter_cmd_t; // up to 16 commands
		enum cter_enum_cmd_e {
			R_MAX_GLOBAL_COUNTER, // read a global counter [ page_idx ]
			R_MAX_COUNTER,		  // read counter [ page_idx ] [ node_idx ]
			R_MAX_ROUNDED_COUNTER,// read to rounded pow 2 counter [ page_idx ] [ node_idx ]
			R_MAX_ID_PAGE,	  // read idx of max page
			W_RESET_PAGE_CTER,  // reset counters [ page_idx ] [ all ]
			ELECT,
			NOP
		};

		// Clock & Reset
		sc_in< bool > p_clk;
		sc_in< bool > p_resetn;

		// Core interface
		sc_in< bool >		 p_enable;
		sc_in< sc_uint<32> > p_page_sel;
		sc_in< sc_uint<32> > p_node_id; // TODO width can be reduced
		sc_in< sc_uint<32> > p_cost;    // TODO set types correctly

		// TODOXXX implement cost, page_sel etc in core (or in s_ram ?), maybe
		// has nothing to do with core!
		// test if every thing is working fine

		// Control interface
		sc_in< bool >		p_ctrl_req;	
		sc_in< cter_cmd_t >	p_ctrl_cmd;	
		sc_in< sc_uint < 32 > > p_ctrl_page_id;	
		sc_in< sc_uint<32> > p_ctrl_node_id; // TODO width can be reduced

		sc_out< bool >		p_ctrl_ack;	
		sc_out< bool >		p_ctrl_valid;	
		sc_out< sc_uint < 32 > >	p_ctrl_output;	
		

		// ? interface
		sc_out< bool > p_contention;

	private :
		enum cter_fsm_state_e
		{
			CTER_IDLE,
			CTER_SAVE,		// Save counters value when threshold is reached
			CTER_COMPUTING, // Compute counter incremet
			CTER_SEND_RSP,  // Send response to ctrl
			CTER_WRITE, 	// Unused
			CTER_WAIT
		};

	public :

		Counters(sc_module_name insname,
					unsigned int nb_nodes,
					unsigned int nb_pages,
					unsigned int base_addr,
					unsigned int page_size);
		~Counters();

	protected :

		SC_HAS_PROCESS(Counters);

	private : 
		// Registers
		
		sc_signal < counter_t >  ** r_counters; // One per node per page
		sc_signal < counter_t >  * r_global_counters; // One per page
		sc_signal < counter_t >  * d_global_counters; // Debug/stats only, One per page
		sc_signal < unsigned int > r_max_page; // index of most accessed page
		sc_signal < unsigned int > r_index_p;  // 

		// saved valued on threashold :
		sc_signal < unsigned int > r_out_max_p; 
		sc_signal < counter_t >    r_out_global_counter;
		sc_signal < counter_t >  * r_out_counter; 

		// returned value on a read :
		sc_signal < sc_uint < 32 > > r_out_value;  // 

		sc_signal < bool >	r_raise_threshold;  // TODO
		bool				m_raise_threshold;

		// Save a request :
		
		sc_uint<32>  r_save_page_sel;
		sc_uint<32>  r_save_node_id; // TODO width can be reduced
		sc_uint<32>  r_save_cost;

		sc_signal< uint32_t >	r_CTER_FSM;
		
		// Structural parameters
		unsigned int m_BASE_ADDR;
		unsigned int m_PAGE_SHIFT;
		unsigned int m_NB_PAGES;
		unsigned int m_NB_NODES;
		unsigned int m_COMPUTE_TIME;
		unsigned int m_cycles_simu;

		// On a basis of 128 nodes, 0x8000 pages, 16bits per counter, 32 bits per id
		//   [22 0x8000 pages 8] [7 7bits for 128 nodes 1] [0 1bit sel 16bits  0]
		unsigned int m_ADDR_NCOUNT_PSEL_SHIFT; // 7 + 1  = 8 bits 
		unsigned int m_ADDR_NCOUNT_PSEL_MASK;  // 0xFFF << 8 = 0xfff00
		unsigned int m_ADDR_NCOUNT_NSEL_SHIFT; // 1 bit 
		unsigned int m_ADDR_NCOUNT_NSEL_MASK;  // 0x7F << 1  = 0xFE

		//   [15 0x8000 pages 1] [0 1bit sel 16bits  0]
		unsigned int m_ADDR_GCOUNT_PSEL_SHIFT; // 1 bit
		unsigned int m_ADDR_GCOUNT_PSEL_MASK;  // 0xFFF << 1 0x1FFE


		//   [16 0x8000 pages 2]  [1 2bit id 32bits 0]
		unsigned int m_ADDR_MAXIP_PSEL_SHIFT; // 2bits
		unsigned int m_ADDR_MAXIP_PSEL_MASK;  // 0xFFF << 2 = 0x3FFC

		//    [1 0bit 1] [1 2bit id 32bits 0]
		unsigned int m_ADDR_MAXP_SHIFT; // 2 bits
		unsigned int m_ADDR_MAXP_MASK;  // 0x1 << 2 = 0x4 


		// Methods
		void transition();
		void genMoore();

		// Utils 
		void reset_counters();
		unsigned int reset_reg(unsigned int addr);
		unsigned int read_reg(unsigned int addr);


		counter_t read_global_counters(unsigned int page_index);
		counter_t read_counter(unsigned int page_index, unsigned int node_index);
		counter_t read_rounded_counter(unsigned int page_index, unsigned int node_index);
		counter_id_t read_max_id_perpage(unsigned int page_index);
		counter_id_t read_max_page();
		counter_id_t read_top_id_perpage(unsigned int page_index,unsigned int top_index);
		counter_id_t read_top_pages(unsigned int top_index);
	
		counter_id_t evict_sel();


		typedef enum{S,G,L} magnitud_order;

		magnitud_order order(unsigned int old_v, unsigned int new_v,unsigned int bit_order);

		// For check
		bool is_pow2(unsigned int value);

		// For debug only, stats & co
		counter_t partitionner(counter_t tableau[], counter_t indices[], counter_t p, counter_t r);
		void quickSort(counter_t tableau[], counter_t indices[], counter_t p, counter_t r);

#ifdef USE_STATS
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
};

}}
#endif
