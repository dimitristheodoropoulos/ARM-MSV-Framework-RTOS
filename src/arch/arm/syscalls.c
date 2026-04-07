// Empty stubs for bare-metal
void _exit(int code) { while(1); }
int _kill(int pid, int sig) { return -1; }
int _getpid(void) { return 1; }
