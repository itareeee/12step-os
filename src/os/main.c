#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

//#include "defines.h"
//#include "intr.h"
//#include "interrupt.h"
//#include "serial.h"
//#include "lib.h"
//
//static void intr(softvec_type_t type, unsigned long sp)
//{
//  int c;
//  static char buf[32];
//  static int len;
//
//  c = getc();
//
//  if (c != '\n') {
//    buf[len++] = c;
//
//  } else {
//    buf[len++] = '\0';
//
//    if (!strncmp(buf, "echo", 4)) {
//      puts(buf + 4);
//      puts("\n");
//    } else {
//      puts("unknown.\n");
//    }
//    puts("> ");
//    len = 0;
//  }
//}

static int start_threads(int argc, char *argv[])
{
  puts("start_threads:beginning\n");

  int cnt = 0;
  int big_cnt = 0;
  while (1) {
    if (cnt > (10 * 1000)) {
      puts("initial thread yay\n");
      cnt = 0;
      big_cnt++;
    }
    
    if (big_cnt > 10){
      puts("initial thread END!!\n");
      break;
    }
    cnt++;
  }
  return 0;
}

int main(void)
{
  INTR_DISABLE;

  puts("kozos boot succeed!\n");

  /* OSの動作開始 */
  kz_start(start_threads, "start", 0x100, 0, NULL);
  /* ここには戻ってこない */

  return 0;

  //softvec_setintr(SOFTVEC_TYPE_SERINTR, intr);
  //serial_intr_recv_enable(SERIAL_DEFAULT_DEVICE);

  //puts("> ");
  //
  //INTR_ENABLE;

  //while (1) {
  //  asm volatile ("sleep"); /* 省電力モードに移行 */
  //}

  //return 0;
}

