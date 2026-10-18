#define	setcontext(u)	_setmcontext(&(u)->mc)
#define	getcontext(u)	_getmcontext(&(u)->mc)
typedef struct mcontext mcontext_t;
typedef struct ucontext ucontext_t;
struct mcontext
{
    ulong gpr[10];   /* x19-x28 */
    ulong fp;        /* x29 */
    ulong lr;        /* x30 */
    ulong sp;        /* stack pointer */
    ulong x0;        /* return value / first argument */
    ulong x1;        /* second argument */
    ulong fpr[8];    /* d8-d15 */
};

struct ucontext
{
	struct {
		void *ss_sp;
		uint ss_size;
	} uc_stack;
	sigset_t uc_sigmask;
	mcontext_t mc;
};

void makecontext(ucontext_t*, void(*)(void), int, ...);
int swapcontext(ucontext_t*, const ucontext_t*);
int _getmcontext(mcontext_t*);
void _setmcontext(const mcontext_t*);

