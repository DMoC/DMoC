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
#include "module_8.h"
#include "arithmetics.h"
#include "globals.h"

namespace soclib {
namespace caba {

	using namespace soclib;
	using namespace std;
	using soclib::common::uint32_log2;

	Module_8::Module_8(const string & insname):
		m_name(insname),
		m_cycle(1),
		m_was_contention(false)
	{
		CTOR_OUT
#ifdef USE_STATS
		stats_chemin = "./";
		stats_chemin += m_name.c_str();
		stats_chemin += ".stats"; 
		file_stats.setf(ios_base::hex);
		file_stats.open(stats_chemin.c_str(),ios::trunc);
#endif
	}

	void Module_8::reset(){
		r_contention = false;
		r_pressure = 0;
		r_nb_request = 0;
		D8COUT << name() << "reset " <<  endl; 
	}
	uint32_t Module_8::read_pressure(){
		return r_pressure;
	}
	void Module_8::compute(){
		m_cycle++;
		if (*(p_in.is_abort))
		{
			r_contention = false;
			r_pressure = 0;
			r_nb_request = 0;
			D8COUT << name() << "abort " <<  endl; 
			return;
		}
		if (*(p_in.is_req)) r_nb_request++;
		if ((m_cycle%SAT_TIME_SLOT) == 0)
		{	 // EACH time slot, save the number of requests of
			// the current time slot, this defines the pressure (nb_req/cycle)
			// if this pressure > threshold, rise contention signal
			r_pressure = r_nb_request;
			D8COUT << name() << " p  : " << dec << r_pressure << endl; 
#ifdef USE_STATS
			file_stats << " p  : " << dec << r_pressure << endl; 
#endif
			r_nb_request = 0;
			m_cycle = 0;
		}
		if ((r_pressure > SAT_THRESHOLD)  && (r_contention == false))
		{
			D8COUT << name() << " ---> raised contention at p= " << dec << r_pressure << endl; 
		}
		r_contention = (r_pressure > SAT_THRESHOLD);
	}

	void Module_8::gen_outputs(){
		*(p_out.contention) = r_contention; // 1 cycle delay
		m_was_contention = m_was_contention || r_contention;
		if ((m_cycle%1000000) == 0)
		{ 
			m_was_contention = false;
		}

	}

	const string & Module_8::name(){
		return this->m_name;
	}

	Module_8::~Module_8()
	{
		DTOR_OUT
#ifdef USE_STATS
			file_stats.close();
#endif

	}

}}
