#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

kz_thread_id_t test09_1_id;
kz_thread_id_t test09_2_id;
kz_thread_id_t test09_3_id;

/* システム・タスクとユーザ・スレッドの起動 */
static int start_threads(int argc, char *argv[])
{
  puts("run 1 in\n");
  test09_1_id = kz_run(test09_1_main, "test09_1", 1, 0x100, 0, NULL);
                                                          
  puts("run 2 in\n");                                       
  test09_2_id = kz_run(test09_2_main, "test09_2", 2, 0x100, 0, NULL);
                                                          
  puts("run 3 in\n");                                       
  test09_3_id = kz_run(test09_3_main, "test09_3", 3, 0x100, 0, NULL);

  kz_chpri(15); /* 優先順位を下げて、アイドルスレッドに移行する */
  INTR_ENABLE; /* TODO: 8th step では有効化してなかった？？ */
  while (1) {
    asm volatile ("sleep"); /* 省電力モード */
  }
  return 0;
}

int main(void)
{
  INTR_DISABLE;

  puts("kozos boot succeed!\n");

  /* OSの動作開始 */
  kz_start(start_threads, "idle", 0, 0x100, 0, NULL);
  /* ここには戻ってこない */

  return 0;
}

