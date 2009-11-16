/* -*- c++ -*-
 *
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
 *         Perre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr> 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#include "../include/cc_ram_core.h"
#include "soclib_endian.h"
#include "arithmetics.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace soclib {
namespace caba {
#define tmpl(x) template<typename vci_param, typename sram_param> x CcRamCore<vci_param,sram_param>

	/////////////////////////////////////////////////////
	//    genMealy() method
	/////////////////////////////////////////////////////
	tmpl(void)::genMealy()
	{
		if (p_sram_ack.read()) // S-Ram sends a response (on a read)
		{
			// set "directly" the data field of the builded response packet in the target fsm
			*m_fsm_data = p_sram_din.read();
		}
		if (r_RAM_FSM.read() == RAM_IDLE) // step a cycle on the target fsm (ie. send responses)
		{
				// This MUST be done after setting the data (*m_fsm_data=data) in order to write
				// the correct value on the vci port.	
				m_vci_fsm . genMoore();
		}
	};
}}
