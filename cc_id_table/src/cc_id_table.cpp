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
 * Maintainers: Pierre Guironnet de Massas
 */


#include "cc_id_table.h"
#include <iostream>
#include <cassert>

namespace soclib {
namespace common {
    // Initialisation des membres statiques
    CcIdTable * CcIdTable::unique_ref(NULL);
    std::map<unsigned int ,  soclib::common::IntTab >  * CcIdTable::relation_id_target_map(NULL);
    std::map<unsigned int , unsigned int >  * CcIdTable::relation_srcid_id_map(NULL);
    
    CcIdTable::CcIdTable(void)
    {
        relation_id_target_map = new std::map<unsigned int ,  soclib::common::IntTab >;
        relation_srcid_id_map  = new std::map<unsigned int , unsigned int >;
    }

    CcIdTable::~CcIdTable(void)
    {
    }

    void CcIdTable::register_me(unsigned int id, unsigned int i_index, soclib::common::IntTab t_index)
    {
        assert((*relation_id_target_map).find(id) == relation_id_target_map->end());
        assert((*relation_srcid_id_map).find(i_index) == relation_srcid_id_map->end());
        (*relation_id_target_map)[id] = t_index;
        (*relation_srcid_id_map)[i_index] = id;

    }
    unsigned int CcIdTable::translate_to_id(unsigned int i_index)
    {
        assert((*relation_srcid_id_map).find(i_index) != relation_srcid_id_map->end());
        return (*relation_srcid_id_map)[i_index];
    }
    soclib::common::IntTab CcIdTable::translate_to_target(unsigned int id)
    {
        assert((*relation_id_target_map).find(id) != relation_id_target_map->end());
        return (*relation_id_target_map)[id];
    }

    CcIdTable * CcIdTable::CreateCcIdTable(void)
    {
        if (unique_ref == NULL)
        {
            unique_ref = new CcIdTable;
        }
        else
        {
            std::cerr << "Error, trying to create multiple instances of CcIdTable" << std::endl; 
        }
        return unique_ref;
    }
}}

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

