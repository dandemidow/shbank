#ifndef _SHMOBANK_QUEUE_H_
#define _SHMOBANK_QUEUE_H_

#include <pthread.h>

#include "msg.h"
#include <sharedmem/shalloc.h>

typedef struct _msg_queue_t {
  pthread_mutex_t *mtx;
  pthread_cond_t *wcd;
  int *active;
  msgblk_t *blk_first;
  msgblk_t *blk_last;
} msg_queue_t;

struct _msg_bank_t;
typedef struct _msg_bank_t msg_bank_t;

msg_queue_t *new_msg_queue(const shared_mem_t *const,
                           const msg_bank_t *const);
void free_msg_queue(const shared_mem_t *const,
                    msg_queue_t *mq);

#endif  // _SHMOBANK_QUEUE_H_
