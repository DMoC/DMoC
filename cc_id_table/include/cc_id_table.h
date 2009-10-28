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
 * Maintainers: Pierre Guironnet de Massas
 */
#ifndef SOCLIB_CC_ID_TABLE_H
#define SOCLIB_CC_ID_TABLE_H

#include <map>
#include "int_tab.h"
namespace soclib {
namespace common {

class CcIdTable
{
public :

    static void register_me(unsigned int id, unsigned int i_index, soclib::common::IntTab t_index);
    static soclib::common::IntTab translate_to_target(unsigned int id);
    static unsigned int translate_to_id(unsigned int i_index);

    // Used to create the singleton
    static CcIdTable * CreateCcIdTable(void);

    ~CcIdTable(void);
private :
    // This structure will be used to save the source and target ids on the NoC of a processor
    // absolute id (ie. between 0 and p-1, p : nbr of processors in the system)
    static CcIdTable * unique_ref;
    static std::map<unsigned int , soclib::common::IntTab > * relation_id_target_map;
    static std::map<unsigned int ,unsigned int > * relation_srcid_id_map;
    CcIdTable(void);
};


}}

#endif /* SOCLIB_CC_ID_TABLE_H */

// Local Variables:
// tab-width: 4
// c-basic-offset: 4
// c-file-offsets:((innamespace . 0)(inline-open . 0))
// indent-tabs-mode: nil
// End:

// vim: filetype=cpp:expandtab:shiftwidth=4:tabstop=4:softtabstop=4

