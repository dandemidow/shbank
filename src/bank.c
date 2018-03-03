#include "bank.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NONCONST_MUTEX(m, bank) pthread_mutex_t *m = (pthread_mutex_t *)(&bank->mtx);
#define NONCONST_COND(w, bank) pthread_cond_t *w = (pthread_cond_t *)(&bank->wcd);
#define NONCONST_SYNC(m, w, bank) \
  NONCONST_MUTEX(m, bank) \
  NONCONST_COND(w, bank)

msg_queue_t *bank_queue(const shared_mem_t *const mem,
                        const msg_bank_t *const bank,
                        int number) {
  return locl_cast(msg_queue_t, mem, glob_bank_channel(bank, number));
}

msg_bank_t *init_msg_bank(const shared_mem_t *const mem, size_t size, int tag) {
  msg_bank_t *bank = (msg_bank_t*)alloc_shared_mem(mem, sizeof(msg_bank_t));
  shared_mutex_init(&bank->mtx);
  shared_cond_init(&bank->wcd);
  memset(bank->channels, 0, sizeof(bank->channels));
  bank->active = 0;
  bank->size = size;
  for_each_queue(bank,
    glob_bank_queue(bank) =
      glob_cast(msg_queue_t, mem, new_msg_queue(mem, bank));
  );
  tag_shared_mem(mem, bank, tag);
  return bank;
}

msg_bank_t *join_msg_bank(const shared_mem_t *const mem, int tag) {
  msg_bank_t* bank = (msg_bank_t*)find_tagged_mem(mem, tag);
  if ( bank ) active_msg_bank(bank);
  return bank;
}

msg_bank_t *init_playback_msg_bank(const shared_mem_t *const mem, size_t size) {
  return init_msg_bank(mem, size, Playback);
}

msg_bank_t *init_capture_msg_bank(const shared_mem_t *const mem, size_t size) {
  return init_msg_bank(mem, size, Capture);
}

int free_msg_bank(const shared_mem_t *const mem, msg_bank_t *bank) {
  if (bank) {
    deactive_msg_bank(bank);
    if ( ! clients_shared_mem(mem) ) {
      for_each_queue(bank, free_msg_queue(mem, bank_current_queue(mem, bank)) );
    } else
      return BANK_HAS_CLIENTS;
  }
  return BANK_SECCESS;
}

msg_bank_t *join_playback_msg_bank(const shared_mem_t *const mem) {
  return join_msg_bank(mem, Playback);
}

msg_bank_t *join_capture_msg_bank(const shared_mem_t *const mem) {
  return join_msg_bank(mem, Capture);
}

void unjoin_msg_bank(msg_bank_t *bank) {
  deactive_msg_bank(bank);
}

int is_active_bank(const msg_bank_t *const bank) {
  int act = 0;
  NONCONST_MUTEX(m, bank);
  pthread_mutex_lock(m);
  act = bank->active;
  pthread_mutex_unlock(m);
  return act;
}

void active_msg_bank(msg_bank_t *const bank) {
  pthread_mutex_lock(&bank->mtx);
  bank->active = 1;
  pthread_cond_signal(&bank->wcd);
  pthread_mutex_unlock(&bank->mtx);
}

void deactive_msg_bank(msg_bank_t *const bank) {
  pthread_mutex_lock(&bank->mtx);
  bank->active = 0;
  pthread_cond_signal(&bank->wcd);
  pthread_mutex_unlock(&bank->mtx);
}

void wait_bank_activate(const msg_bank_t *const bank) {
  NONCONST_SYNC(m, w, bank);
  pthread_mutex_lock(m);
  while ( ! bank->active ) {
    pthread_cond_wait(w, m);
  }
  pthread_mutex_unlock(m);
}

int wait_timeout_bank_activate(const msg_bank_t *const bank, int sec) {
  NONCONST_SYNC(mtx, cond, bank);
  pthread_mutex_lock(mtx);
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += sec;
  while ( ! bank->active ) {
    int err = pthread_cond_timedwait(cond, mtx, &ts);
    if ( err != 0 ) {
      pthread_mutex_unlock(mtx);
      return err;
    }
  }
  pthread_mutex_unlock(mtx);
  return 0;
}

static int bank_has_msg(const shared_mem_t *const mem, const msg_bank_t *const bank) {
  int packs = 0;
  for_each_queue(bank, packs+=bank_current_queue(mem, bank)->blk_first?1:0);
  return packs;
}

void pop_all_msg_safe(const shared_mem_t *const mem,
                      const msg_bank_t *const bank,
                      msgblk_t **msg) {
  NONCONST_SYNC(mtx, cond, bank);
  pthread_mutex_lock(mtx);
  while ( !bank_has_msg(mem, bank) && bank->active ) {
    pthread_cond_wait(cond, mtx);
  }
  pthread_mutex_unlock(mtx);
  if ( ! bank->active ) {
    msg = NULL;
    return;
  }
  for_each_queue(bank,
    msg[__number] =
      pop_msg_nonblock(mem, bank_current_queue(mem, bank));
  );
}
