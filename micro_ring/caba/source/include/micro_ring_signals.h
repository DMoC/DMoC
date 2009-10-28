/*
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
 * Author   : Pierre Guironnet de Massas
 * Date     : 08 Oct 2008
 * Copyright: TIMA
 */

#ifndef SOCLIB_CABA_MICRO_RING_SIGNALS_H_
#define SOCLIB_CABA_MICRO_RING_SIGNALS_H_

#include "micro_ring_param.h"
#include <systemc.h>

namespace soclib { 
namespace caba {

struct no_sc_MicroRingSignals
{
	// signals
	micro_ring_cmd_t    cmd;        
	micro_ring_id_t     id;   
	micro_ring_data_t   data;   
	micro_ring_ack_t    ack;   
	micro_ring_id_t     srcid;   
};

class MicroRingSignals
{

public:
	// signals
	sc_signal< micro_ring_cmd_t >    cmd;        
	sc_signal< micro_ring_id_t >     id;   
	sc_signal< micro_ring_data_t >   data;   
	sc_signal< micro_ring_ack_t >    ack;   
	sc_signal< micro_ring_id_t >     srcid;   
#define ren(x) x(((std::string)(name_ + "_"#x)).c_str())

	MicroRingSignals(std::string name_ = (std::string)sc_gen_unique_name("micro_ring_signals"))
	  : ren(cmd),
	  ren(id),
	  ren(data),
	  ren(ack),
	  ren(srcid)
	  {
	  }
#undef ren
};

}} // end namespace

#endif /* SOCLIB_CABA_MICRO_RING_SIGNALS_H_ */
