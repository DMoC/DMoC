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
#ifndef TLB_32_H
#define TLB_32_H

#include <stdint.h>
#include <systemc>

#define DEBUG_TLB
#define DEBUG_TLB_LEVEL 3
#ifdef DEBUG_TLB
#define DTLBCOUT(x) if (x>=(DEBUG_TLB_LEVEL)) cout
#else
#define DTLBCOUT(x) if(0) cout
#endif
namespace soclib {
namespace caba {


template< unsigned int addr_bit_width >
struct TLB_32{

typedef sc_dt::sc_uint< addr_bit_width > addr_t;
typedef sc_dt::sc_uint< addr_bit_width > tag_t;

private :
// The TLB, nb_entries of ENTRY_TAG -> ENTRY_T_ADDR
addr_t * ENTRY_TAG;
addr_t * ENTRY_T_ADDR; 

uint32_t m_NB_ENTRIES;
uint32_t m_PAGE_SHIFT;
uint32_t m_PAGE_SIZE;

bool m_reset_done;
uint32_t m_MIGRABILITY_MASK;
bool is_pow2(uint32_t value);
uint32_t index;

// tracking purpose :
uint64_t m_nb_requests;
uint64_t m_nb_hit;
uint64_t m_nb_miss;

public : 
// returns the index of the entry to evict according to
// some algorithm (LRU,..)
uint32_t elect();

// update the entry index with the higher bits of  n_t_addr.
void add_entry(uint32_t index, addr_t n_v_addr, addr_t n_t_addr);

// true if there is a valid entry for v_addr
bool hit(addr_t v_addr);

// reset the tlb entry
void reset(tag_t entry);

// returns the translated address, if there is no entry, returns the
// same input address
sc_dt::sc_uint< addr_bit_width > translate(addr_t v_addr);
// TODO : why cannot use addr_t translate(addr_t v_addr); ? hate c++ templates


// reset the TLB
void Reset();

// display the TLB
void Print();
// nb_entries in the TLB, and page_size in bytes, and migrablitity mask
TLB_32(uint32_t nb_entries, uint32_t page_size, uint32_t migrability_mask);
~TLB_32();
};
}}
#endif
