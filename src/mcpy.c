#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <malloc.h>

#include "pkey_simalloc.h"
#include "config.h"

void* function_pointer(void* arg) {
    // allocate private buffer
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    size_t plen = _DATA_PAGE * page_size;
    char *pbuf = (char*)memalign(page_size, plen);
    uint64_t pres = 0;

    // thread id and pkey id
    int tid = arg ? *(uint8_t*)arg : 0;
    int i, j;
    for (j = 0; j < plen; j++) {
        pbuf[j] = (uint8_t)(j + 1 + tid);
    }

    for (i = 0; i < _ITER; i++) {
#ifdef _DOM_SWITCH
        pk_sim_begin(tid, (void *)pbuf);
#endif
        // do some memory copy to access private data, consuming only 1/5 time
        for (j = 0; j < (plen - 1); j++) {
            pres += pow((uint8_t)pbuf[j], ((uint8_t)pbuf[j + 1]) & (uint8_t)0x0F);
        }
#ifdef _DOM_SWITCH
        pk_sim_end(tid, (void *)pbuf);
#endif
        // consuming 1+ time
        if (i % (_ITER / 2) == 0) {
            printf("tid[%d], \titer[%d], \tres[%ld]\n", tid, i, pres);
        }
        for (j = 0; j < plen - 1; j++) {
            pres -= pow((uint8_t)pbuf[j], ((uint8_t)pbuf[j + 1]) & (uint8_t)0x0F);
        }
    }

    free(pbuf);
}
