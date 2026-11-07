/* Reconstructed from agent_g.dll 10001918, 1000296B--10002BD7,
 * 100030F6--100031DD. See tests/native/agent115-evidence.md. */
#include "StubPreamble.h"
#include "interpreter.h"
#include "java_lang_Thread.h"
#include "sun_tools_debug_Agent.h"

JavaFrame *agent115_stack_frame(Hjava_lang_Thread *thread, long number)
{
    JavaFrame *frame = ((ExecEnv *)unhand(thread)->eetop)->current_frame;
    long index = 0;
    for (; frame; frame = frame->prev)
        if (frame->current_method && index++ == number) break;
    return frame;
}
static stack_item *stack_value(Hjava_lang_Thread *thread, long number, long slot)
{
    JavaFrame *frame;
    if (!unhand(thread)->eetop) {
        SignalError(NULL, "java/lang/IllegalAccessError", "invalid slot");
        return NULL;
    }
    frame = agent115_stack_frame(thread, number);
    if (!frame) {
        SignalError(NULL, "java/lang/NullPointerException", NULL);
        return NULL;
    }
    return slot == -1 ? NULL : frame->vars + slot;
}
#define GET_STACK(name,type,member) \
type sun_tools_debug_Agent_getStack##name(Hsun_tools_debug_Agent *self, Hjava_lang_Thread *thread, long number, long slot) { \
    stack_item *value = stack_value(thread, number, slot); \
    return value ? (type)value->member : (type)0; }
GET_STACK(Object,Hjava_lang_Object *,p)
GET_STACK(Array,HArrayOfObject *,p)
GET_STACK(Boolean,long,i)
GET_STACK(Int,long,i)
GET_STACK(Float,float,f)
#define GET_WIDE(name,type,operation) \
type sun_tools_debug_Agent_getStack##name(Hsun_tools_debug_Agent *self, Hjava_lang_Thread *thread, long number, long slot) { \
    stack_item *value = stack_value(thread, number, slot); Java8 temp; \
    return value ? operation(temp,value) : (type)0; }
GET_WIDE(Long,int64_t,GET_INT64)
GET_WIDE(Double,double,GET_DOUBLE)
#define SET_STACK(name) \
void sun_tools_debug_Agent_setStack##name(Hsun_tools_debug_Agent *self,Hjava_lang_Thread *thread,long number,long slot,long data) { \
    stack_item *value = stack_value(thread,number,slot); if (value) value->i = data; }
SET_STACK(Boolean)
SET_STACK(Int)
#define SET_WIDE(name,type,operation) \
void sun_tools_debug_Agent_setStack##name(Hsun_tools_debug_Agent *self,Hjava_lang_Thread *thread,long number,long slot,type data) { \
    stack_item *value = stack_value(thread,number,slot); Java8 temp; if (value) operation(temp,value,data); }
SET_WIDE(Long,int64_t,SET_INT64)
SET_WIDE(Double,double,SET_DOUBLE)
