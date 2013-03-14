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
 *
 * Authors:
 *    Kenney Phillis
 *
 */

/** @file error-codes.c
 *
 * Test glx_sgi_video sync error codes to verify the outputs are correct.
 */

#include "piglit-util-gl-common.h"
#include "piglit-glx-util.h"
#include "common.h"

int piglit_width = 50, piglit_height = 50;

enum piglit_result
draw(Display *dpy)
{
	int res = 0;
	unsigned int count = 0;
	enum piglit_result result = PIGLIT_PASS;
#define report_error_fail(text) \
	piglit_report_subtest_result(PIGLIT_FAIL,text);\
	piglit_merge_result(&result,PIGLIT_FAIL);
	/* Test Direct Contexts */	
	res = glXGetVideoSyncSGI(&count);
	if(res == 0) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXGetVideoSyncSGI - Direct Context.");
	} else {
		printf("glXGetVideoSyncSGI: Expected 0 return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXGetVideoSyncSGI - Direct Context");
	}

	res = glXWaitVideoSyncSGI(1,0,&count);
	if(res == 0) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - Direct Context.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected 0 return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - Direct Context");
	}

	/* From Spec:
	    Should glXWaitVideoSyncSGI return GLX_BAD_VALUE if <remainder> is
	    greater than or equal to <divisor>? (No.)
    */
	res = glXWaitVideoSyncSGI(1,1,&count);
	if(res == 0) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - Remainder Equals Divisor.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected 0 return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - Remainder Equals Divisor");
	} 
	res = glXWaitVideoSyncSGI(1,3,&count);
	if(res == 0) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - Remainder Greater Divisor.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected 0 return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - Remainder Greater Divisor.");
	}    	
	/* Spec Says: 	
		glXWaitVideoSyncSGI returns GLX_BAD_VALUE if parameter <divisor> is less
		than or equal to zero, or if parameter <remainder> is less than zero.
	*/
	res = glXWaitVideoSyncSGI(1,-1,&count);
	if(res == GLX_BAD_VALUE) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - Remainder less than zero.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected GLX_BAD_VALUE return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - Remainder less than zero");
	}
	
	res = glXWaitVideoSyncSGI(0,1,&count);
	if(res == GLX_BAD_VALUE) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - divisor equals zero.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected GLX_BAD_VALUE return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - divisor equals zero");
	}
	
	res = glXWaitVideoSyncSGI(-1,1,&count);
	if(res == GLX_BAD_VALUE) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - divisor less than zero.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected GLX_BAD_VALUE return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - divisor less than zero");
	}
	
	/* Test Indirect Context return values */
	glXMakeCurrent(dpy, win, indirectCtx);
	
	/* Spec says:
	    glXGetVideoSyncSGI returns GLX_BAD_CONTEXT if there is no current
	    GLXContext.
	    
	   Implied Meaning: glXGetVideoSyncSGI should returns 0 as long as a
	    context is set. This includes Indirect contexts.
    */
	res = glXGetVideoSyncSGI(&count);
	if(res == 0 ) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXGetVideoSyncSGI - Indirect Context.");
	} else {
		printf("glXWaitVideoSyncSGI: Expected 0 return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXGetVideoSyncSGI - Indirect Context");
	}

	/* Spec Says:
	    glXWaitVideoSyncSGI returns GLX_BAD_CONTEXT if the current context is
	    not direct, or if there is no current context.
    */	
	res = glXWaitVideoSyncSGI(1,1,&count);
	if(res == GLX_BAD_CONTEXT) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - Indirect Context");
	} else {
		printf("glXWaitVideoSyncSGI: Expected GLX_BAD_CONTEXT return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - Indirect Context");
	}
	/* From GLX 1.4 spec, Section 3.5:
		To release the current context without assigning a new one, use
		NULL for ctx and None for draw.
	*/
	glXMakeCurrent(dpy,None,NULL);
	res = glXWaitVideoSyncSGI(1,1,&count);
	if(res == GLX_BAD_CONTEXT ) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXWaitVideoSyncSGI - No Context");

	} else {
		printf("glXWaitVideoSyncSGI: Expected GLX_BAD_CONTEXT return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXWaitVideoSyncSGI - No Context");
	}
	
	/* Spec says:
	    glXGetVideoSyncSGI returns GLX_BAD_CONTEXT if there is no current
	    GLXContext.
    */
	res = glXGetVideoSyncSGI(&count);
	if(res == GLX_BAD_CONTEXT) {
		piglit_report_subtest_result(PIGLIT_PASS,
			"glXGetVideoSyncSGI - No Context");
	} else {
		printf("glXGetVideoSyncSGI: Expected GLX_BAD_CONTEXT return code. Got"
			" %s ( %d )\n", piglit_glx_error_string(res), res);
		report_error_fail("glXGetVideoSyncSGI - No Context");

	}
	piglit_report_result(result);
	exit(1);	
}

int
main(int argc, char **argv)
{
	piglit_sgi_video_sync_test_run(draw);

	return 0;
}
