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
#include "phys_page_table.h"
#include "arithmetics.h"

#define V_BIT 0x80000000
namespace soclib {
namespace caba {

using namespace soclib;
using namespace std;
using soclib::common::uint32_log2;

void PHYS_PAGE_TABLE::update_virt(unsigned int  index, addr_t tag, bool is_local)
{
	assert(index < m_NB_ENTRIES);
	ENTRY_T_VIRT_TAG[index] =  tag;
}

void PHYS_PAGE_TABLE::update_wiam(unsigned int  index, addr_t tag)
{
	assert(index < m_NB_ENTRIES);
	//cout << "--------------> update_wiam : index " << dec << index << " tag 0x" << hex << tag <<  " page 0x" << (tag << m_PAGE_SHIFT) << endl;
	ENTRY_T_WHO_I_AM_TAG[index] = tag;
}

bool PHYS_PAGE_TABLE::hit(addr_t v_addr){
	assert(false); // Cette fonction ne sert plus à rien!
	assert((v_addr >> m_PAGE_SHIFT) < m_NB_ENTRIES);
	return (ENTRY_T_VIRT_TAG[v_addr >> m_PAGE_SHIFT] & V_BIT);
}

addr_t PHYS_PAGE_TABLE::translate(addr_t v_addr){
	assert((v_addr >> m_PAGE_SHIFT) < m_NB_ENTRIES);
	if (ENTRY_T_VIRT_TAG[v_addr >> m_PAGE_SHIFT] & V_BIT){
		// TODO "WARNING : returned address has a wrong page offset (physical page table)" << endl;
		//cout << " Translation réalisée (vbit) : 0x" << hex  << v_addr << " -> 0x" << ((m_BASE_ADDR | v_addr) & (~(m_PAGE_SIZE-1))) << endl;
		return ((m_BASE_ADDR | v_addr) & (~(m_PAGE_SIZE-1))); // Zero the least significant bits
		// Si V-bit, alors on revoi la même addresse!
		// très très bizarre cela
		// TODO : is_local is meaningless
	}else{
		// Here we return a TAG
		//cout << " Translation réalisée : 0x" << hex  << v_addr << " -> 0x" << (ENTRY_T_VIRT_TAG[v_addr >> m_PAGE_SHIFT] << m_PAGE_SHIFT) << endl;
		return (ENTRY_T_VIRT_TAG[v_addr >> m_PAGE_SHIFT] << m_PAGE_SHIFT);
		
	}
}

addr_t PHYS_PAGE_TABLE::wiam(addr_t v_addr){
	assert((v_addr >> m_PAGE_SHIFT) < m_NB_ENTRIES);
	if (ENTRY_T_WHO_I_AM_TAG[v_addr >> m_PAGE_SHIFT] & V_BIT){
		//cout << "--------------> translate_wiam (return): index " << dec << (v_addr >> m_PAGE_SHIFT) << " tag 0x" << hex << ENTRY_T_WHO_I_AM_TAG[v_addr >> m_PAGE_SHIFT] <<  " page 0x" << ((m_BASE_ADDR | v_addr) & (~(m_PAGE_SHIFT-1))) << endl;
		return ((m_BASE_ADDR | v_addr) & (~(m_PAGE_SIZE-1))); // Zero the least significant bits
	}else{
		//cout << "--------------> translate_wiam : index " << dec << (v_addr >> m_PAGE_SHIFT) << " tag 0x" << hex << ENTRY_T_WHO_I_AM_TAG[v_addr >> m_PAGE_SHIFT] <<  " page 0x" <<  (ENTRY_T_WHO_I_AM_TAG[v_addr >> m_PAGE_SHIFT] << m_PAGE_SHIFT)<< endl;
		return (ENTRY_T_WHO_I_AM_TAG[v_addr >> m_PAGE_SHIFT] << m_PAGE_SHIFT);
	}
}

void PHYS_PAGE_TABLE::Reset()
{
	for (uint32_t i=0; i < m_NB_ENTRIES; i++)
	{
		ENTRY_T_VIRT_TAG[i] = V_BIT;
		ENTRY_T_WHO_I_AM_TAG[i] = V_BIT;
	}
}


void PHYS_PAGE_TABLE::Print()
{
	for (uint32_t i=0; i < m_NB_ENTRIES; i++)
	{
		cout << std::dec << "pagetable: [" << (i<< m_PAGE_SHIFT) << "-" << std::hex << ENTRY_T_VIRT_TAG[i] << endl;
	}
}

bool PHYS_PAGE_TABLE::is_pow2(uint32_t value)
{
	for(uint32_t i=0; i < 32 ; i++)
	{
		if (value & 0x1) return  (value==1);
		value = value >> 1;
	}
	return false;
}

PHYS_PAGE_TABLE::PHYS_PAGE_TABLE(uint32_t nb_entries,uint32_t page_size, uint32_t base_addr): 
	m_NB_ENTRIES(nb_entries),
	m_PAGE_SHIFT(uint32_log2(page_size)),
	m_PAGE_SIZE(page_size),
	m_BASE_ADDR(base_addr)
{
	std::cout << "PAGE TABLE | page size " << std::dec << page_size << std::endl;
	std::cout << "           | nb entries " << std::dec << nb_entries << std::endl;
	std::cout << "           | this       0x" << std::hex <<  this << std::endl;
	assert(is_pow2(page_size));
	ENTRY_T_VIRT_TAG = new addr_t[nb_entries]; 
	ENTRY_T_WHO_I_AM_TAG = new addr_t[nb_entries];
	
}

PHYS_PAGE_TABLE::~PHYS_PAGE_TABLE()
{
	delete [] ENTRY_T_VIRT_TAG;
	delete [] ENTRY_T_WHO_I_AM_TAG;
} 
}}
