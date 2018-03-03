#include "msg.h"

#include "queue.h"

static int queue_has_msg(msg_queue_t *queue) {
  return queue->blk_first?1:0;
}

static void add_new(const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *blk) {
  msgblk_t *glob_blk = glob_cast(msgblk_t, mem, blk);
  mq->blk_first = glob_blk;
  mq->blk_last = glob_blk;
}

static void add_last(const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *blk) {
  msgblk_t *glob_blk = glob_cast(msgblk_t, mem, blk);
  locl_cast(msgblk_t, mem, mq->blk_last)->next = glob_blk;
  mq->blk_last = glob_blk;
}

static void add_first(const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *blk) {
  msgblk_t *glob_blk = glob_cast(msgblk_t, mem, blk);
  blk->next = mq->blk_first;
  mq->blk_first = glob_blk;
}

static int __push_msg(_msg_adder adder, const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *blk) {
  if ( ! blk ) return MSG_NULL_PTR;
  if ( mem->addr < (char*)blk && (char*)blk < mem->addr + mem->buf_size ) {
    blk->next = NULL; // only one
    if ( mq->blk_first ) {
      adder(mem, mq, blk);
    } else {
      add_new(mem, mq, blk);
    }
    return MSG_SUCESS;
  }
  return MSG_NOT_SHARED_PTR;
}

msgblk_t *__pop_msg(const shared_mem_t *const mem, msg_queue_t *mq) {
  msgblk_t *p = locl_cast(msgblk_t, mem, mq->blk_first);
  if ( p ) {
    mq->blk_first = p->next;
    p->next = NULL;
    return p;
  }
  return NULL;
}

msgblk_t *__prep_msg(const shared_mem_t *const mem) {
  msgblk_t *blk = NULL;
  blk = alloc_shared_mem(mem, sizeof(msgblk_t));
  if (!blk) return NULL;
  blk->next = NULL;
  blk->len = 0;
  blk->err = 0;
  time(&(blk->tm));
  return blk;
}

static int push_msg_common(_msg_adder adder, const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *msg) {
  int ret = MSG_SUCESS;
  pthread_mutex_t *mtx = locl_cast(pthread_mutex_t, mem, mq->mtx);
  pthread_cond_t *wcd = locl_cast(pthread_cond_t, mem, mq->wcd);
  int *active = locl_cast(int, mem, mq->active);
  pthread_mutex_lock(mtx);
  ret = *active ? __push_msg(adder, mem, mq, msg) : MSG_NOT_ACTIVE_BANK;
  pthread_cond_signal(wcd);
  pthread_mutex_unlock(mtx);
  return ret;
}

int push_msg_safe(const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *msg) {
  return push_msg_common(&add_last, mem, mq, msg);
}

int push_msg_first_safe(const shared_mem_t *const mem, msg_queue_t *mq, msgblk_t *msg) {
  return push_msg_common(&add_first, mem, mq, msg);
}

msgblk_t *pop_msg_nonblock(const shared_mem_t *const mem, msg_queue_t *queue) {
  msgblk_t *msg;
  pthread_mutex_t *mtx = locl_cast(pthread_mutex_t, mem, queue->mtx);
  pthread_mutex_lock(mtx);
  msg = __pop_msg(mem, queue);
  pthread_mutex_unlock(mtx);
  return msg;
}

msgblk_t *pop_msg_safe(const shared_mem_t *const mem, msg_queue_t *queue, int *status) {
  msgblk_t *msg = NULL;
  pthread_mutex_t *mtx = locl_cast(pthread_mutex_t, mem, queue->mtx);
  pthread_cond_t *cnd = locl_cast(pthread_cond_t, mem, queue->wcd);
  int *active = locl_cast(int, mem, queue->active);
  pthread_mutex_lock(mtx);
  while ( !queue_has_msg(queue) && *active ) {
    pthread_cond_wait(cnd, mtx);
  }
  if ( *active ) {
    if (status) *status = MSG_SUCESS;
    msg = __pop_msg(mem, queue);
  } else
    if (status) *status = MSG_NOT_ACTIVE_BANK;
  pthread_mutex_unlock(mtx);
  return msg;
}

void free_msg(const shared_mem_t *const mem, msgblk_t *msg) {
  free_shared_mem(mem, msg);
}
