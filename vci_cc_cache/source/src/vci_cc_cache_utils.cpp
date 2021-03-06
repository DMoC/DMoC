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
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2006-2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#include "vci_cc_cache.h"

#define tmpl(x)  template<typename vci_param, typename iss_t> x VciCcCache<vci_param, iss_t>
namespace soclib { 
namespace caba {

tmpl(typename vci_param::data_t)::be_to_mask(typename iss_t::be_t be)
{
    int i;
    typename vci_param::data_t ret = 0;
    const typename iss_t::be_t be_up = (1<<(vci_param::B -1));

    for (i=0 ; i < vci_param::B ; ++i)
		{
        ret <<= 8;
        if ( be_up & be )
            ret |= 0xff;
        be <<= 1;
    }
    return ret;
}

}}
