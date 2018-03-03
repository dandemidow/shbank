#include "monmsg.h"

#include <stdlib.h>
#include <string.h>

shared_mem_t *join_to_shared_banks(const char *name, int *state) {
  shared_mem_t *mem = init_link_shared_mem(SHM_MEM_SIZE, name, state);
  if ( (state && (*state == BUF_SUCCESS)) || !state ) {
    if ( clients_shared_mem(mem) > 1 ) {
      close_link_shared_mem(mem);
      fprintf(stderr, "shared banks is busy!");
      mem = NULL;
    }
  }
  return mem;
}

void unjoin_shared_banks(shared_mem_t *mem) {
  close_link_shared_mem(mem);
}

msgblk_t *prep_msg(const shared_mem_t *const mem, int *status) {
   msgblk_t *msg = __prep_msg(mem);
   if ( status )
     *status = msg?MSG_SUCESS:MSG_NO_SHARED_MEM;
   return msg;
}

typedef int (*_pusher)(const shared_mem_t *const, msg_queue_t *, msgblk_t *);
static int _push(_pusher pusher,
                 const shared_mem_t *const mem,
                 const msg_bank_t *const bank,
                 msgblk_t *msg,
                 int num) {
  msg_queue_t *mq = bank_queue(mem, bank, num);
  if ( ! mq ) return MSG_NO_QUEUE;
  return pusher(mem, mq, msg);
}

static int _push_copy(_pusher pusher,
                      const shared_mem_t *const mem,
                      const msg_bank_t *const bank,
                      msgblk_t *loc_msg,
                      int num) {
  msg_queue_t *mq = bank_queue(mem, bank, num);
  int status;
  if ( ! mq ) return MSG_NO_QUEUE;
  msgblk_t *msg = prep_msg(mem, &status);
  if ( status < 0 ) return status;
  memcpy(msg, loc_msg, sizeof(msgblk_t));
  return pusher(mem, mq, msg);
}

int push_msg(const shared_mem_t *const mem,
             const msg_bank_t *const bank,
             msgblk_t *msg,
             int num) {
  return _push(&push_msg_safe, mem, bank, msg, num);
}

int push_msg_first(const shared_mem_t *const mem,
                   const msg_bank_t *const bank,
                   msgblk_t *msg,
                   int num) {
  return _push(&push_msg_first_safe, mem, bank, msg, num);
}

int push_msg_copy(const shared_mem_t *const mem,
                  const msg_bank_t *const bank,
                  msgblk_t *loc_msg,
                  int num) {
  return _push_copy(&push_msg_safe, mem, bank, loc_msg, num);
}

int push_msg_first_copy(const shared_mem_t *const mem,
                        const msg_bank_t *const bank,
                        msgblk_t *msg,
                        int num) {
  return _push_copy(&push_msg_first_safe, mem, bank, msg, num);
}

msgblk_t *pop_msg(const shared_mem_t *const mem,
                  const msg_bank_t *const bank,
                  int num,
                  int *status) {
  msg_queue_t *mq = bank_queue(mem, bank, num);
  if ( ! mq ) {
    if (status) *status = MSG_NO_QUEUE;
    return NULL;
  }
  if (status) *status = MSG_SUCESS;
  return pop_msg_safe(mem, mq, status);
}

pid_t get_master_pid(const shared_mem_t *const mem) {
  return master_pid(mem);
}
