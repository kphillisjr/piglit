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

/** @file execution.c
 *
 * Test glx_sgi_video sync execution.
 */

#include "piglit-util-gl-common.h"
#include "piglit-glx-util.h"
#include "common.h"

int piglit_width = 150, piglit_height = 150;

unsigned int startVblank = 0;
unsigned int prevVblank = 0;
unsigned int currVblank = 0;
unsigned int nextVblank = 0;
unsigned int expVblank = 0; // Expected vblank
int vBlankErrors = 0;

bool RenderFrame(Display *dpy,bool clearFrame, bool renderTri)
{
	bool pass = true;
	/* From Spec: glXWaitVideoSyncSGI returns the current video
		sync counter value in <count>. 
	*/
	prevVblank = currVblank;
	nextVblank = currVblank + 1;
	glXWaitVideoSyncSGI(2, nextVblank%2,&currVblank);
	glXGetVideoSyncSGI(&expVblank);
	if(expVblank !=currVblank) {
		printf("glXGetVideoSyncSGI: Expected Count output to be 0x%08X.\n\t"
			" Actual result is 0x%08X\n", expVblank, currVblank);
		currVblank = expVblank;
		pass = false;
	}
	if(1 < currVblank - prevVblank) {
		// The expected difference should be 1 vblank
		pass = false;
	}
	if(clearFrame) 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if(renderTri)
	{
		glColor3f(1.0f, 0.0f,1.0f); //Set The Color To Purple.
		/* just render a simple Triangle */
		glBegin(GL_TRIANGLES);                      // Drawing Using Triangles
			glVertex3f( 0.0f, 0.5f, 0.0f);              // Top
			glVertex3f(-0.5f,-0.5f, 0.0f);              // Bottom Left
			glVertex3f( 0.5f,-0.5f, 0.0f);              // Bottom Right
		glEnd();                            // Finished Drawing The Triangle
	}
	glFinish();
	glXSwapBuffers( dpy, win ); 
	return pass;
}

enum piglit_result
draw(Display *dpy)
{
	unsigned int i = 0;
	unsigned int passes = 60;
	unsigned int errors = 0;
	enum piglit_result result = PIGLIT_PASS;
	bool pass = true;
#define report_error_fail(test_result, text) \
	piglit_report_subtest_result(test_result,text);\
	piglit_merge_result(&result,test_result);
	if(0 < nRefreshRate) {
		// Render for 5 seconds.
		passes	= nRefreshRate * 5;
	} 
	/* Test With Clearing Frame */
	glXGetVideoSyncSGI(&startVblank);
	currVblank = startVblank;
	for(i = 0; i < passes; i++){
		if(!RenderFrame(dpy,true, false))
			pass = false;
	}
	glXGetVideoSyncSGI(&currVblank);
	if((currVblank - startVblank) != passes){
		pass = false;
		printf("Error: Test ran longer than expected, took %d vblanks, expected %d\n", currVblank - startVblank, passes);
	}
	report_error_fail( pass ? PIGLIT_PASS : PIGLIT_FAIL,"glXGetVideoSyncSGI: Clear frame");

	// Reset State.
	pass = true;
	vBlankErrors = 0;
	startVblank = currVblank;
	for(i = 0; i < passes; i++){
		if(!RenderFrame(dpy,false, false))
			pass = false;
	}
	glXGetVideoSyncSGI(&currVblank);
	if((currVblank - startVblank) != passes){
		pass = false;
		printf("Error: Test ran longer than expected, took %d vblanks, expected %d\n", currVblank - startVblank, passes);
	}
	report_error_fail( pass ? PIGLIT_PASS : PIGLIT_FAIL,
		"glXGetVideoSyncSGI: Swap Buffers");

	/* Test Null Render where only swapping buffers Occurs */
	// Reset State.
	pass = true;
	vBlankErrors = 0;
	startVblank = currVblank;
	
	for(i = 0; i < passes; i++){
		if(!RenderFrame(dpy,true, false))
			pass = false;
	}
	glXGetVideoSyncSGI(&currVblank);
	if((currVblank - startVblank) != passes){
		pass = false;
		printf("Error: Test ran longer than expected, took %d vblanks, expected %d\n", currVblank - startVblank, passes);
	}
	report_error_fail( pass ? PIGLIT_PASS : PIGLIT_FAIL,
		"glXGetVideoSyncSGI: Null Render w/ buffer swap");

	// Reset State.
	pass = true;
	vBlankErrors = 0;
	startVblank = currVblank;

	for(i = 0; i < passes; i++){
		if(!RenderFrame(dpy,true, false))
			pass = false;
	}
	glXGetVideoSyncSGI(&currVblank);
	if((currVblank - startVblank) != passes)
	{
		pass = false;
		printf("Error: Test ran longer than expected, took %d vblanks, expected %d\n", currVblank - startVblank, passes);
	}
	report_error_fail( pass ? PIGLIT_PASS : PIGLIT_FAIL,
		"glXGetVideoSyncSGI: Render tri without clearing frame.");
	// Reset State.
	pass = true;
	vBlankErrors = 0;
	startVblank = currVblank;

	for(i = 0; i < passes; i++){
		if(!RenderFrame(dpy,true, false))
			pass = false;
	}

	if((currVblank - startVblank) != passes){
		pass = false;
		printf("Error: Test ran longer than expected, took %d vblanks, expected %d\n", currVblank - startVblank, passes);
	}
	report_error_fail( pass ? PIGLIT_PASS : PIGLIT_FAIL,
		"glXGetVideoSyncSGI: Render tri with Clearing frame");

	piglit_report_result(result);
	exit(1);	
}

int
main(int argc, char **argv)
{
	piglit_sgi_video_sync_test_run(draw);

	return 0;
}
