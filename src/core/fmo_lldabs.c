/*
 * fmo_lldabs.c
 *
 *  Created on: 2014. 4. 26.
 *      Author: innocentevil
 */


#include "fmo_lldabs.h"
#include "fmo_thread.h"


#ifndef IOBUFFER_TRY_TIMEOUT
#define IOBUFFER_TRY_TIMEOUT                                (uint32_t) 2000
#endif

#ifdef FEATURE_MTHREAD
#define SB_RD_LOCK(sbp)     {tch_Mtx_lockt(&sbp->b_rlock,TRUE);}
#define SB_WR_LOCK(sbp)     {tch_Mtx_lockt(&sbp->b_wlock,TRUE);}
#define SB_RD_UNLOCK(sbp)   {tch_Mtx_unlockt(&sbp->b_rlock);}
#define SB_WR_UNLOCK(sbp)   {tch_Mtx_unlockt(&sbp->b_wlock);}
#else
#define SB_RD_LOCK(sbp)     {}
#define SB_WR_LOCK(sbp)     {}
#endif


#define STR_BUFFER_FLAG_OPEN                                (uint16_t) (1 << 0)
#define STR_BUFFER_FLAG_ICHANNEL_OPEN                       (uint16_t) (1 << 1)
#define STR_BUFFER_FLAG_OCHANNEL_OPEN                       (uint16_t) (1 << 2)
#define STR_BUFFER_FLAG_WRBUSY                              (uint16_t) (1 << 3)
#define STR_BUFFER_FLAG_RDBUSY                              (uint16_t) (1 << 4)
#define STR_BUFFER_FLAG_RDTHSAFE                            (uint16_t) (1 << 5)
#define STR_BUFFER_FLAG_WRTHSAFE                            (uint16_t) (1 << 6)

#define STR_BUFFER_SET_OPENED(strbp)                         (strbp->b_flag |= STR_BUFFER_FLAG_OPEN)
#define STR_BUFFER_CLR_OPENED(strbp)                         (strbp->b_flag &= ~STR_BUFFER_FLAG_OPEN)
#define STR_BUFFER_IS_OPENED(strbp)                          (strbp->b_flag & STR_BUFFER_FLAG_OPEN)


#define STR_BUFFER_SET_ICHANNEL_OPENED(strbp)                (strbp->b_flag |= STR_BUFFER_FLAG_ICHANNEL_OPEN)
#define STR_BUFFER_CLR_ICHANNEL_OPENED(strbp)                (strbp->b_flag &= ~STR_BUFFER_FLAG_ICHANNEL_OPEN)
#define STR_BUFFER_IS_ICHANNEL_OPENED(strbp)                 (strbp->b_flag & STR_BUFFER_FLAG_ICHANNEL_OPEN)

#define STR_BUFFER_SET_OCHANNEL_OPENED(strbp)                (strbp->b_flag |= STR_BUFFER_FLAG_OCHANNEL_OPEN)
#define STR_BUFFER_CLR_OCHANNEL_OPENED(strbp)                (strbp->b_flag &= ~STR_BUFFER_FLAG_OCHANNEL_OPEN)
#define STR_BUFFER_IS_OCHANNEL_OPENED(strbp)                 (strbp->b_flag & STR_BUFFER_FLAG_OCHANNEL_OPEN)

#define STR_BUFFER_SET_WRBUSY(strbp)                         (strbp->b_flag |= STR_BUFFER_FLAG_WRBUSY)
#define STR_BUFFER_CLR_WRBUSY(strbp)                         (strbp->b_flag &= ~STR_BUFFER_FLAG_WRBUSY)
#define STR_BUFFER_IS_WRBUSY(strbp)                          (strbp->b_flag & STR_BUFFER_FLAG_WRBUSY)

#define STR_BUFFER_SET_RDBUSY(strbp)                         (strbp->b_flag |= STR_BUFFER_FLAG_RDBUSY)
#define STR_BUFFER_CLR_RDBUSY(strbp)                         (strbp->b_flag &= ~STR_BUFFER_FLAG_RDBUSY)
#define STR_BUFFER_IS_RDBUSY(strbp)                          (strbp->b_flag & STR_BUFFER_FLAG_RDBUSY)

#define STR_BUFFER_SET_RDTHSAFE(strbp)                         (strbp->b_flag |= STR_BUFFER_FLAG_RDTHSAFE)
#define STR_BUFFER_CLR_RDTHSAFE(strbp)                         (strbp->b_flag &= ~STR_BUFFER_FLAG_RDTHSAFE)
#define STR_BUFFER_IS_RDTHSAFE(strbp)                          (strbp->b_flag & STR_BUFFER_FLAG_RDTHSAFE)

#define STR_BUFFER_SET_WRTHSAFE(strbp)                         (strbp->b_flag |= STR_BUFFER_FLAG_WRTHSAFE)
#define STR_BUFFER_CLR_WRTHSAFE(strbp)                         (strbp->b_flag &= ~STR_BUFFER_FLAG_WRTHSAFE)
#define STR_BUFFER_IS_WRTHSAFE(strbp)                          (strbp->b_flag & STR_BUFFER_FLAG_WRTHSAFE)

static BOOL tch_ostream_write_ts(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err);
static BOOL tch_ostream_putc_ts(tch_ostream* stream,const uint8_t c,uint8_t* err);
static uint32_t tch_ostream_available(tch_ostream* stream);
static void tch_ostream_close(tch_ostream* stream);
static BOOL tch_istream_read_ts(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err);
static uint8_t tch_istream_getc_ts(tch_istream* stream,uint8_t* rb,uint8_t* err);
static uint32_t tch_istream_available(tch_istream* stream);
static void tch_istream_close(tch_istream* stream);

static BOOL tch_ostream_write_nts(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err);
static BOOL tch_ostream_putc_nts(tch_ostream* stream,const uint8_t c,uint8_t* err);
static BOOL tch_istream_read_nts(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err);
static uint8_t tch_istream_getc_nts(tch_istream* stream,uint8_t* rb,uint8_t* err);


BOOL tch_iostreamBuffer_init(tch_iostream_buffer* bp,void* mem,uint32_t size){
	bp->_bp = mem;
	bp->b_rdIdx = 0;
	bp->b_wrIdx = 0;
	bp->b_flag = 0;
//	bp->b_updateSize = 0;
	bp->b_size = size;
	bp->b_istream._bp = bp;
	bp->b_ostream._bp = bp;
	tch_Mtx_init(&bp->b_rlock);
	tch_Mtx_init(&bp->b_wlock);
	tch_genericQue_Init(&bp->b_rdQue);
	tch_genericQue_Init(&bp->b_wrQue);

	return TRUE;
}

tch_istream* tch_iostreamBuffer_openInputStream(tch_iostream_buffer* bp, tch_iostream_ths_opt opt){
	if(STR_BUFFER_IS_ICHANNEL_OPENED(bp)){
		return NULL;
	}
	STR_BUFFER_SET_ICHANNEL_OPENED(bp);

	tch_istream* istr = &bp->b_istream;
	istr->_bp = bp;
	istr->close = tch_istream_close;
	istr->available = tch_istream_available;
	if(opt == ThreadSafe){
		STR_BUFFER_SET_RDTHSAFE(bp);
		istr->getc = tch_istream_getc_ts;
		istr->read = tch_istream_read_ts;
	}else{
		istr->getc = tch_istream_getc_nts;
		istr->read = tch_istream_read_nts;
	}

	return istr;
}

tch_ostream* tch_iostreamBuffer_openOutputStream(tch_iostream_buffer* bp,tch_iostream_ths_opt opt){
	if(STR_BUFFER_IS_OCHANNEL_OPENED(bp)){
		return NULL;
	}
	STR_BUFFER_SET_OCHANNEL_OPENED(bp);

	tch_ostream* ostr = &bp->b_ostream;
	ostr->close = tch_ostream_close;
	ostr->available = tch_ostream_available;
	if(opt == ThreadSafe){
		STR_BUFFER_SET_WRTHSAFE(bp);
		ostr->putc = tch_ostream_putc_ts;
		ostr->write = tch_ostream_write_ts;
	}else{
		ostr->putc = tch_ostream_putc_nts;
		ostr->write = tch_ostream_write_nts;
	}

	return ostr;
}

BOOL tch_iostreamBuffer_close(tch_iostream_buffer* bp){
	if(STR_BUFFER_IS_ICHANNEL_OPENED(bp)){
		bp->b_istream.close(&bp->b_istream);
	}
	if(STR_BUFFER_IS_OCHANNEL_OPENED(bp)){
		bp->b_ostream.close(&bp->b_ostream);
	}
	return TRUE;
}



BOOL tch_ostream_write_ts(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	uint8_t* mp = (uint8_t*) iostr->_bp;
	uint8_t* wbp = (uint8_t*) wb;
	if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
		return FALSE;
	}
	if(!__get_IPSR()){                                     //   check if function called in thread or handler mode
		if(STR_BUFFER_IS_WRBUSY(iostr)){                   //   check another thread is writing this stream
			tchThread_sleep(0);                             //   then wait for the other thread finish its write op.
			if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
				return FALSE;
			}
			// check stream is closed
		}
		SB_WR_LOCK(iostr);
		if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
			SB_WR_UNLOCK(iostr);
			return FALSE;
		}
		// check stream is closed
		STR_BUFFER_SET_WRBUSY(iostr);                      //   set write busy
		SB_WR_UNLOCK(iostr);
		while(size--){
			while(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){   //   check updated data is consumed
				tchThread_wait(&iostr->b_wrQue);            //   or wait in write queue
				if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
					return FALSE;
				}
				// check stream is closed
			}
			*(mp + iostr->b_wrIdx++) = *wbp++;
			if(iostr->b_wrIdx == iostr->b_size){
				iostr->b_wrIdx = 0;
			}
//			iostr->b_updateSize++;
		}
		while(iostr->b_rdQue.entry){
			tchThread_wake(&iostr->b_rdQue);
		}
		STR_BUFFER_CLR_WRBUSY(iostr);
		return TRUE;

	}else{
		while(size--){
			if(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){
				return FALSE;
			}
			*(mp + iostr->b_wrIdx++) = *wbp++;
			if(iostr->b_wrIdx == iostr->b_size){
				iostr->b_wrIdx = 0;
			}
//			iostr->b_updateSize++;
		}
		if(iostr->b_rdQue.entry){
			tchThread_wake(&iostr->b_wrQue);
		}
		return TRUE;
	}
	return FALSE;
}


BOOL tch_ostream_putc_ts(tch_ostream* stream,const uint8_t c,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	uint8_t* mp = iostr->_bp;
	if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
		return FALSE;
	}
	if(!__get_IPSR()){
		if(STR_BUFFER_IS_WRBUSY(iostr)){
			tchThread_sleep(0);
			if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
				return FALSE;
			}
			//return FALSE;
		}
		SB_WR_LOCK(iostr);
		if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
			SB_WR_UNLOCK(iostr);
			return FALSE;
		}
		// check stream is closed
		STR_BUFFER_SET_WRBUSY(iostr);
		SB_WR_UNLOCK(iostr);

		while(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){
			tchThread_wait(&iostr->b_wrQue);
			if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
//				STR_BUFFER_CLR_WRBUSY(iostr);
				return FALSE;
			}
			// check stream is closed
		}
		*(mp + iostr->b_wrIdx++) = c;
		if(iostr->b_wrIdx == iostr->b_size){
			iostr->b_wrIdx = 0;
		}
	//	iostr->b_updateSize++;
		while(iostr->b_rdQue.entry){
			tchThread_wake(&iostr->b_rdQue);
		}
		STR_BUFFER_CLR_WRBUSY(iostr);
		return TRUE;
	}else{
		if(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){
			return FALSE;
		}
		*(mp + iostr->b_wrIdx++) = c;
		if(iostr->b_wrIdx == iostr->b_size){
			iostr->b_wrIdx = 0;
		}
	//	iostr->b_updateSize++;
		if(iostr->b_rdQue.entry){
			tchThread_wake(&iostr->b_rdQue);
		}
		return TRUE;
	}
	return FALSE;
}

uint32_t tch_ostream_available(tch_ostream* stream){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	return iostr->b_wrIdx > iostr->b_rdIdx? ((iostr->b_size - iostr->b_wrIdx) + iostr->b_rdIdx - 1) : (iostr->b_rdIdx - iostr->b_wrIdx - 1);
	// iostr->b_wrIdx > iostr->b_rdIdx? iostr->b_wrIdx - iostr->b_rdIdx : (iostr->b_size - iostr->b_rdIdx) + iostr->b_wrIdx;
}

void tch_ostream_close(tch_ostream* stream){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	while(STR_BUFFER_IS_WRBUSY(iostr)){
		tchThread_sleep(0);
	}
	SB_WR_LOCK(iostr);
	STR_BUFFER_SET_WRBUSY(iostr);
	STR_BUFFER_CLR_OCHANNEL_OPENED(iostr);
	SB_WR_UNLOCK(iostr);
	while(iostr->b_wrQue.entry){
		tchThread_wake(&iostr->b_wrQue);
	}
	STR_BUFFER_CLR_WRBUSY(iostr);
}

BOOL tch_istream_read_ts(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	uint8_t* mp = (uint8_t*)iostr->_bp;
	if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
		return FALSE;
	}
	if(!__get_IPSR()){
		if(STR_BUFFER_IS_RDBUSY(iostr)){
			tchThread_sleep(0);
			if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){                                          /// thread check istream is closed
				return FALSE;
			}
			//	return FALSE;
		}
		SB_RD_LOCK(iostr);///  thread check istream is valid after lock mtx, so that threads waiting in mtx queue can be guaranteed
		if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
			SB_RD_UNLOCK(iostr);
			return FALSE;
		}
		STR_BUFFER_SET_RDBUSY(iostr);
		SB_RD_UNLOCK(iostr);
		while(size--){
			while(iostr->b_rdIdx == iostr->b_wrIdx){
				tchThread_wait(&iostr->b_rdQue);
				if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){                                         ///   check whether istream is closed after waiting thread wake up, if it's closed thread will skip next procedure of routine and return false
			//		STR_BUFFER_CLR_RDBUSY(iostr);
					return FALSE;
				}
			}
			*rb++ = *(mp + iostr->b_rdIdx++);
			if(iostr->b_rdIdx == iostr->b_size){
				iostr->b_rdIdx = 0;
			}
//			iostr->b_updateSize--;
		}
		while(iostr->b_wrQue.entry){
			tchThread_wake(&iostr->b_wrQue);
		}
		STR_BUFFER_CLR_RDBUSY(iostr);
		return TRUE;
	}else{
		if(size--){
			if(iostr->b_rdIdx == iostr->b_wrIdx){tchThread_wait(&iostr->b_rdQue);}
			*rb++ = *(mp + iostr->b_rdIdx++);
			if(iostr->b_rdIdx == iostr->b_size){
				iostr->b_rdIdx = 0;
			}
//			iostr->b_updateSize--;
		}
		if(iostr->b_wrQue.entry){
			tchThread_wake(&iostr->b_wrQue);
		}
		return TRUE;
	}
	return FALSE;
}


BOOL tch_istream_getc_ts(tch_istream* stream,uint8_t* rb,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	uint8_t* mp = iostr->_bp;
	if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
		return FALSE;
	}
	if(!__get_IPSR()){
		if(STR_BUFFER_IS_RDBUSY(iostr)){
			tchThread_sleep(0);
			if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
				return FALSE;
			}
			//return FALSE;
		}
		SB_RD_LOCK(iostr);
		if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
			SB_RD_UNLOCK(iostr);
			return FALSE;
		}
		STR_BUFFER_SET_RDBUSY(iostr);
		SB_RD_UNLOCK(iostr);
		while(iostr->b_rdIdx == iostr->b_wrIdx){
			tchThread_wait(&iostr->b_rdQue);
			if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
//				STR_BUFFER_CLR_RDBUSY(iostr);
				return FALSE;
			}
		}
		*rb = *(mp + iostr->b_rdIdx++);
	//	iostr->b_updateSize--;
		if(iostr->b_rdIdx == iostr->b_size){
			iostr->b_rdIdx = 0;
		}
		while(iostr->b_wrQue.entry){
			tchThread_wake(&iostr->b_wrQue);
		}
		STR_BUFFER_CLR_RDBUSY(iostr);
	}else{
		if(iostr->b_rdIdx == iostr->b_wrIdx){
			return FALSE;
		}
		*rb = *(mp + iostr->b_rdIdx++);
	//	iostr->b_updateSize--;
		if(iostr->b_rdIdx == iostr->b_size){
			iostr->b_rdIdx = 0;
		}
		if(iostr->b_wrQue.entry){
			tchThread_wake(&iostr->b_wrQue);
		}
		STR_BUFFER_CLR_RDBUSY(iostr);
	}
	return TRUE;
}

uint32_t tch_istream_available(tch_istream* stream){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	return  iostr->b_wrIdx >= iostr->b_rdIdx? iostr->b_wrIdx - iostr->b_rdIdx : (iostr->b_size - iostr->b_rdIdx) + iostr->b_wrIdx;
}

void tch_istream_close(tch_istream* stream){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	while(STR_BUFFER_IS_RDBUSY(iostr)){
		tchThread_sleep(0);
	}
	SB_RD_LOCK(iostr);
	STR_BUFFER_SET_RDBUSY(iostr);
	STR_BUFFER_CLR_ICHANNEL_OPENED(iostr);
	SB_RD_UNLOCK(iostr);
	while(iostr->b_rdQue.entry){
		tchThread_wake(&iostr->b_rdQue);
	}
	STR_BUFFER_CLR_RDBUSY(iostr);
}

BOOL tch_ostream_write_nts(tch_ostream* stream,const uint8_t* wb,uint32_t size,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	if((!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)) || STR_BUFFER_IS_WRBUSY(iostr)){
		return FALSE;
	}
	STR_BUFFER_SET_WRBUSY(iostr);
	uint8_t* twb = (uint8_t*) wb;
	while(size--){
		if(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){
			if(!__get_IPSR()){
				tchThread_wait(&iostr->b_wrQue);
				if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
					return FALSE;
				}
			}else{
				STR_BUFFER_CLR_WRBUSY(iostr);
				return FALSE;
			}
		}
		*((uint8_t*)iostr->_bp + iostr->b_wrIdx++) = *twb++;
	//	iostr->b_updateSize++;
		if(iostr->b_wrIdx == iostr->b_size){
			iostr->b_wrIdx = 0;
		}
	}
	if(iostr->b_rdQue.entry){
		tchThread_wake(&iostr->b_rdQue);
	}
	STR_BUFFER_CLR_WRBUSY(iostr);
	return TRUE;
}


BOOL tch_ostream_putc_nts(tch_ostream* stream,const uint8_t c,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	if((!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)) || STR_BUFFER_IS_WRBUSY(iostr)){
		return FALSE;
	}
	if(iostr->b_wrIdx == (iostr->b_rdIdx - 1)){
		if(!__get_IPSR()){
			tchThread_wait(&iostr->b_wrQue);
			if(!STR_BUFFER_IS_OCHANNEL_OPENED(iostr)){
				return FALSE;
			}
		}else{
			return FALSE;
		}
	}
	STR_BUFFER_SET_WRBUSY(iostr);
	*((uint8_t*)iostr->_bp + iostr->b_wrIdx++) = c;
//	iostr->b_updateSize++;
	if(iostr->b_wrIdx == iostr->b_size){
		iostr->b_wrIdx = 0;
	}
	if(iostr->b_rdQue.entry){
		tchThread_wake(&iostr->b_rdQue);
	}
	STR_BUFFER_CLR_WRBUSY(iostr);
	return TRUE;
}


BOOL tch_istream_read_nts(tch_istream* stream,uint8_t* rb,uint32_t size,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	if((!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)) || STR_BUFFER_IS_RDBUSY(iostr)){
		return FALSE;
	}
	STR_BUFFER_SET_RDBUSY(iostr);
	while(size--){
		if(iostr->b_rdIdx == iostr->b_wrIdx){
			if(!__get_IPSR()){
				tchThread_wait(&iostr->b_rdQue);
				if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
					return FALSE;
				}
			}else{
				STR_BUFFER_CLR_RDBUSY(iostr);
				return FALSE;
			}
		}
		*rb++ = *((uint8_t*) iostr->_bp + iostr->b_rdIdx++);
//		iostr->b_updateSize--;
		if(iostr->b_rdIdx == iostr->b_size){
			iostr->b_rdIdx = 0;
		}
	}
	if(iostr->b_wrQue.entry){
		tchThread_wake(&iostr->b_wrQue);
	}
	STR_BUFFER_CLR_RDBUSY(iostr);
	return TRUE;
}


BOOL tch_istream_getc_nts(tch_istream* stream,uint8_t* rb,uint8_t* err){
	tch_iostream_buffer* iostr = (tch_iostream_buffer*) stream->_bp;
	if((!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)) || STR_BUFFER_IS_RDBUSY(iostr)){
		return FALSE;
	}
	if(iostr->b_rdIdx == iostr->b_wrIdx){
		//return FALSE;
		if(!__get_IPSR()){
			tchThread_wait(&iostr->b_rdQue);
			if(!STR_BUFFER_IS_ICHANNEL_OPENED(iostr)){
				return FALSE;
			}
		}else{
			return FALSE;
		}
	}
	STR_BUFFER_SET_RDBUSY(iostr);
	*rb = *((uint8_t*)iostr->_bp + iostr->b_rdIdx++);
//	iostr->b_updateSize--;
	if(iostr->b_rdIdx == iostr->b_size){
		iostr->b_rdIdx = 0;
	}
	if(iostr->b_wrQue.entry){
		tchThread_wake(&iostr->b_wrQue);
	}
	STR_BUFFER_CLR_RDBUSY(iostr);
	return TRUE;
}

