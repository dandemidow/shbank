// Copyright (C) 2015, Danila Demidow
// Author: dandemidow@gmail.com (Danila Demidow)

#ifndef _MONITORMSG_H_
#define _MONITORMSG_H_

#include <time.h>

#include "bank.h"

/*
 * There is common rule for the all functions:
 *  all input ptr should be local
 *  all output ptr should be local
 */

#define open_shared_banks(name, state) init_shared_mem(SHM_MEM_SIZE, (name), (state))
#define wait_banks_join(mem) wait_shared_client_init(mem)
#define wait_banks_unjoin(mem) wait_shared_client_exit(mem)
#define close_shared_banks(mem) close_shared_mem(mem)

shared_mem_t *join_to_shared_banks(const char *name, int *state);
void unjoin_shared_banks(shared_mem_t *);

pid_t get_master_pid(const shared_mem_t *const mem);

// a simple func for a msg processing in the first bank's channel
msgblk_t *prep_msg(const shared_mem_t *const, int *status);
int push_msg(const shared_mem_t *const,
             const msg_bank_t *const bank,
             msgblk_t *msg,
             int num);

int push_msg_first(const shared_mem_t *const,
                   const msg_bank_t *const bank,
                   msgblk_t *msg,
                   int num);

int push_msg_copy(const shared_mem_t *const,
                  const msg_bank_t *const bank,
                  msgblk_t *msg,
                  int num);

int push_msg_first_copy(const shared_mem_t *const,
                        const msg_bank_t *const bank,
                        msgblk_t *msg,
                        int num);

msgblk_t *pop_msg(const shared_mem_t *const,
                  const msg_bank_t *const bank,
                  int num,
                  int *status);
#endif  // _MONITORMSG_H_
