#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __ANDROID__
/* Snapdragon 8 Gen 2 / 8 Elite 2 CPU Topology:
   Cores 0-3: Efficiency Cores
   Cores 4-6: Performance Cores (Use for GPU/Audio)
   Core 7: Cortex-X Prime Core (Use for CPU TCG Translation)
*/

void xemu_set_thread_affinity_prime(void) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(7, &cpuset); /* Pin to Prime Core */
    
    sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset);
    printf("Pinned TCG CPU Thread to Prime Core 7\n");
}

void xemu_set_thread_affinity_performance(void) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(4, &cpuset);
    CPU_SET(5, &cpuset);
    CPU_SET(6, &cpuset);
    
    sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset);
    printf("Pinned Subsystem Thread to Performance Cores 4-6\n");
}
#else
void xemu_set_thread_affinity_prime(void) {}
void xemu_set_thread_affinity_performance(void) {}
#endif
