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
#ifndef MODULE_10_H
#define MODULE_10_H

#include <stdint.h>
#include <string>
#include <fstream>
#include <list>
#include "module_6.h"
#include "module_7.h"
#include "module_9.h"
#include "module_8.h"

#ifdef DEBUG_M10
#define D10COUT(x) if ((d_cycles > START_TRACE_CYCLE) && (x >= DEBUG_M10_LEVEL)) cout
#else
#define D10COUT(x) if(0) cout
#endif

#define USE_STATS

namespace soclib {
namespace caba {

using namespace std;
class Module_10 {

	public :
		typedef struct{
			unsigned long long cycles;
			unsigned long long c_mig;
			unsigned long long e_mig;
			unsigned long long i_mig;
			unsigned long long aborted;
		}m10_stats_t;
		typedef struct{
			// from module_8
			bool * contention;

			// from module_6 
			m6_cmd_t  * m6_cmd;
			m6_data_t * m6_data;
			m6_ack_t  * m6_ack;
			m6_req_t  * m6_req;
			m6_fifo_t * m6_fifo;

			// from module_7
			Module_7::m7_req_t  * m7_req;
			Module_7::m7_cmd_t  * m7_cmd;
			Module_7::m7_rsp_t  * m7_rsp;
	
			// from module_9
			bool	* m9_is_busy;	
			bool	* m9_is_threshold;	

			// from ram_fsm
			bool    * poison_ack;
			unsigned int * virt_poison_page;
		
			bool    * updt_tlb_ack;
			bool    * updt_wiam_ack;
			bool    * unpoison_ok;
			
		} m10_inputs_t;

		typedef struct{
			// ram_fsm
			unsigned int * poison_page;
			bool         * poison_req;
			bool         * unpoison_req;

			bool         * updt_tlb_req;
			bool         * updt_wiam_req;
			unsigned int * home_entry;
			unsigned int * new_entry;
			
			// to module_6
			m6_cmd_t  * m6_cmd;
			m6_data_t * m6_data;
			m6_ack_t  * m6_ack;
			m6_req_t  * m6_req;
			m6_fifo_t * m6_fifo;
			uint32_t * m6_pressure;

			// to module_9
			bool	* m9_freeze;	
			// to module_8
			bool	* m8_abort;	
		} m10_outputs_t;

		m10_inputs_t p_in;
		m10_outputs_t p_out;

		Module_10(const string & insname, Module_9 * counters, Module_8 * contention,unsigned int nbproc);
		~Module_10();
		void compute();
		void gen_outputs();
		void reset();
		const string & name();

	private :

		enum fsm_state_e{
			idle,
			raise_contention,
			counter_lock,
			counter_unlock,
			access_counters,
			access_value,
			poison,
			poison_ok,
			unpoison,
			abort,
			finish,
			unpoison_ok,
			elect,
			updt_inv,
			updt_table,
			updt_wiam,
			updt_wiam_ok,
			send_home_addr
		};

		string m_name;
		unsigned int m_NBPROC;
		unsigned int m_inter_fifo_size;
		Module_9 * m_counters;
		Module_8 * m_contention;
		bool r_master_contention;
		bool r_slave;
		bool r_waiting_time;
		unsigned int m_wait_cycles;
		uint32_t m_cycles;
		uint64_t d_cycles;
		bool r_pending_reset;
		unsigned int r_fsm;	


		counter_id_t r_max_p;	
		unsigned int r_virt_max_p_addr;

		bool r_mig_c;
		unsigned int r_index_cpt;
		// update and inv tlb
		unsigned int r_page_local_addr;
		unsigned int r_page_new_addr;
		unsigned int r_page_wiam_addr;

#if 0
		unsigned int r_page_aborted[3]; // a page not_migrable that we should set to migrable after the next successful migration!
		unsigned int r_page_mig[3]; 
		// used as a lock to arbitrate accesses to the counters, 1 per segment
		unsigned int r_page_aborted_index;
		counter_id_t r_last_aborted;	
#endif
		bool r_m9_locked;
		// Debug and misc purpose

#ifdef USE_STATS
		m10_stats_t stats;
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
};

}}
#endif
