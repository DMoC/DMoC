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
 * 	   Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 * Maintainers: Pierre Guironnet de Massas
 */

#ifndef SOCLIB_CABA_VCI_CC_CACHE_STATS_H
#define SOCLIB_CABA_VCI_CC_CACHE_STATS_H
#include <inttypes.h>
namespace soclib {
namespace caba {
	typedef struct {
		uint64_t ncycles;
		uint64_t dcache_req;
		uint64_t dcache_hit;
		uint64_t dcache_nack;
		uint64_t dcache_miss;
		uint64_t dcache_writes;
		uint64_t icache_req;
		uint64_t icache_hit;
		uint64_t icache_nack;
		uint64_t icache_miss;
		uint64_t iread_nack;
		uint64_t dread_nack;
		uint64_t dwrite_nack;
		uint64_t idelay;
		uint64_t nack_idelay;
		uint64_t inack_chunck;
		uint64_t idelay_chunck;
		uint64_t ddelay;
		uint64_t nack_ddelay;
		uint64_t dnack_chunck;
		uint64_t ddelay_chunck;
		uint64_t dcache_chunck_miss;
		uint64_t icache_chunck_miss;
		float delta_data_latency;
		float delta_inst_latency;
		float global_data_latency;
		float global_inst_latency;
		float global_data_nonack_latency;
		float global_inst_nonack_latency;
		float global_dnack_latency;
		float global_inack_latency;
		uint64_t total_reqs;
		uint64_t total_dist_reqs;
	}cache_info_t;
}
}
#endif
