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
#ifndef PHYS_PAGE_TABLE_H
#define PHYS_PAGE_TABLE_H

#include <stdint.h>

namespace soclib {
namespace caba {

typedef uint32_t addr_t;
struct PHYS_PAGE_TABLE{

private :
// The page table
addr_t * ENTRY_T_VIRT_TAG; 
addr_t * ENTRY_T_WHO_I_AM_TAG; 

uint32_t m_NB_ENTRIES;
uint32_t m_PAGE_SHIFT;
uint32_t m_PAGE_SIZE;
uint32_t m_BASE_ADDR;

bool is_pow2(uint32_t value);

public : 

// update the entry for v_addr with n_t_addr. 
// is_local is true if the new entry is present in the local node
void update_virt(unsigned int  index, addr_t tag, bool is_local);
void update_wiam(unsigned int  index, addr_t tag);

// true if the requested page is not translated
bool hit(addr_t v_addr);

// returns the translated address, if the page has never moved, return the same address
// this is the virtual address of this physical frame
addr_t translate(addr_t v_addr);
// returns the tag of the page which is contained in this frame (v_addr)
addr_t wiam(addr_t v_addr);


// reset the TLB
void Reset();

// display the TLB
void Print();
// nb_entries in the TLB, and page_size in bytes
PHYS_PAGE_TABLE(uint32_t nb_entries, uint32_t page_size,uint32_t base_addr);
~PHYS_PAGE_TABLE();
};
}}
#endif
