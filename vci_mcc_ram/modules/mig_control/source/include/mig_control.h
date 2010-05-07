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
#ifndef MIG_CONTROL_H
#define MIG_CONTROL_H

#include <systemc>
#include <stdint.h>
#include <string>
#include <fstream>
#include <list>

#include <manometer.h>
#include <counters.h>

#include "caba_base_module.h"

#define DEBUG_MIG_CONTROL
#ifdef DEBUG_MIG_CONTROL
#define D10COUT(x) if (x >= DEBUG_MIG_CONTROL_LEVEL) cout
#else
#define D10COUT(x) if(0) cout
#endif


namespace soclib {
namespace caba {

using namespace std;
using namespace sc_core;

class MigControl : public soclib::caba::BaseModule {

	public :
		typedef enum { CMD_NOP, CMD_BEGIN_TRANSACTION, CMD_PAGE_ADDR_HOME,
			CMD_POISON, CMD_UNPOISON,
			CMD_ELECT, CMD_UPDT_INV, CMD_UPDT_WIAM, CMD_ABORT } m6_cmd_t;
		// MigControl has 4 interfaces : 
		// 	-> Manometer, which raises a contention signal
		// 	-> Counters, which raises a contention signal and provide r/w
		// 	interface to counters
		// 	-> Core_Pages : interface to control poisonning/unpoisonning of
		// 	pages
		// 	-> Core_PTable : interface to control page table entries

		typedef enum {CTRL_PP_NOP, CTRL_PP_POISON_REQ, CTRL_PP_UNPOISON_REQ} ctrl_pape_poison_cmd_t;
		typedef enum {CTRL_PT_NOP, CTRL_PT_F1_REQ, CTRL_PT_F2_REQ } ctrl_page_table_cmd_t;
		typedef enum {CTRL_C_NOP, CTRL_C_COUNTER_READ, CTRL_C_COUNTER_RESET, CTRL_C_GLOBAL_RESET, CTRL_C_READ_MAX} ctrl_counters_cmd_t;
	public :
		typedef struct{
			unsigned long long cycles;
			unsigned long long c_mig;
			unsigned long long e_mig;
			unsigned long long i_mig;
			unsigned long long aborted;
		} mig_control_stats_t;

		// Inputs 
		sc_in <bool>	p_clk;	
		sc_in <bool>	p_resetn;

		// from Manometer
		sc_in < bool > p_in_manometer_contention;

		sc_in < bool > p_in_manometer_ack;
		sc_in < bool > p_in_manometer_valid;
		sc_in < soclib::caba::Manometer::mnter_pressure_t > p_in_manometer_pressure;

		// To Manometer
		sc_out < bool > p_out_manometer_req;
		sc_out < soclib::caba::Manometer::mnter_cmd_t > p_out_manometer_cmd;
		sc_out < bool > p_out_manometer_contention_ack;

#if 0
		// from module_6 
		sc_in < soclib::caba::MigEngine::mig_cmd_t>  p_in_mig_engine_cmd;
		sc_in < soclib::caba::MigEngine::mig_data_t> p_in_mig_engine_data_0;
		sc_in < soclib::caba::MigEngine::mig_data_t> p_in_mig_engine_data_1;
		sc_in < soclib::caba::MigEngine::mig_ack_t>  p_in_mig_engine_ack;
		sc_in < soclib::caba::MigEngine::mig_req_t>  p_in_mig_engine_req;
#else
		sc_in < bool >  p_in_mig_engine_ack;
		sc_in < bool >  p_in_mig_engine_req;
		sc_in < m6_cmd_t >  p_in_mig_engine_cmd;
#endif
		// from/to counters
		sc_in< bool > p_in_counters_contention;

		sc_out< bool > p_out_counters_req;
		sc_out< Counters::cter_cmd_t > p_out_counters_cmd;
		sc_out< sc_uint < 32 > > p_out_counters_page_id;
		sc_out< sc_uint < 32 > > p_out_counters_node_id;

		sc_in < bool > p_in_counters_ack;
		sc_in < bool > p_in_counters_valid;
		sc_in < sc_uint < 32 > > p_in_counters_output;

#if 0
		// core PP
		sc_out<bool>   	   p_out_1_core_req;
		sc_out<uint32_t>   p_out_1_core_cmd;
		sc_out<uint32_t>   p_out_1_core_data_0;
		sc_out<uint32_t>   p_out_1_core_data_1;
		sc_out<bool>	   p_out_1_core_rsp_ack; 

		// core PP
		sc_out<bool>   	   p_out_2_core_req;
		sc_out<uint32_t>   p_out_2_core_cmd;
		sc_out<uint32_t>   p_out_2_core_data_0;
		sc_out<uint32_t>   p_out_2_core_data_1;
		sc_out<bool>	   p_out_2_core_rsp_ack; 

		// plutot que des vagues "poison_core_ok!"
		sc_in<bool>  	p_in_1_core_req_ack; 
		sc_in<uint32_t> p_in_1_core_cmd;
		sc_in<bool>  	p_in_1_core_rsp; 
		sc_in<uint32_t> p_in_1_core_data_0; 

		// plutot que des vagues "poison_core_ok!"
		sc_in<bool>  	p_in_2_core_req_ack; 
		sc_in<uint32_t> p_in_2_core_cmd;
		sc_in<bool>  	p_in_2_core_rsp; 
		sc_in<uint32_t> p_in_2_core_data_0; 
#endif

		// to module_6
#if 0
		m6_cmd_t  * m6_cmd;
		m6_data_t * m6_data;
		m6_ack_t  * m6_ack;
		m6_req_t  * m6_req;
		m6_fifo_t * m6_fifo;
		uint32_t * m6_pressure;
#endif
#if 0
		// from module_6 
		sc_out < soclib::caba::MigEngine::mig_cmd_t>  p_out_mig_engine_cmd;
		sc_out < soclib::caba::MigEngine::mig_data_t> p_out_mig_engine_data;
		sc_out < soclib::caba::MigEngine::mig_ack_t>  p_out_mig_engine_ack;
		sc_out < soclib::caba::MigEngine::mig_req_t>  p_out_mig_engine_req;
		sc_out < uint32_t >  p_out_mig_engine_pressure;
		// TODO : no more fifo
#endif


		MigControl(sc_module_name insname, unsigned int nbproc);
		~MigControl();
		void transition();
		void genMoore();

	protected:

		SC_HAS_PROCESS(MigControl);

	private :

		enum fsm_m_state_e{
			m_idle,
			m_raise_contention,
			m_abort
		};

		enum fsm_c_state_e{
			c_idle,
			c_elect,
			c_elect_wait,
			c_abort
		};

		enum fsm_pp_state_e{
			pp_idle,
			pp_poison,
			pp_poison_ok,
			pp_unpoison,
			pp_unpoison_ok,
			pp_send_home_addr,
			pp_finish,
		};

		enum fsm_pt_state_e{
			pt_idle,
			pt_updt_inv,
			pt_updt_table,
			pt_updt_wiam,
			pt_updt_wiam_ok,
		};

		enum fsm_todo_state_e{
			t_idle,
			t_register
		};

		// Some control signals
		sc_signal<bool> r_disable_contention_raise;
		sc_signal<bool> r_master_transaction;
		sc_signal<bool> r_slave_transaction;
		sc_signal<bool> r_pending_reset;

		sc_signal< unsigned int > r_MIG_TODO_FSM;	
		sc_signal< bool > r_todo_board[16];	// At least as many as todo_commands (6 ?)

		sc_signal< unsigned int > r_MIG_M_CTRL_FSM;	
		sc_signal< unsigned int > r_MIG_PP_CTRL_FSM;	
		sc_signal< unsigned int > r_MIG_PT_CTRL_FSM;	
		sc_signal< unsigned int > r_MIG_C_CTRL_FSM;	

		unsigned int m_NBPROC;

		uint64_t d_cycles;

		// Access counters signals
		sc_signal< counter_id_t > r_c_max_p;
		sc_signal< counter_id_t > r_c_elect_p;	

		sc_signal< unsigned int > r_c_virt_max_p_addr;
		sc_signal< unsigned int > r_c_index_cpt;

		// Page Table signals
		sc_signal <unsigned int> r_pt_page_local_addr;
		sc_signal <unsigned int> r_pt_page_new_addr;

		// TODO : all addresses should be "address" type!

#ifdef USE_STATS
		mig_control_stats_t stats;
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
};

}}
#endif
