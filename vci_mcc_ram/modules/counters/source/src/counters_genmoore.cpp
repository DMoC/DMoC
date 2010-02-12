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
 * 	Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 */

#include "counters.h"

namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;

void Counters::genMoore( void ){
	switch (r_CTER_FSM)
	{
		case CTER_IDLE :
			p_contention = r_raise_threshold; 
#ifndef NO_CTRL
			p_output = 0; 
			p_ack = 0;	
			p_valid = 0;	
#endif
			break;
		default :
			p_contention = false; 
#ifndef NO_CTRL
			p_output = 0; 
			p_ack = 0;	
			p_valid = 0;	
#endif
			break;

	}
}

}}
