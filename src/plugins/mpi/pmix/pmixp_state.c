/*****************************************************************************\
 **  pmix_state.c - PMIx agent state related code
 *****************************************************************************
 *  Copyright (C) 2014 Institude of Semiconductor Physics Siberian Branch of
 *                     Russian Academy of Science
 *  Written by Artem Polyakov <artpol84@gmail.com>.
 *  All rights reserved.
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

#include "pmixp_common.h"
#include "pmixp_debug.h"
#include "pmixp_info.h"
#include "pmixp_state.h"
#include "pmixp_db.h"

pmixp_state_t _pmixp_state;
// Deferred requests
List cli_req_sent, cli_req_wait; // TODO: use hash table instead of list?
// The array of lists.
// i'th element - list of requests for i'th local client
List *srv_req;

void pmixp_state_init()
{
	size_t size, i;
#ifndef NDEBUG
	_pmixp_state.magic = PMIX_STATE_MAGIC;
#endif
	_pmixp_state.cli_size = pmixp_info_ltasks();
	size = _pmixp_state.cli_size * sizeof(client_state_t);
	_pmixp_state.cli_state = xmalloc( size );
	size = _pmixp_state.cli_size * sizeof(List *);
	srv_req = xmalloc( size );
	for( i = 0; i < _pmixp_state.cli_size; i++ ){
		_pmixp_state.cli_state[i].state = PMIX_CLI_UNCONNECTED;
		_pmixp_state.cli_state[i].localid = i;
		srv_req[i] = list_create(pmixp_xfree_buffer);
	}
	cli_req_wait = list_create(pmixp_xfree_buffer);
	cli_req_sent = list_create(pmixp_xfree_buffer);
/*
	_pmixp_state.coll.state = PMIX_COLL_SYNC;
	_pmixp_state.coll.local_joined = 0;
	_pmixp_state.coll.nodes_joined = 0;
	_pmixp_state.coll.local_contrib = xmalloc(sizeof(uint8_t) * pmixp_info_ltasks());
	memset(_pmixp_state.coll.local_contrib, 0, sizeof(uint8_t) * pmixp_info_ltasks());
	_pmixp_state.coll.nodes_contrib = xmalloc(sizeof(uint8_t) * pmix_info_childs());
	memset(_pmixp_state.coll.nodes_contrib, 0, sizeof(uint8_t) * pmix_info_childs());
*/
}

/*
static bool _prepare_new_coll(uint32_t gen, int idx)
{

	// If we are in synced state - add 1 to the next generation counter
	// It will be updated once
	pmix_db_start_update();
	uint32_t my_gen = pmix_db_generation_next();
	if( my_gen != gen){
		// TODO: respond with error!
		char *p = pmixp_info_nth_child_name(idx);
		PMIXP_ERROR("%s [%d]: Inconsistent contribution from node %s [%d]: data generation mismatch",
				   pmix_info_this_host(), pmix_info_nodeid(), p, pmix_info_nth_child(idx) );
		xfree(p);
		return false;
	}
	return true;
}
*/

/*
// Check events
static int _coll_new_task_contrib()
{
	switch( pmixp_state.coll.state ){
	case PMIX_COLL_SYNC:
		PMIXP_DEBUG("Start collective");
		pmixp_state.coll.state = PMIX_COLL_GATHER;
	case PMIX_COLL_GATHER:
		PMIXP_DEBUG("New contribution");
		return SLURM_SUCCESS;
	case PMIX_COLL_FORWARD:
		// This is not ok. Task shouldn't contribute during forward phase
		PMIXP_ERROR_NO(0,"New task contribution during FORWARD phase");
		return SLURM_ERROR;
	default:
		PMIXP_ERROR("pmix_state.coll.state has incomplete value %d", pmixp_state.coll.state);
		xassert( 0 );
		return SLURM_ERROR;
	}
}
*/

/*
// Check events
static int _coll_new_node_contrib()
{
	switch( pmixp_state.coll.state ){
	case PMIX_COLL_SYNC:
		PMIXP_DEBUG("Start collective");
		pmixp_state.coll.state = PMIX_COLL_GATHER;
	case PMIX_COLL_GATHER:
		PMIXP_DEBUG("New contribution");
		return SLURM_SUCCESS;
	case PMIX_COLL_FORWARD:
		// Node might contribute during forward phase. This may happen if our children receives
		// broadcast from the srun before us and somehow enters next Fence while this node
		// still waiting for the previous Fence result.
		// The reason is that upward and downward flows are implemented differently.
		PMIXP_DEBUG("NOTE: New node contribution during FORWARD phase");
		return SLURM_SUCCESS;
	default:
		PMIXP_ERROR("pmix_state.coll.state has incomplete value %d", pmixp_state.coll.state);
		xassert( 0 );
		return SLURM_ERROR;
	}
}
*/

// Check events
static int _coll_forward()
{
	/*
	switch( pmixp_state.coll.state ){
	case PMIX_COLL_SYNC:
		PMIXP_ERROR("Inconsistency: can't go to FORWARD from SYNC state");
		return SLURM_ERROR;
	case PMIX_COLL_GATHER:
		pmixp_state.coll.state = PMIX_COLL_FORWARD;
		PMIXP_DEBUG("Transit to FORWARD state");
		return SLURM_SUCCESS;
	case PMIX_COLL_FORWARD:
		PMIXP_ERROR("FORWARD phase was already enabled");
		return SLURM_ERROR;
	default:
		PMIXP_ERROR("pmix_state.coll.state has incomplete value %d", pmixp_state.coll.state);
		xassert( 0 );
		return SLURM_ERROR;
	}
	*/
	return SLURM_SUCCESS;
}

// Check events
static int _coll_sync()
{
	/*
	pmix_db_commit();
	switch( pmixp_state.coll.state ){
	case PMIX_COLL_SYNC:
		PMIXP_ERROR("SYNC phase is already enabled");
		return SLURM_ERROR;
	case PMIX_COLL_GATHER:
		PMIXP_ERROR("Cannot transit from GATHER phase to SYNC phase");
		return SLURM_ERROR;
	case PMIX_COLL_FORWARD:
		PMIXP_DEBUG("Go to SYNC state");
		pmixp_state.coll.state = PMIX_COLL_SYNC;
		// Check if we seen new contributions from our childrens
		if( pmixp_state.coll.nodes_joined > 0 ){
			// Next Fence already started

			// 1. Emulate node contribution (need just one such emulation)
			if( SLURM_SUCCESS != _coll_new_node_contrib() ){
				return SLURM_ERROR;
			}
			// 2. Prepare database to the next collective
			if( !_prepare_new_coll(pmix_db_generation()+1,0) ){
				return SLURM_ERROR;
			}
		}
		return SLURM_SUCCESS;
	default:
		PMIXP_ERROR("pmix_state.coll.state has incomplete value %d", pmixp_state.coll.state);
		xassert( 0 );
		return SLURM_ERROR;
	}
	*/
	return SLURM_SUCCESS;
}

bool pmix_state_node_contrib_ok(uint32_t gen, int idx)
{
	/*
	// Check state consistence
	if( _coll_new_node_contrib() ){
		// TODO: respond with error!
		char *p = pmixp_info_nth_child_name(idx);
		PMIXP_ERROR("%s [%d]: Inconsistent contribution from node %s [%d]",
				   pmix_info_this_host(), pmix_info_nodeid(), p, pmix_info_nth_child(idx) );
		xfree(p);
		return false;
	}

	// Initiate new collective only if we are in synced state
	// Otherwise - just save this contribution
	if( pmixp_state.coll.state == PMIX_COLL_FORWARD ){
		// Check that DB generation matches our expectations.
		if( gen != pmix_db_generation() + 2) {
			// TODO: respond with error!
			char *p = pmixp_info_nth_child_name(idx);
			PMIXP_ERROR("%s [%d]: Inconsistent contribution from node %s [%d]: data generation mismatch",
					   pmix_info_this_host(), pmix_info_nodeid(), p, pmix_info_nth_child(idx) );
			xfree(p);
			return false;
		}
	} else {
		if( !_prepare_new_coll(gen, idx) ){
			return false;
		}
	}

	if( pmixp_state.coll.nodes_contrib[idx] ){
		// TODO: respond with error!
		char *p = pmixp_info_nth_child_name(idx);
		PMIXP_ERROR("%s [%d]: Node %s [%d] already contributed to the collective",
				   pmix_info_this_host(), pmix_info_nodeid(), p, pmix_info_nth_child(idx) );
		xfree(p);
		return false;
	}

	pmixp_state.coll.nodes_contrib[idx] = 1;
	pmixp_state.coll.nodes_joined++;
	*/
	return true;
}

bool pmix_state_task_contrib_ok(int idx, bool blocking)
{
	/*
	// Check state consistence
	if( _coll_new_task_contrib() ){
		PMIXP_ERROR("%s [%d]: Inconsistent contribution from task %d",
				   pmix_info_this_host(), pmix_info_nodeid(), pmixp_info_task_id(idx) );
		return false;
	}

	pmix_db_start_update();

	if( pmixp_state.coll.local_contrib[idx] ){
		PMIXP_ERROR("%s [%d]: Task %d already contributed to the collective",
				   pmix_info_this_host(), pmix_info_nodeid(), pmixp_info_task_id(idx) );
		return false;
	}

	if( blocking ){
		pmixp_state.cli_state[idx].state = PMIX_CLI_COLL;
	} else {
		pmixp_state.cli_state[idx].state = PMIX_CLI_COLL_NB;
	}
	pmixp_state.coll.local_contrib[idx] = 1;
	pmixp_state.coll.local_joined++;
	*/
	return true;
}

bool pmix_state_coll_local_ok()
{
	return (_pmixp_state.coll.local_joined == pmixp_info_ltasks() ) &&
			(_pmixp_state.coll.nodes_joined == pmix_info_childs());
}


bool pmix_state_node_contrib_cancel(int idx)
{
	/*
	// Check state consistence
	if( pmixp_state.coll.state != PMIX_COLL_FORWARD ){
		PMIXP_DEBUG("WARNING: trying to cancel contrib for node %s [%d] during wrong phase %d\n",
				   pmixp_info_nth_child_name(idx), pmix_info_nth_child(idx), pmixp_state.coll.state );
		return false;
	}
	// We need to contribute before we cancel!
	xassert(pmixp_state.coll.nodes_contrib[idx]);
	pmixp_state.coll.nodes_contrib[idx] = 0;
	pmixp_state.coll.nodes_joined--;
	xassert(pmixp_state.coll.nodes_joined >=0 );
*/
	return true;
}

bool pmix_state_task_contrib_cancel(int idx)
{
	// Check state consistence
	if( _pmixp_state.coll.state != PMIX_COLL_FORWARD ){
		PMIXP_DEBUG("%s [%d]: WARNING: trying to cancel contrib for task [%d] during wrong phase %d\n",
				   pmix_info_this_host(), pmix_info_nodeid(), pmixp_info_task_id(idx), _pmixp_state.coll.state );
		return false;
	}
	// We need to contribute before we cancel!
	xassert(_pmixp_state.coll.local_contrib[idx]);
	_pmixp_state.coll.local_contrib[idx] = 0;
	_pmixp_state.coll.local_joined--;
	xassert(_pmixp_state.coll.local_joined >=0 );
	return true;
}

bool pmix_state_coll_forwad()
{
	if( _coll_forward() ){
		PMIXP_ERROR("Cannot transit to FORWARD state!");
		return false;
	}
	return true;
}

bool pmix_state_coll_sync()
{
	if( _coll_sync() ){
		PMIXP_ERROR("Cannot transit to SYNC state!");
		return false;
	}
	return true;
}

typedef struct {
	uint32_t localid;
	uint32_t taskid;
} deferred_t;


/*
 * Mark dmdx request as being sent
 */
bool pmix_state_remote_sent(uint32_t taskid)
{
	uint32_t *ptr = NULL;
	ListIterator i;

	i  = list_iterator_create(cli_req_sent);
	while ((ptr = list_next(i))) {
		if ( *ptr == taskid ) {
			// request about this taskid was already sent
			return true;
		}
	}
	list_iterator_destroy(i);
	ptr = xmalloc(sizeof(uint32_t));
	*ptr = taskid;
	list_enqueue(cli_req_sent,ptr);
	// The request wasn't sent
	return false;
}

/*
 * Check if we already sent the request to this taskid
 */
void pmix_state_remote_received(uint32_t taskid)
{
	uint32_t *ptr = NULL;
	ListIterator i;

	i  = list_iterator_create(cli_req_sent);
	while ((ptr = list_next(i))) {
		if ( *ptr == taskid ) {
			list_delete_item(i);
			break;
		}
	}
	list_iterator_destroy(i);
}

/*
 * Mark that we wait for the responce about the taskid.
 */

void pmix_state_remote_wait(uint32_t localid, uint32_t taskid)
{
	deferred_t *elem = xmalloc( sizeof(deferred_t) );
	elem->localid = localid;
	elem->taskid = taskid;
	// TODO: use hash table here (key = src_gid)
	list_enqueue(cli_req_wait,elem);
}

List pmix_state_remote_to(uint32_t taskid)
{
	List ret = list_create(pmixp_xfree_buffer);
	ListIterator i;
	deferred_t *elem;

	i  = list_iterator_create(cli_req_wait);
	while ((elem = list_next(i))) {
		if ( elem->taskid == taskid ) {
			uint32_t *ptr = xmalloc(sizeof(uint32_t));
			*ptr = elem->localid;
			list_append(ret,ptr);
			list_delete_item(i);
		}
	}
	list_iterator_destroy(i);
	return ret;
}

List pmix_state_remote_from(uint32_t localid)
{
	List ret = list_create(pmixp_xfree_buffer);
	ListIterator i;
	deferred_t *elem;

	i  = list_iterator_create(cli_req_wait);
	while ((elem = list_next(i))) {
		if ( elem->localid == localid ) {
			uint32_t *ptr = xmalloc(sizeof(uint32_t));
			*ptr = elem->taskid;
			list_append(ret,ptr);
			list_delete_item(i);
		}
	}
	list_iterator_destroy(i);
	return ret;
}

/*
 *  Remote process dst_gid requests blob of the local process with id = src_lid.
 */
void pmix_state_local_defer(uint32_t src_lid, uint32_t nodeid)
{
	xassert( src_lid < _pmixp_state.cli_size );
	int *ptr = xmalloc( sizeof(uint32_t) );
	*ptr = nodeid;
	list_enqueue(srv_req[src_lid], ptr);
}

int pmix_state_local_reqs_cnt(uint32_t localid)
{
	return list_count(srv_req[localid]);
}

List pmix_state_local_reqs_to(uint32_t localid)
{
	List ret = srv_req[localid];
	srv_req[localid] = list_create(pmixp_xfree_buffer);
	return ret;
}
