/*
 * Copyright © 2013 Kenney Phillis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file piglit-sighandler-posix.c
 *
 * Implements posix compatible signal handler.
 *
 */
#include "config.h"
#include "piglit-util.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_SIGINFO_H
#include <sys/siginfo.h>
#endif

#ifdef HAVE_SYSCALL_H
#include <syscall.h>
#elif defined( HAVE_SYS_SYSCALL_H )
#include <sys/syscall.h>
#endif

// for Backtrace Support
#if defined(HAVE_LIBUNWIND_H)
#include <libunwind.h>
#elif defined(HAVE_EXECINFO_H)
#include <execinfo.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#elif defined( HAVE_WAIT_H)
#include <wait.h>
#endif

void piglit_sighandler(int sig, siginfo_t *info, void *secret);
bool piglit_sighandler_gdb_report( char*name_buf,  char* pid_buf);
void piglit_sighandler_backtrace(int sig, siginfo_t *info, void *secret);

bool piglit_register_signal_handler()
{
	/* Register the Piglit signal action handler. */
	struct sigaction sa;
	sigset_t block_mask;
	sigemptyset (&block_mask);
	/**
	 * Most signals are already set to need a block. However, this is the
	 * only signal that you absolutely haft to block to be able to capture
	 * output
	 */
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
	sigaction( SIGILL,  &sa, NULL); /*  4 - Illegal Instruction*/
	sigaction( SIGTRAP, &sa, NULL); /*  5 - Trace trap (POSIX) */
	sigaction( SIGABRT, &sa, NULL); /*  6 - Abort ( ANSI )*/
	sigaction( SIGFPE,  &sa, NULL); /*  8 - FPE ( Floating Point Exception) */
	sigaction( SIGUSR1, &sa, NULL); /* 10 - User Defined Signal*/
	sigaction( SIGSEGV, &sa, NULL); /* 11 - Segmentation Fault */
	/* This next Entry is not normal, but Could happen */
	sigaction( SIGBUS,  &sa, NULL); /* 13 -  Broken pipe: write to pipe with no readers */
	sigaction( SIGPIPE, &sa, NULL); /* 13 -  Broken pipe: write to pipe with no readers */
	return true;
}



void piglit_sighandler(int sig, siginfo_t *info, void *secret) {
	char proc_pid_buf[30];
	char proc_name_buf[512];
	// Get executable name.
	proc_name_buf[readlink("/proc/self/exe", proc_name_buf, 511)]='\0';
	sprintf(proc_pid_buf, "%d", getpid());
	fprintf(stderr, "\nPiglit: Crash Detected on %s ( PID: %s ).\n"
		"\tGenerating Debug information\n", proc_name_buf, proc_pid_buf);
	fprintf(stderr, "info.si_signo = %d\n", sig);
	fprintf(stderr, "info.si_code  = %d\n", info->si_code);
	fprintf(stderr, "info.si_addr  = %p\n", info->si_addr);
	fprintf(stderr, "info.si_errno = %d\n", info->si_errno);
	piglit_sighandler_backtrace(sig, info, secret);

	fprintf(stdout, "PIGLIT: {'result': 'crash' }\n");
	fflush(stderr);
	fflush(stdout);

	/* Maintain the Signal, and use that as the return code. */
	exit(sig);
}

void piglit_sighandler_backtrace(int sig, siginfo_t *info, void *secret)
{
#if defined( HAVE_UNWIND_H)
	/* Unwind Capable */
	unw_context_t uc;
	unw_cursor_t cursor;
	unw_word_t ip, sp, offp;
	char functionName[256];
	int res;
	/* Initialize the Context */
	unw_getcontext(&uc);
	unw_init_local(&cursor, &uc);
	while (unw_step(&cursor) !=0)
	{

		functionName[0] = '\0';
		res = unw_get_proc_name(&cursor, functionName, 256, &offp);
		if(res == UNW_EUNSPEC) {
			fprintf(stderr, "ERROR: unwind: Unspecified Error.");
			continue;
		}
		if(res == UNW_ENOINFO){
			fprintf(stderr, "ERROR: unwind: Unspecified to determine Name of Procedure.\n");
			continue;
		}
		unw_get_reg(&cursor, UNW_REG_IP, &ip); /* Instruction Pointer*/
		unw_get_reg(&cursor, UNW_REG_SP, &sp); /* Stack Pointer*/
#ifdef UNW_ENOMME
		if(res== UNW_ENOMME) {
			fprintf(stderr, "Warning: unwind: Procedure Name to Long... Name is Truncated.\n");
		}
#endif
#if __WORDSIZE == 64
		/* Print Stack Pointer and Instruction Pointer */
		fprintf(stderr, "SP=0x%016lx IP=0x%016lx" ,sp, ip);
		/* Print Symbol name and Instruction Offset */
		fprintf(stderr, ": (%s+0x%016lx)  [%016lx]\n", functionName, offp, ip);
#elif __WORDSIZE == 32
		/* Print Stack Pointer and Instruction Pointer */
		fprintf(stderr, "SP=0x%08x IP=0x%08x" ,sp, ip);
		/* Print Symbol name and Instruction Offset */
		fprintf(stderr, ": (%s+0x%08x)  [%08x]\n", functionName, offp, ip);
#else
		/* Print Stack Pointer and Instruction Pointer */
		fprintf(stderr, "SP=0x%04x IP=0x%04x" ,sp, ip);
		/* Print Symbol name and Instruction Offset */
		fprintf(stderr, ": (%s+0x%04x)  [%04x]\n", functionName, offp, ip);
#endif
	}
#elif defined(HAVE_EXECINFO_H)
	void *trace[64];
	char **messages = (char **)NULL;
	int i, trace_size = 0;
	trace_size = backtrace(trace, 64);
	/* overwrite sigaction with caller's address */
	//trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	fprintf(stderr, "[backtrace] Execution path\n");
	for (i=0; i<trace_size; ++i) {
		fprintf(stderr,"%d - %s\n",i, messages[i]);
	}
#else
	fprintf(stderr,"Piglit: Warning, Unable to generate Backtraces.\n");
#endif
}
