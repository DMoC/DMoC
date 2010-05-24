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
#ifndef MODULE_9_H
#define MODULE_9_H

#include <stdint.h>
#include <string>
#include <fstream>
#include "globals.h"

#ifdef DEBUG_M9
	#define D9COUT cout
#else
	#define D9COUT if(0) cout
#endif
#define USE_STATS
namespace soclib {
namespace caba {

using namespace std;

typedef enum{S,G,L} magnitud_order;


class Module_9 {

	public :
		typedef struct{
			// from module 5 
			unsigned int  * node_id;
			unsigned int  * page_sel;
			unsigned int  * cost;
			bool          * enable;
			// from module 10
			bool          * freeze;
		}m9_inputs_t;

		typedef struct{
			// to module 10
			bool * is_busy;
			bool * is_threshold;
		}m9_outputs_t;


		m9_inputs_t  p_in;
		m9_outputs_t p_out;

		Module_9(const string & insname,unsigned int nb_nodes,unsigned int nb_pages,unsigned int base_addr,unsigned int page_size);
		~Module_9();

		bool busy();
		void compute();
		void gen_outputs();
		const string & name();
		void reset();
		void reset_counters();

		// For software use ie: 32bits alligned address received on the vci interface
		unsigned int reset_reg(unsigned int addr);
		unsigned int read_reg(unsigned int addr);

		// For hardware use ie: direct connect on the wires, access by index
		// Warning, accessing one of this functions can be done only once at a cycle!
		// remember that they implies a lot of wire for page_index and a command to select
		// the action (read, reset etc).
		counter_t read_global_counters(unsigned int page_index);
		counter_t read_counter(unsigned int page_index, unsigned int node_index);
		counter_t read_rounded_counter(unsigned int page_index, unsigned int node_index);
		counter_id_t read_max_id_perpage(unsigned int page_index);
		counter_id_t read_max_page();
		counter_id_t read_top_id_perpage(unsigned int page_index,unsigned int top_index);
		counter_id_t read_top_pages(unsigned int top_index);
	
		counter_id_t evict_sel();

		void set_not_migrable(unsigned int page_index);
		void set_migrable(unsigned int page_index);

	private :
		magnitud_order order(unsigned int old_v, unsigned int new_v,unsigned int bit_order);
		counter_t partitionner(counter_t tableau[], counter_t indices[], counter_t p, counter_t r);
		void quickSort(counter_t tableau[], counter_t indices[], counter_t p, counter_t r);
		bool is_pow2(unsigned int value);

		string       m_name;
		unsigned int m_BASE_ADDR;
		unsigned int m_PAGE_SHIFT;
		unsigned int m_NB_PAGES;
		unsigned int m_NB_NODES;
		unsigned int m_COMPUTE_TIME;
		unsigned int m_cycles_simu;
		// On a basis of 128 nodes, 0x8000 pages, 4 in top (n/p), 16bits per counter, 32 bits per id
		//   [22 0x8000 pages 8] [7 7bits for 128 nodes 1] [0 1bit sel 16bits  0]
		unsigned int m_ADDR_NCOUNT_PSEL_SHIFT; // 7 + 1  = 8 bits 
		unsigned int m_ADDR_NCOUNT_PSEL_MASK;  // 0xFFF << 8 = 0xfff00
		unsigned int m_ADDR_NCOUNT_NSEL_SHIFT; // 1 bit 
		unsigned int m_ADDR_NCOUNT_NSEL_MASK;  // 0x7F << 1  = 0xFE

		//   [15 0x8000 pages 1] [0 1bit sel 16bits  0]
		unsigned int m_ADDR_GCOUNT_PSEL_SHIFT; // 1 bit
		unsigned int m_ADDR_GCOUNT_PSEL_MASK;  // 0xFFF << 1 0x1FFE

		//   [19 0x8000 pages 5] [4 2bits (TOP_N = 3) 2] [1 2bit id 32bits 0]
		unsigned int m_ADDR_TIDP_PSEL_SHIFT;  //  2 + 2 = 4btis
		unsigned int m_ADDR_TIDP_PSEL_MASK;   //  0xFFF << 4 = 0xFFF0
		unsigned int m_ADDR_TIDP_NSEL_SHIFT;  //  2 = 2btis
		unsigned int m_ADDR_TIDP_NSEL_MASK;   //  0x3 << 2 = 0xC

		//   [16 0x8000 pages 2]  [1 2bit id 32bits 0]
		unsigned int m_ADDR_MAXIP_PSEL_SHIFT; // 2bits
		unsigned int m_ADDR_MAXIP_PSEL_MASK;  // 0xFFF << 2 = 0x3FFC

		//   [4 2bits (TOP_P) 2]  [1 2bit id 32bits 0]
		unsigned int m_ADDR_TP_SHIFT; // 2 bits
		unsigned int m_ADDR_TP_MASK;  // 0x3 << 2 = 0xC

		//    [1 0bit 1] [1 2bit id 32bits 0]
		unsigned int m_ADDR_MAXP_SHIFT; // 2 bits
		unsigned int m_ADDR_MAXP_MASK;  // 0x1 << 2 = 0x4 
		counter_id_t m_last_evicted;

		counter_t  ** r_counters;
		counter_t  * r_global_counters;
		counter_t  * d_global_counters;
		bool  * r_migrable; 
		unsigned int * r_max_id_perpage;
		unsigned int r_max_page;
		int ** r_top3_id_perpage;
		int * r_top3_pages; // indice
		unsigned int r_index_p;
		unsigned int r_index_id;
		bool r_raise_threshold; 

#ifdef USE_STATS
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
};

}}
#endif
