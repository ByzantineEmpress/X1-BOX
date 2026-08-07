#ifndef THREAD_AFFINITY_H
#define THREAD_AFFINITY_H

#ifdef __cplusplus
extern "C" {
#endif

void xemu_set_thread_affinity_prime(void);
void xemu_set_thread_affinity_performance(void);

#ifdef __cplusplus
}
#endif

#endif
