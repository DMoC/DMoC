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

#ifndef SOCLIB_CABA_SIGNAL_MICRO_RING_PORTS_H_
#define SOCLIB_CABA_SIGNAL_MICRO_RING_PORTS_H_

#include "micro_ring_signals.h"

namespace soclib {
namespace caba {

class MicroRing_In
{
public:
	sc_in< micro_ring_cmd_t>    cmd;        
	sc_in< micro_ring_id_t> id;   
	sc_in< micro_ring_data_t>   data;   
	sc_out< micro_ring_ack_t>    ack;   
	sc_in< micro_ring_id_t> srcid;   

    MicroRing_In(const std::string &name = sc_gen_unique_name("micro_ring_in"))
        : cmd    	((name+"_cmd").c_str()),
        id   	((name+"_id").c_str()),
        data  	((name+"_data").c_str()),
        ack  	((name+"_ack").c_str()),
        srcid  	((name+"_srcid").c_str())
	{
	}
    
	void operator () (MicroRingSignals &sig)
	{
        cmd   (sig.cmd);
        id    (sig.id);
        data  (sig.data);
        ack   (sig.ack);
        srcid (sig.srcid);
	}
};

/*
 * Ring Wrappor out port
 */
class MicroRing_Out
{
public:
	sc_out< micro_ring_cmd_t>    cmd;        
	sc_out< micro_ring_id_t> id;   
	sc_out< micro_ring_data_t>   data;   
	sc_in< micro_ring_ack_t>    ack;   
	sc_out< micro_ring_id_t> srcid;   
		
    MicroRing_Out(const std::string &name = sc_gen_unique_name("micro_ring_out"))
        : cmd    	((name+"_cmd").c_str()),
        id   	((name+"_id").c_str()),
        data  	((name+"_data").c_str()),
        ack  	((name+"_ack").c_str()),
        srcid  	((name+"_srcid").c_str())
	{
	}
    
	void operator () (MicroRingSignals &sig)
	{
        cmd   (sig.cmd);
        id    (sig.id);
        data  (sig.data);
        ack   (sig.ack);
        srcid (sig.srcid);
	}
};

}}

#endif 

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

