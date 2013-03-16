/*
 * Copyright © 2013 Kenney phillis
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

/** @file common.c
 *
 * Support code for running tests of GLX_SGI_video_sync.
 */

#include "piglit-util-gl-common.h"
#include "piglit-glx-util.h"
#include "common.h"

PFNGLXGETVIDEOSYNCSGIPROC __piglit_glXGetVideoSyncSGI;
PFNGLXWAITVIDEOSYNCSGIPROC __piglit_glXWaitVideoSyncSGI;
PFNGLXSWAPINTERVALEXTPROC __piglit_glXSwapIntervalEXT;
Window win;
XVisualInfo *visinfo;
GLXContext directCtx;
GLXContext indirectCtx;
GLXDrawable drawable;
bool bEXT_swap_control_Supported;
unsigned int nRefreshRate;
void
piglit_sgi_video_sync_test_run(enum piglit_result (*draw)(Display *dpy))
{
	Display *dpy;
	const int proc_count = 3;
	__GLXextFuncPtr *procs[proc_count];
	const char *names[proc_count];
	int i;

#define ADD_FUNC(name)							\
	do {								\
		procs[i] = (__GLXextFuncPtr *)&(__piglit_##name);	\
		names[i] = #name;					\
		i++;							\
	} while (0)

	i = 0;
	ADD_FUNC(glXGetVideoSyncSGI);
	ADD_FUNC(glXWaitVideoSyncSGI);
	ADD_FUNC(glXSwapIntervalEXT);

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "couldn't open display\n");
		piglit_report_result(PIGLIT_FAIL);
	}
	bEXT_swap_control_Supported = 
		piglit_is_glx_extension_supported(dpy, "GLX_EXT_swap_control");
	piglit_require_glx_extension(dpy, "GLX_SGI_video_sync");
	piglit_glx_get_all_proc_addresses(procs, names, ARRAY_SIZE(procs));
	visinfo = piglit_get_glx_visual(dpy);
	win = piglit_get_glx_window(dpy, visinfo);
	directCtx = glXCreateContext(dpy, visinfo, NULL, True);
	if (directCtx == NULL) {
		fprintf(stderr,
			"Could not create initial direct-rendering context.\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	if (!glXIsDirect(dpy, directCtx)) {
		glXDestroyContext(dpy, directCtx);
		directCtx = NULL;
	}

	indirectCtx = glXCreateContext(dpy, visinfo, NULL, False);
	if (indirectCtx == NULL) {
		fprintf(stderr,
			"Could not create initial indirect-rendering "
			"context.\n");
		piglit_report_result(PIGLIT_FAIL);
	}
	glXMakeCurrent(dpy, win, directCtx);

	piglit_dispatch_default_init();

	XMapWindow(dpy, win);

	piglit_glx_event_loop(dpy, draw);
	
	drawable = glXGetCurrentDrawable();
	if(bEXT_swap_control_Supported){
		// Swap Interval is the current refresh rate.
		glXQueryDrawable(dpy, drawable, GLX_SWAP_INTERVAL_EXT, &nRefreshRate);
		printf("GLX_EXT_swap_control supported: %d interval rate\n", nRefreshRate);
	} else {
		nRefreshRate = 0;
	}
}
