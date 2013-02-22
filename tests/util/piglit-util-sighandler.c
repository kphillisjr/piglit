/*
 * Copyright 2013 - Kenney Phillis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
#include "piglit-util.h"

#if !defined(_WIN32)

#if defined( HAVE_SIGNAL_H )  && defined (HAVE_EXECINFO_H)
#include <signal.h>
#include <execinfo.h>

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#elif defined(HAVE_SYSCALL_H)
#include <syscall.h>
#endif

#if defined(HAVE_SYS_WAIT_H)
#include <sys/wait.h>
#elif defined( HAVE_WAIT_H )
#include <wait.h>
#endif
#ifdef HAVE_UCONTEXT_H
#include <ucontext.h>
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
#ifdef HAVE_UCONTEXT_H
bool piglit_print_context_registers(ucontext_t *uc)
{
	
#if defined(__linux__)
	mcontext_t *ctx = &uc->uc_mcontext;
#if defined(__i386__)
	fprintf(stderr, "TRAPNO: %08x ERR: %08x",
        ctx->gregs[12], ctx->gregs[13]);
	fprintf(stderr, "EAX:%08x EBX:%08lx ECX:%08lx EDX:%08lx",
        ctx->gregs[11], ctx->gregs[8], ctx->gregs[10],ctx->gregs[9] );
	fprintf(stderr, "EDI:%08lx ESI:%08lx EBP:%08lx ESP:%08lx",
        ctx->gregs[4], ctx->gregs[5], ctx->gregs[6], ctx->gregs[7] );
	fprintf(stderr, "SS :%08lx EFL:%08lx EIP:%08lx CS:%08lx",
        ctx->gregs[18], ctx->gregs[17], ctx->gregs[14], ctx->gregs[15] );
	fprintf(stderr, "DS :%08lx ES :%08lx FS :%08lx GS:%08lx",
        ctx->gregs[3], ctx->gregs[2], ctx->gregs[1], ctx->gregs[0] );

#elif defined(__x86_64__)
	/* Linux amd64 compatible Register dump */
	fprintf(stderr,"ERR: %016lx TRAPNO: %016lx\nRIP: %016lx EFLAGS: %016lx",
		ctx->gregs[19], ctx->gregs[20], ctx->gregs[16], ctx->gregs[17] );		
	fprintf(stderr,"RAX: %016lx RBX: %016lx\nRCX: %016lx RDX %016lx",
		ctx->gregs[13], ctx->gregs[11], ctx->gregs[15], ctx->gregs[12]);
	fprintf(stderr,"RSI: %016lx RDI: %016lx\nRBP: %016lx RSP %016lx",
		ctx->gregs[9], ctx->gregs[8], ctx->gregs[15], ctx->gregs[15]);
	fprintf(stderr,"CR2: %016lx", ctx->gregs[22]);
	fprintf(stderr,"CSGSFS: %016lx OLDMASK: %016lx",
		ctx->gregs[18], ctx->gregs[21]);		
	fprintf(stderr,"R08: %016lx R09: %016lx\nR10: %016lx R11: %016lx",
		ctx->gregs[0], ctx->gregs[1], ctx->gregs[2], ctx->gregs[3]);		
	fprintf(stderr,"R12: %016lx R13: %016lx\nR14: %016lx R15: %016lx",
		ctx->gregs[4], ctx->gregs[5],
		ctx->gregs[6], ctx->gregs[8]);				
#else 
	/* TODO: Implement Other Linux Platforms, for example, Arm and PowerPC. */
	return false;
#endif

#else
	/* TODO: Implement Other platforms. FreeBSD, OSX, etc */
	return false;
#endif

	return true;
}

void piglit_print_registers()
{
	ucontext_t uc;
	
	if(getcontext(&uc) == 0)
	{
		fprintf(stderr,"[Backtrace] Register Dump:");
		if(!piglit_print_context_registers(&uc)) {
			fprintf(stderr,"Warning: Unable to get context information"
				" on registers.");
		}
	} else {
		fprintf(stderr,"Warning: Unable to get context information on"
			" registers. ( Unable to Get Context)");
	}
}

#else

void piglit_print_registers()
{
	fprintf(stderr,"Warning: Unable to get context information on registers.");
}
#endif

void piglit_print_backtrace()
{

#define MAX_TRACE_SIZE 32
	// failed to get gdb report... use other method.
	void *trace[MAX_TRACE_SIZE];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	trace_size = backtrace(trace, 16);
	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	fprintf(stderr,"[Backtrace] Execution path");
	for (i=0; i<trace_size; ++i)
		fprintf(stderr,"%d - %p - %s",i,&trace[i], messages[i]);

}

#ifdef SA_SIGINFO
void piglit_sighandler(int sig, siginfo_t *info,
				   void *secret) {
	// TODO: Improve GDB handling.
	//if(piglit_sighandler_gdb_report())
	//	exit(sig);				   
	fprintf(stderr,"Caught Signal: %d (%s)", sig, sys_siglist[sig]);				   
	/* Do something useful with siginfo_t */
	// sys_siglist is found on BSD 4.2 and up.
	fprintf(stderr,"info.si_addr  = %p", info->si_addr);
	fprintf(stderr,"info.si_code  = %d", info->si_code);
	fprintf(stderr,"info.si_errno = %d", info->si_errno);
	piglit_print_context_registers((ucontext_t*)secret);
	piglit_print_registers();	
	piglit_print_backtrace();
	// Done with our signal handler... Return default handler.
	signal(sig, SIG_DFL);
	kill(getpid(), sig);
}
#else
void piglit_sighandler(int sig) {
	sigsegv_outp("Caught Signal: %d (%s)", sig, sys_siglist[sig]);
	piglit_print_registers();	
	piglit_print_backtrace();
	// Done with our signal handler... Return default handler.        
	signal(sig, SIG_DFL);
	kill(getpid(), sig);

}
#endif

bool piglit_register_signal_handler()
	{
	/* Register the Piglit signal action handler. */
	struct sigaction sa;
	sigset_t block_mask;
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

// Win32
#endif

