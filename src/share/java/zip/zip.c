/* Reconstructed from Sun JDK 1.1.5 zip_g.dll; see the ZIP evidence plan. */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "StubPreamble.h"
#include "zlib.h"

typedef struct {
    z_stream *strm;
    HArrayOfByte *buf;
    long off, len;
    long level, strategy, setParams, finish, finished;
} Deflater;
typedef struct {
    z_stream *strm;
    HArrayOfByte *buf;
    long off, len, finished, needsDictionary;
} Inflater;

static int bounds(HArrayOfByte *b, long off, long len)
{
    if (!b) { SignalError(0, "java/lang/NullPointerException", 0); return 0; }
    if (off < 0 || len < 0 || (unsigned long)off > obj_length(b) || (unsigned long)len > obj_length(b) - off) {
        SignalError(0, "java/lang/ArrayIndexOutOfBoundsException", 0); return 0;
    }
    return 1;
}
static int streamcheck(z_stream *s)
{
    if (s) return 1;
    SignalError(0, "java/lang/NullPointerException", 0); return 0;
}
#define BODY(type) type *p = (type *)unhand(sp[0].h)
#define ARRAY ((HArrayOfByte *)sp[1].h)
#define STUB(c,m) stack_item *Java_java_util_zip_##c##_##m##_stub(stack_item *sp, struct execenv *ee)
#define CHECKSUM(c,fn) \
STUB(c,update) { long *p=(long *)unhand(sp[0].h); \
 if(bounds(ARRAY,sp[2].i,sp[3].i) && sp[3].i>0) \
 *p=fn(*p,(Bytef *)unhand(ARRAY)->body+sp[2].i,sp[3].i); return sp; } \
STUB(c,update1) { long *p=(long *)unhand(sp[0].h); Byte b=(Byte)sp[1].i; \
 *p=fn(*p,&b,1); return sp; }
CHECKSUM(CRC32,crc32)
CHECKSUM(Adler32,adler32)

STUB(Deflater,init)
{
    BODY(Deflater); int ret; char *msg;
    z_stream *s=(z_stream *)calloc(1,sizeof(z_stream));
    if (!s) { SignalError(0,"java/lang/OutOfMemoryError",0); return sp; }
    ret=deflateInit2(s,p->level,Z_DEFLATED,sp[1].i ? -15 : 15,8,p->strategy);
    if (ret==Z_OK) p->strm=s;
    else { msg=s->msg; free(s); SignalError(0,ret==Z_MEM_ERROR ? "java/lang/OutOfMemoryError" : ret==Z_STREAM_ERROR ? "java/lang/IllegalArgumentException" : "java/lang/InternalError",msg); }
    return sp;
}
STUB(Inflater,init)
{
    BODY(Inflater); int ret; char *msg;
    z_stream *s=(z_stream *)calloc(1,sizeof(z_stream));
    if (!s) { SignalError(0,"java/lang/OutOfMemoryError",0); return sp; }
    ret=inflateInit2(s,sp[1].i ? -15 : 15);
    if(ret==Z_OK) p->strm=s;
    else { msg=s->msg; free(s); SignalError(0,ret==Z_MEM_ERROR ? "java/lang/OutOfMemoryError" : "java/lang/InternalError",msg); }
    return sp;
}
STUB(Deflater,setDictionary)
{
    BODY(Deflater); int ret;
    if(!streamcheck(p->strm) || !bounds(ARRAY,sp[2].i,sp[3].i)) return sp;
    ret=deflateSetDictionary(p->strm,(Bytef *)unhand(ARRAY)->body+sp[2].i,sp[3].i);
    if(ret!=Z_OK) SignalError(0,ret==Z_STREAM_ERROR ? "java/lang/IllegalArgumentException" : "java/lang/InternalError",p->strm->msg);
    return sp;
}
STUB(Inflater,setDictionary)
{
    BODY(Inflater); int ret;
    if(!streamcheck(p->strm) || !bounds(ARRAY,sp[2].i,sp[3].i)) return sp;
    ret=inflateSetDictionary(p->strm,(Bytef *)unhand(ARRAY)->body+sp[2].i,sp[3].i);
    if(ret==Z_OK) p->needsDictionary=0;
    else SignalError(0,ret==Z_STREAM_ERROR || ret==Z_DATA_ERROR ? "java/lang/IllegalArgumentException" : "java/lang/InternalError",p->strm->msg);
    return sp;
}
STUB(Deflater,deflate)
{
    BODY(Deflater); z_stream *s=p->strm; int ret, len=sp[3].i, result=0;
    if(!streamcheck(s) || !bounds(ARRAY,sp[2].i,len) || !bounds(p->buf,p->off,p->len)) goto done;
    s->next_in=(Bytef *)unhand(p->buf)->body+p->off; s->avail_in=p->len;
    s->next_out=(Bytef *)unhand(ARRAY)->body+sp[2].i; s->avail_out=len;
    if(p->setParams) {
        ret=deflateParams(s,p->level,p->strategy);
        if(ret==Z_OK || ret==Z_BUF_ERROR) p->setParams=0;
    } else { ret=deflate(s,p->finish ? Z_FINISH : Z_NO_FLUSH); if(ret==Z_STREAM_END) p->finished=1; }
    if(ret==Z_OK || ret==Z_STREAM_END) { p->off+=p->len-s->avail_in; p->len=s->avail_in; result=len-s->avail_out; }
    else if(ret!=Z_BUF_ERROR) SignalError(0,"java/lang/InternalError",s->msg);
 done: sp[0].i=result; return sp+1;
}
STUB(Inflater,inflate)
{
    BODY(Inflater); z_stream *s=p->strm; int ret, len=sp[3].i, result=0;
    if(!streamcheck(s) || !bounds(ARRAY,sp[2].i,len) || !bounds(p->buf,p->off,p->len)) goto done;
    s->next_in=(Bytef *)unhand(p->buf)->body+p->off; s->avail_in=p->len;
    s->next_out=(Bytef *)unhand(ARRAY)->body+sp[2].i; s->avail_out=len;
    ret=inflate(s,Z_PARTIAL_FLUSH);
    if(ret==Z_STREAM_END) p->finished=1;
    if(ret==Z_NEED_DICT) p->needsDictionary=1;
    if(ret==Z_OK || ret==Z_STREAM_END || ret==Z_NEED_DICT) {
        p->off+=p->len-s->avail_in; p->len=s->avail_in;
        if(ret!=Z_NEED_DICT) result=len-s->avail_out;
    } else if(ret!=Z_BUF_ERROR) SignalError(0,ret==Z_DATA_ERROR ? "java/util/zip/DataFormatException" : ret==Z_MEM_ERROR ? "java/lang/OutOfMemoryError" : "java/lang/InternalError",s->msg);
 done: sp[0].i=result; return sp+1;
}
#define GET(c,m,f) STUB(c,m) { BODY(c); sp[0].i=streamcheck(p->strm) ? p->strm->f : 0; return sp+1; }
GET(Deflater,getAdler,adler)
GET(Deflater,getTotalIn,total_in)
GET(Deflater,getTotalOut,total_out)
GET(Inflater,getAdler,adler)
GET(Inflater,getTotalIn,total_in)
GET(Inflater,getTotalOut,total_out)
STUB(Deflater,reset) {
    BODY(Deflater); if(!streamcheck(p->strm)) return sp;
    if(deflateReset(p->strm)!=Z_OK) SignalError(0,"java/lang/InternalError",0);
    else { p->off=p->len=0; p->finish=p->finished=0; } return sp;
}
STUB(Inflater,reset) {
    BODY(Inflater); if(!streamcheck(p->strm)) return sp;
    if(inflateReset(p->strm)!=Z_OK) SignalError(0,"java/lang/InternalError",0);
    else { p->off=p->len=0; p->finished=p->needsDictionary=0; } return sp;
}
#define END(c,fn) STUB(c,end) { BODY(c); if(p->strm) { \
 if(fn(p->strm)==Z_STREAM_ERROR) SignalError(0,"java/lang/InternalError",0); \
 else { free(p->strm); p->strm=0; } } return sp; }
END(Deflater,deflateEnd)
END(Inflater,inflateEnd)

/* Per-call state retains the bootstrap reader's existing concurrency fix.
 * Malformed input always fails, unlike Sun's success flag on some errors. */
int inflateFully(int fd, unsigned int size, void *buf, unsigned int len, char **msg)
{
    z_stream s; unsigned char input[512]; unsigned int remaining=size, count;
    int ret, got, valid;
    memset(&s,0,sizeof(s));
    ret=inflateInit2(&s,-15);
    if(ret!=Z_OK) { *msg=s.msg; return 0; }
    s.next_out=(Bytef *)buf; s.avail_out=len;
    do {
        if(s.avail_in==0 && remaining) {
            count=remaining<sizeof(input) ? remaining : sizeof(input);
            got=read(fd,input,count);
            if(got<=0) { *msg=got<0 ? strerror(errno) : "Unexpected EOF"; inflateEnd(&s); return 0; }
            s.next_in=input; s.avail_in=got; remaining-=got;
        }
        ret=inflate(&s,Z_PARTIAL_FLUSH);
    } while(ret==Z_OK);
    valid=ret==Z_STREAM_END && s.total_in==size && s.total_out==len;
    if(!valid) *msg=s.msg ? s.msg : "Invalid entry compressed size";
    inflateEnd(&s); return valid;
}
