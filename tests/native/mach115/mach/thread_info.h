/* Public i386 Mach basic-info layout used by the priority probe. */
#ifndef TEST_MACH115_THREAD_INFO_H
#define TEST_MACH115_THREAD_INFO_H
typedef struct {
    int user_time[2], system_time[2];
    int cpu_usage, base_priority, cur_priority, run_state, flags;
    int suspend_count, sleep_time;
} thread_basic_info_data_t;
#define THREAD_BASIC_INFO 1
#define THREAD_BASIC_INFO_COUNT 11
#endif
