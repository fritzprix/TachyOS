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

extern tchStatus do_test_lock(const tch* ctx);
extern tchStatus do_test_msgq(const tch* ctx);
extern tchStatus do_test_mailq(const tch* ctx);
extern tchStatus do_test_event(const tch* ctx);
extern tchStatus do_test_thread(const tch* ctx);
extern tchStatus do_test_semaphore(const tch* ctx);
extern tchStatus do_test_barrier(const tch* ctx);
extern tchStatus do_test_rendezvous(const tch* ctx);


#if defined(__cplusplus)
}
#endif

#endif /* SOURCE_USER_APP_DEFAULT_UTEST_H_ */
