/* Minimal test-only Mach thread API. */
#ifndef TEST_MACH115_MACH_H
#define TEST_MACH115_MACH_H
typedef unsigned int thread_t;
typedef int kern_return_t;
typedef unsigned int vm_address_t, vm_size_t, vm_offset_t, boolean_t, memory_object_name_t;
typedef int vm_prot_t, vm_inherit_t;
extern int task_self_;
extern vm_size_t vm_page_size;
kern_return_t vm_allocate(unsigned int, vm_address_t *, vm_size_t, boolean_t);
kern_return_t vm_deallocate(unsigned int, vm_address_t, vm_size_t);
#define task_self() task_self_
kern_return_t vm_region(unsigned int, vm_address_t *, vm_size_t *, vm_prot_t *,
                        vm_prot_t *, vm_inherit_t *, boolean_t *, memory_object_name_t *, vm_offset_t *);
kern_return_t port_deallocate(unsigned int, int);
kern_return_t thread_suspend(thread_t);
kern_return_t thread_resume(thread_t);
typedef int *thread_info_t;
typedef unsigned int *thread_state_t;
kern_return_t thread_get_state(thread_t, int, thread_state_t, unsigned int *);
kern_return_t thread_info(thread_t, int, thread_info_t, unsigned int *);
#endif
