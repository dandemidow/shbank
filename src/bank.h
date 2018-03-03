#ifndef _SHMOBANK_BANK_H_
#define _SHMOBANK_BANK_H_

#include "queue.h"

#define BANK_SECCESS 0
#define BANK_HAS_CLIENTS -10

// return channels[number] global pointer;
#define glob_bank_channel(bank, number)  \
  ( *((bank)->channels + number))

msg_queue_t *bank_queue(const shared_mem_t *const mem, const msg_bank_t *const bank, int number);

#define bank_current_queue(mem, bank) \
  bank_queue(mem, bank, __number)

#define glob_bank_queue(bank) \
  glob_bank_channel(bank, __number)

#define for_each_queue(bank, expr) \
  { \
    size_t __number; \
    for ( __number=0; __number < (bank)->size; ++__number ) { \
      { expr; } \
    } \
  }

// msg_bank_t or an another shared struct should not contains any shared_mem_t element
typedef struct _msg_bank_t {
  pthread_mutex_t mtx;
  pthread_cond_t wcd;
  size_t size;
  msg_queue_t *channels[MAXNBCHANNELS];
  int active;
} msg_bank_t;

enum { Playback = 1, Capture };

msg_bank_t *init_msg_bank(const shared_mem_t *const, size_t size, int tag);
msg_bank_t *init_playback_msg_bank(const shared_mem_t *const, size_t size);
msg_bank_t *init_capture_msg_bank(const shared_mem_t *const, size_t size);
int free_msg_bank(const shared_mem_t *const, msg_bank_t *bank);

msg_bank_t *join_msg_bank(const shared_mem_t *const, int tag);
msg_bank_t *join_playback_msg_bank(const shared_mem_t *const);
msg_bank_t *join_capture_msg_bank(const shared_mem_t *const);
void unjoin_msg_bank(msg_bank_t *);

int is_active_bank(const msg_bank_t *const bank);
void active_msg_bank(msg_bank_t *const bank);
void deactive_msg_bank(msg_bank_t *const bank);
void wait_bank_activate(const msg_bank_t *const bank);
int wait_timeout_bank_activate(const msg_bank_t *const bank, int sec);

void pop_all_msg_safe(const shared_mem_t *const,
                      const msg_bank_t *const bank,
                      msgblk_t **msg);
#endif  // _SHMOBANK_BANK_H_
