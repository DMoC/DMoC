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
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 */

#ifndef SOCLIB_CABA_SIGNAL_MICRO_RING_PARAM_H_
#define SOCLIB_CABA_SIGNAL_MICRO_RING_PARAM_H_

namespace soclib {
namespace caba {

typedef unsigned int micro_ring_cmd_t;
typedef unsigned int micro_ring_data_t;
typedef unsigned int micro_ring_id_t;
typedef bool micro_ring_ack_t;


enum{
    RING_NOP,
    RING_LOCK,
    RING_LOCATE,
    RING_COST,
    RING_SELECT,
    RING_FINISH,
    RING_UPDATE,
    RING_UPDATE_ACK,
    RING_TRANSFER
};


}}

#endif /* SOCLIB_CABA_SIGNAL_MICRO_RING_PARAM_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

