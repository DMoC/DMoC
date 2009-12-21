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
#include "manometer.h"
#include "arithmetics.h"

namespace soclib {
namespace caba {

	using namespace soclib;
	using namespace std;
	using soclib::common::uint32_log2;

	Manometer::Manometer(sc_module_name insname):
		soclib::caba::BaseModule(insname),
		r_pressure("PRESSURE"),
		r_nb_request("NB_REQUESTS"),
		m_cycles(0),
		m_was_contention(false)
		// TODO : should I initialize ports with a name ?
	{
		CTOR_OUT
#ifdef USE_STATS
		stats_chemin = "./";
		stats_chemin += m_name.c_str();
		stats_chemin += ".stats"; 
		file_stats.setf(ios_base::hex);
		file_stats.open(stats_chemin.c_str(),ios::trunc);
#endif

		SC_METHOD(transition);
		sensitive_pos << p_clk;
		dont_initialize();

		SC_METHOD(genMoore);
		sensitive_pos << p_clk;
		dont_initialize();

	}

	void Manometer::transition()
	{
		if(!p_resetn.read())
		{
			r_pressure		= 0;
			r_nb_request	= 0;
			r_MNTER_FSM      = MNTER_IDLE;
			DCOUT << name() << " reset " << endl;
		}
		m_cycles++;
		switch ((mnter_fsm_state_e)r_MNTER_FSM.read())
		{
			case MNTER_IDLE :

				if (p_req.read()) // priority ovec core requests
				{
					switch (p_cmd.read())
					{
						case MNTER_CMD_READ :
							r_MNTER_FSM = MNTER_READ;
							break;

						case MNTER_CMD_RESET : // Rese sent by another module
							r_pressure = 0;
							r_nb_request = 0;
							break;

						default :
							assert(false);
							break;
					}
				}
				else if (p_core_req.read())
				{
					if ((m_cycles % SAT_TIME_SLOT) == 0)
					// Compute each SAT_TIME_SLOT pressure value and determine
					// if it is necessary to raise a contention signal :
					// ( pressure > SAT_THRESHOLD )
					{	
						r_pressure = r_nb_request;
						r_nb_request = 0;
						m_cycles = 0;
						if (r_nb_request.read() > SAT_THRESHOLD) // raise contention
						{
							r_MNTER_FSM = MNTER_CONTENTION;
						}

						// Some traces
						DCOUT << name() << " p  : " << dec << r_pressure << endl; 
#ifdef USE_STATS
						file_stats << " p  : " << dec << r_pressure << endl; 
#endif
					}
					else
					{
						r_nb_request = r_nb_request.read() + 1;
					}
				}
				break;

			case  MNTER_CONTENTION :
#ifndef QUICK_RAISE
				if (p_contention_ack.read())
				{
					DCOUT << name() << " ---> raised contention at p= " << dec << r_pressure << endl; 
					r_MNTER_FSM = MNTER_IDLE;
				}
#else
	// Idea : don't wait for acknowlegde, the contention signal is up for only
	// one cycle, if it is missed by the target module it will probably raise
	// again on the next time slot
	#error not implemented
#endif
			break;

			case  MNTER_READ :
				r_MNTER_FSM = MNTER_IDLE;
			break;

			default :
				assert(false);
				break;
		}
	}




	void Manometer::genMoore( void )
	{
		
		switch ((mnter_fsm_state_e)r_MNTER_FSM.read())
		{
			case MNTER_IDLE :
				p_contention = false;
				p_ack		 = true; // Accept requests
				p_valid		 = false;


			case  MNTER_CONTENTION :
				p_contention = true; // Raise contention
				p_ack		 = false;
				p_valid		 = false;
			break;

			case  MNTER_READ :
				p_contention = false;
				p_ack		 = false;
				p_valid		 = true; // ouput value
				p_pressure	 = r_pressure.read();
			break;

			default :
				assert(false);
				break;
		}
	}

	Manometer::~Manometer()
	{
		DTOR_OUT
#ifdef USE_STATS
			file_stats.close();
#endif

	}

}}
