/*
 * quantum.h
 *
 *  Created on: Jun 1, 2016
 *      Author: innocentevil
 */

#ifndef INCLUDE_QUANTUM_H_

#define INCLUDE_QUANTUM_H_


#include "owtree.h"
#include "cdsl_nrbtree.h"
#include "cdsl_slist.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef QUANTUM_SPACE
#define QUANTUM_SPACE           ((uint16_t) 2)
#endif

#ifndef QUANTUM_MAX
#define QUANTUM_MAX             ((uint16_t) 32)
#endif

#ifndef QUANTUM_COUNT_MAX
#define QUANTUM_COUNT_MAX		((uint16_t) 2048)
#endif


typedef struct {
	nrbtreeRoot_t    addr_rbroot;
	nrbtreeRoot_t    quantum_tree;
	owtreeRoot_t      quantum_pool;
	slistEntry_t     clr_lentry;
	wt_adapter       adapter;
}quantumRoot_t;

extern void quantum_root_init(quantumRoot_t* root,wt_map_func_t mapper, wt_unmap_func_t unmapper);
extern void* quantum_reclaim_chunk(quantumRoot_t* root, ssize_t sz);
extern size_t quantum_get_chunk_size(quantumRoot_t* root, void* chunk);
extern int quantum_free_chunk(quantumRoot_t* root,void* chunk);
extern void quantum_try_purge_cache(quantumRoot_t* root);
extern void quantum_cleanup(quantumRoot_t* root);
extern void quantum_print(quantumRoot_t* root);
extern size_t quantum_size(quantumRoot_t* root, void* chunk);


#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_QUANTUM_H_ */
