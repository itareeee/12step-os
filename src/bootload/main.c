#include "defines.h"
#include "serial.h"
#include "xmodem.h"
#include "lib.h"

static int init(void)
{
  /* 以下はリンカ・スクリプトで定義してあるシンボル */
  extern int erodata, data_start, edata, bss_start, ebss;

  /*
   * データ領域とBSS領域を初期化する. この処理行こうでないと,
   * グローバル変数が初期化されていないので注意.
   */
  memcpy(&data_start, &erodata, (long)&edata - (long)&data_start);
  memset(&bss_start, 0, (long)&ebss - (long)&bss_start);

  serial_init(SERIAL_DEFAULT_DEVICE);

  return 0;
}

/* メモリの16進ダンプ出力 */
static int dump(char *buf, long size)
{
  long i;
  if (size < 0) {
    puts("no data.\n");
    return -1;
  } 

  for (i = 0; i < size; i++) {
    putxval(buf[i], 2);
    if ((i & 0xf) == 15) {
      puts("\n");
    } else {
      if ((i & 0xf) == 7) puts(" ");
      puts(" ");
    }
  }
  puts("\n");

  return 0;
}

static void wait()
{
  volatile long i;
  for (i = 0; i < 300000; i++)
    ;
}


//int global_data = 0x10;
//int global_bss;
//static int static_data = 0x20;
//static int static_bss;
//
//static void printval(void)
//{
//  puts("global_data = "); putxval(global_data, 0); puts("\n");
//  puts("global_bss  = "); putxval(global_bss,  0); puts("\n");
//  puts("static_data = "); putxval(static_data, 0); puts("\n");
//  puts("static_bss  = "); putxval(static_bss,  0); puts("\n");
//}

int main(void)
{
  static char buf[16];
  static long size = -1;
  static unsigned char *loadbuf = NULL;
  extern int buffer_start;
  
  init();

  puts("kzload (kozos boot loader) started.\n");

  while (1) {
    puts("kzload> "); /* プロンプト表示 */
    gets(buf); /* シリアルからのコマンド受信 */

    if (!strcmp(buf, "load")) { /* XMODEM でのファイルのダウンロード */
      loadbuf = (char *)(&buffer_start);
      size = xmodem_recv(loadbuf);
      wait(); /* 転送アプリが終了し端末アプリに制御が戻るまで待ち合わせる */

      if (size < 0) {
       puts("\nXMODEM receive error!\n");
      } else {
       puts("\nXMODEM receive suceeded!\n");
      }

    } else if (!strcmp(buf, "dump")) { /* メモリの16進数ダンプ出力 */
      puts("size: ");
      putxval(size, 0);
      puts("\n");
      dump(loadbuf, size);

    } else {
      puts("unknown\n");
    }
  }

//  puts("Hellow World\n");

//  printval();
//  puts("overwrite variables.\n");
//  global_data = 0x20;
//  global_bss = 0x30;
//  static_data = 0x40;
//  static_bss = 0x50;
//  printval();
  
//  while (1)
//    ;

  return 0;
}

