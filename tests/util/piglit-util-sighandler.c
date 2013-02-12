
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <eh.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "config.h"

#if defined(HAVE_SYS_TYPES_H) && defined(HAVE_SYS_STAT_H) && \
	defined(HAVE_UNISTD_H) && defined( HAVE_FCNTL_H)
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#else
# define USE_STDIO
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif
#ifdef HAVE_SYSCALL_H
#include <syscall.h>
#else
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#else
#ifdef HAVE_WAIT_H
#include <wait.h>
#endif
#endif

#endif

#include "piglit-util.h"


#if defined(_WIN32)
void piglit_signal_handler_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
   	 unsigned int   i;
     void         * stack[ 100 ];
     unsigned short frames;
     SYMBOL_INFO  * symbol;
     HANDLE         process;
	
     process = GetCurrentProcess();
	 fprintf(pExp->ExceptionCode)
     SymInitialize( process, NULL, TRUE );

     frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
     symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
     symbol->MaxNameLen   = 255;
     symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

     for( i = 0; i < frames; i++ )
     {
         SymFromAddr( process, ( DWORD64 )( stack[ i ] ), 0, symbol );

         printf( "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address );
     }

    free( symbol );
    // TODO: Integrate other exceptions that can be of use.
	//	http://msdn.microsoft.com/en-us/library/windows/desktop/aa363082%28v=vs.85%29.aspx
	if( (pExp->ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO ) &&
		(pExp->ExceptionCode == EXCEPTION_FLT_DIVIDE_BY_ZERO ) &&
		(pExp->ExceptionCode == EXCEPTION_INT_DIVIDE_BY_ZERO ) )
	{

		// Some other exception occured.
		fprintf(stdout,"PIGLIT: {'result': 'crash' }\n", result_str);
		fflush(stdout);
		ExitProcess(-1);
	}
	ExitProcess(0);
}

bool piglit_register_signal_handler()
{
	// use _set_se_translator to handle exception translater.
	//	http://msdn.microsoft.com/en-us/library/5z4bw5h5%28VS.80%29.aspx
	_set_se_translator(my_trans_func);
	return true;
}

#else
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#ifdef HAVE_SETJUMP_H
#include <setjmp.h>
jmp_buf  piglit_trapbuf;
#endif
// Simple signal handler that makes use of gdb.
bool piglit_sighandler_gdb_report( char*name_buf,  char* pid_buf)
{
#if defined(HAVE_SYS_SYSCALL_H) || defined(HAVE_SYSCALL_H)
    int child_pid = syscall(SYS_fork);
#else
    int child_pid = fork();
#endif
    int status;
    if (!child_pid) {
        char path[1024];
        char gdb_command[1024];
        FILE *fp;
        dup2(2,1); // redirect output to stderr
	    sprintf(gdb_command, "gdb --batch -n -ex thread -ex bt %s -p %s",
	    	name_buf, pid_buf);
	    fp = popen(gdb_command,"r");
		if (fp == NULL) {
    		// gdb is not installed.
    		// Generate stack trace from
    		exit(1);
		}
		/* Read the output a line at a time - output it. */
		while (fgets(path, sizeof(path)-1, fp) != NULL) {
			fprintf(stdout,"%s", path);
		}
		status = pclose(fp);
		fprintf(stdout, "gdb exited with code: %d\n", status);
		if(status != 0)
		{
			fprintf(stdout, "GDB failed on debugging, Using alternate"
				" method.\n");
			exit(1);
		}
        exit(0); /* If gdb failed to start */
    } else {
#if defined(HAVE_SYS_WAIT_H) || defined( HAVE_WAIT_H )
        waitpid(child_pid,&status,0);
        if(status != 0) {
        	return false;
        }
#endif
    }
    return true;
}

// Generate Old Fashioned segmentation reports.
#define sigsegv_outp(x, ...)    fprintf(stderr, x "\n", ##__VA_ARGS__)

#if __WORDSIZE == 64
# define SIGSEGV_STACK_IA64
# define REGFORMAT "%016lx"
#elif __WORDSIZE == 32
# define SIGSEGV_STACK_X86
# define REGFORMAT "%08x"
#else
# define SIGSEGV_STACK_GENERIC
# define REGFORMAT "%x"
#endif

void piglit_print_backtrace(int sig, siginfo_t *info, void *secret)
{
#ifdef HAVE_EXECINFO_H
	// Failed to get a gdb debug output. Instead use 
	// failed to get gdb report... use other method.
	void *trace[16];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	ucontext_t *uc = (ucontext_t *)secret;
	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	//trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[backtrace] Execution path\n");
	for (i=0; i<trace_size; ++i) {
		fprintf(stderr,"%d - %s\n",i, messages[i]);
	}
	printf("[backtrace] Register Dump:\n");
	for(i = 0; i < NGREG; i++) {
		sigsegv_outp("reg[%02d] = 0x" REGFORMAT, \
			i, uc->uc_mcontext.gregs[i]);
	}
#else
	fprintf(stdout,"Piglit: Warning, Unable to generate Backtraces.\n");
#endif
}

volatile sig_atomic_t fatal_error_in_progress = 0;
void piglit_sighandler(int sig, siginfo_t *info, void *secret) {
    char proc_pid_buf[30];
	char proc_name_buf[512];
	// Get executable name.
	proc_name_buf[readlink("/proc/self/exe", proc_name_buf, 511)]='\0';
    sprintf(proc_pid_buf, "%d", getpid());
	fprintf(stdout,"\nPiglit: Crash Detected on %s ( PID: %s ).\n"
		"\tGenerating Debug information\n", proc_name_buf, proc_pid_buf);
	sigsegv_outp("info.si_signo = %d", sig);
    sigsegv_outp("info.si_code  = %d", info->si_code);
    sigsegv_outp("info.si_addr  = %p", info->si_addr);
    sigsegv_outp("info.si_errno = %d", info->si_errno);
    if(sig == SIGABRT) {
    	/* For some reason It's not possible to use GDB to debug */
    	/* Crashes signaled by SIGABRT. So always generate the */
    	/* Stack trace using the alternative method. */
	    fprintf(stdout, "Piglit: Crash signal was 6 ( SIGABRT ), Using "\
	    	"backup method\n");
    	piglit_print_backtrace(sig, info, secret);
    } else {
		if(!piglit_sighandler_gdb_report(proc_name_buf,proc_pid_buf)) {
			fprintf(stdout,"Piglit: Warning gdb not installed... Using"
				"backup method to create Backtraces.\n");
			/* Failed to auto-run gdb... Use backup method to generate */
			/* The callback trace trace */
			piglit_print_backtrace(sig, info, secret);
		}
	}
	fprintf(stdout,"PIGLIT: {'result': 'crash' }\n");
	fflush(stdout);

  /* Maintain the Signal, and use that as the return code. */
  exit(sig);
}

bool piglit_register_signal_handler()
{
	/* Register the Piglit signal action handler. */
	struct sigaction sa;
	sigset_t block_mask;
#ifdef HAVE_SETJUMP_H	
	setjmp (piglit_trapbuf );
#endif
	sigemptyset (&block_mask);
	/* Most signals are already set to need a block. However, this is the */
	/* only signal that you absolutely haft to block to be able to capture */
	/* output */
	sigaddset (&block_mask, SIGABRT);
    sigaddset (&block_mask, SIGINT);
    sigaddset (&block_mask, SIGQUIT);
	sa.sa_handler = (void *)piglit_sighandler;
	sa.sa_mask = block_mask;
	sa.sa_flags = 0;
#ifdef SA_SIGINFO
	sa.sa_flags |= SA_SIGINFO;
	sa.sa_sigaction  = (void *)piglit_sighandler;
#endif
#ifdef SA_RESTART
	sa.sa_flags |= SA_RESTART;
#endif
	/* Actually Register the Signal Actions */
	sigaction( SIGTRAP, &sa, NULL);
	sigaction( SIGABRT, &sa, NULL);
	sigaction( SIGFPE, &sa, NULL); /* Floating Point Exception */
	sigaction( SIGUSR1, &sa, NULL);
	sigaction( SIGSEGV, &sa, NULL); /* Segmentation Fault */

	return true;
}

#else
bool piglit_register_signal_handler()
{
	fprintf(stdout,"Warning: Signal Handler not defined, Please Port"\
		" to your system and/or architecture.");
	return false;
}
#endif
#endif

