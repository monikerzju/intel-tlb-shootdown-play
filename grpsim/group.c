#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "group.h"

typedef struct {
    int ts;
    int asid;
} tvt_t;

static tvt_t thread_vkey_table[NTHREADS + 1][NVKEYS];
static int asid = 0;

static void print_conf() {
    printf("In this test, there are %d pkeys, %d vkeys, %d threads\n\n",
        NPKEYS, NVKEYS, NTHREADS  
    );
}

static void print_graph() {
    int i, j, id;
    for (i = 0; i < NVKEYS + 2; i++) {
        printf("--------");
    }
    printf("\n");
    printf("\t\t");
    for (i = 0; i < NVKEYS; i++) {
        printf("vk%d\t", i);
    }
    printf("\n");
    for (id = 0; id <= asid; id++) {
        for (i = 0; i < NTHREADS; i++) {
            if (thread_vkey_table[i][0].asid == id) {
                printf("t%02d d%d\t\t", i, thread_vkey_table[i][0].asid);
                for (j = 0; j < NVKEYS; j++) {
                    if (thread_vkey_table[i][j].ts == INACTIVE) printf("-\t");
                    else printf("%d\t", thread_vkey_table[i][j].ts);
                }
                printf("\n");
            }
        }
    }
    for (i = 0; i < NVKEYS + 2; i++) {
        printf("--------");
    }
    printf("\n\n\n");
}
 
static int get_ts() {
    static int ts = 0;
    return ++ts;
}

static int calc_asid_vkey(int curr_thread) {
    int i, j;
    for (i =  0; i < NVKEYS; i++) {
        thread_vkey_table[NTHREADS][i].ts = INACTIVE;
    }
    for (i = 0; i < NTHREADS; i++) {
        if (thread_vkey_table[i][0].asid == thread_vkey_table[curr_thread][0].asid) {
            for (j = 0; j < NVKEYS; j++) {
                if (thread_vkey_table[i][j].ts != INACTIVE) {
                    thread_vkey_table[NTHREADS][j].ts = thread_vkey_table[i][j].ts;
                }
            }
        }
    }
}

static int try_merge(int to, int from) {
    int i;
    int vpkey_with_others = 0;
    calc_asid_vkey(to);
    for (i = 0; i < NVKEYS; i++) {
        if (thread_vkey_table[NTHREADS][i].ts != INACTIVE || thread_vkey_table[from][i].ts != INACTIVE) {
            vpkey_with_others++;
        }
    }
    return (vpkey_with_others > NPKEYS);
}

static int self_overfow(int tid) {
    int i;
    int active_vkey = 0;
    for (i = 0; i < NVKEYS; i++) {
        if (thread_vkey_table[tid][i].ts != INACTIVE) {
            active_vkey++;
        }
    }
    return active_vkey > NPKEYS;
}
 
int main() {
    int curr_ts;
    int curr_thread;
    int curr_access_vkey;
    int vpkey_with_others;
    int need_div;
    int i, j;

    print_conf();
    for (i = 0; i < NTHREADS; i++) {
        thread_vkey_table[i][0].asid = 0;
        for (j = 0; j < NVKEYS; j++) {
            thread_vkey_table[i][j].ts = INACTIVE;
        }
    }

    while((curr_ts = get_ts()) < DBGSTP) {
        curr_thread = rand() % NTHREADS;
        curr_access_vkey = rand() % NVKEYS;
        printf("Thread%d is accessing vkey%d\n", curr_thread, curr_access_vkey);
        thread_vkey_table[curr_thread][curr_access_vkey].ts = curr_ts;
        if (self_overfow(curr_thread)) {
            printf("Self overflow\n");
            print_graph();
            continue;
        }
        calc_asid_vkey(curr_thread);
        vpkey_with_others = 0;
        for (i = 0; i < NVKEYS; i++) {
            if (thread_vkey_table[NTHREADS][i].ts != INACTIVE) {
                vpkey_with_others++;
            }
        }
        if (need_div = vpkey_with_others > NPKEYS) {
            printf("Need change address space division\n");
            for (i = 0; i < NTHREADS; i++) {
                if (i != curr_thread && thread_vkey_table[i][0].asid != thread_vkey_table[curr_thread][0].asid) {
                    need_div = try_merge(i, curr_thread);   // success is 0, fail is 1
                    if (need_div == 0) {
                        thread_vkey_table[curr_thread][0].asid = thread_vkey_table[i][0].asid;
                        break;
                    }
                }
            }
            if (need_div) {     // allocate new asid
                printf("Need NEW address space division\n");
                thread_vkey_table[curr_thread][0].asid = ++asid;
            }
        }
        print_graph();
    }

    return 0;
}
