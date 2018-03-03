#include "queue.h"

#include "bank.h"

msg_queue_t *new_msg_queue(const shared_mem_t *const mem,
                           const msg_bank_t *const bank) {
  msg_queue_t *mq = alloc_shared_mem(mem ,sizeof(msg_queue_t));
  mq->blk_first = mq->blk_last = NULL;
  mq->mtx = (pthread_mutex_t*)&bank->mtx;
  mq->wcd = (pthread_cond_t*)&bank->wcd;
  mq->active = (int*)&bank->active;
  return mq;
}

void free_msg_queue(const shared_mem_t *const mem, msg_queue_t *mq) {
  msgblk_t *msg = __pop_msg(mem, mq);
  while ( msg ) {
    free_shared_mem(mem, msg);
    msg = __pop_msg(mem, mq);
  }
  free_shared_mem(mem, mq);
}
