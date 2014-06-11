/*
 * fmo_lldabs.h
 *
 *  Created on: 2014. 4. 26.
 *      Author: innocentevil
 */

#ifndef FMO_LLDABS_H_
#define FMO_LLDABS_H_

#include "port/tch_stdtypes.h"
#include "../util/generic_data_types.h"
#include "fmo_synch.h"


#define ISTREAM_INIT                    {NULL,NULL,NULL,NULL,NULL}
#define OSTREAM_INIT                    {NULL,NULL,NULL,NULL,NULL}

#define IOSTREAM_INIT                   {\
	                                      NULL,\
	                                      MTX_INIT,\
	                                      MTX_INIT,\
	                                      0,\
	                                      0,\
	                                      0,\
	                                      0,\
	                                      ISTREAM_INIT,\
	                                      OSTREAM_INIT,\
	                                      GENERIC_LIST_QUEUE_INIT,\
	                                      GENERIC_LIST_QUEUE_INIT}



typedef struct _tch_ostream_t tch_ostream;
typedef struct _tch_istream_t tch_istream;
typedef struct _tch_iostream_buffer_t tch_iostream_buffer;
typedef enum {
	ThreadSafe, ThreadUnsafe
} tch_iostream_ths_opt;


struct _tch_ostream_t {
	void*                      _bp;
	BOOL (*write)(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err);
	BOOL (*putc)(tch_ostream* stream,const uint8_t c,uint8_t* err);
	uint32_t (*available)(tch_ostream* stream);
	void (*close)(tch_ostream* stream);
};

struct _tch_istream_t {
	void*                       _bp;
	BOOL (*read)(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err);
	BOOL (*getc)(tch_istream* stream,uint8_t* rb,uint8_t* err);
	uint32_t (*available)(tch_istream* stream);
	void (*close)(tch_istream* stream);
};


struct _tch_iostream_buffer_t{
	void*                        _bp;
	tch_mtx_lock                      b_rlock;
	tch_mtx_lock                      b_wlock;
	uint16_t                      b_flag;
	uint32_t                      b_size;
	uint32_t                      b_wrIdx;
	uint32_t                      b_rdIdx;
//	uint32_t                      b_updateSize;
	tch_istream                   b_istream;
	tch_ostream                   b_ostream;
	tch_genericList_queue_t       b_rdQue;
	tch_genericList_queue_t       b_wrQue;
};


BOOL tch_iostreamBuffer_init(tch_iostream_buffer* bp,void* mem,uint32_t size);
tch_istream* tch_iostreamBuffer_openInputStream(tch_iostream_buffer* bp,tch_iostream_ths_opt opt);
tch_ostream* tch_iostreamBuffer_openOutputStream(tch_iostream_buffer* bp,tch_iostream_ths_opt opt);
BOOL tch_iostreamBuffer_close(tch_iostream_buffer* bp);

#endif /* FMO_LLDABS_H_ */
