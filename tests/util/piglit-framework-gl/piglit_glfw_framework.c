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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "piglit_glfw_framework.h"
#include "piglit-util-gl-common.h"
#include <GLFW/glfw3.h>

typedef struct piglit_glfw_framework {
	struct piglit_gl_framework gl_fw;

	/**
	 * Has the user requested a redisplay with
	 * piglit_gl_framework::post_redisplay?
	 */
	bool need_redisplay;

	enum piglit_result result;
	GLFWwindow* pGL_Window;

	void (*glfw_keyboard_func)(unsigned char key, int x, int y);
	void (*glfw_reshape_func)(int w, int h);
} piglit_glfw_framework_t;

piglit_glfw_framework_t* glfw_framework(struct piglit_gl_framework *gl_fw )
{
	return (piglit_glfw_framework_t*) gl_fw;
}

static void piglit_glfw_loop(piglit_glfw_framework_t *glfw_fw)
{

	glfw_framework(gl_fw)->need_redisplay = true;
	while(!glfwWindowShouldClose(glfw_fw->pGL_Window))
	{
		if (gl_fw->test_config->display) {
			if(glfw_fw->need_redisplay)
			{
				glfw_framework(gl_fw)->need_redisplay = false;
				glfw_fw->result = gl_fw->test_config->display();
			}
		}

		if (piglit_automatic) {
			glfwSetWindowShouldClose(glfw_fw->pGL_Window, GL_TRUE);
		}

		glfwPollEvents();
	}
}

static void
glfw_run_test(struct piglit_gl_framework *gl_fw,
         int argc, char *argv[])
{
	piglit_glfw_framework_t *glfw_fw = glfw_framework(gl_fw);
	if (gl_fw->test_config->init)
		gl_fw->test_config->init(argc, argv);

	piglit_glfw_loop(glfw_fw);
	piglit_report_result(glfw_fw->result);
}

static void
glfw_swap_buffers(struct piglit_gl_framework *gl_fw)
{
	glfwSwapBuffers(glfw_framework(gl_fw)->pGL_Window);
}

static void
glfw_post_redisplay(struct piglit_gl_framework *gl_fw)
{
	glfw_framework(gl_fw)->need_redisplay = true;
}

static void
glfw_set_keyboard_func(struct piglit_gl_framework *gl_fw,
                  void (*func)(unsigned char key, int x, int y))
{
	glfw_framework(gl_fw)->glfw_keyboard_func = func;
}

static void
glfw_set_reshape_func(struct piglit_gl_framework *gl_fw,
                 void (*func)(int w, int h))
{
	glfw_framework(gl_fw)->glfw_reshape_func = func;
}

static void
glfw_destroy(struct piglit_gl_framework *gl_fw)
{
	piglit_glfw_framework_t *glfw_fw = glfw_framework(gl_fw);

	if (glfw_fw == NULL)
		return;

	if(glfw_fw->pGL_Window) {
		glfwDestroyWindow(glfw_fw->pGL_Window);
	}

	free(glfw_fw);
	glfwTerminate();
}

static void piglit_glfw_resize_callback(GLFWwindow *pWin, int w, int h)
{
	if(glfwGetWindowUserPointer(pWin) !=NULL)
	{
		piglit_glfw_framework_t *glfw_fw =
			(piglit_glfw_framework_t*) glfwGetWindowUserPointer(pWin);
		glfw_fw->need_redisplay = true;
		if(glfw_fw->glfw_reshape_func)
		{
			glfw_fw->glfw_reshape_func(w,h);
		} else {
			if (piglit_automatic &&
				(w != piglit_width ||
				 h != piglit_height)) {
				printf("Got spurious window resize in automatic run "
					   "(%d,%d to %d,%d)\n", piglit_width, piglit_height, w, h);
				piglit_report_result(PIGLIT_WARN);
			}

		}
	}
	piglit_width = w;
	piglit_height = h;
	glfwMakeContextCurrent(pWin);
	glViewport(0, 0, w, h);
}

static void piglit_glfw_refresh_callback(GLFWwindow *pWin)
{
	if(glfwGetWindowUserPointer(pWin) !=NULL)
	{
		piglit_glfw_framework_t *glfw_fw =
			(piglit_glfw_framework_t*) glfwGetWindowUserPointer(pWin);
		glfw_fw->need_redisplay = true;
	}
}

char glfw_key_to_ascii(int key)
{
	if(64 < key) {
		if( key < 91) {
		return (key + 32);
		}
	}
	return key;
}

static void piglit_glfw_key_callback(GLFWwindow* pWin, int key, int scancode, int action, int mods)
{
	piglit_glfw_framework_t *glfw_fw = NULL;
	glfw_fw = (piglit_glfw_framework_t*) glfwGetWindowUserPointer(pWin);
	if (action == GLFW_PRESS)
	{
		if(key == GLFW_KEY_ESCAPE)
		{
			// Static Code.
			glfwSetWindowShouldClose(pWin, GL_TRUE);
		} else {
			if(glfw_fw)
			{
				/* TODO: Fix the ascii Table */
				if(glfw_fw->glfw_keyboard_func){
					glfw_fw->glfw_keyboard_func(glfw_key_to_ascii(key),0,0);
				}
			}
		}
	}
}

static void piglit_glfw_terminate()
{
	GLFWwindow* pWin;
	pWin = glfwGetCurrentContext();
	if(pWin)
		glfwDestroyWindow(pWin);
	glfwTerminate();
}

bool piglit_glfw_init_context(piglit_glfw_framework_t * glfw_fw,
	const struct piglit_gl_test_config *test_config, bool isOpenGL)
{
	if(glfw_fw->pGL_Window) {
		glfwMakeContextCurrent(NULL);
		/* Clean Up Previous Window if this exists. */
		glfwDestroyWindow(glfw_fw->pGL_Window);
		glfw_fw->pGL_Window = NULL;
	}
	glfwDefaultWindowHints();
	if(test_config->window_samples){
		printf("piglit: info: GLFW Multi-sample Window. %d\n",
			test_config->window_samples);
		glfwWindowHint(GLFW_SAMPLES,test_config->window_samples);
	}
    if(test_config->window_visual &PIGLIT_GL_VISUAL_ACCUM){
		printf("piglit: info: GLFW ACCUM Window.\n");
	if (test_config->window_visual & PIGLIT_GL_VISUAL_RGBA) {
		glfwWindowHint(GLFW_ACCUM_ALPHA_BITS,8);
	}
	if(test_config->window_visual &
	  (PIGLIT_GL_VISUAL_RGB|PIGLIT_GL_VISUAL_RGBA)) {
			glfwWindowHint(GLFW_ACCUM_RED_BITS,8);
			glfwWindowHint(GLFW_ACCUM_GREEN_BITS,8);
			glfwWindowHint(GLFW_ACCUM_BLUE_BITS,8);
		}
	}
	if (test_config->window_visual & PIGLIT_GL_VISUAL_DOUBLE) {
		/* FIXME: GLFW only supports Double Buffered Visuals*/
	}
	if (!(test_config->window_visual & PIGLIT_GL_VISUAL_DEPTH)) {
		//printf("piglit: info: GLFW Depth not specified.\n");
		glfwWindowHint(GLFW_DEPTH_BITS,0);
	}
	if (!(test_config->window_visual & PIGLIT_GL_VISUAL_STENCIL)) {
		//printf("piglit: info: GLFW Stencil not specified.\n");
		glfwWindowHint(GLFW_STENCIL_BITS,0);
	}

#if 0
	/* TODO: Support this */
	if(isOpenGL) {
		if(test_config->require_forward_compatible_context) {
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE)
		}
		if(test_config->require_debug_context) {
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE)
		}
	}
#endif
	return true;
}


bool
piglit_glfw_check_gl_version(piglit_glfw_framework_t * glfw_fw,
	const struct piglit_gl_test_config *test_config)
{

	printf("piglit: info: Attempting to create GLFW Window.\n");
	piglit_glfw_init_context( glfw_fw, test_config,false);
	if(test_config->supports_gl_es_version) {
		printf("piglit: info: Creating OpenGL ES %d.%d Context.\n",
			test_config->supports_gl_es_version/10,
			test_config->supports_gl_es_version%10);
		glfwWindowHint(GLFW_CLIENT_API,GLFW_OPENGL_ES_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, test_config->supports_gl_es_version/10);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, test_config->supports_gl_es_version%10);
		glfw_fw->pGL_Window = glfwCreateWindow(test_config->window_width, test_config->window_height, "Piglit", NULL, NULL);
		if(glfw_fw->pGL_Window != NULL) {
			goto GLFW_GOOD_CONTEXT;
		} else {
			printf("piglit: info: Failed to create OpenGL ES %d.%d Context.\n",
				test_config->supports_gl_es_version/10,
				test_config->supports_gl_es_version%10);
		}
	}
	piglit_glfw_init_context( glfw_fw, test_config,true);
	/* Attempt to Make an OpenGL 3.x Core Profile Context */
	if(test_config->supports_gl_core_version){
		printf("piglit: info: Creating OpenGL %d.%d Context (Core Profile).\n",
				test_config->supports_gl_core_version/10,
				test_config->supports_gl_core_version%10 );
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, test_config->supports_gl_core_version/10);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, test_config->supports_gl_core_version%10);
		glfw_fw->pGL_Window = glfwCreateWindow(test_config->window_width, test_config->window_height, "Piglit", NULL, NULL);
		if(glfw_fw->pGL_Window != NULL) {
			glfwMakeContextCurrent(glfw_fw->pGL_Window);
			if(test_config->supports_gl_core_version == 31) {
				if(!glfwExtensionSupported("GL_ARB_compatibility")) {
					goto GLFW_GOOD_CONTEXT;
				}
				printf("piglit: info: Requested OpenGL 3.1 Core context, but got Compatibility Context.\n");
			}  else {
				piglit_is_core_profile = true;
				return true;
			}
		} else {
			printf("piglit: info: Failed to create OpenGL %d.%d Core context.\n",
				test_config->supports_gl_core_version/10,
				test_config->supports_gl_core_version%10 );
		}
	}

	piglit_glfw_init_context( glfw_fw, test_config,true);
    if(test_config->supports_gl_compat_version) {
		if (test_config->supports_gl_core_version) {
			/* The above attempt to create a core context failed. */
			printf("piglit: info: Falling back to GL compatibility context\n");
		}
		printf("piglit: info: Creating OpenGL %d.%d Compatibility context.\n",
			test_config->supports_gl_compat_version/10,
			 test_config->supports_gl_compat_version%10);
		/* Do not Specify Profile Type when falling back to Compatibility Profile */
		if(31 <= test_config->supports_gl_compat_version)
		{
			//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, test_config->supports_gl_compat_version/10);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, test_config->supports_gl_compat_version%10);
		}
		printf("piglit: info: Window Width %d.%d.\n",
			test_config->window_width,test_config->window_height);
		glfw_fw->pGL_Window  = glfwCreateWindow(test_config->window_width, test_config->window_height, "Piglit", NULL, NULL);
		if(glfw_fw->pGL_Window  != NULL) {
			printf("piglit: info: Created OpenGL %d.%d context.\n",
				glfwGetWindowAttrib(glfw_fw->pGL_Window,GLFW_CONTEXT_VERSION_MAJOR),
				 glfwGetWindowAttrib(glfw_fw->pGL_Window,GLFW_CONTEXT_VERSION_MINOR));
			goto GLFW_GOOD_CONTEXT;
		}
	}
	printf("piglit: info: Failed To create any Context.\n");
	return false;
GLFW_GOOD_CONTEXT:
	glfwMakeContextCurrent(glfw_fw->pGL_Window);
	return true;
}


struct piglit_gl_framework*
piglit_glfw_framework_create(const struct piglit_gl_test_config *test_config)
{
	bool ok;
	GLint flags = 0;
	piglit_glfw_framework_t * glfw_fw;

	// Attempt to initialize Framework.
	if(glfwInit() == GL_FALSE)
	{
		printf("piglit: info: Failed to Initialize GLFW context\n");
		return NULL;
	}

	atexit(piglit_glfw_terminate);

	glfw_fw = calloc(sizeof(piglit_glfw_framework_t),1);
	if(glfw_fw == NULL)
	{
		printf("piglit: Failed to Allocate Framework\n");
		goto FAILURE;
	}

	ok = piglit_gl_framework_init(&glfw_fw->gl_fw, test_config);
	if(!ok) {
		printf("piglit: Failed to Initialize Framework\n");
		goto FAILURE;
	}

	piglit_is_core_profile = false; // Default is non-core.
	if (!piglit_glfw_check_gl_version(glfw_fw, test_config))
	{
		printf("piglit: Failed to create Context\n");
		goto FAILURE;
	}


#ifdef PIGLIT_USE_OPENGL
	piglit_dispatch_default_init(PIGLIT_DISPATCH_GL);
#elif defined(PIGLIT_USE_OPENGL_ES2) || defined(PIGLIT_USE_OPENGL_ES3)
	piglit_dispatch_default_init(PIGLIT_DISPATCH_ES2);
#endif


	glfw_fw->gl_fw.swap_buffers = glfw_swap_buffers;
	glfw_fw->gl_fw.run_test = glfw_run_test;
	glfw_fw->gl_fw.post_redisplay = glfw_post_redisplay;
	glfw_fw->gl_fw.set_keyboard_func = glfw_set_keyboard_func;
	glfw_fw->gl_fw.set_reshape_func = glfw_set_reshape_func;
	glfw_fw->gl_fw.destroy = glfw_destroy;

	// set GLFW Callbacks.
	glfwSetKeyCallback(glfw_fw->pGL_Window, piglit_glfw_key_callback);
	glfwSetWindowSizeCallback(glfw_fw->pGL_Window,piglit_glfw_resize_callback);
	glfwSetWindowRefreshCallback(glfw_fw->pGL_Window,piglit_glfw_refresh_callback);
	glfwSetWindowUserPointer (glfw_fw->pGL_Window,(void*)&glfw_fw->gl_fw);
	return &glfw_fw->gl_fw;
FAILURE:
	piglit_report_result(PIGLIT_SKIP);
	return NULL;
}
