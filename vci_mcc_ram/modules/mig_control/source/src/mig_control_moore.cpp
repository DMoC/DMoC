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
 * 	Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 */



#include <cassert>
#include <iostream>
#include "mig_control.h"
#include "arithmetics.h"
#include "mcc_globals.h"

namespace soclib {
	namespace caba {

		using namespace soclib;
		using namespace std;
		using soclib::common::uint32_log2;

		void MigControl::genMoore( void )
		{
#if 0
			// default values
			*(p_out.m6_ack) = false;
			*(p_out.m6_cmd) = CMD_NOP;
			*(p_out.m6_data)= 0;
			*(p_out.m6_req) = false;
#endif



			// freeze = counter_lock OR r_m9_locked
			// this should avoid race condition on test busy signal and set freeze signal
#if 0
			*(p_out.m6_pressure) = m_contention->read_pressure(); // Always send pressure signal
#endif
			switch((fsm_todo_state_e)r_MIG_TODO_FSM.read())
			{
				case t_idle :
					break;
				case t_register :
					break;
			}

			switch((fsm_m_state_e)r_MIG_M_CTRL_FSM.read())
			{
				case m_idle :
					p_out_manometer_req = false;
					p_out_manometer_contention_ack = true;
					p_out_manometer_cmd = Manometer::MNTER_CMD_NOP; 
					break;
				case m_raise_contention :
					p_out_manometer_req = false;
					p_out_manometer_contention_ack = true;
					p_out_manometer_cmd = Manometer::MNTER_CMD_NOP; 
					break;
				case m_abort :
					p_out_manometer_req = true;
					p_out_manometer_contention_ack = false;
					p_out_manometer_cmd = Manometer::MNTER_CMD_RESET; 
					break;
			}

			switch((fsm_pp_state_e)r_MIG_PP_CTRL_FSM.read())
			{
				case pp_idle :
					break;
				case pp_poison :
					break;
				case pp_poison_ok :
					break;
				case pp_unpoison :
					break;
				case pp_unpoison_ok :
					break;
				case pp_send_home_addr :
					break;
				case pp_finish :
					break;
			}

			switch((fsm_pt_state_e)r_MIG_PT_CTRL_FSM.read())
			{
				case pt_idle :
					break;
				case pt_updt_inv :
					break;
				case pt_updt_table :
					break;
				case pt_updt_wiam :
					break;
				case pt_updt_wiam_ok :
					break;
			}

			switch((fsm_c_state_e)r_MIG_C_CTRL_FSM.read())
			{
				case c_idle :
					p_out_counters_req = false;
					p_out_counters_cmd = Counters::NOP;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					break;

				case c_access_counters :
					p_out_counters_req = true;
					p_out_counters_cmd = Counters::R_MAX_ID_PAGE;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					assert(false);
					break;

				case c_access_value :
					p_out_counters_req = true;
					p_out_counters_cmd = Counters::R_MAX_ROUNDED_COUNTER;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					assert(false);
					break;

				case c_elect :
					p_out_counters_req = true;
					p_out_counters_cmd = Counters::ELECT;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					break;
				case c_elect_wait :
					p_out_counters_req = false;
					p_out_counters_cmd = Counters::NOP;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					break;
				case c_abort :
					p_out_counters_req = true;
					p_out_counters_cmd = Counters::W_RESET_PAGE_CTER;
					p_out_counters_page_id = 0xDEADDEAD;
					p_out_counters_node_id = 0xDEADDEAD;
					assert(false);
					break;
			}
		}

}}
