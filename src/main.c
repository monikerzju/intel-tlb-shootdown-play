#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <pthread.h>
#include <wait.h>

#include "pkey_simalloc.h"
#include "config.h"

extern void* function_pointer(void* arg);

// if accessing the secret, such as data-base using 1/5 of the time accessing critical data
// 8 keys are used for 12 hardware threads

// no domain switch
// The result [num_proc=4, num_thread=32, iter=1000, data_page=16, num_switch=2000] is 16158431822
// The result [num_proc=1, num_thread=32, iter=1000, data_page=16, num_switch=2000] is 16137918552

// domain switch
// The result [num_proc=4, num_thread=32, iter=1000, data_page=16, num_switch=2000] is 16160738646
// The result [num_proc=1, num_thread=32, iter=1000, data_page=16, num_switch=2000] is 17390171020
// Domain switch causes 7% overhead, while proc-based only cause 0.12% overhead

// Only 4 keys, threads' is 20067863612 (23.28% overhead), processes' is 16152986252, base is 16284836914

// If some protected domain are accessed through 1/2 of the time and 8 keys, then libmpk based fine-grained 25.29%
// proc 4 is 25989937880, base is 25921965164, thread-style is 32476525932

// If some protected domain are accessed through 1/2 of the time and 4 keys, then thread based fine-grained 123.64%
// proc 8 is 25863526336, base is 25886427860, thread-style is 57904148708

// TODO:
// Find more use cases where MPK's scalability for code and contexts is important, and MPK be active for longer time
// Can MPK be more scalable (fine-grained enough) for Many-core systems?
//      Such as Intel Xeon Platinum 8380 has 80 threads, and many has 56 or 64 threads
// Can MPK be more scalable for code domains?
// Can MPK be safer and never let the kernel to be a confused deputy?

int main(void) {
    pthread_t threads[_NUM_THREADS];
    uint8_t tids[_NUM_THREADS];
    int last_proc_num_threads = _NUM_THREADS % _NUM_PK;
    last_proc_num_threads = (last_proc_num_threads == 0) ? _NUM_PK : last_proc_num_threads;

    uint64_t start, end;
    int iter;
    int status = 0;
    pid_t wpid;

    pk_init();

#if _USE_PROCESS_G16 == 1
    _mm_lfence();
    start = __rdtsc();
    _mm_lfence();
    for (iter = 1; iter < (_NUM_THREADS + _NUM_PK - 1) / _NUM_PK; iter++) {   // 0 is for parent
        if (fork() == 0) {  // child
            for (iter = 0; iter < _NUM_PK; iter++) {
                tids[iter] = (uint8_t)(iter + 1);       // count from 1
                pthread_create(&threads[iter], NULL, function_pointer, (void*)(&(tids[iter])));
            }
            for (iter = 0; iter < _NUM_PK; iter++) {
                pthread_join(threads[iter], NULL);
            }
            exit(0);
        }
    }
    for (iter = 0; iter < last_proc_num_threads; iter++) {
        tids[iter] = (uint8_t)(iter + 1);       // count from 1
        pthread_create(&threads[iter], NULL, function_pointer, (void*)(&(tids[iter])));
    }
    for (iter = 0; iter < last_proc_num_threads; iter++) {
        pthread_join(threads[iter], NULL);
    }
    while ((wpid = wait(&status)) > 0);
    _mm_lfence();
    end = __rdtsc();
    _mm_lfence();
    printf("The result[num_proc=%d, num_thread=%d, iter=%d, data_page=%d, num_switch=%d] is %ld\n", 
        (_NUM_THREADS + _NUM_PK - 1) / _NUM_PK,
        _NUM_THREADS, _ITER, _DATA_PAGE, (_PKEY_REAL_ALLOC == 0) ? 0 : 2 * _ITER, 
        end - start
    );
#else
    _mm_lfence();
    start = __rdtsc();
    _mm_lfence();
    for (iter = 0; iter < _NUM_THREADS; iter++) {
        tids[iter] = (uint8_t)(iter + 1);       // count from 1
        pthread_create(&threads[iter], NULL, function_pointer, (void*)(&(tids[iter])));
    }
    for (iter = 0; iter < _NUM_THREADS; iter++) {
        pthread_join(threads[iter], NULL);
    }
    _mm_lfence();
    end = __rdtsc();
    _mm_lfence();
    printf("The result[num_proc=1, num_thread=%d, iter=%d, data_page=%d, num_switch=%d] of is %ld\n", 
        _NUM_THREADS, _ITER, _DATA_PAGE, (_PKEY_REAL_ALLOC == 0) ? 0 : 2 * _ITER, 
        end - start
    );
#endif
    return 0;
}
