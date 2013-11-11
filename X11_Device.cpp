#include "X11_Device.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/Xlibint.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>

#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

_XDisplay	*dpy      = NULL;
/*
 *  For now it's useless using XSHM
 */
#ifdef X11_USE_XSHM_
	int		 use_shm;
	XShmSegmentInfo  shminfo;
#endif /*X11_USE_XSHM_*/


XImage		*image    = NULL;
XWindowAttributes gwa;
Window CaptureWin = None;
int default_src = 0;
Window Select_Window();

void InitializeX11(uint16_t x_off, uint16_t y_off, uint16_t *w, uint16_t *h){
	printf("Loading X11 Module\n");

	dpy = XOpenDisplay("");

	default_src = XDefaultScreen(dpy);
	CaptureWin = Select_Window();
	XGetWindowAttributes(dpy, CaptureWin, &gwa);
	*w = gwa.width;
	*h = gwa.height;
#ifdef X11_USE_XSHM_
	use_shm = XShmQueryExtension(dpy);
	if(use_shm) {
		printf("XShmQueryExtension(dpy);\n");
		image = XShmCreateImage(dpy,
					DefaultVisual(dpy, default_src),
					DefaultDepth(dpy, default_src),
					ZPixmap,
					NULL,
					&shminfo,
					gwa.width, gwa.height);
		shminfo.shmid = shmget(	IPC_PRIVATE,
					image->bytes_per_line * image->height,
					IPC_CREAT|0777);
		if(shminfo.shmid == -1) {
			printf("shminfo.shmid == -1\n");
			return;
		}
		shminfo.shmaddr = image->data = (char*) shmat(shminfo.shmid, 0, 0);
		shminfo.readOnly = False;

		if (!XShmAttach(dpy, &shminfo)) {
			printf("!XShmAttach(dpy, &shminfo)\n");
			return;
		}
	}
	else{
#endif /*X11_USE_XSHM_*/
		image = XGetImage(dpy, CaptureWin,
				  x_off,y_off,
				  gwa.width,gwa.height,
				  AllPlanes, ZPixmap);
#ifdef X11_USE_XSHM_
	}
#endif /*X11_USE_XSHM_*/
	printf("X11 Module Running\n");
}

void StopX11(){
	XCloseDisplay(dpy);
	XFree(image);
	printf("Closing X11 Module\n");
}

uint32_t *GetX11(uint16_t x, uint16_t y, uint16_t *w, uint16_t *h){
	xGetImageReply rep;
	xGetImageReq *req;
	long nbytes;

	if(!image){
		return 0;
	}
	XGetWindowAttributes(dpy, CaptureWin, &gwa);

	if(image->width != gwa.width || image->height != gwa.height){
		printf("X11 Window: Changed W/H\n");
		*w = gwa.width;
		*h = gwa.height;
		XFree(image);
#ifdef X11_USE_XSHM_
		if(use_shm) {
			int scr = XDefaultScreen(dpy);
			image = XShmCreateImage(dpy,
						DefaultVisual(dpy, scr),
						DefaultDepth(dpy, scr),
						ZPixmap,
						NULL,
						&shminfo,
						w, h);
			shminfo.shmid = shmget(	IPC_PRIVATE,
						image->bytes_per_line * image->height,
						IPC_CREAT|0777);
			if(shminfo.shmid == -1){
				printf("shminfo.shmid == -1\n");
				return 0;
			}
			shminfo.shmaddr = image->data = (char*) shmat(shminfo.shmid, 0, 0);
			shminfo.readOnly = False;

			if(!XShmAttach(dpy, &shminfo)){
				printf("!XShmAttach(dpy, &shminfo)\n");
				return 0;
			}
		}
		else{
#endif /*X11_USE_XSHM_*/
			image = XGetImage(dpy, CaptureWin,
					  x,y,
					  gwa.width,gwa.height,
					  AllPlanes, ZPixmap);
#ifdef X11_USE_XSHM_
		}
#endif /*X11_USE_XSHM_*/
	}

	/* Here i should add a check to be sure that the window is still open */
	LockDisplay(dpy);
	GetReq(GetImage, req);

	req->drawable = CaptureWin;
	req->x = x;
	req->y = y;
	req->width = gwa.width;
	req->height = gwa.height;
	req->planeMask = (unsigned int)AllPlanes;
	req->format = ZPixmap;

	if (!_XReply(dpy, (xReply *)&rep, 0, xFalse) || !rep.length) {
		UnlockDisplay(dpy);
		SyncHandle();
		return 0;
	}

	nbytes = (long)rep.length << 2;
	_XReadPad(dpy, image->data, nbytes);

	UnlockDisplay(dpy);
	SyncHandle();

	return (uint32_t*) image->data;
}

/* from xwininfo , thanks*/
Window Select_Window(){
//	Display *dpy;
	int status;
	Cursor cursor;
	XEvent event;
	Window target_win = None, root = RootWindow(dpy, default_src);
	int buttons = 0;

	/* Make the target cursor */
	cursor = XCreateFontCursor(dpy, XC_crosshair);

	/* Grab the pointer using target cursor, letting it room all over */
	status = XGrabPointer(	dpy, root, False,
				ButtonPressMask|ButtonReleaseMask, GrabModeSync,
				GrabModeAsync, root, cursor, CurrentTime);
	if (status != GrabSuccess)
		printf("Can't grab the mouse.");

	/* Let the user select a window... */
	while ((target_win == None) || (buttons != 0)) {
		/* allow one more event */
		XAllowEvents(dpy, SyncPointer, CurrentTime);
		XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
		switch (event.type) {
		case ButtonPress:
			if (target_win == None) {
			target_win = event.xbutton.subwindow; /* window selected */
			if (target_win == None) target_win = root;
			}
			buttons++;
			break;
		case ButtonRelease:
			if (buttons > 0) /* there may have been some down before we started */
			buttons--;
			 break;
		}
	} 

	XUngrabPointer(dpy, CurrentTime); /* Done with pointer */

	if (target_win) {
		Window root;
		int dummyi;
		uint32_t dummy;
		if ( XGetGeometry (dpy, target_win, &root, &dummyi, &dummyi,
				   &dummy, &dummy, &dummy, &dummy) &&  target_win != root)
			target_win = XmuClientWindow (dpy, target_win);
	}

	return(target_win);
}




