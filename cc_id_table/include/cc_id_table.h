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

// This component is used to provide a set of T-uplets < Absolute_id, NoC_source_id, NoC_target_id > as
// follow :

// Absolute_id : an ID beetween [-1.. max_coherent_id] of an initiator that can access a vci_cc/mcc_ram. This ID
//             is used to access directory and consequently caches and other "coherent initiators" (??) have
//             an ID comprised in [0.. max_coherent_id[. The ID of any component that does'nt support coherence
//             is set to -1  (for example the fd_access).

// NoC_source_id (i_index) : The flat index of an initiator (vci srcid field) which can be obtained 
//                 through the method ,maptab.indexForId(IntTab(... of an initiator ...)) 

// NoC_target_id (t_index) : The intTab value in the mapping table of the TARGET interface of this component (ie. where
//                 to send invalidations if requiered).

class CcIdTable
{
public :

		// Register a new T-uplet for a coherent initiator (cc_cache etc..)
    void register_coherent_initiator(unsigned int i_index, soclib::common::IntTab t_index);

		// Register a new T-uplet for a non coherent initiator (fd_access etc..)
    void register_non_coherent_initiator(unsigned int i_index);

		// Retrieve a target IntTab from an Absolute Id
    soclib::common::IntTab translate_to_target(unsigned int id);

		// Retrieve an Absolute Id from a NoC_source_id.,
    // returns -1 if this initiator is "non coherent"
    // returns [0..max_coherent_id[ if the initiator is a "coherent" one.
    int translate_to_id(unsigned int i_index);

    CcIdTable(void);
    ~CcIdTable(void);
private :
    std::map<unsigned int , soclib::common::IntTab > * relation_cc_id_target_map;
    std::map<unsigned int ,unsigned int > * relation_srcid_id_map;
		int coherent_id;
		unsigned int max_coherent_id;
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

