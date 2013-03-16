
extern PFNGLXGETVIDEOSYNCSGIPROC __piglit_glXGetVideoSyncSGI;
#define glXGetVideoSyncSGI(count) (*__piglit_glXGetVideoSyncSGI)(count)

extern PFNGLXWAITVIDEOSYNCSGIPROC __piglit_glXWaitVideoSyncSGI;
#define glXWaitVideoSyncSGI(divisor, remainder, count)\
	 (*__piglit_glXWaitVideoSyncSGI)(divisor, remainder, count)	

extern PFNGLXSWAPINTERVALEXTPROC __piglit_glXSwapIntervalEXT;
#define glXSwapIntervalEXT (dpy,  drawable,  interval) \
	 (*__piglit_glXSwapIntervalEXT)(dpy,  drawable,  interval)	

extern Window win;
extern XVisualInfo *visinfo;
extern GLXContext directCtx;
extern GLXContext indirectCtx;
extern bool bEXT_swap_control_Supported;
extern unsigned int nRefreshRate;
void piglit_sgi_video_sync_test_run(enum piglit_result (*draw)(Display *dpy));
