/*
 * tch_usig.c
 *
 *  Created on: 2014. 11. 29.
 *      Author: innocentevil
 */


#include "tch_usig.h"
#include "tch_kernel.h"
#include "tch_thread.h"


static tch_sigFuncPtr tch_uSignalSet(int sig,tch_sigFuncPtr fn);
static int tch_uSignalRaise(tch_threadId thread,int sig,uint32_t arg);

static void tch_uSignalKill(int sig,uint32_t arg);
static void tch_uSignalNoop(int sig,uint32_t arg);
static void __tch_uSignalHandler(tch_sigFuncPtr fn,int sig,uint32_t arg) __attribute__((naked));

__attribute__((section(".data"))) static tch_usignal_ix uSig_StaticInstance = {
		tch_uSignalSet,
		tch_uSignalRaise
};

const tch_usignal_ix* uSig = &uSig_StaticInstance;


static tch_sigFuncPtr tch_uSignalSet(int sig,tch_sigFuncPtr fn){
	if(tch_port_isISR()){
		return tch_uSignalSetK(sig,fn);
	}
	return (tch_sigFuncPtr)tch_port_enterSvFromUsr(SV_SIG_SET,sig,(uint32_t) fn);
}


tch_sigFuncPtr tch_uSignalSetK(int sig,tch_sigFuncPtr fn){
	tch_sigFuncPtr sigPrev = tch_currentThread->t_usig.sigHdrTable[sig];
	if(fn == SIG_DFL){
		fn = (sig == SIGUSR? tch_uSignalNoop : tch_uSignalKill);
	}
	if(sig == SIGKILL)                                             /// sigkill
		return sigPrev;                                            /// can not change handler
	if(fn == SIG_IGN)
		fn = tch_uSignalNoop;
	tch_currentThread->t_usig.sigHdrTable[sig] = fn;
	return sigPrev;
}


static int tch_uSignalRaise(tch_threadId thread,int sig,uint32_t arg){
	tch_uSiganlArg sigk_arg;
	sigk_arg.sigarg = arg;
	sigk_arg.signum = sig;
	if(tch_port_isISR()){
		return tch_uSignalRaiseK(thread,&sigk_arg);
	}
	return tch_port_enterSvFromUsr(SV_SIG_RAISE,(uint32_t) thread,(uint32_t) &sigk_arg);
}


int tch_uSignalRaiseK(tch_threadId thread,tch_uSiganlArg* sig){
	tch_thread_header* th_p = (tch_thread_header*) thread;
	if(th_p->t_flag & THREAD_SIG_BIT){
		return -1;
	}
	th_p->t_usig.sig = sig->signum;
	th_p->t_usig.sig_arg = sig->sigarg;
	th_p->t_flag |= THREAD_SIG_BIT;
	return sig->signum;
}

void tch_usigInit(tch_usigHandle* sig){
	sig->sig = 0;
	sig->sig_arg = NULL;
	sig->sigHdrTable[SIGKILL] = tch_uSignalKill;
	sig->sigHdrTable[SIGINT] = tch_uSignalKill;
	sig->sigHdrTable[SIGSEGV] = tch_uSignalKill;
	sig->sigHdrTable[SIGUSR] = tch_uSignalNoop;
}

void tch_uSignalJmpToHanlder(tch_threadId thread){
	tch_thread_header* th_p = (tch_thread_header*) thread;
	if(!(th_p->t_flag & THREAD_SIG_BIT))
		return;
	int signum = th_p->t_usig.sig;
	th_p->t_flag &= ~THREAD_SIG_BIT;                             // mark signal handled
	tch_port_jmpToKernelModeThread(__tch_uSignalHandler,(uword_t)th_p->t_usig.sigHdrTable[signum],(uword_t)signum,(uword_t)th_p->t_usig.sig_arg);
}

static void tch_uSignalKill(int sig,uint32_t arg){
	// destroy & release used system resources
	tch_mem_ix* mem = NULL;
	tch_thread_header* th_p = tch_currentThread;
	tch_thread_header* ch_p = NULL;
	uStdLib->stdio->iprintf("\rThread (%s) Exit with (%d)\n",th_p->t_name,(int)arg);
	uMem->forceRelease(th_p);
	shMem->forceRelease(th_p);
	if(th_p->t_flag & THREAD_ROOT_BIT){
		while(!tch_listIsEmpty(th_p->t_refNode.childs)){
			ch_p = (tch_thread_header*)((uint32_t) tch_listDequeue(th_p->t_refNode.childs) - 3 * sizeof(tch_lnode_t));
			if(ch_p){
				uSig->raise(ch_p,SIGKILL,arg);
				Thread->join(ch_p,osWaitForever);        // wait child termination
			}
		}
		tch_listRemove((tch_lnode_t*)&tch_procList,&th_p->t_childNode);
		mem = (tch_mem_ix*)kMem;
	}else{
		tch_listRemove((tch_lnode_t*)((tch_thread_header*)th_p->t_refNode.root)->t_refNode.childs,&th_p->t_childNode);
		mem = (tch_mem_ix*)uMem;
		tch_currentThread = tch_currentThread->t_refNode.root;
	}
	mem->free(th_p->t_chks);
	tch_port_enterSvFromUsr(SV_THREAD_TERMINATE,(uint32_t) th_p,arg);
}

static void tch_uSignalNoop(int sig,uint32_t arg){
	return;
}


__attribute__((naked)) static void __tch_uSignalHandler(tch_sigFuncPtr fn,int sig,uint32_t arg) {
	tch_port_kernel_unlock();
	fn(sig,arg);
	tch_port_kernel_lock();
	asm volatile(
			"ldr r0,=%0\n"
			"svc #0" : : "i"(SV_EXIT_FROM_SV) :);
	/*
	asm volatile(
#ifdef MFEATURE_HFLOAT
			"vpush {s16-s31}\n"
#endif
			"push {r4-r11,lr}\n"                 ///< save thread context in the thread stack
			"str sp,[%0]\n"                      ///< store
			: : "r"(&tch_currentThread->t_ctx):);

	tch_port_kernel_unlock();
	fn(sig,arg);
	tch_port_kernel_lock();
	asm volatile(
			"ldr sp,[%0]\n"
			"pop {r4-r11,lr}\n"
#ifdef MFEATURE_HFLOAT
			"vpop {s16-s31}\n"
#endif
			"ldr r0,=%1\n"
			"svc #0" : : "r"(&tch_currentThread->t_ctx),"i"(SV_EXIT_FROM_SV):);
			*/
}

