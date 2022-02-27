#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "pkey_simalloc.h"
#include "config.h"

typedef struct {
    int ppkey;  // physical pkey
    uint8_t valid;
    uint8_t busy;
    void* ptr;
    uint64_t ts;
} pkey_entry_t;
#if _USE_PROCESS_G16 == 1
#define vpkey_number _NUM_PK
#else
#define vpkey_number _NUM_THREADS
#endif

static pkey_entry_t mapping[vpkey_number];   // vpkey, process
static uint64_t gts;
static int available_ppkey_num = 0;
static pthread_mutex_t locks[2];

// usespace mutex lock acquire
static void pk_laq(int i) {
    pthread_mutex_lock(&locks[i]);
}

// userspace mutex lock release
static void pk_lrl(int i) {
    pthread_mutex_unlock(&locks[i]);
}

void pk_init() {
    int i;
    gts = 0;
    for (i = 0; i < vpkey_number; i++) {
        mapping[i].valid = 0;
        mapping[i].busy  = 0;
        mapping[i].ts    = 0;
    }
    pthread_mutex_init(&locks[0], NULL);
    pthread_mutex_init(&locks[1], NULL);
}

void pk_sim_begin(int tid, void *pbuf) {
#if _PKEY_REAL_ALLOC == 1
    // compare and compare and swap
    // check if the vkey is still mapped to a pkey
    // this lock is for [busy] and [valid] and [gts]
    pk_laq(1);
    if (mapping[tid].valid == 1) {
        mapping[tid].busy = 1;
        mapping[tid].ts = gts++;
        pk_lrl(1);
        // TODO: wrpkru add permission
        return;
    }
    pk_lrl(1);

    // the vkey is not mapped to pkey
    // lock for [available key]
    pk_laq(0);
    if (available_ppkey_num < _NUM_PK) {
        // this lock is for available_ppkey_num
        mapping[tid].ppkey = available_ppkey_num++;
        pk_lrl(0);
        pk_laq(1);
        mapping[tid].valid = 1;
        mapping[tid].busy  = 1;
        mapping[tid].ts    = gts++;
        pk_lrl(1);
        mapping[tid].ptr   = pbuf;
        // here, in fact we should give it a pkey, but here we use PROT_READ instaed of RW to generate TLB flush likewise
        mprotect(pbuf, _DATA_PAGE * sysconf(_SC_PAGE_SIZE), PROT_READ);
        // TODO: wrpkru
        return;
    }
    pk_lrl(0);

    // LRU, busy waiting (or we can yield and re-schedule, which will cause large overhead also)
    while (1) {
        uint64_t lru_ts = mapping[0].ts;
        int lru_vindex = 0;

        pk_laq(1);
        uint8_t has_available = mapping[0].busy == 0 && mapping[0].valid == 1;
        for (int i = 1; i < vpkey_number; i++) {
            if (mapping[i].busy == 0 && mapping[i].valid == 1) {
                if (has_available) {
                    if (lru_ts > mapping[i].ts) {
                        lru_ts = mapping[i].ts;
                        lru_vindex = i;
                    }
                } else {
                    has_available = 1;
                    lru_ts = mapping[i].ts;
                    lru_vindex = i;
                }
            }
        }
        // use the least recently used, compare and cas
        if (has_available) {
            // printf("evict %d with %d, using pkey %d\n", lru_vindex, tid, mapping[lru_vindex].ppkey);
            mapping[lru_vindex].valid = 0;
            mapping[tid].valid = 1;
            mapping[tid].ptr = pbuf;
            mapping[tid].ppkey = mapping[lru_vindex].ppkey;
            mapping[tid].ts = gts++;
            mapping[tid].busy = 1;
            pk_lrl(1);
            mprotect(mapping[lru_vindex].ptr, _DATA_PAGE * sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE);
            mprotect(mapping[tid].ptr, _DATA_PAGE * sysconf(_SC_PAGE_SIZE), PROT_READ);
            // TODO: wrpkru
            return;
        } else {
            pk_lrl(1);
        }
    }
#endif
}

void pk_sim_end(int tid, void *pbuf) {
#if _PKEY_REAL_ALLOC == 1
    // no need to add lock, because when busy == 1,
    // other threads cannot evict or overwrite mapping.busy
    // lazy end
    pk_laq(1);
    mapping[tid].busy = 0;
    pk_lrl(1);
#endif
}
