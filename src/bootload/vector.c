#include "defines.h"

extern void start(void);        /* スタートアップ */
extern void intr_softerr(void); /* ソフトウェアエラー */
extern void intr_syscall(void); /* システムコール */
extern void intr_serintr(void); /* シリアル割り込み */

/*
 * 割り込みベクタの指定.
 * リンカ・スクリプトの定義により、先頭番地に配置される.
 */
void (*vectors[])(void) = {
  start, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_syscall, intr_softerr, intr_softerr, intr_softerr,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  intr_serintr, intr_serintr, intr_serintr, intr_serintr, 
  intr_serintr, intr_serintr, intr_serintr, intr_serintr, 
  intr_serintr, intr_serintr, intr_serintr, intr_serintr, 
};
