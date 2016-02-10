/*
 * utest.h
 *
 *  Created on: 2015. 12. 31.
 *      Author: hyunbok
 */

#ifndef SOURCE_USER_APP_DEFAULT_UTEST_H_
#define SOURCE_USER_APP_DEFAULT_UTEST_H_

#include "tch.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern tchStatus do_test_lock(const tch_core_api_t* ctx);
extern tchStatus do_test_msgq(const tch_core_api_t* ctx);
extern tchStatus do_test_mailq(const tch_core_api_t* ctx);
extern tchStatus do_test_event(const tch_core_api_t* ctx);
extern tchStatus do_test_thread(const tch_core_api_t* ctx);
extern tchStatus do_test_semaphore(const tch_core_api_t* ctx);
extern tchStatus do_test_barrier(const tch_core_api_t* ctx);
extern tchStatus do_test_rendezvous(const tch_core_api_t* ctx);


#if defined(__cplusplus)
}
#endif

#endif /* SOURCE_USER_APP_DEFAULT_UTEST_H_ */
