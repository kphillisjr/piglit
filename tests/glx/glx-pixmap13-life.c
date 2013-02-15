/*
 * Copyright 2011 Red Hat, Inc.
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
 *      Adam Jackson <ajax@redhat.com>
 *
 */

/** @file glx-pixmap13-life.c
 *
 * Test that GLXPixmaps and their associated X Pixmaps can be destroyed in
 * either order, without error.  Same as glx-pixmap-life.c but with
 * fbconfigs.
 */

#include "piglit-util-gl-common.h"
#include "piglit-glx-util.h"

int piglit_width = 50, piglit_height = 50;
static Display *dpy;
static XVisualInfo *visinfo;

int pass = 1;

static int
handler(Display *dpy, XErrorEvent *err)
{
	pass = 0;

#if 0
	printf("Error serial %x error %x request %x minor %x xid %x\n",
	       err->serial, err->error_code, err->request_code, err->minor_code,
	       err->resourceid);
#endif

	return 0;
}

int
main(int argc, char **argv)
{
	Pixmap p;
	GLXPixmap g;
	GLXFBConfig fbc;

	/* Register Signal handler that is used to capture crashes */
	piglit_register_signal_handler();
	
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "couldn't open display\n");
		piglit_report_result(PIGLIT_FAIL);
	}

	piglit_require_glx_version(dpy, 1, 3);

	visinfo = piglit_get_glx_visual(dpy);
	fbc = piglit_glx_get_fbconfig_for_visinfo(dpy, visinfo);

	XSetErrorHandler(handler);

	/* test destroying glxpixmap before pixmap */
	p = XCreatePixmap(dpy, DefaultRootWindow(dpy), piglit_width,
			  piglit_height, visinfo->depth);
	g = glXCreatePixmap(dpy, fbc, p, NULL);

	glXDestroyPixmap(dpy, g);
	XFreePixmap(dpy, p);

	XSync(dpy, 0);

	/* test destroying pixmap before glxpixmap */
	p = XCreatePixmap(dpy, DefaultRootWindow(dpy), piglit_width,
			  piglit_height, visinfo->depth);
	g = glXCreatePixmap(dpy, fbc, p, NULL);

	XFreePixmap(dpy, p);
	glXDestroyPixmap(dpy, g);

	XSync(dpy, 0);

	piglit_report_result(pass ? PIGLIT_PASS : PIGLIT_FAIL);

	return 0;
}
