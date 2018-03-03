#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <shmobank/monmsg.h>

static void handler() {
//  (void)(signum);
  return;
}

START_TEST(test_queue)
{
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *shared_bank = init_playback_msg_bank(mem, 2);  // for 2 channels with tag 1
  ck_assert_ptr_ne(shared_bank, NULL);
  ck_assert_int_eq(shared_bank->size, 2);

  msgblk_t loc_msg;
  msgblk_t *msg = prep_msg(mem, NULL);
  ck_assert_ptr_ne(msg, NULL);
  msg->lvl = 10;
  msg->txt[0] = '0';
  msg->txt[1] = '\0';

  int ret = 0;
  ret = push_msg(mem, shared_bank, msg, 0);
  ck_assert_int_lt(ret, 0);
  active_msg_bank(shared_bank);
  ret = push_msg(mem, shared_bank, msg, 0);
  ck_assert_int_eq(ret, 0);
  ret = push_msg(mem, shared_bank, &loc_msg, 0);
  ck_assert_int_lt(ret, 0);

  msgblk_t *blk = pop_msg(mem, shared_bank, 0, NULL);
  ck_assert_ptr_ne(blk, NULL);
  ck_assert_int_eq(blk->lvl, 10);
  ck_assert_int_eq(blk->txt[0], '0');
  ck_assert_int_eq(blk->txt[1], 0);
  free_msg(mem, blk);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
}
END_TEST

START_TEST(test_process_queue)
{
  pid_t pid = fork();
  if ( !pid ) {
    // the child path
    signal(SIGUSR1, handler);
    pause();
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *shared_bank = join_playback_msg_bank(mem);
    ck_assert_int_eq(shared_bank->size, 2);
    msgblk_t *blk = pop_msg(mem, shared_bank, 0, NULL);
    ck_assert_int_eq(blk->lvl, 10);
    free_msg(mem, blk);
    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *shared_bank = init_playback_msg_bank(mem, 2);  // for 2 channels with tag 1
  ck_assert_ptr_ne(shared_bank, NULL);
  ck_assert_int_eq(shared_bank->size, 2);
  usleep(100);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(shared_bank);

  msgblk_t *msg = prep_msg(mem, NULL);
  ck_assert_ptr_ne(msg, NULL);
  msg->lvl = 10;
  msg->next = 0;
  msg->txt[0] = '0';
  msg->txt[1] = '\0';
  push_msg(mem, shared_bank, msg, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_process_join_unjoin)
{
  pid_t pid = fork();
  if ( !pid ) {
    signal(SIGUSR1, handler);
    pause();
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *shared_bank = join_playback_msg_bank(mem);
    ck_assert_ptr_ne(shared_bank, NULL);
    usleep(1000);
    unjoin_msg_bank(shared_bank);
    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *shared_bank = init_playback_msg_bank(mem, 2);  // for 2 channels with tag 1
  ck_assert_ptr_ne(shared_bank, NULL);
  usleep(100);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(shared_bank);

  int cli = clients_shared_mem(mem);
  int active = is_active_bank(shared_bank);
  ck_assert_int_eq(cli, 1);
  ck_assert_int_eq(active, 1);

  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);

  cli = clients_shared_mem(mem);
  active = is_active_bank(shared_bank);
  ck_assert_int_eq(cli, 0);
  ck_assert_int_eq(active, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
}
END_TEST

START_TEST(test_bank)
{
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *shared_bank = init_playback_msg_bank(mem, 2);  // for 2 channels with tag 1
  ck_assert_ptr_ne(shared_bank, NULL);
  ck_assert_int_eq(shared_bank->size, 2);

  msgblk_t *msg = prep_msg(mem, NULL);
  ck_assert_ptr_ne(mem, NULL);
  msg->lvl = 10;
  msg->txt[0] = '0';
  msg->txt[1] = '\0';
  active_msg_bank(shared_bank);
  int psh_ret = push_msg(mem, shared_bank, msg, 0);
  ck_assert_int_eq(psh_ret, 0);

  msgblk_t *blk[shared_bank->size];
  int msg_qnt = 0;

  pop_all_msg_safe(mem, shared_bank, blk);
  for_each_queue(shared_bank,
    if ( blk[__number] ) {
      ck_assert_int_eq(blk[__number]->lvl, 10);
      msg_qnt++;
      free_msg(mem, blk[__number]);
      } );
  ck_assert_int_eq(msg_qnt, 1);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
}
END_TEST


START_TEST(test_process_bank)
{
  pid_t pid = fork();
  if ( !pid ) {
    // the child path
    signal(SIGUSR1, handler);
    pause();
    int msg_qnt = 0;
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
//    printf("shared mem addr 0x%08X : 0x%08X\n", (int)mem->addr, (int)mem->base);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *shared_bank = join_playback_msg_bank(mem);
    ck_assert_int_eq(shared_bank->size, 2);
    msgblk_t *blk[shared_bank->size];

    pop_all_msg_safe(mem, shared_bank, blk);
    for_each_queue(shared_bank,
      if ( blk[__number] ) {
        ck_assert_int_eq(blk[__number]->lvl, 10);
        msg_qnt++;
        free_msg(mem, blk[__number]);
        } );
    ck_assert_int_eq(msg_qnt, 1);

    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
//  printf("shared mem addr 0x%08X\n", (int)mem->addr);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *shared_bank = init_playback_msg_bank(mem, 2);  // for 2 channels with tag 1
  ck_assert_ptr_ne(shared_bank, NULL);
  ck_assert_int_eq(shared_bank->size, 2);

  usleep(100);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);

  wait_bank_activate(shared_bank);
  msgblk_t *msg = prep_msg(mem, NULL);
  ck_assert_ptr_ne(msg, NULL);
  msg->lvl = 10;
  msg->next = 0;
  msg->txt[0] = '0';
  msg->txt[1] = '\0';
  push_msg(mem, shared_bank, msg, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_process_echo)
{
  pid_t pid = fork();
  if ( !pid ) {
    signal(SIGUSR1, handler);
    pause();
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *play_bank = join_playback_msg_bank(mem);
    ck_assert_ptr_ne(play_bank, NULL);
    ck_assert_int_eq(play_bank->size, 2);
    msg_bank_t *capt_bank = join_capture_msg_bank(mem);
    ck_assert_ptr_ne(capt_bank, NULL);
    ck_assert_ptr_ne(capt_bank, play_bank);
    ck_assert_int_eq(capt_bank->size, 2);

    int last;
    while ( 1 ) {
      msgblk_t *msg = pop_msg(mem, play_bank, 0, NULL);
      ck_assert_int_eq(msg->lvl, 10);
      last = msg->err;
      free_msg(mem, msg);
      msg = prep_msg(mem, NULL);
      ck_assert_ptr_ne(msg, NULL);
      msg->lvl = 20;
      msg->err = last;
      push_msg(mem, capt_bank, msg, 0);
      if (last) break;
    }
    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *play_bank = init_playback_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(play_bank, NULL);
  ck_assert_int_eq(play_bank->size, 2);
  msg_bank_t *capt_bank = init_capture_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(capt_bank, NULL);
  ck_assert_ptr_ne(capt_bank, play_bank);
  ck_assert_int_eq(capt_bank->size, 2);

  usleep(1000);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(play_bank);
  wait_bank_activate(capt_bank);

  int i, qnt = 100;
  for(i=0; i<qnt; ++i) {
    msgblk_t *msg = prep_msg(mem, NULL);
    msg->lvl = 10;
    msg->err = (i==qnt-1?1:0);
    push_msg(mem, play_bank, msg, 0);
    msg = pop_msg(mem, capt_bank, 0, NULL);
    ck_assert_int_eq(msg->lvl, 20);
    free_msg(mem, msg);
    if ( msg->err ) break;
  }

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);
}
END_TEST

msg_bank_t *banks[2];
static void handler_exit() {
//  (void)(signum);
  unjoin_msg_bank(banks[0]);
  unjoin_msg_bank(banks[1]);
  return;
}

typedef struct {
  shared_mem_t *mem;
  msg_bank_t *bank;
  int lvl;
  int count;
} test_args;

#define init_test_args(name, _mem, _bank, _lvl) \
  test_args name;\
  name.mem  = _mem; \
  name.bank = _bank; \
  name.count = 0; \
  name.lvl = _lvl;

static void *get_msg_thread(void *arg) {
  test_args *args = (test_args *)arg;
  while ( is_active_bank(args->bank) ) {
    msgblk_t *msg = pop_msg(args->mem, args->bank, 0, NULL);
    if ( !msg ) break;
    ck_assert_int_eq(msg->lvl, args->lvl);
    printf("#%d free msg 0x%08x\n", args->count++, (int)msg);
    free_msg(args->mem, msg);
  }
  return NULL;
}

static void *set_msg_thread(void *arg) {
  test_args *args = (test_args *)arg;
  while ( is_active_bank(args->bank) ) {
    usleep(1000*10);
    msgblk_t *msg = prep_msg(args->mem, NULL);
    printf("#%d prep msg 0x%08x\n", args->count, (int)msg);
    if ( !msg ) break;
    msg->lvl = args->lvl;
    push_msg(args->mem, args->bank, msg, 0);
    args->count++;
  }
  return NULL;
}

START_TEST(test_process_single_threads)
{
  pid_t pid = fork();
  if ( !pid ) {
    signal(SIGUSR1, handler);
    signal(SIGINT, handler_exit);
    pause();
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *play_bank = join_playback_msg_bank(mem);
    ck_assert_ptr_ne(play_bank, NULL);
    msg_bank_t *capt_bank = join_capture_msg_bank(mem);
    ck_assert_ptr_ne(capt_bank, NULL);
    ck_assert_ptr_ne(capt_bank, play_bank);
    banks[0] = play_bank;
    banks[1] = capt_bank;
    init_test_args(tstplay, mem, capt_bank, 20);

    pthread_t play_th;
    pthread_create(&play_th, 0, &get_msg_thread, &tstplay);

    pthread_join(play_th, NULL);

    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *play_bank = init_playback_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(play_bank, NULL);
  ck_assert_int_eq(play_bank->size, 2);
  msg_bank_t *capt_bank = init_capture_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(capt_bank, NULL);
  ck_assert_ptr_ne(capt_bank, play_bank);
  ck_assert_int_eq(capt_bank->size, 2);

  usleep(1000);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(play_bank);
  wait_bank_activate(capt_bank);

  init_test_args(tstcapt, mem, capt_bank, 20);

  pthread_t capt_th;
  pthread_create(&capt_th, 0, &set_msg_thread, &tstcapt);

  usleep(500000);
  kill(pid, SIGINT);

  pthread_join(capt_th, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);
}
END_TEST

START_TEST(test_process_rdwr_threads)
{
  pid_t pid = fork();
  if ( !pid ) {
    signal(SIGUSR1, handler);
    signal(SIGINT, handler_exit);
    pause();
    int status;
    shared_mem_t *mem = join_to_shared_banks("acars_modem", &status);
    ck_assert_ptr_ne(mem, NULL);
    ck_assert_int_eq(status, BUF_SUCCESS);
    msg_bank_t *play_bank = join_playback_msg_bank(mem);
    ck_assert_ptr_ne(play_bank, NULL);
    msg_bank_t *capt_bank = join_capture_msg_bank(mem);
    ck_assert_ptr_ne(capt_bank, NULL);
    ck_assert_ptr_ne(capt_bank, play_bank);
    banks[0] = play_bank;
    banks[1] = capt_bank;
    init_test_args(tstplay, mem, play_bank, 10);
    init_test_args(tstcapt, mem, capt_bank, 20);

    pthread_t play_th, capt_th;
    pthread_create(&play_th, 0, &get_msg_thread, &tstplay);
    pthread_create(&capt_th, 0, &set_msg_thread, &tstcapt);

    pthread_join(play_th, NULL);
    ck_assert_int_gt(tstplay.count, 0);
    pthread_join(capt_th, NULL);
    ck_assert_int_gt(tstcapt.count, 0);

    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  int status;
  shared_mem_t *mem = open_shared_banks("acars_modem", &status);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  ck_assert_int_eq(status, BUF_SUCCESS);
  msg_bank_t *play_bank = init_playback_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(play_bank, NULL);
  ck_assert_int_eq(play_bank->size, 2);
  msg_bank_t *capt_bank = init_capture_msg_bank(mem, 2);  // for 2 channels
  ck_assert_ptr_ne(capt_bank, NULL);
  ck_assert_ptr_ne(capt_bank, play_bank);
  ck_assert_int_eq(capt_bank->size, 2);

  usleep(1000);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(play_bank);
  wait_bank_activate(capt_bank);

  init_test_args(tstplay, mem, play_bank, 10);
  init_test_args(tstcapt, mem, capt_bank, 20);

  pthread_t play_th, capt_th;
  pthread_create(&play_th, 0, &set_msg_thread, &tstplay);
  pthread_create(&capt_th, 0, &get_msg_thread, &tstcapt);

  usleep(200000);
  kill(pid, SIGINT);

  pthread_join(play_th, 0);
  ck_assert_int_gt(tstplay.count, 0);
  pthread_join(capt_th, 0);
  ck_assert_int_gt(tstcapt.count, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int proc_status;
  waitpid(pid, &proc_status, 0);
  ck_assert_int_eq(proc_status, 0);
}
END_TEST

static void *get_msg_overflow(void *arg) {
  test_args *args = (test_args *)arg;
  while ( is_active_bank(args->bank) ) {
    usleep(1000*100);
    msgblk_t *msg = pop_msg(args->mem, args->bank, 0, NULL);
    if ( !msg ) break;
    int count = 0;
    memcpy(&count, msg->txt, sizeof(count));
    ck_assert_int_eq(msg->lvl, args->lvl+10);
    printf("#%d: %d free msg 0x%08x\n", args->count, count, (int)msg);
    free_msg(args->mem, msg);
    args->count++;
  }
  return NULL;
}

static void *set_msg_overflow(void *arg) {
  test_args *args = (test_args *)arg;
  while ( is_active_bank(args->bank) ) {
    usleep(100*10);
    msgblk_t *msg = prep_msg(args->mem, NULL);
    printf("#%d prep msg 0x%08x\n", args->count, (int)msg);
    if ( !msg ) {
      printf("# prep fail\n");
      usleep(50000);
      continue;
    }
    msg->lvl = args->lvl;
    memcpy(msg->txt, &args->count, sizeof(args->count));
    push_msg(args->mem, args->bank, msg, 0);
    args->count++;
  }
  return NULL;
}

START_TEST(test_process_overflow)
{
  pid_t pid = fork();
  if ( !pid ) {
    signal(SIGUSR1, handler);
    signal(SIGINT, handler_exit);
    pause();
    shared_mem_t *mem = join_to_shared_banks("acars_modem", NULL);
    ck_assert_ptr_ne(mem, NULL);
    msg_bank_t *play_bank = join_playback_msg_bank(mem);
    ck_assert_ptr_ne(play_bank, NULL);
    msg_bank_t *capt_bank = join_capture_msg_bank(mem);
    ck_assert_ptr_ne(capt_bank, NULL);
    ck_assert_ptr_ne(capt_bank, play_bank);
    banks[0] = play_bank;
    banks[1] = capt_bank;
    init_test_args(tstplay, mem, capt_bank, 10);

    pthread_t play_th;
    pthread_create(&play_th, 0, &get_msg_overflow, &tstplay);

    printf("test 9\n");
    pthread_join(play_th, NULL);

    unjoin_shared_banks(mem);
    return;
  }
  char *shifter = malloc(1000);
  shared_mem_t *mem_shifter = init_shared_mem(64, "acars_tst", NULL);
  shared_mem_t *mem = open_shared_banks("acars_modem", NULL);
  free(shifter);
  close_shared_mem(mem_shifter);
  ck_assert_ptr_ne(mem, NULL);
  msg_bank_t *play_bank = init_playback_msg_bank(mem, 1);  // for 1 channel
  ck_assert_ptr_ne(play_bank, NULL);
  ck_assert_int_eq(play_bank->size, 1);
  msg_bank_t *capt_bank = init_capture_msg_bank(mem, 1);  // for 1 channel
  ck_assert_ptr_ne(capt_bank, NULL);
  ck_assert_ptr_ne(capt_bank, play_bank);
  ck_assert_int_eq(capt_bank->size, 1);

  usleep(1000);
  kill(pid, SIGUSR1);
  wait_banks_join(mem);
  wait_bank_activate(play_bank);
  wait_bank_activate(capt_bank);

  init_test_args(tstcapt, mem, capt_bank, 20);

  pthread_t capt_th;
  pthread_create(&capt_th, 0, &set_msg_overflow, &tstcapt);

  usleep(5000000);
  kill(pid, SIGINT);

  pthread_join(capt_th, 0);

  wait_banks_unjoin(mem);
  close_shared_banks(mem);
  int status;
  waitpid(pid, &status, 0);
  ck_assert_int_eq(status, 0);
}
END_TEST

Suite * console_client_suite(void) {
    Suite *s;
    TCase *tc_core;

    s = suite_create("Modem MSK");
    tc_core = tcase_create("Core");

    tcase_set_timeout(tc_core, 0);

    tcase_add_test(tc_core, test_queue);
    tcase_add_test(tc_core, test_process_queue);
    tcase_add_test(tc_core, test_process_join_unjoin);
    tcase_add_test(tc_core, test_bank);
    tcase_add_test(tc_core, test_process_bank);
    tcase_add_test(tc_core, test_process_echo);
    tcase_add_test(tc_core, test_process_single_threads);
    tcase_add_test(tc_core, test_process_rdwr_threads);
    tcase_add_test(tc_core, test_process_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int number_failed;
    Suite *s;
    SRunner *sr;
    s = console_client_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
