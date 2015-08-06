/*****************************************************************************\
 **  pmix_server.h - PMIx server side functionality
 *****************************************************************************
 *  Copyright (C) 2014-2015 Artem Polyakov. All rights reserved.
 *  Copyright (C) 2015      Mellanox Technologies. All rights reserved.
 *  Written by Artem Polyakov <artpol84@gmail.com>.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <http://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#ifndef PMIXP_SERVER_H
#define PMIXP_SERVER_H

#include "pmixp_common.h"

typedef enum {
	PMIXP_MSG_FAN_IN,
	PMIXP_MSG_FAN_OUT,
	PMIX_MSG_DMDX
} pmixp_srv_cmd_t;


int pmixp_stepd_init(const stepd_step_rec_t *job, char ***env);
int pmixp_stepd_finalize();
int pmix_srun_init(const mpi_plugin_client_info_t *job, char ***env);
void pmix_server_new_conn(int fd);
int pmixp_server_send(char *hostlist, pmixp_srv_cmd_t type, uint32_t seq,
		      const char *addr, void *data, size_t size);

#endif // PMIXP_SERVER_H
