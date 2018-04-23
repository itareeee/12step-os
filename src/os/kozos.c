#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

/* =============================================================
 *                                         構造体定義  (private)
 * ============================================================= */

/* スレッド・コンテキスト */
typedef struct _kz_context { uint32 sp; /* スタック・ポインタ */ } kz_context;

/* タスク・コントロール・ブロック(TCB) */
typedef struct _kz_thread {

  struct _kz_thread *next;
  char name[THREAD_NAME_SIZE + 1];
  int priority;
  char *stack;

  uint32 flags;
#define KZ_THREAD_FLAG_READY (1<<0)

  struct { kz_func_t func; int argc; char **argv; } init; /* thread_init() に渡すパラメータ */
  struct { kz_syscall_type_t type; kz_syscall_param_t *param; } syscall; /* システム・コール用バッファ */

  kz_context context; /* コンテキスト情報 */

} kz_thread;

typedef struct _kz_msgbuf {
  struct _kz_msgbuf *next;
  kz_thread *sender;
  struct { int size; char *p; } param;
} kz_msgbuf;

typedef struct _kz_msgbox {
  kz_thread *receiver;
  kz_msgbuf *head;
  kz_msgbuf *tail;

  /*
   * H8は16ビットCPUなので、32ビット整数に対する乗算命令が無い。
   * よって構造体のサイズが2の塁乗になってないと、構造体の配列のインデックス計算で
   * 乗算が使われて「___mulsi3 が無い」などのリンクエラーとなる場合がある。
   * （2の累乗ならばシフト演算でソフトウェアエミュレーションされるので問題ない）
   */
  long dummy[1];
} kz_msgbox;

/* スレッドのレディー・キュー */
static struct { 
  kz_thread *head; 
  kz_thread *tail; 
} readyque[PRIORITY_NUM];

static kz_thread *current; /* カレントスレッド */
static kz_thread threads[THREAD_NUM]; /* TCB: タスク・コントロール・ブロック */
static kz_handler_t handlers[SOFTVEC_TYPE_NUM]; /* 割り込みハンドラ */
static kz_msgbox msgboxes[MSGBOX_ID_NUM];

void dispatch(kz_context *context);

/* =============================================================
 *                                      readyque 操作  (private)
 * ============================================================= */

static int getcurrent(void)
{
  if (current == NULL) return -1;
  if (!(current->flags & KZ_THREAD_FLAG_READY)) return 1; /* すでに無い場合は無視 */

  /* カレント・スレッドは必ず先頭にあるはずなので、先頭から抜き出す */
  readyque[current->priority].head = current->next;
  if (readyque[current->priority].head == NULL) readyque[current->priority].tail = NULL;

  current->flags &= ~KZ_THREAD_FLAG_READY;
  current->next = NULL;

  return 0;
}

static int putcurrent(void)
{
  if (current == NULL) return -1;
  if (current->flags & KZ_THREAD_FLAG_READY) return 1; /* すでにある場合は無視 */

  /* レディーキューの末尾に接続する */
  if (readyque[current->priority].tail) readyque[current->priority].tail->next = current;
  else readyque[current->priority].head = current;

  readyque[current->priority].tail = current;
  current->flags |= KZ_THREAD_FLAG_READY;

  return 0;
}

static void thread_end(void)
{
  kz_exit();
}

/* =============================================================
 *                                         thread 操作 (private)
 * ============================================================= */
/* スレッドのスタート・アップ */
static void thread_init(kz_thread *thp)
{
  /* スレッドのメイン関数を呼び出す */ 
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end();
}

/* システム・コールの処理(kz_run(): スレッドの起動) */
static kz_thread_id_t thread_run(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
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
  thp->priority = priority;
  thp->flags = 0;
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
   * スレッドの優先度がゼロの時には、割り込み禁止スレッドとする
   */
  *(--sp) = (uint32)thread_init | ((uint32)(priority ? 0 : 0xc0) << 24);

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
  putcurrent();

  /* 新規作成したスレッドをレディーキューに接続する */
  current = thp;
  putcurrent();

  return (kz_thread_id_t)current;
}

/* =============================================================
 *                              システムコール個別処理 (private)
 * ============================================================= */

/* kz_exit(): スレッドの終了 */
static int thread_exit(void) 
{
  /*
   * 本来ならスタックも開放して再利用できるようにすべきだが省略
   * このためスレッドを頻繁に生成・消去するようなことは現状できない
   */
  puts(current->name);
  puts(" EXIT.\n");
  memset(current, 0, sizeof(*current));
  return 0;
}

/* kz_wait(): スレッドの実行権破棄 */
static int thread_wait(void)
{
  putcurrent();
  return 0;
}

/* kz_sleep(): スレッドのスリープ */
static int thread_sleep(void)
{
  return 0;
}

/* kz_wakeup(): スレッドのウェイクアップ */
static int thread_wakeup(kz_thread_id_t id)
{
  putcurrent(); /* ウェイクアップを呼び出したスレッドをレディーキューに戻す */

  /* 指定されたスレッドをレディーキューに接続してウェイクアップする */
  current = (kz_thread *)id;
  putcurrent();

  return 0;
}

/* kz_getid(): スレッドIDの取得 */
static kz_thread_id_t thread_getid(void)
{
  putcurrent();
  return (kz_thread_id_t)current;
}

/* kz_chpri(): スレッド優先度変更 */
static int  thread_chpri(int priority)
{
  int old = current->priority;
  if (priority >= 0) current->priority = priority;
  putcurrent(); /* 新しい優先度のレディーキューに繋ぎ直す */
  return old;
}

/* kz_kmalloc(): 動的メモリ確保 */
static void *thread_kmalloc(int size)
{
  putcurrent();
  return kzmem_alloc(size);
}

/* kz_kmfree(): 動的メモリ解放 */
static int thread_kmfree(char *p)
{
  kzmem_free(p);
  putcurrent();
  return 0;
}

static void sendmsg(kz_msgbox *mboxp, kz_thread *thp, int size, char *p)
{
  kz_msgbuf *mp;
  mp = (kz_msgbuf *)kzmem_alloc(sizeof(*mp));
  if (mp == NULL)
    kz_sysdown();

  mp->next        = NULL;
  mp->sender      = thp;
  mp->param.size  = size;
  mp->param.p     = p;
  
  /* メッセージボックスの末尾にメッセージを接続する */
  if (mboxp->tail) {
    mboxp->tail->next = mp;
  } else {
    mboxp->head = mp;
  }
  mboxp->tail = mp;
}

static void recvmsg(kz_msgbox *mboxp)
{
  kz_msgbuf *mp;
  kz_syscall_param_t *p;

  /* メッセージボックスの先頭メッセージを抜き出す */
  mp = mboxp->head;
  mboxp->head = mp->next;
  if (mboxp->head == NULL)
    mboxp->tail = NULL;
  mp->next = NULL;
  
  /* メッセージを受信するスレッドに返す値を設定する */
  p = mboxp->receiver->syscall.param;
  p->un.recv.ret = (kz_thread_id_t)mp->sender;
  if (p->un.recv.sizep)
    *(p->un.recv.sizep) = mp->param.size;
  if (p->un.recv.pp)
    *(p->un.recv.pp) = mp->param.p;

  /* 受信待ちスレッドがいなくなるのでNULLに戻す */
  mboxp->receiver = NULL;

  kzmem_free(mp);
}

/* kz_send(): メッセージ送信 */
static int thread_send(kz_msgbox_id_t id, int size, char *p)
{
  kz_msgbox *mboxp = &msgboxes[id];

  putcurrent();
  sendmsg(mboxp, current, size, p);

  if (mboxp->receiver) {
    current = mboxp->receiver; /* 受信待ちスレッド */
    recvmsg(mboxp); /* メッセージの受信処理 */
    putcurrent(); /* 受信により動作可能になったのでブロック解除する */
  }

  return size;
}

/* kz_recv(): メッセージ受信 */
static kz_thread_id_t thread_recv(kz_msgbox_id_t id, int *sizep, char **pp)
{
  kz_msgbox *mboxp = &msgboxes[id];

  if (mboxp->receiver)
    kz_sysdown();

  mboxp->receiver = current;

  if (mboxp->head == NULL) {
    /*
     * メッセージボックスにメッセージが無いのでスレッドをスリープ。
     * システムコールがブロックする
     * (システムコール処理先頭で getcurrent() により呼び出し元スレッドをレディーキューから抜き出してる)
     */
    return -1;
  }

  recvmsg(mboxp);
  putcurrent();

  return current->syscall.param->un.recv.ret;
}

/* kz_setintr(): 割り込みハンドラの登録(kz_start() からの呼び出しもある) */
static int thread_setintr(softvec_type_t type, kz_handler_t handler)
{
  static void thread_intr(softvec_type_t type, unsigned long sp);

  /*
   * 割り込みを受け付けるために、ソフトウェア割り込みベクタに
   * OSの割り込み処理の入り口となる関数を登録する
   */
  softvec_setintr(type, thread_intr);

  handlers[type] = handler; /* OS側から呼び出す割り込みハンドラを登録 */

  /* kz_start() 経由ならNULLで無意味、システムコール経由ならレディーキューに戻す、*/
  putcurrent(); 

  return 0;
}

/* =============================================================
 *                      システムコール／サービスコール (private)
 * ============================================================= */

static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *p)
{
  /* システムコールの実行中に current が書き換わるので注意 */
  switch (type) {
    case KZ_SYSCALL_TYPE_RUN:
      p->un.run.ret = thread_run( p->un.run.func, p->un.run.name,
                                  p->un.run.priority, p->un.run.stacksize,
                                  p->un.run.argc, p->un.run.argv);
      break;

    case KZ_SYSCALL_TYPE_EXIT:
      thread_exit(); /* TCB が消去されるので、戻り値を書き込んではいけない*/
      break;

    case KZ_SYSCALL_TYPE_WAIT:
      p->un.wait.ret = thread_wait();
      break;

    case KZ_SYSCALL_TYPE_SLEEP:
      p->un.sleep.ret = thread_sleep();
      break;

    case KZ_SYSCALL_TYPE_WAKEUP:
      p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
      break;

    case KZ_SYSCALL_TYPE_GETID:
      p->un.getid.ret = thread_getid();
      break;

    case KZ_SYSCALL_TYPE_CHPRI:
      p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
      break;

    case KZ_SYSCALL_TYPE_KMALLOC:
      p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
      break;

    case KZ_SYSCALL_TYPE_KMFREE:
      p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
      break;

    case KZ_SYSCALL_TYPE_SEND:
      p->un.send.ret = thread_send(p->un.send.id, p->un.send.size, p->un.send.p);
      break;

    case KZ_SYSCALL_TYPE_RECV:
      p->un.recv.ret = thread_recv(p->un.recv.id, p->un.recv.sizep, p->un.recv.pp);
      break;

    case KZ_SYSCALL_TYPE_SETINTR:
      p->un.setintr.ret = thread_setintr(p->un.setintr.type, p->un.setintr.handler);
      break;

    default:
      break;
  }
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
  /*
   * システム・コールを呼び出したスレッドをレディーキューから
   * 外した状態で処理関数を呼び出す。このためシステムコールを
   * 呼び出したスレッドをそのまま動作継続させたい場合には
   * 処理関数の内部で putcurrent() を行う必要がある。
   */
  getcurrent();
  call_functions(type, p);
}

static void srvcall_proc(kz_syscall_type_t type, kz_syscall_param_t *p)
{
  /*
   * システム・コールとサービス・コールの処理関数の内部で、
   * システムコールの実行したスレッドIDを得るために current を参照している
   * 部分があり（たとえば thread_send() など）、current が残っていると
   * 誤作動するためNULLに設定する。
   * サービスコールは thread_intrvec() 内部の割り込みハンドラ呼び出しの
   * 延長で呼ばれているはずなので、呼び出し後に thread_intrvec() で
   * スケジューリング処理が行われ、current は再設定される。
   */
  current = NULL;
  call_functions(type, p);
}

/* =============================================================
 *                                            割り込み (private)
 * ============================================================= */
static void schedule(void)
{
  int i;
  for (i = 0; i < PRIORITY_NUM; i++) {
    if (readyque[i].head) break;
  }
  if (i == PRIORITY_NUM) kz_sysdown();
  current = readyque[i].head; 
}

static void syscall_intr(void)
{
  syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void)
{
  puts(current->name);
  puts(" DOWN.\n");
  getcurrent(); /* レディーキューから外す */
  thread_exit();
}

/* 割り込み処理の入口関数 */
static void thread_intr(softvec_type_t type, unsigned long sp)
{
  /* カレンド・スレッドのコンテキストを保存する */
  current->context.sp = sp;

  /*
   * 割り込みごとの処理を実行する
   * SOFTVEC_TYPE_SYSCALL, SOFTVEC_TYPE_SOFTERR の場合は
   * syscall_intr(), softerr_intr() がハンドラに登録されているので、
   * それらが実行される
   * それ以外の場合は、kz_setintr() によってユーザ登録されたハンドラが実行される
   */
  if (handlers[type])
    handlers[type]();

  schedule(); /* スレッドのスケジューリング */

  /*
   * スレッドのディスパッチ
   * (dispatch() 関数の本体は startup.s にあり,アセンブラで記述されている)
   */
  dispatch(&current->context);
  /* ここには返ってこない */
}

/* =============================================================
 *                                       ライブラリ関数 (public)
 * ============================================================= */

void kz_start(kz_func_t func, char *name, int priority, int stacksize, int argc, char *argv[])
{
  kzmem_init(); /* 動的メモリの初期化 */

  /*
   * 以降で呼び出すスレッド関連のライブラリ関連の内部で current を
   * 見ている場合があるので、current を NULL に初期化しておく
   */
  current = NULL;

  memset(readyque, 0, sizeof(readyque));
  memset(threads, 0, sizeof(threads));
  memset(handlers, 0, sizeof(handlers));
  memset(msgboxes, 0, sizeof(msgboxes));

  /* 割り込みハンドラの登録 */
  thread_setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr); /* システム・コール */
  thread_setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr); /* ダウン要因発生 */

  //TODO: システム・コール不可なのはなぜ？
  /* システム・コール発行不可なので直接関数を呼び出してスレッド作成する */
  current = (kz_thread *)thread_run(func, name, priority, stacksize, argc, argv);

  dispatch(&current->context); /* 最初のスレッドを起動 */

  /* ここには返ってこない */
}

void kz_sysdown(void)
{
  puts("system error!\n");
  while (1)
    ;
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
  current->syscall.type = type;
  current->syscall.param = param;
  asm volatile ("trapa #0"); /* トラップ割り込み発行 */
}

void kz_srvcall(kz_syscall_type_t type, kz_syscall_param_t *param)
{
  srvcall_proc(type, param);
}
