#ifndef _PKEY_SIMALLOC_H
#define _PKEY_SIMALLOC_H

void pk_init();
void pk_sim_begin(int tid, void *pbuf);
void pk_sim_end(int tid, void *pbuf);

#endif
