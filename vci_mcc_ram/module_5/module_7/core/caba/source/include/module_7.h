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
#ifndef MODULE_7_H
#define MODULE_7_H

#include <stdint.h>
#include <string>
#include "module_9.h"

#ifdef DEBUG_M7
#define D7COUT cout
#else
#define D7COUT if(0) cout
#endif

namespace soclib {
namespace caba {

using namespace std;
class Module_7 {

	public :

		typedef bool m7_req_t;
		typedef bool m7_rsp_t;
		typedef enum {CMD7_NOP, CMD7_READ, CMD7_WRITE, CMD7_LOCK, CMD7_UNLOCK} m7_cmd_t;
		
		typedef struct{

			// from module_5 
			m7_req_t  * req;
			m7_cmd_t  * cmd;
			m7_rsp_t  * rsp;
			
		} m7_inputs_t;

		typedef struct{
			// to module_10
			m7_req_t  * req;
			m7_cmd_t  * cmd;
			m7_rsp_t  * rsp;

		} m7_outputs_t;

		m7_inputs_t p_in;
		m7_outputs_t p_out;
		bool r_locked;

		Module_7(const string & insname, Module_9 * link);
		~Module_7();

		unsigned int reset_reg(unsigned int addr);
		unsigned int read_reg(unsigned int addr);

		void compute();
		void gen_outputs();
		void reset();
		const string & name();

	private :
		string m_name;
		Module_9 * m_linked_counter_set;
};

}}
#endif
