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
#ifndef MODULE_8_H
#define MODULE_8_H

#include <stdint.h>
#include <string>
#include "globals.h"
#include <fstream>

#ifdef DEBUG_M8
	#define D8COUT cout
#else
	#define D8COUT if(0) cout
#endif
#define USE_STATS
namespace soclib {
namespace caba {
using namespace std;

class Module_8 {

	public :
		typedef struct{
			bool * is_req;
			bool * is_abort;
		}m8_inputs_t;

		typedef struct{
			bool * contention;
		}m8_outputs_t;

		m8_inputs_t p_in;
		m8_outputs_t p_out;


		Module_8(const string & insname);
		~Module_8();
		void compute();
		void gen_outputs();
		void reset();
		uint32_t read_pressure();
		const string & name();

	private :
		
		string m_name;

		uint32_t r_pressure;
		uint32_t r_nb_request;
		bool r_contention;
		unsigned int m_cycle;
		bool m_was_contention;
#ifdef USE_STATS
		std::ofstream file_stats;
		std::string stats_chemin; 	// set it to : ./insname.stats
#endif

};

}}
#endif
