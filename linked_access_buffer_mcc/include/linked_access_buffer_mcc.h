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
 *         Pierre Guironnet de Massas <pierre.guironnet-de-massas@imag.fr>
 *
 * Maintainers: Pierre
 */
#ifndef SOCLIB_COMMON_LINKED_ACCESS_BUFFER_MCC_H_
#define SOCLIB_COMMON_LINKED_ACCESS_BUFFER_MCC_H_

#include <sys/types.h>
#include <stdint.h>
#include <assert.h>
#include <systemc>

namespace soclib {
namespace common {

template<typename addr_t, typename id_t>
struct LinkedAccessEntryPlus
{
	addr_t address;
	bool atomic;

	inline void invalidate()
	{
		address = 0;
		atomic = false;
	}
	inline void invalidate(addr_t addr)
	{
        if ( address == addr )
            atomic = false;
	}
	inline void invalidate_page(unsigned int page_index, unsigned int page_shift)
	{
        if ( (address >> page_shift) == page_index )
            atomic = false;
	}
    inline void set( addr_t addr )
    {
        address = addr;
        atomic = true;
    }
    inline bool is_atomic( addr_t addr ) const
    {
        return (address == addr) && atomic;
    }
};

template<typename addr_t, typename id_t>
class LinkedAccessBuffer_mcc
{
	typedef LinkedAccessEntryPlus<addr_t, id_t> entry_t;
	entry_t * const m_access;
	const size_t m_n_entry;
	unsigned int m_page_shift;
public:
	LinkedAccessBuffer_mcc( size_t n_entry, unsigned int page_shift );
	~LinkedAccessBuffer_mcc();

	void clearAll()
    {
        for ( size_t i=0; i<m_n_entry; ++i )
            m_access[i].invalidate();
    }

	void clearPage(unsigned int page_index )
    {
		assert(m_page_shift != 0);
        for ( size_t i=0; i<m_n_entry; ++i )
            m_access[i].invalidate_page(page_index, m_page_shift);
    }
	void accessDone( addr_t address )
    {
        for ( size_t i=0; i<m_n_entry; ++i )
            m_access[i].invalidate(address);
    }

	void doLoadLinked( addr_t address, id_t id )
    {
        assert(id < m_n_entry && "Access out of bounds");
        m_access[id].set(address);
    }

	bool isAtomic( addr_t address, id_t id ) const
    {
        assert(id < m_n_entry && "Access out of bounds");
        return m_access[id].is_atomic(address);
    }
};

}}

#endif /* SOCLIB_COMMON_LINKED_ACCESS_BUFFER_MCC_H_ */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

