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
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * SOCLIB_LGPL_HEADER_END
 *
 * Copyright (c) TIMA	
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>
 *
 */
#ifndef MODULE_5_H
#define MODULE_5_H

#ifdef DEBUG_M5
	#define D5COUT cout
#else
	#define D5COUT if(0) cout
#endif
#include <stdint.h>

#include "module_8.h"
#include "module_9.h"
#include "module_10.h"
#include "module_6.h"
#include "mapping_table.h"
#include <list>
#include <string>


namespace soclib {
namespace caba {

class Module_5 {

	public :
		typedef struct{
			// for module_10
			bool 		poison_ack;
			bool 		unpoison_ok;
			unsigned int 	virt_poison_page;

			bool		updt_tlb_ack;
			bool		updt_wiam_ack;

			// from module_6, for control
			m6_cmd_t  m6_cmd;
			m6_data_t m6_data;
			m6_ack_t  m6_ack;
			m6_req_t  m6_req;
			m6_fifo_t m6_fifo;
			//bool m6_fifo_wok;

			// for counters
			unsigned int  node_id;
			unsigned int  page_sel;
			unsigned int  cost;
			unsigned int  segnum; 
			bool  enable; // also for compute contention

			// for compute_contention

			// for module_7
			Module_7::m7_cmd_t  m7_cmd;
			Module_7::m7_req_t  m7_req;
			Module_7::m7_rsp_t  m7_rsp;
		} m5_inputs_t;

		typedef struct{
			m6_cmd_t  m6_cmd;
			m6_data_t m6_data;
			m6_ack_t  m6_ack;
			m6_req_t  m6_req;
			m6_fifo_t m6_fifo;
			uint32_t  m6_pressure;
			// from module 10 to ram_fsm
			unsigned int poison_page;
			bool poison_req;
			bool unpoison_req;
			
			bool updt_tlb_req;
			bool updt_wiam_req;
			unsigned int home_entry;
			unsigned int new_entry;
			
		} m5_outputs_t;

		m5_inputs_t  * p_in;
		m5_outputs_t * p_out;

		Module_5(const string & insname, std::list<soclib::common::Segment> * seglist,unsigned int nb_procs,unsigned int page_size);
		~Module_5();

		void compute(); //kind of transition
		void gen_outputs(); //kind of genMoore
		void reset(); 
		#if 1
		unsigned int reset_reg(unsigned int addr,unsigned int segnum);
		unsigned int read_reg(unsigned int addr,unsigned int segnum);
		#endif
		const string & name();

	private :
		typedef struct{
			Module_7::m7_cmd_t  m7_cmd;
			Module_7::m7_req_t  m7_req;
			Module_7::m7_rsp_t  m7_rsp;
		} m7_m10_link_t;
		// internal parameters
		std::string m_name;
		unsigned int m_nb_segments;
		std::list<soclib::common::Segment> * m_seglist;
		// 5 -> 9/7 signals
		bool * m_enable_counter;
		bool * m_enable_access_register;
		// 8 <-> 10 signals
		bool m_8_to_10_contention;
		bool m_10_to_8_abort;
		// 7 <-> 10 signals
		m7_m10_link_t m7_m10_link;
		Module_7::m7_req_t * m_unresolved_output_m7_m10_link_req;
		Module_7::m7_cmd_t * m_unresolved_output_m7_m10_link_cmd;
		// 9 <-> 10 signals
		bool m_9_to_10_is_busy;
		bool m_9_to_10_is_threshold;
		bool m_10_to_9_freeze;
		bool * m_unresolved_output_m9_m10_busy;
		bool * m_unresolved_output_m9_m10_threshold;

		// internal modules
		Module_8 * compute_contention;
		Module_10 * control;
		Module_9 ** counters; // todo : private!
		Module_7 ** register_access;
	private :

		const char* gen_name(const char* name_pref,const char* name, unsigned int id);

};

}}
#endif
