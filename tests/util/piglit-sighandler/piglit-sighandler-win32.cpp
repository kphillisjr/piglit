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

/** @file piglit-sighandler-win32.c
 *
 * Helper function for handling Segmentation faults on windows. This handler
 * is especially important for Windows XP and up that include a built in
 * vectored exception handling support.
 * See: http://msdn.microsoft.com/en-us/magazine/cc301714.aspx
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <windows.h>
#include <eh.h>
#include <DbgHelp.h>
#include <signal.h>
#include "piglit-util.h"

LONG WINAPI piglit_ExceptionHandler( EXCEPTION_POINTERS* pExp );
void piglit_PrintExceptionRecord(PCONTEXT pRecord);
void piglit_SignalHandler(int signal);
void piglit_PrintBacktrace(HANDLE  process);
bool piglit_register_signal_handler()
{
	OSVERSIONINFO mSysVer;
	DWORD dwMode;

	ZeroMemory(&mSysVer, sizeof(OSVERSIONINFO));
    mSysVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&mSysVer);
	if( ( mSysVer.dwMajorVersion < 5) ||
		( (mSysVer.dwMajorVersion == 5) && (mSysVer.dwMinorVersion < 1) ) ) {
		/* Do not bother trying to Register a Handler on windows versions
		   Older than XP */
		return false;
	}
	/* Debug Help, Updated Platform help
		See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms681408%28v=vs.85%29.aspx
	*/
	/* Note: Windows XP or newer is Required For this to work properly */
	/* XP is when Vectored Exception handlers where introduced */
	AddVectoredExceptionHandler(0, piglit_ExceptionHandler);
	//EnableCrashingOnCrashes();
	signal(SIGABRT, piglit_SignalHandler);
	/* Disable Windows Crash dialog
	   See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms680621%28v=vs.85%29.aspx
	*/
	/*Get Current Error Mode*/
	dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
	/* Set New Error mode without Crash Dialog */
	SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
	return true;
}

LONG WINAPI piglit_ExceptionHandler( EXCEPTION_POINTERS* pExp ) {
	unsigned int code = pExp->ExceptionRecord->ExceptionCode;
    // TODO: Integrate other exceptions that can be of use.
	//	http://msdn.microsoft.com/en-us/library/windows/desktop/aa363082%28v=vs.85%29.aspx
	if( (code == EXCEPTION_FLT_DIVIDE_BY_ZERO ) ||
		(code == EXCEPTION_INT_DIVIDE_BY_ZERO ) ||
		(code == EXCEPTION_ACCESS_VIOLATION) ||
		(code == EXCEPTION_INVALID_DISPOSITION) || // Not normal For c/c++ ( assembly issue )
		(code == EXCEPTION_ARRAY_BOUNDS_EXCEEDED) || // Array Bounds Checks
		(code == EXCEPTION_ILLEGAL_INSTRUCTION ) || // Invalid Instruction
		(code == EXCEPTION_STACK_OVERFLOW) // Stack Overflow
		)
	{
		PEXCEPTION_RECORD pExceptionRec;
		PCONTEXT pExceptionContext;
        DWORD  lineDisplacement = 0;
		HANDLE         process;
        IMAGEHLP_LINE64 line; /* For Line and File Information */
        DWORD symOptions;
        SYMBOL_INFO * symbol;
        symbol               = ( SYMBOL_INFO * )calloc(1,
                            sizeof( SYMBOL_INFO ) + 256 * sizeof( char ) );
        symbol->MaxNameLen   = 255;
        symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		process = GetCurrentProcess();
		/* Configure Symbol Options for print stack */
		symOptions = SymGetOptions();
		symOptions |= SYMOPT_LOAD_LINES;
		symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
		symOptions &= ~SYMOPT_UNDNAME; /* Automatically Un-Decorate Symbol Name */
		symOptions &= ~SYMOPT_DEFERRED_LOADS;
		symOptions = SymSetOptions(symOptions);
		/* Initialize Dbghelp */
		SymInitialize( process, NULL, TRUE );
		pExceptionRec = pExp->ExceptionRecord;
		pExceptionContext = pExp->ContextRecord;
		fprintf(stderr, "Exception Caught:\n\tAddress  - 0x%0X ( Code:  0x%08lx)\n",
			pExceptionRec->ExceptionAddress ,
			pExceptionRec->ExceptionCode );
		SymFromAddr( process, ( DWORD64 )( pExceptionRec->ExceptionAddress ),
			0, symbol );
		/* get Exception Address Symbol Name */
		fprintf(stderr, "Exception Location: %s - 0x%0X\n",
			symbol->Name, symbol->Address );
		if (SymGetLineFromAddr64(process, ( DWORD64 )
			(pExceptionRec->ExceptionAddress ) ,
			&lineDisplacement, &line))
		{
			/* Print the FIle Name and line number if Available.
			   SymGetLineFromAddr64 returned success */
			fprintf(stderr, "\tFile name:\t%s\n\tLine Number:\t%d\n",
				line.FileName, line.LineNumber	);
		}
		piglit_PrintBacktrace(process);
		fprintf(stderr, "===========================================\n");
		fprintf(stderr, "Register Dump:\n");
		fprintf(stderr, "===========================================\n");
		piglit_PrintExceptionRecord(pExceptionContext);

		fprintf(stdout,"PIGLIT: {'result': 'crash' }\n");
		fflush(stderr);
		fflush(stdout);
		/* Report The Crash now... This may need some work on results that are
		   teired.
		*/
		//ExitProcess(-1);
        //exit(code);
		return EXCEPTION_EXECUTE_HANDLER; /* This shouldn't generate an Error. */
	}

    /*
	* Ignore C++ exceptions
	* http://support.microsoft.com/kb/185294
	* http://blogs.msdn.com/b/oldnewthing/archive/2010/07/30/10044061.aspx
    *
	*/
    if (code == 0xe06d7363 ) {
		// C++ Exceptions
        return EXCEPTION_EXECUTE_HANDLER;
    }

	/* Didn't Handle Exception, Continue */
	return EXCEPTION_CONTINUE_SEARCH;
}


void piglit_PrintBacktrace(HANDLE  process)
{
	unsigned int   i;
	SYMBOL_INFO    *symbol;
	HANDLE         thread;
	unsigned short frames;
	void           *pCallStack[64];
	PIMAGEHLP_SYMBOL64 ImageData;
	IMAGEHLP_LINE64 line; /* For Line and File Information */
    DWORD  lineDisplacement = 0;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	fprintf(stderr, "===========================================\n");
	fprintf(stderr, "Call Stack:\n");
	fprintf(stderr, "===========================================\n");

	frames  = CaptureStackBackTrace( 0, 63, pCallStack, NULL );


	thread = GetCurrentThread();
	ImageData = (IMAGEHLP_SYMBOL64*)calloc(1,sizeof(ImageData)+sizeof(CHAR)*512);
	ImageData->MaxNameLength = 512;
	symbol               = ( SYMBOL_INFO * )calloc(1,
						sizeof( SYMBOL_INFO ) + 256 * sizeof( char ) );
	symbol->MaxNameLen   = 255;
	symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

	for( i = 0; i < frames; i++ )
	{
		/* Get Symbol Name */
		SymFromAddr( process, ( DWORD64 )( pCallStack[ i ] ), 0, symbol );
		fprintf(stderr, "%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address );
		if(SymGetLineFromAddr64(process, ( DWORD64 )( pCallStack[ i ] ) ,
			&lineDisplacement, &line))
		{
		// Print Line Number and Source file if Available
		fprintf(stderr,
			"\tFile name:    %s\n\tLine Number:     %d\n\n",
			line.FileName, line.LineNumber	);
		}
	}
	free( symbol );

}

void piglit_PrintExceptionRecord(PCONTEXT pRecord){
#if defined(_WIN64)
	// Create a Clean Register Dump of the Current Context.
	fprintf(stderr, "Register Dump x64 ( AMD64 )\n");
	if(pRecord->ContextFlags & CONTEXT_CONTROL) {
		fprintf(stderr, "Control Registers:\n");
		//egSs, Rsp, SegCs, Rip, and EFlags.
		fprintf(stderr, "\tRip:  0x%016llX Rsp: 0x%016llX\n"
			"\tEFLags: 0x%0lX SegCS:  0x%0lX SegSs:0x%0lX\n",
			pRecord->Rip,pRecord->Rsp, pRecord->EFlags,
			pRecord->SegCs, pRecord->SegSs);
	}

	if(pRecord->ContextFlags & CONTEXT_SEGMENTS) {
		fprintf(stderr, "Segment Registers:\n");
		fprintf(stderr, "\tSegDS:  0x%0X SegEs:0x%0X SegFs:0x%0X SegGs:0x%0X\n",
			pRecord->SegDs, pRecord->SegEs, pRecord->SegFs, pRecord->SegGs);
	}
	if(pRecord->ContextFlags & CONTEXT_INTEGER) {
		fprintf(stderr, "Integer Registers:\n");
		fprintf(stderr, "\tRAX: 0x%016llX RBX: 0x%016llX\n\tRCX: 0x%016llX RDX: 0x%016llX\n",
			pRecord->Rax, pRecord->Rbx,	pRecord->Rcx, pRecord->Rdx);
		fprintf(stderr, "\t R8: 0x%016llX  R9: 0x%016llX \n\tR10: 0x%016llX R11: 0x%016llX\n",
			pRecord->R8, pRecord->R9,	pRecord->R10, pRecord->R11);
		fprintf(stderr, "\tR12: 0x%016llX R13: 0x%016llx\n\tR14: 0x%016llx R15: 0x%016llX\n",
			pRecord->R12, pRecord->R13,	pRecord->R14, pRecord->R15);
	}
	if(pRecord->ContextFlags & CONTEXT_FLOATING_POINT) {
		int i;
		fprintf(stderr, "Floating Point Registers:\n");
		fprintf(stderr, "\tMxCsr: 0x%0lX MxCsr_Mask 0x%0lX\n", pRecord->FltSave.MxCsr, pRecord->FltSave.MxCsr_Mask);
		for( i = 0; i < 8; i++) {
			fprintf(stderr, "\tFloat Register%-2d: 0x%016llX%016llX \n",
				i ,
				pRecord->FltSave.FloatRegisters[i].High,
				pRecord->FltSave.FloatRegisters[i].Low);
		}

		/* XMM 1 to 7 - Available on 32-bit and 64 bit chips.*/
		/* XMM 8 to 15- Available on 64-bit mode.*/
		for( i = 0; i < 16; i++) {
			fprintf(stderr, "\tXmm%-2d: 0x%016llX%016llX \n",
				i ,
				pRecord->FltSave.XmmRegisters[i].High,
				pRecord->FltSave.XmmRegisters[i].Low);
		}
	}
	if(pRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) {
		fprintf(stderr, "Debug Registers:\n");
		fprintf(stderr, "\tDr0: %016llX Dr1: %016llX\n\tDr2: %016llX Dr3: %016llX\n"
			"\tDr6: %016llX Dr7: %016llX\n",
			pRecord->Dr0, pRecord->Dr1,	pRecord->Dr2, pRecord->Dr3,
			pRecord->Dr6, pRecord->Dr7);
	}

#else
	fprintf(stderr, "Register Dump x86\n");
	if(pRecord->ContextFlags & CONTEXT_CONTROL) {
		fprintf(stderr, "\tEip:  0x%08lX Esp: 0x%08lX EFLags: 0x%08lX\n"
			"\tSegCS:  0x%08lX SegSs:0x%08lX\n",
			pRecord->Eip,pRecord->Esp, pRecord->EFlags,
			pRecord->SegCs, pRecord->SegSs);
	}
	if(pRecord->ContextFlags & CONTEXT_INTEGER) {
		fprintf(stderr, "Integer Registers:\n");
		fprintf(stderr, "\tEax: 0x%08lX Ebx: 0x%08lX Ecx: 0x%08lX  Edx: 0x%08lX\n",
			pRecord->Eax, pRecord->Ebx, pRecord->Ecx, pRecord->Edx);
		fprintf(stderr, "\tEdi: 0x%08lX Esi: 0x%08lX\n",pRecord->Edi, pRecord->Esi);
	}
	if(pRecord->ContextFlags & CONTEXT_SEGMENTS) {
		fprintf(stderr, "Segment Registers:\n");
		fprintf(stderr, "\tSegDSl:  0x%08lX SegEs:0x%08lX SegFs:0x%08lX SegGs:0x%08lX\n",
			pRecord->SegDs, pRecord->SegEs, pRecord->SegFs, pRecord->SegGs);
	}
	if(pRecord->ContextFlags & CONTEXT_EXTENDED_REGISTERS) {
		int i;
		PXSAVE_FORMAT fltRegs = (PXSAVE_FORMAT)pRecord->ExtendedRegisters;
		fprintf(stderr, "Floating Point Registers:\n");
		fprintf(stderr, "\tMxCsr: 0x%0lX MxCsr_Mask 0x%0lX\n", fltRegs->MxCsr, fltRegs->MxCsr_Mask);
		for( i = 0; i < 8; i++) {
			fprintf(stderr, "\tFloat Register%-2d: 0x%016llX%016llX \n",
				i ,
				fltRegs->FloatRegisters[i].High,
				fltRegs->FloatRegisters[i].Low);
		}

		/* XMM 1 to 7 - Available on 32-bit and 64 bit chips.*/
		/* XMM 8 to 15- Available on 64-bit mode.*/
		for( i = 0; i < 8; i++) {
			fprintf(stderr, "\tXmm%-2d: 0x%016llX%016llX \n",
				i ,
				fltRegs->XmmRegisters[i].High,
				fltRegs->XmmRegisters[i].Low);
		}
	}
	if(pRecord->ContextFlags & CONTEXT_DEBUG_REGISTERS) {
		fprintf(stderr, "Debug Registers:\n");
		fprintf(stderr, "\tDr0: 0x%08lX Dr1: 0x%08lX Dr2: 0x%08lX Dr3: 0x%08lX\n"
			"\tDr6: 0x%08lX Dr7: 0x%08lX\n",
			pRecord->Dr0, pRecord->Dr1,	pRecord->Dr2, pRecord->Dr3,
			pRecord->Dr6, pRecord->Dr7);
	}
#endif
}
void piglit_SignalHandler(int signal)
{
    fprintf(stderr, "Application aborting...\n");
	HANDLE  process;
    DWORD   symOptions;
	process = GetCurrentProcess();
	/* Configure Symbol Options for print stack */
	symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES;
	symOptions |= SYMOPT_FAIL_CRITICAL_ERRORS;
	symOptions &= ~SYMOPT_UNDNAME; /* Automatically Un-Decorate Symbol Name */
	symOptions &= ~SYMOPT_DEFERRED_LOADS;
	symOptions = SymSetOptions(symOptions);
	/* Initialize Dbghelp */
	SymInitialize( process, NULL, TRUE );

	/* Print backtrace */
	piglit_PrintBacktrace(process);

	fprintf(stdout,"PIGLIT: {'result': 'crash' }\n");
	fflush(stdout);
	fflush(stderr);
	ExitProcess(signal);
}
