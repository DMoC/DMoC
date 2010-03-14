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
#ifndef MANOMETER_H
#define MANOMETER_H

#include <stdint.h>
#include <string>
#include <systemc>
#include "mcc_globals.h"
#include "caba_base_module.h"
#include <fstream>

#define DEBUG_MANOMETER
#ifdef DEBUG_MANOMETER
	#define DCOUT cout
#else
	#define DCOUT if(0) cout
#endif

namespace soclib {
namespace caba {
using namespace std;
using namespace sc_core;

class Manometer : public soclib::caba::BaseModule {

	public :
		// some public types (or commands)
		typedef sc_uint < 2 > mnter_cmd_t;
		typedef sc_uint< 32 > mnter_pressure_t;
		enum mnter_cmd_types_e
		{
			MNTER_CMD_NOP,
			MNTER_CMD_READ,
			MNTER_CMD_RESET
		};

	public :

		// Inputs 
		sc_in<bool>	p_clk;		// clock and reset
		sc_in<bool>	p_resetn;


		// From the core
		sc_in < bool > p_core_req;		// There is a request to memory

		// From other modules
		sc_in < bool > p_req;
		sc_in < mnter_cmd_t > p_cmd;
		sc_in < bool > p_contention_ack;

		// Outputs
		sc_out < bool > p_contention;
		sc_out < bool > p_ack; 			// Acknownledge of requests (maybe unecessary)


		sc_out < mnter_pressure_t > p_pressure;	// value : pressure, 
		sc_out < bool > p_valid;				// valid output


	private :
		enum mnter_fsm_state_e
		{
			MNTER_IDLE,
			MNTER_READ,
			MNTER_RESET,
			MNTER_COMPUTE,
			MNTER_CONTENTION
		};

	public :

		Manometer(sc_module_name insname);
		~Manometer();

	protected :

		SC_HAS_PROCESS(Manometer);

	private :
		
		// Registers
		sc_signal< mnter_pressure_t >	r_pressure;
		sc_signal< mnter_pressure_t >	r_nb_request;

		sc_signal< uint32_t >		r_MNTER_FSM;

		// Structural parameters
		uint64_t	m_cycles;
		bool		m_was_contention;

		void transition();
		void genMoore();
#ifdef READ_ONE_CYCLE
		#error not implemented
		void genMealy(); // Idea : read in one cycle (mealy signal the register value)
#endif

#ifdef USE_STATS
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif
};

}}
#endif
