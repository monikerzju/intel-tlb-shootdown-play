#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>
#include <sys/mman.h>
#include <x86intrin.h>
#include <pthread.h>

static char* buffer;
static uint64_t total = 100000;
static void* function_pointer(void* arg);
static uint64_t test(void);

int main() {
    uint64_t avg = 0;
    int count = 10;
    int i;
    printf("\n\n");
    for (i = 0; i < count; i++) {
        avg += test();
    }
    system("cat /proc/interrupts | grep TLB");
    printf("average time is %ld\n", avg / count / total / 2);
    return 1;

    //                          null       math       th_null      th_math
    // basic                  | 2        | 2336     | 10         | 3392
    // mprot                  | 2543     | 4759     | 2885       | 7658
    // 1 sw cycle consumption | 2541     | 2423     | 2875       | 4226
    // 1 core extra shootdown | < 500    | < 500    | 2000       | 600000
    // if 1 core has 1 extra shootdown, 6000 cycles will be consumed
}

uint64_t test(void) {
    int iter = 0;
    const int nt = 12;

    pthread_t threads[nt];
    uint8_t tdes[nt];
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    buffer = (char*)memalign(page_size, 8 * page_size);
    uint64_t start, end;

    for (; iter < 8 * page_size; iter++) {
        buffer[iter] = (uint8_t)iter;
    }

    _mm_lfence();
    start = __rdtsc();
    _mm_lfence();

#ifdef WTHREAD
    for (iter = 0; iter < nt; iter++) {
        tdes[iter] = (iter == 0);
        pthread_create(&threads[iter], NULL, function_pointer, (void*)(&(tdes[iter])));
    }
    for (iter = 0; iter < nt; iter++) {
        pthread_join(threads[iter], NULL);
    }
#else
    (*function_pointer)(NULL);
#endif

    _mm_lfence();
    end = __rdtsc();
    _mm_lfence();

    return (end - start);
}

void* function_pointer(void* arg) {
    int iter, j;
    float result = 0;
    uint8_t master = arg ? *(uint8_t*)arg : 0;
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    for (iter = 0; iter < total; iter++) {
#ifdef SWITCH
        // switch to secure access
#ifdef WTHREAD
        if (master)
#endif
        if(mprotect(buffer, page_size, PROT_READ | PROT_WRITE) == -1)
            printf("mprotect failed\n");
#endif
        // do some calc
#ifdef WMATH
        if (result == 0) {
            result = pow((uint8_t)buffer[2 * iter / 10], ((uint8_t)buffer[2 * iter / 10 + 1]) & (uint8_t)0x0F);
        } else {
            result = result + pow((uint8_t)buffer[2 * iter / 10], ((uint8_t)buffer[2 * iter / 10 + 1]) & (uint8_t)0x0F);
            for (j = 0; j < 20; j++) {
                if (sin(result) > 0) {
                    result = result - pow((uint8_t)buffer[2 * iter / 20], ((uint8_t)buffer[2 * iter / 20 + 1]) & (uint8_t)0x0F);
                }
            }
        }
#endif
#ifdef SWITCH
        // switch to insecure world
#ifdef WTHREAD
        if (master)
#endif
        if(mprotect(buffer, page_size, PROT_READ) == -1)
            printf("mprotect failed\n");
#endif
        // do some simple syscall
#ifdef WMATH
        if(!getpid()) exit(9);
#endif
    }
}
