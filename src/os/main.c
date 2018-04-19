#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

/* システム・タスクとユーザ・スレッドの起動 */
static int start_threads(int argc, char *argv[])
{
  kz_run(test10_1_main, "test10_1", 1, 0x100, 0, NULL);

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

