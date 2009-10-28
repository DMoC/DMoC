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
 * 	Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>, 2008
 *
 */



#include <cassert>
#include <iostream>
#include "tlb_32.h"
#include "arithmetics.h"
#include "stdlib.h"
#include "tlb_32.h"

#define V_BIT 0x80000000
namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;
using soclib::common::uint32_log2;

template< unsigned int addr_bit_width> 
void TLB_32<addr_bit_width>::add_entry(uint32_t index, addr_t n_v_addr, addr_t n_t_addr)
{
	if (m_reset_done)
	{
		assert(index < m_NB_ENTRIES);
		ENTRY_T_ADDR[index] = (n_t_addr >> m_PAGE_SHIFT) ;
		ENTRY_TAG[index] = (n_v_addr >> m_PAGE_SHIFT) | V_BIT;
	}
	else
	{
		assert(false);
	}
}

template< unsigned int addr_bit_width> 
void TLB_32<addr_bit_width>::reset(tag_t entry)
{
	if (m_reset_done)
	{
		for (uint32_t i=0; i < m_NB_ENTRIES; i++)
		{
			if (ENTRY_TAG[i] == ( entry | V_BIT) )
			{
				DTLBCOUT(2) << " tlb_inv hit " <<  endl;;
				ENTRY_TAG[i] = 0;
				return;
			}	
		}
		DTLBCOUT(2) << " tlb_inv miss " << endl;;
	}
	else
	{
		assert(false);
	}
}

template< unsigned int addr_bit_width> 
bool TLB_32<addr_bit_width>::hit(addr_t v_addr)
{
	static int t_hit=0;
	static int t_req=0;
	assert(ENTRY_TAG != NULL);
	t_req++;
	if (m_reset_done)
	{
		if ((v_addr & m_MIGRABILITY_MASK) == 0)
		{
			return true;
		}
		for (uint32_t i=0; i < m_NB_ENTRIES; i++)
		{
			if (ENTRY_TAG[i] ==( (v_addr >> m_PAGE_SHIFT) | V_BIT))
			{
				t_hit++;
				return true;
			}	
		}
	}
	else
	{
		assert(false);
	}
	return false;
}

template<unsigned int addr_bit_width> 
sc_dt::sc_uint<addr_bit_width> TLB_32<addr_bit_width>::translate(addr_t v_addr)
{
	if (m_reset_done)
	{
		if ((v_addr & m_MIGRABILITY_MASK) == 0)
		{
			return v_addr; // TODO
		}
		for (uint32_t i=0; i < m_NB_ENTRIES; i++)
		{
			if (ENTRY_TAG[i] ==( (v_addr >> m_PAGE_SHIFT) | V_BIT))
			{
				return (((ENTRY_T_ADDR[i] << m_PAGE_SHIFT)) | (v_addr & (m_PAGE_SIZE -1))) ; // translation produces a complete address
				// including lower bits
			}	
		}
		return v_addr;
	}
	else
	{
		assert(false);
	}
	return 0xDEADBEFF;
}


template< unsigned int addr_bit_width> 
void TLB_32<addr_bit_width>::Reset()
{
	for (uint32_t i=0; i < m_NB_ENTRIES; i++)
	{
		ENTRY_TAG[i] = 0;
	}
	m_reset_done = true;
}


template< unsigned int addr_bit_width> 
void TLB_32<addr_bit_width>::Print()
{
	if (m_reset_done) // Print only if the TLB has meaningfull values
	{
		for (uint32_t i=0; i < m_NB_ENTRIES; i++)
		{
			cout << std::hex << "tlb: [" << ENTRY_TAG[i] << "-" << ENTRY_T_ADDR[i] << endl;
		}
	}
	else
	{
		assert(false);
	}
}

template< unsigned int addr_bit_width> 
bool TLB_32<addr_bit_width>::is_pow2(uint32_t value)
{
	for(uint32_t i=0; i < 32 ; i++)
	{
		if (value & 0x1) return  (value==1);
		value = value >> 1;
	}
	return false;
}

template< unsigned int addr_bit_width> 
uint32_t TLB_32< addr_bit_width>::elect(){
// elect acording to a fifo order
#if 0
#if 1
	index ++;
	if (index == m_NB_ENTRIES)
		index = 0;
	return index;
#else
	return 0;
#endif
#else
	return  (uint32_t)((double)rand() / ((double)RAND_MAX + 1) * m_NB_ENTRIES); 
#endif
}

template< unsigned int addr_bit_width> 
TLB_32<addr_bit_width>::TLB_32(uint32_t nb_entries, uint32_t page_size, uint32_t migrability_mask): 
	m_NB_ENTRIES(nb_entries),
	m_PAGE_SHIFT(uint32_log2(page_size)),
	m_PAGE_SIZE(page_size),
	m_reset_done(false),
	m_MIGRABILITY_MASK(migrability_mask),	
	index(0)
{
	std::cout << "TLB 32 : page size " << std::dec << page_size << std::endl;
	std::cout << "TLB 32 : nb entries " << std::dec << nb_entries << std::endl;
	assert(is_pow2(page_size));
	ENTRY_TAG = new addr_t[nb_entries];
	ENTRY_T_ADDR = new addr_t[nb_entries];
	//srand(35);
	
	//for (uint32_t i=0; i < m_NB_ENTRIES; i++)
	//{
	//	ENTRY_TAG[i] = 0;
	//	ENTRY_T_ADD[i] = 0;
	//}
}

template< unsigned int addr_bit_width> 
TLB_32<addr_bit_width>::~TLB_32()
{
	delete [] ENTRY_T_ADDR;
	delete [] ENTRY_TAG;
} 
}}
