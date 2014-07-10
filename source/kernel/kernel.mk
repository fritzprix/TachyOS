KERNEL_SRCS=\
	    tch_ipc.c\
	    tch_mpool.c\
	    tch_mtx.c\
	    tch_sched.c\
	    tch_sig.c\
	    tch_sys.c\
	    tch_thread.c\
	    tch_vtimer.c

KERNEL_OBJS=$(KERNEL_SRCS:%.c=%.o)


