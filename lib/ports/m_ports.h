/*******************************************************************************
 *  Concrete Server                                                            *
 *  Copyright (c) 2005-2019 Raphael Prevost <raph@el.bzh>                      *
 *                                                                             *
 *  This software is a computer program whose purpose is to provide a          *
 *  framework for developing and prototyping network services.                 *
 *                                                                             *
 *  This software is governed by the CeCILL  license under French law and      *
 *  abiding by the rules of distribution of free software.  You can  use,      *
 *  modify and/ or redistribute the software under the terms of the CeCILL     *
 *  license as circulated by CEA, CNRS and INRIA at the following URL          *
 *  "http://www.cecill.info".                                                  *
 *                                                                             *
 *  As a counterpart to the access to the source code and  rights to copy,     *
 *  modify and redistribute granted by the license, users are provided only    *
 *  with a limited warranty  and the software's author,  the holder of the     *
 *  economic rights,  and the successive licensors  have only  limited         *
 *  liability.                                                                 *
 *                                                                             *
 *  In this respect, the user's attention is drawn to the risks associated     *
 *  with loading,  using,  modifying and/or developing or reproducing the      *
 *  software by the user in light of its specific status of free software,     *
 *  that may mean  that it is complicated to manipulate,  and  that  also      *
 *  therefore means  that it is reserved for developers  and  experienced      *
 *  professionals having in-depth computer knowledge. Users are therefore      *
 *  encouraged to load and test the software's suitability as regards their    *
 *  requirements in conditions enabling the security of their systems and/or   *
 *  data to be ensured and,  more generally, to use and operate it in the      *
 *  same conditions as regards security.                                       *
 *                                                                             *
 *  The fact that you are presently reading this means that you have had       *
 *  knowledge of the CeCILL license and that you accept its terms.             *
 *                                                                             *
 ******************************************************************************/

#ifndef M_PORTS_H

#define M_PORTS_H

#define CONCRETE_MINIMAL
#include "../m_core_def.h"
#undef CONCRETE_MINIMAL

#if defined(_USE_BIG_FDS)
    #if ( (_USE_BIG_FDS > FD_SETSIZE) && defined(HAS_POLL) )
        /* can not allow more than SOCKET_MAX descriptors */
        #if (_USE_BIG_FDS > 0x0FFF)
            #warning "Can not allow more than SOCKET_MAX descriptors."
            #undef _USE_BIG_FDS
            #define _USE_BIG_FDS 0x0FFF
        #endif

        /* redefine FD_SETSIZE to its maximal value */
        #ifndef FD_SETSIZE
            #define FD_SETSIZE _USE_BIG_FDS
        #endif

        /* this ugly hack is necessary with glibc */
        #ifdef __linux__
            #include <bits/types.h>
            #undef __FD_SETSIZE
            #define __FD_SETSIZE _USE_BIG_FDS
        #endif
    #endif
#endif

#include "m_port_std.h"
#include "m_port_file.h"
#include "m_port_time.h"
#include "m_port_mmap.h"
#include "m_port_socket.h"
#include "m_port_plugin.h"

#endif
