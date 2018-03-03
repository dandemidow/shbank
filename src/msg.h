#ifndef _SHMOBANK_MSG_H_
#define _SHMOBANK_MSG_H_

#include <time.h>

#include <sharedmem/shalloc.h>

#include "limits.h"

#define MSG_SUCESS 0
#define MSG_NOT_ACTIVE_BANK -1
#define MSG_NULL_PTR -2
#define MSG_NOT_SHARED_PTR -3
#define MSG_NO_SHARED_MEM -4
#define MSG_NO_QUEUE -5

typedef struct mskblk_s {
  struct mskblk_s *next;
  time_t tm;
  unsigned int len;
  int lvl,err;
  unsigned char txt[SHM_MSG_TEXT];
} msgblk_t;

struct _msg_queue_t;
typedef struct _msg_queue_t msg_queue_t;
typedef void (*_msg_adder)(const shared_mem_t *const, msg_queue_t *, msgblk_t *);

msgblk_t *__pop_msg(const shared_mem_t *const, msg_queue_t *mq);
msgblk_t *__prep_msg(const shared_mem_t *const);

int push_msg_safe(const shared_mem_t *const, msg_queue_t *mq, msgblk_t *msg);
int push_msg_first_safe(const shared_mem_t *const, msg_queue_t *mq, msgblk_t *msg);
msgblk_t *pop_msg_nonblock(const shared_mem_t *const, msg_queue_t *queue);
msgblk_t *pop_msg_safe(const shared_mem_t *const, msg_queue_t *queue, int *status);
void free_msg(const shared_mem_t *const, msgblk_t *msg);

#endif  // _SHMOBANK_MSG_H_
