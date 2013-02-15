/*
 * Copyright © 2009 Intel Corporation
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

#pragma once
#ifndef PIGLIT_FRAMEWORK_H
#define PIGLIT_FRAMEWORK_H

#include <assert.h>
#include <stdbool.h>

#include "piglit-util.h"

/**
 * A bitmask of these enums specifies visual attributes for the test's window.
 *
 * Each enum has the same value of its corresponding GLUT enum. That is, for
 * each X, `PIGLIT_GL_VISUAL_X == GLUT_X`.
 *
 * Note that PIGLIT_GL_VISUAL_RGBA is an alias for PIGLIT_GL_VISUAL_RGB and is
 * always selected.  From the documentation of glutInitDisplayMode in
 * Kilgard's GLUT:
 *
 *     Note that GLUT_RGBA selects the RGBA color model, but it does not
 *     request any bits of alpha (sometimes called an alpha buffer or
 *     destination alpha) be allocated. To request alpha, specify GLUT_ALPHA.
 *
 * \see piglit_gl_test_config::window_visual
 */
enum piglit_gl_visual {
	PIGLIT_GL_VISUAL_RGB 		= 0,
	PIGLIT_GL_VISUAL_RGBA 		= 0,
	PIGLIT_GL_VISUAL_SINGLE 	= 0,
	PIGLIT_GL_VISUAL_DOUBLE 	= 1 << 1,
	PIGLIT_GL_VISUAL_ACCUM 		= 1 << 2,
	PIGLIT_GL_VISUAL_ALPHA 		= 1 << 3,
	PIGLIT_GL_VISUAL_DEPTH 		= 1 << 4,
	PIGLIT_GL_VISUAL_STENCIL 	= 1 << 5,
};

/**
 * @brief Configuration for running an OpenGL test.
 *
 * To run a test, pass this to piglit_gl_test_run().
 *
 * This is named piglit_gl_test_config, rather than piglit_test_config, in
 * order to distinguish it from other test types, such as EGL and GLX tests.
 *
 * At least one of the `supports` fields must be set.
 *
 * If `supports_gl_core_version` and `supports_gl_compat_version` are both
 * set, then Piglit will first attempt to run the test under a GL core context
 * of the requested version. If context creation fails, then Piglit will then
 * attempt to run the test under a GL compatibility context of the requested
 * version.
 */
struct piglit_gl_test_config {
	/**
	 * If this field is non-zero, then the test is able to run under any
	 * OpenGL ES context whose version is backwards-compatible with the
	 * given version.
	 *
	 * For example, if this field's value is '10', then Piglit will
	 * attempt to run the test under an OpenGL ES 1.0 context. Likewise
	 * for '20' and OpenGL ES 2.0.
	 *
	 * If Piglit fails to acquire the waffle_config or to create the
	 * waffle_context, then it skips its attempt to run the test under
	 * an OpenGL ES context.
	 *
	 * If this field is 0, then the test is not able to run under an
	 * OpenGL ES context of any version.
	 */
	int supports_gl_es_version;

	/**
	 * If this field is non-zero, then the test is able to run under a GL
	 * core context having at least the given version.
	 *
	 * When attempting run a test under a GL core context, Piglit chooses
	 * a waffle_config with the following attributes set.  (Note that
	 * Waffle ignores the profile attribute for versions less than 3.2).
	 *     - WAFFLE_CONTEXT_PROFILE       = WAFFLE_CONTEXT_CORE_PROFILE
	 *     - WAFFLE_CONTEXT_MAJOR_VERSION = supports_gl_core_version / 10
	 *     - WAFFLE_CONTEXT_MINOR_VERSION = supports_gl_core_version % 10
	 * If Piglit fails to acquire the waffle_config or to create the
	 * waffle_context, then it skips its attempt to run the test under
	 * a GL core context.
	 *
	 * It is an error if this field is less than 3.1 because the concept
	 * of "core context" does not apply before GL 3.1.
	 *
	 * Piglit handles a request for a GL 3.1 core context as a special
	 * case.  As noted above, Waffle ignores the profile attribute when
	 * choosing a 3.1 config. However, the concept of "core profile" is
	 * still applicable to 3.1 contexts and is indicated by the context's
	 * lack of support for the GL_ARB_compatibility extension. Therefore,
	 * Piglit attempts to run the test under a GL 3.1 core context by
	 * first creating the context and then skipping the attempt if the
	 * context supports the GL_ARB_compatibility extension.
	 *
	 * If this field is 0, then the test is not able to run under a GL
	 * core context of any version.
	 */
	int supports_gl_core_version;

	/**
	 * If this field is non-zero, then the test is able to run under a GL
	 * compatibility context having at least the given version.
	 *
	 * When attempting run a test under a GL compatibility context, Piglit
	 * chooses a waffle_config with the following attributes set.  (Note
	 * that Waffle ignores the profile attribute for versions less than
	 * 3.2).
	 *     - WAFFLE_CONTEXT_PROFILE       = WAFFLE_CONTEXT_COMPATIBILITY_PROFILE
	 *     - WAFFLE_CONTEXT_MAJOR_VERSION = supports_gl_core_version / 10
	 *     - WAFFLE_CONTEXT_MINOR_VERSION = supports_gl_core_version % 10
	 * If Piglit fails to acquire the waffle_config or to create the
	 * waffle_context, then it skips its attempt to run the test under
	 * a GL compatibility context.
	 *
	 * Piglit handles a request for a GL 3.1 compatibility context as
	 * a special case.  As noted above, Waffle ignores the profile
	 * attribute when choosing a 3.1 config. However, the concept of
	 * "compatibility profile" is still applicable to 3.1 contexts and is
	 * indicated by the context's support for the GL_ARB_compatibility
	 * extension. Therefore, Piglit attempts to run under a GL 3.1
	 * compatibility context by first creating the context and then
	 * skipping the attempt if the context lacks the GL_ARB_compatibility
	 * extension.
	 *
	 * Be aware that, if this field is greater than 10, then the test will
	 * skip on platforms for which specifying a context version is
	 * unsupported (that is, GLX that lacks GLX_ARB_create_context and EGL
	 * that lacks EGL_KHR_create_context). If the test requires a GL
	 * version greater than 1.0, then consider setting this field to 10
	 * and checking the GL version from within the test with
	 * piglit_require_gl_version().
	 *
	 * If this field is 0, then the test is not able to run under a GL
	 * compatibility context of any version.
	 */
	int supports_gl_compat_version;

	int window_width;
	int window_height;

	/**
	 * A bitmask of `enum piglit_gl_visual`.
	 */
	int window_visual;

	/**
	 * The test requires the window to be displayed in order to run
	 * correctly. Tests that read from the front buffer must enable
	 * this.
	 */
	bool requires_displayed_window;

	/**
	 * This is called once per test, after the GL context has been created
	 * and made current but before display() is called.
	 */
	void
	(*init)(int argc, char *argv[]);

	/**
	 * If the test is run in auto mode, then this is called once after
	 * init(). Otherwise, it is called repeatedly from some ill-defined
	 * event loop.
	 */
	enum piglit_result
	(*display)(void);
};

/**
 * Initialize @a config with default values.
 */
void
piglit_gl_test_config_init(struct piglit_gl_test_config *config);

/**
 * Run the OpenGL test described by @a config. Does not return.
 */
void
piglit_gl_test_run(int argc, char *argv[],
		   const struct piglit_gl_test_config *config);

#ifdef __cplusplus
#  define PIGLIT_EXTERN_C_BEGIN extern "C" {
#  define PIGLIT_EXTERN_C_END   }
#else
#  define PIGLIT_EXTERN_C_BEGIN
#  define PIGLIT_EXTERN_C_END
#endif

#define PIGLIT_GL_TEST_CONFIG_BEGIN                                          \
                                                                             \
        PIGLIT_EXTERN_C_BEGIN                                                \
                                                                             \
        void                                                                 \
        piglit_init(int argc, char *argv[]);                                 \
                                                                             \
        enum piglit_result                                                   \
        piglit_display(void);                                                \
                                                                             \
        PIGLIT_EXTERN_C_END                                                  \
                                                                             \
        int                                                                  \
        main(int argc, char *argv[])                                         \
        {                                                                    \
                struct piglit_gl_test_config config;                         \
                                                                             \
                /* Register Signal Handler to assist with automatically */   \
                /* capturing call stack when crash occurs. This is to */     \
                /* improve the distributed testing. */                       \
                piglit_register_signal_handler();                            \
                                                                             \
                piglit_gl_test_config_init(&config);                         \
                                                                             \
                config.init = piglit_init;                                   \
                config.display = piglit_display;                             \
                                                                             \
                /* Default window size.  Note: Win7's min window width */    \
                /* seems to be 116 pixels.  When the window size is */       \
                /* unexpectedly resized, tests are marked as "WARN". */      \
                /* Let's use a larger default to avoid that. */              \
                config.window_width = 150;                                   \
                config.window_height = 150;


#define PIGLIT_GL_TEST_CONFIG_END                                            \
                                                                             \
                piglit_gl_test_run(argc, argv, &config);                     \
                                                                             \
                assert(false);                                               \
                return 0;                                                    \
        }

extern int piglit_automatic;

extern int piglit_width;
extern int piglit_height;
extern bool piglit_use_fbo;
extern unsigned int piglit_winsys_fbo;

void piglit_swap_buffers(void);
void piglit_present_results();
void piglit_post_redisplay(void);
void piglit_set_keyboard_func(void (*func)(unsigned char key, int x, int y));
void piglit_set_reshape_func(void (*func)(int w, int h));

#endif /* PIGLIT_FRAMEWORK_H */
