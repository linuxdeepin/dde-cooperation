// LD_PRELOAD 桩: 捕获终止信号, 调用 exit(0) 触发 atexit -> gcov __gcov_exit 落盘。
// 关键: 用 exit() 不能用 _exit() (_exit 跳过 atexit, gcov 不 flush)。
#include <signal.h>
#include <stdlib.h>

static void flush_and_exit(int sig)
{
    // exit() 会运行进程注册的 atexit 处理器, 包括 gcov 的 __gcov_exit -> flush .gcda。
    // 注: 从信号处理器调 exit() 非严格 async-signal-safe, 但覆盖率采集场景实践中可靠。
    signal(sig, SIG_DFL);
    exit(0);
}
__attribute__((constructor))
static void install_handlers(void)
{
    struct sigaction sa;
    sa.sa_handler = flush_and_exit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);
}
