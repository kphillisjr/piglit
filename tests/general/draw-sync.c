/*
 * Copyright © 2011 Intel Corporation
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

/**
 * @file draw-sync.c
 *
 * Test repeated readback of small draw calls.  This stresses render
 * ring IRQ handling on the i965 driver.
 */

#include "piglit-util-gl-common.h"
#include "piglit-framework.h"

PIGLIT_GL_TEST_MAIN(
    64 /*window_width*/,
    64 /*window_height*/,
    GLUT_RGB | GLUT_DOUBLE | GLUT_ALPHA)

enum piglit_result
piglit_display(void)
{
	int x, y;
	float green[4] = {0.0, 1.0, 0.0, 0.0};
	bool pass = true;

	piglit_ortho_projection(piglit_width, piglit_height, GL_FALSE);

	glClearColor(1.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor4fv(green);

	for (y = 0; y < piglit_height; y++) {
		for (x = 0; x < piglit_width; x++) {
			piglit_draw_rect(x, y, 1, 1);
			pass = pass && piglit_probe_pixel_rgba(x, y, green);
		}
	}

	glutSwapBuffers();

	return pass ? PIGLIT_PASS : PIGLIT_FAIL;
}


void
piglit_init(int argc, char **argv)
{
}
