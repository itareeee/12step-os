#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
//#include "sysall.h"
#include "lib.h"

#define THREAD_NUM 6
#define THREAD_NAME_SIZE 15

/* スレッド・コンテキスト */
typedef struct _kz_context {
  uint32 sp; /* スタック・ポインタ */
} kz_context;

/* タスク・コントロール・ブロック(TCB) */
typedef struct _kz_thread {
  struct _kz_thread *next;
  char name[THREAD_NAME_SIZE + 1]; /* スレッド名 */
  char *stack; /* スタック */

  struct { /* スレッドのスタートアップ(thread_init())に渡すパラメータ */
    kz_func_t func; 
    int argc;
    int **argv;
  } init;

  //struct { /* システム・コール用バッファ */
  //  kz_syscall_type_t type;
  //  kz_syscall_param_t *param; 
  //} syscall;

  kz_context context; /* コンテキスト情報 */
} kz_thread;

/* スレッドのレディー・キュー */
static struct {
  kz_thread *head;
  kz_thread *tail;
} readyque;

static kz_thread *current; /* カレントスレッド */
static kz_thread threads[THREAD_NUM]; /* TCB: タスク・コントロール・ブロック */
static kz_handler_t handlers[SOFTVEC_TYPE_NUM]; /* 割り込みハンドラ */

void dispatch(kz_context *context);

static void thread_end(void)
{
  puts("supposed to call kz_exit()...");
  //kz_exit();
}

/* スレッドのスタート・アップ */
static void thread_init(kz_thread *thp)
{
  /* スレッドのメイン関数を呼び出す */ 
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end();
}

/* システム・コールの処理(kz_run(): スレッドの起動) */
static kz_thread_id_t thread_run(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
  int i;
  kz_thread *thp;
  uint32 *sp;
  extern char userstack; /* リンカ・スクリプトで定義されるスタック領域 */
  static char *thread_stack = &userstack;

  /* 空いているタスク・コントロール・ブロックを検索 */
  for (i = 0; i  < THREAD_NUM; i++) {
    thp = &threads[i];
    if (!thp->init.func) /* 見つかった */
      break;
  }
  if (i == THREAD_NUM) /* 見つからなかった */
    return -1;

  memset(thp, 0, sizeof(*thp));

  /* タスク・コントロール・ブロック(TCB)の設定 */
  strcpy(thp->name, name);
  thp->next     = NULL;
  thp->init.func = func;
  thp->init.argc = argc;
  thp->init.argv = argv;

  /* スタック領域を獲得 */
  memset(thread_stack, 0, stacksize);
  thread_stack += stacksize;

  thp->stack = thread_stack;

  /* スタックの初期化 */
  sp = (uint32 *)thp->stack;
  *(--sp) = (uint32)thread_end;

  /*
   * プログラムカウンタを設定する
   */
  *(--sp) = (uint32)thread_init;

  *(--sp) = 0; /* ER6 */
  *(--sp) = 0; /* ER5 */
  *(--sp) = 0; /* ER4 */
  *(--sp) = 0; /* ER3 */
  *(--sp) = 0; /* ER2 */
  *(--sp) = 0; /* ER1 */

  /* スレッドのスタートアップ(thread_init())にわたす引数 */
  *(--sp) = (uint32)thp; /* ER0 */

  /* スレッドのコンテキストを設定 */
  thp->context.sp = (uint32)sp;

  /* システム・コールを呼び出したスレッドをレディーキューに戻す */
  //putcurrent();

  /* 新規作成したスレッドをレディーキューに接続する */
  current = thp;
  //putcurrent();

  return (kz_thread_id_t)current;
}


void kz_start(kz_func_t func, char *name, int stacksize, int argc, char *argv[])
{
  /*
   * 以降で呼び出すスレッド関連のライブラリ関連の内部で current を
   * 見ている場合があるので、current を NULL に初期化しておく
   */
  current = NULL;

  readyque.head = readyque.tail = NULL;
  memset(threads, 0, sizeof(threads));
  memset(handlers, 0, sizeof(handlers));

  //TODO: un-comment later
  /* 割り込みハンドラの登録 */
  //setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); /* システム・コール */
  //setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); /* ダウン要因発生 */

  //TODO: システム・コール不可なのはなぜ？
  /* システム・コール発行不可なので直接関数を呼び出してスレッド作成する */
  current = (kz_thread *)thread_run(func, name, stacksize, argc, argv);

  /* 最初のスレッドを起動 */
  dispatch(&current->context);

  /* ここには返ってこない */
}
