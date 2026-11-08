/* Private integration contracts for the reconstructed native backend. */
#ifndef JDK115_CTHREADS_RUNTIME_H
#define JDK115_CTHREADS_RUNTIME_H
int sysThreadHasPendingException(sys_thread_t *);
void *get_awt_cookie(void *env);
void set_awt_cookie(void *env, void *cookie);
#endif
