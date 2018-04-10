#include "defines.h"
#include "serial.h"
#include "lib.h"

/* 1文字送信 */
int putc(unsigned char c)
{
  if (c == '\n')
    serial_send_byte(SERIAL_DEFAULT_DEVICE, '\r');
  serial_send_byte(SERIAL_DEFAULT_DEVICE, c);
}


/* 文字列送信 */
int puts(unsigned char *str)
{

  while (* str)
    putc(*(str++));
  return 0;
}
