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
    size_t i;
    typename vci_param::data_t ret = 0;
    const typename iss_t::be_t be_up = (1<<(sizeof(typename vci_param::data_t)-1));

    for (i=0; i<sizeof(typename vci_param::data_t); ++i)
		{
        ret <<= 8;
        if ( be_up & be )
            ret |= 0xff;
        be <<= 1;
    }
    return ret;
}

tmpl(int)::address_to_id(typename vci_param::addr_t addr)
{
	for(unsigned int i = 0; i < m_nb_memory_nodes; i++)
	{
		if (addr  >= (m_home_addr_table[i].base_addr ) && (addr < (m_home_addr_table[i].base_addr + m_home_addr_table[i].size)))
			return m_home_addr_table[i].node_id;
	}
	return -1;
}

#ifdef USE_STATS
tmpl(void)::gen_output_stats(void)
{


	file_stats << std::dec << m_id  << " dcache_hit : " << std::dec << (stats.dcache_hit  - stats.dcache_miss) << std::endl;
	file_stats << std::dec << m_id << " dcache_miss : " << std::dec << stats.dcache_miss << std::endl;
	file_stats << std::dec << m_id  << " icache_hit : " << std::dec << (stats.icache_hit - stats.icache_miss) << std::endl;
	file_stats << std::dec << m_id << " icache_miss : " << std::dec << stats.icache_miss << std::endl;
	file_stats << std::dec << m_id  << " iread_nack : " << std::dec << stats.iread_nack << std::endl;
	file_stats << std::dec << m_id  << " dread_nack : " << std::dec << stats.dread_nack << std::endl;
	file_stats << std::dec << m_id  << " dwrite_nack : " << std::dec << stats.dwrite_nack << std::endl;

	stats.global_inst_latency = (float)stats.idelay / (float)stats.icache_miss;
	stats.global_data_latency = (float)stats.ddelay / (float)stats.dcache_miss;

	stats.global_inst_nonack_latency = (float)(stats.idelay - stats.nack_idelay) / (float)stats.icache_miss;
	stats.global_data_nonack_latency = (float)(stats.ddelay  - stats.nack_ddelay) / (float)stats.dcache_miss;

	stats.global_inack_latency = (float)stats.nack_idelay / (float)stats.iread_nack;
	stats.global_dnack_latency = (float)stats.nack_ddelay / (float)stats.dread_nack;

	file_stats << std::dec << m_id << " icache latency : " << std::dec << stats.global_inst_latency << std::endl;
	file_stats << std::dec << m_id << " dcache latency : " << std::dec << stats.global_data_latency << std::endl;

	file_stats << std::dec << m_id  << " icache nonack latency : " << std::dec << stats.global_inst_nonack_latency << std::endl;
	file_stats << std::dec << m_id  << " dcache nonack latency : " << std::dec << stats.global_data_nonack_latency << std::endl;

	file_stats << std::dec << m_id  << " icache nack : " << std::dec << stats.global_inack_latency << std::endl;
	file_stats << std::dec << m_id  << " dcache nack : " << std::dec << stats.global_dnack_latency << std::endl;

	file_stats << std::dec << m_id  << " total_reqs : " << std::dec << stats. total_reqs << std::endl;
	file_stats << std::dec << m_id  << " total_dist_reqs : " << std::dec << stats. total_dist_reqs << std::endl;
	file_stats << std::dec << m_id  << " moyenne distance en hops : "
		<< std::dec << (float)((float) stats. total_dist_reqs / (float)stats.total_reqs) << std::endl;

	file_stats << std::dec << m_id << " total cycles   : " << std::dec << stats.ncycles << std::endl;
	file_stats << std::dec << m_id << " total writes   : " << std::dec << stats.dcache_writes << std::endl;
	file_stats << std::dec << m_id  << " CPI  : " << std::dec << ((float)stats.ncycles/(float)stats.icache_hit) << std::endl;
	file_stats << std::dec << m_id << " warning: number of writes is the number of cells sent " << std::endl;
	file_stats << std::dec << m_id << " warning: latency is latency of misses, this not includes hits " << std::endl;
	file_stats << std::dec << m_id << " warning: latency is latency of misses, this not includes hits " << std::endl;
	file_stats << std::dec << m_id  << " warning: nacks does not include ll/sc/unc access" << std::endl;
	file_stats.close();
}
#endif
}}
