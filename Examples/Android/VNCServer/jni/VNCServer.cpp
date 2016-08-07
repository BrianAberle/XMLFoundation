//
//  NOTE: this file is a consolidation of:
//  -----------------------------------------------
//	LibVNCServer-0.9.7/libvncserver/main.c \
//	LibVNCServer-0.9.7/libvncserver/rfbserver.c \
//	LibVNCServer-0.9.7/libvncserver/rfbregion.c \
//	LibVNCServer-0.9.7/libvncserver/auth.c \
//	LibVNCServer-0.9.7/libvncserver/sockets.c \
//	LibVNCServer-0.9.7/libvncserver/stats.c \
//	LibVNCServer-0.9.7/libvncserver/corre.c \
//	LibVNCServer-0.9.7/libvncserver/hextile.c \
//	LibVNCServer-0.9.7/libvncserver/rre.c \
//	LibVNCServer-0.9.7/libvncserver/translate.c \
//	LibVNCServer-0.9.7/libvncserver/cutpaste.c \
//	LibVNCServer-0.9.7/libvncserver/httpd.c \
//	LibVNCServer-0.9.7/libvncserver/cursor.c \
//	LibVNCServer-0.9.7/libvncserver/font.c \
//	LibVNCServer-0.9.7/libvncserver/draw.c \
//	LibVNCServer-0.9.7/libvncserver/selbox.c \
//	LibVNCServer-0.9.7/libvncserver/d3des.c \
//	LibVNCServer-0.9.7/libvncserver/vncauth.c \
//	LibVNCServer-0.9.7/libvncserver/cargs.c \
//	LibVNCServer-0.9.7/libvncserver/minilzo.c \
//	LibVNCServer-0.9.7/libvncserver/ultra.c \
//	LibVNCServer-0.9.7/libvncserver/scale.c \
//	LibVNCServer-0.9.7/libvncserver/zlib.c \
//	LibVNCServer-0.9.7/libvncserver/zrle.c \
//	LibVNCServer-0.9.7/libvncserver/zrleoutstream.c \
//	LibVNCServer-0.9.7/libvncserver/zrlepalettehelper.c \
//	LibVNCServer-0.9.7/libvncserver/zywrletemplate.c \
//	LibVNCServer-0.9.7/libvncserver/tight.c \
//
//
///////////////////////////////////////////begin fbvncserver.c //////////////////////////////////////////////////
/*
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * This project is an adaptation of the original fbvncserver for the iPAQ
 * and Zaurus.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <sys/stat.h>
#include <sys/sysmacros.h>             /* For makedev() */

#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>


//#include <C:\\USERS/USER/DESKTOP/ANDROIDSDK/NDK-BUNDLE/platforms/android-19/arch-arm/usr/include/linux/input.h>
        // C:\USERS\USER\DESKTOP\ANDROIDSDK\NDK-BUNDLE\platforms\android-19\arch-x86\usr\include\linux\input.h

// values copied from input.h
#define KEY_STAR 227
#define KEY_SHARP 228
#define KEY_SOFT1 229
#define KEY_SOFT2 230
#define KEY_SEND 231
#define KEY_CENTER 232
#define KEY_HEADSETHOOK 233
#define KEY_0_5 234
#define KEY_2_5 235


#include <assert.h>
#include <errno.h>

/* libvncserver */
#include "rfb/rfb.h"
#include "rfb/keysym.h"

/*****************************************************************************/

/* Android does not use /dev/fb0. */
#define FB_DEVICE "/dev/graphics/fb0"
static char KBD_DEVICE[256] = "/dev/input/event3";
static char TOUCH_DEVICE[256] = "/dev/input/event1";
static struct fb_var_screeninfo scrinfo;
static int fbfd = -1;
static int kbdfd = -1;
static int touchfd = -1;
static unsigned short int *fbmmap = MAP_FAILED;
static unsigned short int *vncbuf;
static unsigned short int *fbbuf;

/* Android already has 5900 bound natively. */
#define VNC_PORT 5901
static rfbScreenInfoPtr vncscr;

static int xmin, xmax;
static int ymin, ymax;

/* No idea, just copied from fbvncserver as part of the frame differerencing
 * algorithm.  I will probably be later rewriting all of this. */
static struct varblock_t
{
	int min_i;
	int min_j;
	int max_i;
	int max_j;
	int r_offset;
	int g_offset;
	int b_offset;
	int rfb_xres;
	int rfb_maxy;
} varblock;

/*****************************************************************************/

static void keyevent(rfbBool down, rfbKeySym key, rfbClientPtr cl);
static void ptrevent(int buttonMask, int x, int y, rfbClientPtr cl);

/*****************************************************************************/

static void init_fb(void)
{
	size_t pixels;
	size_t bytespp;

	if ((fbfd = open(FB_DEVICE, O_RDONLY)) == -1)
	{
		printf("cannot open fb device %s\n", FB_DEVICE);
		exit(EXIT_FAILURE);
	}

	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
	{
		printf("ioctl error\n");
		exit(EXIT_FAILURE);
	}

	pixels = scrinfo.xres * scrinfo.yres;
	bytespp = scrinfo.bits_per_pixel / 8;

	fprintf(stderr, "xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n", 
	  (int)scrinfo.xres, (int)scrinfo.yres,
	  (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
	  (int)scrinfo.xoffset, (int)scrinfo.yoffset,
	  (int)scrinfo.bits_per_pixel);

	fbmmap = mmap(NULL, pixels * bytespp, PROT_READ, MAP_SHARED, fbfd, 0);

	if (fbmmap == MAP_FAILED)
	{
		printf("mmap failed\n");
		exit(EXIT_FAILURE);
	}
}

static void cleanup_fb(void)
{
	if(fbfd != -1)
	{
		close(fbfd);
	}
}

static void init_kbd()
{
	if((kbdfd = open(KBD_DEVICE, O_RDWR)) == -1)
	{
		printf("cannot open kbd device %s\n", KBD_DEVICE);
		exit(EXIT_FAILURE);
	}
}

static void cleanup_kbd()
{
	if(kbdfd != -1)
	{
		close(kbdfd);
	}
}

static void init_touch()
{
    struct input_absinfo info;
        if((touchfd = open(TOUCH_DEVICE, O_RDWR)) == -1)
        {
                printf("cannot open touch device %s\n", TOUCH_DEVICE);
                exit(EXIT_FAILURE);
        }
    // Get the Range of X and Y
    if(ioctl(touchfd, EVIOCGABS(ABS_X), &info)) {
        printf("cannot get ABS_X info, %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    xmin = info.minimum;
    xmax = info.maximum;
    if(ioctl(touchfd, EVIOCGABS(ABS_Y), &info)) {
        printf("cannot get ABS_Y, %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ymin = info.minimum;
    ymax = info.maximum;

}

static void cleanup_touch()
{
	if(touchfd != -1)
	{
		close(touchfd);
	}
}

/*****************************************************************************/

static void init_fb_server(int argc, char **argv)
{
	printf("Initializing server...\n");

	/* Allocate the VNC server buffer to be managed (not manipulated) by 
	 * libvncserver. */
	vncbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 2);
	assert(vncbuf != NULL);

	/* Allocate the comparison buffer for detecting drawing updates from frame
	 * to frame. */
	fbbuf = calloc(scrinfo.xres * scrinfo.yres, scrinfo.bits_per_pixel / 2);
	assert(fbbuf != NULL);

	/* TODO: This assumes scrinfo.bits_per_pixel is 16. */
	vncscr = rfbGetScreen(&argc, argv, scrinfo.xres, scrinfo.yres, 5, 2, 2);
	assert(vncscr != NULL);

	vncscr->desktopName = "Android";
	vncscr->frameBuffer = (char *)vncbuf;
	vncscr->alwaysShared = TRUE;
	vncscr->httpDir = NULL;
	vncscr->port = VNC_PORT;

	vncscr->kbdAddEvent = keyevent;
	vncscr->ptrAddEvent = ptrevent;

	rfbInitServer(vncscr);

	/* Mark as dirty since we haven't sent any updates at all yet. */
	rfbMarkRectAsModified(vncscr, 0, 0, scrinfo.xres, scrinfo.yres);

	/* No idea. */
	varblock.r_offset = scrinfo.red.offset + scrinfo.red.length - 5;
	varblock.g_offset = scrinfo.green.offset + scrinfo.green.length - 5;
	varblock.b_offset = scrinfo.blue.offset + scrinfo.blue.length - 5;
	varblock.rfb_xres = scrinfo.yres;
	varblock.rfb_maxy = scrinfo.xres - 1;
}

/*****************************************************************************/
void injectKeyEvent(uint16_t code, uint16_t value)
{
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time,0);
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    if(write(kbdfd, &ev, sizeof(ev)) < 0)
    {
        printf("write event failed, %s\n", strerror(errno));
    }

    printf("injectKey (%d, %d)\n", code , value);    
}

static int keysym2scancode(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
    int scancode = 0;

    int code = (int)key;
    if (code>='0' && code<='9') {
        scancode = (code & 0xF) - 1;
        if (scancode<0) scancode += 10;
        scancode += KEY_1;
    } else if (code>=0xFF50 && code<=0xFF58) {
        static const uint16_t map[] =
             {  KEY_HOME, KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN,
                KEY_SOFT1, KEY_SOFT2, KEY_END, 0 };
        scancode = map[code & 0xF];
    } else if (code>=0xFFE1 && code<=0xFFEE) {
        static const uint16_t map[] =
             {  KEY_LEFTSHIFT, KEY_LEFTSHIFT,
                KEY_COMPOSE, KEY_COMPOSE,
                KEY_LEFTSHIFT, KEY_LEFTSHIFT,
                0,0,
                KEY_LEFTALT, KEY_RIGHTALT,
                0, 0, 0, 0 };
        scancode = map[code & 0xF];
    } else if ((code>='A' && code<='Z') || (code>='a' && code<='z')) {
        static const uint16_t map[] = {
                KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,
                KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
                KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
                KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
                KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z };
        scancode = map[(code & 0x5F) - 'A'];
    } else {
        switch (code) {
            case 0x0003:    scancode = KEY_CENTER;      break;
            case 0x0020:    scancode = KEY_SPACE;       break;
            case 0x0023:    scancode = KEY_SHARP;       break;
            case 0x0033:    scancode = KEY_SHARP;       break;
            case 0x002C:    scancode = KEY_COMMA;       break;
            case 0x003C:    scancode = KEY_COMMA;       break;
            case 0x002E:    scancode = KEY_DOT;         break;
            case 0x003E:    scancode = KEY_DOT;         break;
            case 0x002F:    scancode = KEY_SLASH;       break;
            case 0x003F:    scancode = KEY_SLASH;       break;
            case 0x0032:    scancode = KEY_EMAIL;       break;
            case 0x0040:    scancode = KEY_EMAIL;       break;
            case 0xFF08:    scancode = KEY_BACKSPACE;   break;
            case 0xFF1B:    scancode = KEY_BACK;        break;
            case 0xFF09:    scancode = KEY_TAB;         break;
            case 0xFF0D:    scancode = KEY_ENTER;       break;
            case 0x002A:    scancode = KEY_STAR;        break;
            case 0xFFBE:    scancode = KEY_F1;        break; // F1
            case 0xFFBF:    scancode = KEY_F2;         break; // F2
            case 0xFFC0:    scancode = KEY_F3;        break; // F3
            case 0xFFC5:    scancode = KEY_F4;       break; // F8
            case 0xFFC8:    rfbShutdownServer(cl->screen,TRUE);       break; // F11            
        }
    }

    return scancode;
}

static void keyevent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	int scancode;

	printf("Got keysym: %04x (down=%d)\n", (unsigned int)key, (int)down);

	if ((scancode = keysym2scancode(down, key, cl)))
	{
		injectKeyEvent(scancode, down);
	}
}

void injectTouchEvent(int down, int x, int y)
{
    struct input_event ev;
    
    // Calculate the final x and y
    x = xmin + (x * (xmax - xmin)) / (scrinfo.xres);
    y = ymin + (y * (ymax - ymin)) / (scrinfo.yres);
    
    memset(&ev, 0, sizeof(ev));

    // Then send a BTN_TOUCH
    gettimeofday(&ev.time,0);
    ev.type = EV_KEY;
    ev.code = BTN_TOUCH;
    ev.value = down;
    if(write(touchfd, &ev, sizeof(ev)) < 0)
    {
        printf("write event failed, %s\n", strerror(errno));
    }

    // Then send the X
    gettimeofday(&ev.time,0);
    ev.type = EV_ABS;
    ev.code = ABS_X;
    ev.value = x;
    if(write(touchfd, &ev, sizeof(ev)) < 0)
    {
        printf("write event failed, %s\n", strerror(errno));
    }

    // Then send the Y
    gettimeofday(&ev.time,0);
    ev.type = EV_ABS;
    ev.code = ABS_Y;
    ev.value = y;
    if(write(touchfd, &ev, sizeof(ev)) < 0)
    {
        printf("write event failed, %s\n", strerror(errno));
    }

    // Finally send the SYN
    gettimeofday(&ev.time,0);
    ev.type = EV_SYN;
    ev.code = 0;
    ev.value = 0;
    if(write(touchfd, &ev, sizeof(ev)) < 0)
    {
        printf("write event failed, %s\n", strerror(errno));
    }

    printf("injectTouchEvent (x=%d, y=%d, down=%d)\n", x , y, down);    
}

static void ptrevent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	/* Indicates either pointer movement or a pointer button press or release. The pointer is
now at (x-position, y-position), and the current state of buttons 1 to 8 are represented
by bits 0 to 7 of button-mask respectively, 0 meaning up, 1 meaning down (pressed).
On a conventional mouse, buttons 1, 2 and 3 correspond to the left, middle and right
buttons on the mouse. On a wheel mouse, each step of the wheel upwards is represented
by a press and release of button 4, and each step downwards is represented by
a press and release of button 5. 
  From: http://www.vislab.usyd.edu.au/blogs/index.php/2009/05/22/an-headerless-indexed-protocol-for-input-1?blog=61 */
	
	//printf("Got ptrevent: %04x (x=%d, y=%d)\n", buttonMask, x, y);
	if(buttonMask & 1) {
		// Simulate left mouse event as touch event
		injectTouchEvent(1, x, y);
		injectTouchEvent(0, x, y);
	} 
}

#define PIXEL_FB_TO_RFB(p,r,g,b) ((p>>r)&0x1f001f)|(((p>>g)&0x1f001f)<<5)|(((p>>b)&0x1f001f)<<10)

static void update_screen(void)
{
	unsigned int *f, *c, *r;
	int x, y;

	varblock.min_i = varblock.min_j = 9999;
	varblock.max_i = varblock.max_j = -1;

	f = (unsigned int *)fbmmap;        /* -> framebuffer         */
	c = (unsigned int *)fbbuf;         /* -> compare framebuffer */
	r = (unsigned int *)vncbuf;        /* -> remote framebuffer  */

	for (y = 0; y < scrinfo.yres; y++)
	{
		/* Compare every 2 pixels at a time, assuming that changes are likely
		 * in pairs. */
		for (x = 0; x < scrinfo.xres; x += 2)
		{
			unsigned int pixel = *f;

			if (pixel != *c)
			{
				*c = pixel;

				/* XXX: Undo the checkered pattern to test the efficiency
				 * gain using hextile encoding. */
				if (pixel == 0x18e320e4 || pixel == 0x20e418e3)
					pixel = 0x18e318e3;

				*r = PIXEL_FB_TO_RFB(pixel,
				  varblock.r_offset, varblock.g_offset, varblock.b_offset);

				if (x < varblock.min_i)
					varblock.min_i = x;
				else
				{
					if (x > varblock.max_i)
						varblock.max_i = x;

					if (y > varblock.max_j)
						varblock.max_j = y;
					else if (y < varblock.min_j)
						varblock.min_j = y;
				}
			}

			f++, c++;
			r++;
		}
	}

	if (varblock.min_i < 9999)
	{
		if (varblock.max_i < 0)
			varblock.max_i = varblock.min_i;

		if (varblock.max_j < 0)
			varblock.max_j = varblock.min_j;

		fprintf(stderr, "Dirty page: %dx%d+%d+%d...\n",
		  (varblock.max_i+2) - varblock.min_i, (varblock.max_j+1) - varblock.min_j,
		  varblock.min_i, varblock.min_j);

		rfbMarkRectAsModified(vncscr, varblock.min_i, varblock.min_j,
		  varblock.max_i + 2, varblock.max_j + 1);

		rfbProcessEvents(vncscr, 10000);
	}
}

/*****************************************************************************/

void print_usage(char **argv)
{
	printf("[-k device] [-t device] [-h]\n"
		"-k device: keyboard device node, default is /dev/input/event3\n"
		"-t device: touch device node, default is /dev/input/event1\n"
		"-h : print this help\n");
}

int main(int argc, char **argv)
{
	if(argc > 1)
	{
		int i=1;
		while(i < argc)
		{
			if(*argv[i] == '-')
			{
				switch(*(argv[i] + 1))
				{
					case 'h':
						print_usage(argv);
						exit(0);
						break;
					case 'k':
						i++;
						strcpy(KBD_DEVICE, argv[i]);
						break;
					case 't':
						i++;
						strcpy(TOUCH_DEVICE, argv[i]);
						break;
				}
			}
			i++;
		}
	}

	printf("Initializing framebuffer device " FB_DEVICE "...\n");
	init_fb();
	printf("Initializing keyboard device %s ...\n", KBD_DEVICE);
	init_kbd();
	printf("Initializing touch device %s ...\n", TOUCH_DEVICE);
	init_touch();

	printf("Initializing VNC server:\n");
	printf("	width:  %d\n", (int)scrinfo.xres);
	printf("	height: %d\n", (int)scrinfo.yres);
	printf("	bpp:    %d\n", (int)scrinfo.bits_per_pixel);
	printf("	port:   %d\n", (int)VNC_PORT);
	init_fb_server(argc, argv);

	/* Implement our own event loop to detect changes in the framebuffer. */
	while (1)
	{
		while (vncscr->clientHead == NULL)
			rfbProcessEvents(vncscr, 100000);

		rfbProcessEvents(vncscr, 100000);
		update_screen();
	}

	printf("Cleaning up...\n");
	cleanup_fb();
	cleanup_kdb();
	cleanup_touch();
}

///////////////////////////////////////////end fbvncserver.c //////////////////////////////////////////////////


///////////////////////////////////////////begin main.c //////////////////////////////////////////////////
/*
 *  This file is called main.c, because it contains most of the new functions
 *  for use with LibVNCServer.
 *
 *  LibVNCServer (C) 2001 Johannes E. Schindelin <Johannes.Schindelin@gmx.de>
 *  Original OSXvnc (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  see GPL (latest version) for full details
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#endif
#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#include "private.h"

#include <stdarg.h>
#include <errno.h>

#ifndef false
#define false 0
#define true -1
#endif

#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <signal.h>
#include <time.h>

static int extMutex_initialized = 0;
static int logMutex_initialized = 0;
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
static MUTEX(logMutex);
static MUTEX(extMutex);
#endif

static int rfbEnableLogging=1;

#ifdef LIBVNCSERVER_WORDS_BIGENDIAN
char rfbEndianTest = (1==0);
#else
char rfbEndianTest = (1==1);
#endif

/*
 * Protocol extensions
 */

static rfbProtocolExtension* rfbExtensionHead = NULL;

/*
 * This method registers a list of new extensions.  
 * It avoids same extension getting registered multiple times. 
 * The order is not preserved if multiple extensions are
 * registered at one-go.
 */
void
rfbRegisterProtocolExtension(rfbProtocolExtension* extension)
{
	rfbProtocolExtension *head = rfbExtensionHead, *next = NULL;

	if(extension == NULL)
		return;

	next = extension->next;

	if (! extMutex_initialized) {
		INIT_MUTEX(extMutex);
		extMutex_initialized = 1;
	}

	LOCK(extMutex);

	while(head != NULL) {
		if(head == extension) {
			UNLOCK(extMutex);
			rfbRegisterProtocolExtension(next);
			return;
		}

		head = head->next;
	}

	extension->next = rfbExtensionHead;
	rfbExtensionHead = extension;

	UNLOCK(extMutex);
	rfbRegisterProtocolExtension(next);
}

/*
 * This method unregisters a list of extensions.  
 * These extensions won't be available for any new
 * client connection. 
 */
void
rfbUnregisterProtocolExtension(rfbProtocolExtension* extension)
{

	rfbProtocolExtension *cur = NULL, *pre = NULL;

	if(extension == NULL)
		return;

	if (! extMutex_initialized) {
		INIT_MUTEX(extMutex);
		extMutex_initialized = 1;
	}

	LOCK(extMutex);

	if(rfbExtensionHead == extension) {
		rfbExtensionHead = rfbExtensionHead->next;
		UNLOCK(extMutex);
		rfbUnregisterProtocolExtension(extension->next);
		return;
	}

	cur = pre = rfbExtensionHead;

	while(cur) {
		if(cur == extension) {
			pre->next = cur->next;
			break;
		}
		pre = cur;
		cur = cur->next;
	}

	UNLOCK(extMutex);

	rfbUnregisterProtocolExtension(extension->next);
}

rfbProtocolExtension* rfbGetExtensionIterator()
{
	LOCK(extMutex);
	return rfbExtensionHead;
}

void rfbReleaseExtensionIterator()
{
	UNLOCK(extMutex);
}

rfbBool rfbEnableExtension(rfbClientPtr cl, rfbProtocolExtension* extension,
	void* data)
{
	rfbExtensionData* extData;

	/* make sure extension is not yet enabled. */
	for(extData = cl->extensions; extData; extData = extData->next)
		if(extData->extension == extension)
			return FALSE;

	extData = calloc(sizeof(rfbExtensionData),1);
	extData->extension = extension;
	extData->data = data;
	extData->next = cl->extensions;
	cl->extensions = extData;

	return TRUE;
}

rfbBool rfbDisableExtension(rfbClientPtr cl, rfbProtocolExtension* extension)
{
	rfbExtensionData* extData;
	rfbExtensionData* prevData = NULL;

	for(extData = cl->extensions; extData; extData = extData->next) {
		if(extData->extension == extension) {
			if(extData->data)
				free(extData->data);
			if(prevData == NULL)
				cl->extensions = extData->next;
			else
				prevData->next = extData->next;
			return TRUE;
		}
		prevData = extData;
	}

	return FALSE;
}

void* rfbGetExtensionClientData(rfbClientPtr cl, rfbProtocolExtension* extension)
{
    rfbExtensionData* data = cl->extensions;

    while(data && data->extension != extension)
	data = data->next;

    if(data == NULL) {
	rfbLog("Extension is not enabled !\n");
	/* rfbCloseClient(cl); */
	return NULL;
    }

    return data->data;
}

/*
 * Logging
 */

void rfbLogEnable(int enabled) {
  rfbEnableLogging=enabled;
}

/*
 * rfbLog prints a time-stamped message to the log file (stderr).
 */

static void
rfbDefaultLog(const char *format, ...)
{
    va_list args;
    char buf[256];
    time_t log_clock;

    if(!rfbEnableLogging)
      return;

    if (! logMutex_initialized) {
      INIT_MUTEX(logMutex);
      logMutex_initialized = 1;
    }

    LOCK(logMutex);
    va_start(args, format);

    time(&log_clock);
    strftime(buf, 255, "%d/%m/%Y %X ", localtime(&log_clock));

#ifndef ANDROID
    fprintf(stderr,buf);
#endif

    vfprintf(stderr, format, args);
    fflush(stderr);

    va_end(args);
    UNLOCK(logMutex);
}

rfbLogProc rfbLog=rfbDefaultLog;
rfbLogProc rfbErr=rfbDefaultLog;

void rfbLogPerror(const char *str)
{
    rfbErr("%s: %s\n", str, strerror(errno));
}

void rfbScheduleCopyRegion(rfbScreenInfoPtr rfbScreen,sraRegionPtr copyRegion,int dx,int dy)
{  
   rfbClientIteratorPtr iterator;
   rfbClientPtr cl;

   iterator=rfbGetClientIterator(rfbScreen);
   while((cl=rfbClientIteratorNext(iterator))) {
     LOCK(cl->updateMutex);
     if(cl->useCopyRect) {
       sraRegionPtr modifiedRegionBackup;
       if(!sraRgnEmpty(cl->copyRegion)) {
	  if(cl->copyDX!=dx || cl->copyDY!=dy) {
	     /* if a copyRegion was not yet executed, treat it as a
	      * modifiedRegion. The idea: in this case it could be
	      * source of the new copyRect or modified anyway. */
	     sraRgnOr(cl->modifiedRegion,cl->copyRegion);
	     sraRgnMakeEmpty(cl->copyRegion);
	  } else {
	     /* we have to set the intersection of the source of the copy
	      * and the old copy to modified. */
	     modifiedRegionBackup=sraRgnCreateRgn(copyRegion);
	     sraRgnOffset(modifiedRegionBackup,-dx,-dy);
	     sraRgnAnd(modifiedRegionBackup,cl->copyRegion);
	     sraRgnOr(cl->modifiedRegion,modifiedRegionBackup);
	     sraRgnDestroy(modifiedRegionBackup);
	  }
       }
	  
       sraRgnOr(cl->copyRegion,copyRegion);
       cl->copyDX = dx;
       cl->copyDY = dy;

       /* if there were modified regions, which are now copied,
	* mark them as modified, because the source of these can be overlapped
	* either by new modified or now copied regions. */
       modifiedRegionBackup=sraRgnCreateRgn(cl->modifiedRegion);
       sraRgnOffset(modifiedRegionBackup,dx,dy);
       sraRgnAnd(modifiedRegionBackup,cl->copyRegion);
       sraRgnOr(cl->modifiedRegion,modifiedRegionBackup);
       sraRgnDestroy(modifiedRegionBackup);

       if(!cl->enableCursorShapeUpdates) {
          /*
           * n.b. (dx, dy) is the vector pointing in the direction the
           * copyrect displacement will take place.  copyRegion is the
           * destination rectangle (say), not the source rectangle.
           */
          sraRegionPtr cursorRegion;
          int x = cl->cursorX - cl->screen->cursor->xhot;
          int y = cl->cursorY - cl->screen->cursor->yhot;
          int w = cl->screen->cursor->width;
          int h = cl->screen->cursor->height;

          cursorRegion = sraRgnCreateRect(x, y, x + w, y + h);
          sraRgnAnd(cursorRegion, cl->copyRegion);
          if(!sraRgnEmpty(cursorRegion)) {
             /*
              * current cursor rect overlaps with the copy region *dest*,
              * mark it as modified since we won't copy-rect stuff to it.
              */
             sraRgnOr(cl->modifiedRegion, cursorRegion);
          }
          sraRgnDestroy(cursorRegion);

          cursorRegion = sraRgnCreateRect(x, y, x + w, y + h);
          /* displace it to check for overlap with copy region source: */
          sraRgnOffset(cursorRegion, dx, dy);
          sraRgnAnd(cursorRegion, cl->copyRegion);
          if(!sraRgnEmpty(cursorRegion)) {
             /*
              * current cursor rect overlaps with the copy region *source*,
              * mark the *displaced* cursorRegion as modified since we
              * won't copyrect stuff to it.
              */
             sraRgnOr(cl->modifiedRegion, cursorRegion);
          }
          sraRgnDestroy(cursorRegion);
       }

     } else {
       sraRgnOr(cl->modifiedRegion,copyRegion);
     }
     TSIGNAL(cl->updateCond);
     UNLOCK(cl->updateMutex);
   }

   rfbReleaseClientIterator(iterator);
}

void rfbDoCopyRegion(rfbScreenInfoPtr screen,sraRegionPtr copyRegion,int dx,int dy)
{
   sraRectangleIterator* i;
   sraRect rect;
   int j,widthInBytes,bpp=screen->serverFormat.bitsPerPixel/8,
    rowstride=screen->paddedWidthInBytes;
   char *in,*out;

   /* copy it, really */
   i = sraRgnGetReverseIterator(copyRegion,dx<0,dy<0);
   while(sraRgnIteratorNext(i,&rect)) {
     widthInBytes = (rect.x2-rect.x1)*bpp;
     out = screen->frameBuffer+rect.x1*bpp+rect.y1*rowstride;
     in = screen->frameBuffer+(rect.x1-dx)*bpp+(rect.y1-dy)*rowstride;
     if(dy<0)
       for(j=rect.y1;j<rect.y2;j++,out+=rowstride,in+=rowstride)
	 memmove(out,in,widthInBytes);
     else {
       out += rowstride*(rect.y2-rect.y1-1);
       in += rowstride*(rect.y2-rect.y1-1);
       for(j=rect.y2-1;j>=rect.y1;j--,out-=rowstride,in-=rowstride)
	 memmove(out,in,widthInBytes);
     }
   }
   sraRgnReleaseIterator(i);
  
   rfbScheduleCopyRegion(screen,copyRegion,dx,dy);
}

void rfbDoCopyRect(rfbScreenInfoPtr screen,int x1,int y1,int x2,int y2,int dx,int dy)
{
  sraRegionPtr region = sraRgnCreateRect(x1,y1,x2,y2);
  rfbDoCopyRegion(screen,region,dx,dy);
  sraRgnDestroy(region);
}

void rfbScheduleCopyRect(rfbScreenInfoPtr screen,int x1,int y1,int x2,int y2,int dx,int dy)
{
  sraRegionPtr region = sraRgnCreateRect(x1,y1,x2,y2);
  rfbScheduleCopyRegion(screen,region,dx,dy);
  sraRgnDestroy(region);
}

void rfbMarkRegionAsModified(rfbScreenInfoPtr screen,sraRegionPtr modRegion)
{
   rfbClientIteratorPtr iterator;
   rfbClientPtr cl;

   iterator=rfbGetClientIterator(screen);
   while((cl=rfbClientIteratorNext(iterator))) {
     LOCK(cl->updateMutex);
     sraRgnOr(cl->modifiedRegion,modRegion);
     TSIGNAL(cl->updateCond);
     UNLOCK(cl->updateMutex);
   }

   rfbReleaseClientIterator(iterator);
}

void rfbScaledScreenUpdate(rfbScreenInfoPtr screen, int x1, int y1, int x2, int y2);
void rfbMarkRectAsModified(rfbScreenInfoPtr screen,int x1,int y1,int x2,int y2)
{
   sraRegionPtr region;
   int i;

   if(x1>x2) { i=x1; x1=x2; x2=i; }
   if(x1<0) x1=0;
   if(x2>screen->width) x2=screen->width;
   if(x1==x2) return;
   
   if(y1>y2) { i=y1; y1=y2; y2=i; }
   if(y1<0) y1=0;
   if(y2>screen->height) y2=screen->height;
   if(y1==y2) return;

   /* update scaled copies for this rectangle */
   rfbScaledScreenUpdate(screen,x1,y1,x2,y2);

   region = sraRgnCreateRect(x1,y1,x2,y2);
   rfbMarkRegionAsModified(screen,region);
   sraRgnDestroy(region);
}

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
#include <unistd.h>

static void *
clientOutput(void *data)
{
    rfbClientPtr cl = (rfbClientPtr)data;
    rfbBool haveUpdate;
    sraRegion* updateRegion;

    while (1) {
        haveUpdate = false;
        while (!haveUpdate) {
            if (cl->sock == -1) {
                /* Client has disconnected. */
                return NULL;
            }
	    LOCK(cl->updateMutex);
	    haveUpdate = FB_UPDATE_PENDING(cl);
	    if(!haveUpdate) {
		updateRegion = sraRgnCreateRgn(cl->modifiedRegion);
		haveUpdate = sraRgnAnd(updateRegion,cl->requestedRegion);
		sraRgnDestroy(updateRegion);
	    }

            if (!haveUpdate) {
                WAIT(cl->updateCond, cl->updateMutex);
            }
	    UNLOCK(cl->updateMutex);
        }
        
        /* OK, now, to save bandwidth, wait a little while for more
           updates to come along. */
        usleep(cl->screen->deferUpdateTime * 1000);

        /* Now, get the region we're going to update, and remove
           it from cl->modifiedRegion _before_ we send the update.
           That way, if anything that overlaps the region we're sending
           is updated, we'll be sure to do another update later. */
        LOCK(cl->updateMutex);
	updateRegion = sraRgnCreateRgn(cl->modifiedRegion);
        UNLOCK(cl->updateMutex);

        /* Now actually send the update. */
	rfbIncrClientRef(cl);
        rfbSendFramebufferUpdate(cl, updateRegion);
	rfbDecrClientRef(cl);

	sraRgnDestroy(updateRegion);
    }

    /* Not reached. */
    return NULL;
}

static void *
clientInput(void *data)
{
    rfbClientPtr cl = (rfbClientPtr)data;
    pthread_t output_thread;
    pthread_create(&output_thread, NULL, clientOutput, (void *)cl);

    while (1) {
	fd_set rfds, wfds, efds;
	struct timeval tv;
	int n;

	FD_ZERO(&rfds);
	FD_SET(cl->sock, &rfds);
	FD_ZERO(&efds);
	FD_SET(cl->sock, &efds);

	/* Are we transferring a file in the background? */
	FD_ZERO(&wfds);
	if ((cl->fileTransfer.fd!=-1) && (cl->fileTransfer.sending==1))
	    FD_SET(cl->sock, &wfds);

	tv.tv_sec = 60; /* 1 minute */
	tv.tv_usec = 0;
	n = select(cl->sock + 1, &rfds, &wfds, &efds, &tv);
	if (n < 0) {
	    rfbLogPerror("ReadExact: select");
	    break;
	}
	if (n == 0) /* timeout */
	{
            rfbSendFileTransferChunk(cl);
	    continue;
        }
        
        /* We have some space on the transmit queue, send some data */
        if (FD_ISSET(cl->sock, &wfds))
            rfbSendFileTransferChunk(cl);

        if (FD_ISSET(cl->sock, &rfds) || FD_ISSET(cl->sock, &efds))
            rfbProcessClientMessage(cl);

        if (cl->sock == -1) {
            /* Client has disconnected. */
            break;
        }
    }

    /* Get rid of the output thread. */
    LOCK(cl->updateMutex);
    TSIGNAL(cl->updateCond);
    UNLOCK(cl->updateMutex);
    IF_PTHREADS(pthread_join(output_thread, NULL));

    rfbClientConnectionGone(cl);

    return NULL;
}

static void*
listenerRun(void *data)
{
    rfbScreenInfoPtr screen=(rfbScreenInfoPtr)data;
    int client_fd;
    struct sockaddr_in peer;
    rfbClientPtr cl;
    socklen_t len;

    len = sizeof(peer);

    /* TODO: this thread wont die by restarting the server */
    /* TODO: HTTP is not handled */
    while ((client_fd = accept(screen->listenSock, 
                               (struct sockaddr*)&peer, &len)) >= 0) {
        cl = rfbNewClient(screen,client_fd);
        len = sizeof(peer);

	if (cl && !cl->onHold )
		rfbStartOnHoldClient(cl);
    }
    return(NULL);
}

void 
rfbStartOnHoldClient(rfbClientPtr cl)
{
    pthread_create(&cl->client_thread, NULL, clientInput, (void *)cl);
}

#else

void 
rfbStartOnHoldClient(rfbClientPtr cl)
{
	cl->onHold = FALSE;
}

#endif

void 
rfbRefuseOnHoldClient(rfbClientPtr cl)
{
    rfbCloseClient(cl);
    rfbClientConnectionGone(cl);
}

static void
rfbDefaultKbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientPtr cl)
{
}

void
rfbDefaultPtrAddEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
  rfbClientIteratorPtr iterator;
  rfbClientPtr other_client;
  rfbScreenInfoPtr s = cl->screen;

  if (x != s->cursorX || y != s->cursorY) {
    LOCK(s->cursorMutex);
    s->cursorX = x;
    s->cursorY = y;
    UNLOCK(s->cursorMutex);

    /* The cursor was moved by this client, so don't send CursorPos. */
    if (cl->enableCursorPosUpdates)
      cl->cursorWasMoved = FALSE;

    /* But inform all remaining clients about this cursor movement. */
    iterator = rfbGetClientIterator(s);
    while ((other_client = rfbClientIteratorNext(iterator)) != NULL) {
      if (other_client != cl && other_client->enableCursorPosUpdates) {
	other_client->cursorWasMoved = TRUE;
      }
    }
    rfbReleaseClientIterator(iterator);
  }
}

static void rfbDefaultSetXCutText(char* text, int len, rfbClientPtr cl)
{
}

/* TODO: add a nice VNC or RFB cursor */

#if defined(WIN32) || defined(sparc) || !defined(NO_STRICT_ANSI)
static rfbCursor myCursor = 
{
   FALSE, FALSE, FALSE, FALSE,
   (unsigned char*)"\000\102\044\030\044\102\000",
   (unsigned char*)"\347\347\176\074\176\347\347",
   8, 7, 3, 3,
   0, 0, 0,
   0xffff, 0xffff, 0xffff,
   NULL
};
#else
static rfbCursor myCursor = 
{
   cleanup: FALSE,
   cleanupSource: FALSE,
   cleanupMask: FALSE,
   cleanupRichSource: FALSE,
   source: "\000\102\044\030\044\102\000",
   mask:   "\347\347\176\074\176\347\347",
   width: 8, height: 7, xhot: 3, yhot: 3,
   foreRed: 0, foreGreen: 0, foreBlue: 0,
   backRed: 0xffff, backGreen: 0xffff, backBlue: 0xffff,
   richSource: NULL
};
#endif

static rfbCursorPtr rfbDefaultGetCursorPtr(rfbClientPtr cl)
{
   return(cl->screen->cursor);
}

/* response is cl->authChallenge vncEncrypted with passwd */
static rfbBool rfbDefaultPasswordCheck(rfbClientPtr cl,const char* response,int len)
{
  int i;
  char *passwd=rfbDecryptPasswdFromFile(cl->screen->authPasswdData);

  if(!passwd) {
    rfbErr("Couldn't read password file: %s\n",cl->screen->authPasswdData);
    return(FALSE);
  }

  rfbEncryptBytes(cl->authChallenge, passwd);

  /* Lose the password from memory */
  for (i = strlen(passwd); i >= 0; i--) {
    passwd[i] = '\0';
  }

  free(passwd);

  if (memcmp(cl->authChallenge, response, len) != 0) {
    rfbErr("authProcessClientMessage: authentication failed from %s\n",
	   cl->host);
    return(FALSE);
  }

  return(TRUE);
}

/* for this method, authPasswdData is really a pointer to an array
   of char*'s, where the last pointer is 0. */
rfbBool rfbCheckPasswordByList(rfbClientPtr cl,const char* response,int len)
{
  char **passwds;
  int i=0;

  for(passwds=(char**)cl->screen->authPasswdData;*passwds;passwds++,i++) {
    uint8_t auth_tmp[CHALLENGESIZE];
    memcpy((char *)auth_tmp, (char *)cl->authChallenge, CHALLENGESIZE);
    rfbEncryptBytes(auth_tmp, *passwds);

    if (memcmp(auth_tmp, response, len) == 0) {
      if(i>=cl->screen->authPasswdFirstViewOnly)
	cl->viewOnly=TRUE;
      return(TRUE);
    }
  }

  rfbErr("authProcessClientMessage: authentication failed from %s\n",
	 cl->host);
  return(FALSE);
}

void rfbDoNothingWithClient(rfbClientPtr cl)
{
}

static enum rfbNewClientAction rfbDefaultNewClientHook(rfbClientPtr cl)
{
	return RFB_CLIENT_ACCEPT;
}

/*
 * Update server's pixel format in screenInfo structure. This
 * function is called from rfbGetScreen() and rfbNewFramebuffer().
 */

static void rfbInitServerFormat(rfbScreenInfoPtr screen, int bitsPerSample)
{
   rfbPixelFormat* format=&screen->serverFormat;

   format->bitsPerPixel = screen->bitsPerPixel;
   format->depth = screen->depth;
   format->bigEndian = rfbEndianTest?FALSE:TRUE;
   format->trueColour = TRUE;
   screen->colourMap.count = 0;
   screen->colourMap.is16 = 0;
   screen->colourMap.data.bytes = NULL;

   if (format->bitsPerPixel == 8) {
     format->redMax = 7;
     format->greenMax = 7;
     format->blueMax = 3;
     format->redShift = 0;
     format->greenShift = 3;
     format->blueShift = 6;
   } else {
     format->redMax = (1 << bitsPerSample) - 1;
     format->greenMax = (1 << bitsPerSample) - 1;
     format->blueMax = (1 << bitsPerSample) - 1;
     if(rfbEndianTest) {
       format->redShift = 0;
       format->greenShift = bitsPerSample;
       format->blueShift = bitsPerSample * 2;
     } else {
       if(format->bitsPerPixel==8*3) {
	 format->redShift = bitsPerSample*2;
	 format->greenShift = bitsPerSample*1;
	 format->blueShift = 0;
       } else {
	 format->redShift = bitsPerSample*3;
	 format->greenShift = bitsPerSample*2;
	 format->blueShift = bitsPerSample;
       }
     }
   }
}

rfbScreenInfoPtr rfbGetScreen(int* argc,char** argv,
 int width,int height,int bitsPerSample,int samplesPerPixel,
 int bytesPerPixel)
{
   rfbScreenInfoPtr screen=calloc(sizeof(rfbScreenInfo),1);

   if (! logMutex_initialized) {
     INIT_MUTEX(logMutex);
     logMutex_initialized = 1;
   }


   if(width&3)
     rfbErr("WARNING: Width (%d) is not a multiple of 4. VncViewer has problems with that.\n",width);

   screen->autoPort=FALSE;
   screen->clientHead=NULL;
   screen->pointerClient=NULL;
   screen->port=5900;
   screen->socketState=RFB_SOCKET_INIT;

   screen->inetdInitDone = FALSE;
   screen->inetdSock=-1;

   screen->udpSock=-1;
   screen->udpSockConnected=FALSE;
   screen->udpPort=0;
   screen->udpClient=NULL;

   screen->maxFd=0;
   screen->listenSock=-1;

   screen->httpInitDone=FALSE;
   screen->httpEnableProxyConnect=FALSE;
   screen->httpPort=0;
   screen->httpDir=NULL;
   screen->httpListenSock=-1;
   screen->httpSock=-1;

   screen->desktopName = "LibVNCServer";
   screen->alwaysShared = FALSE;
   screen->neverShared = FALSE;
   screen->dontDisconnect = FALSE;
   screen->authPasswdData = NULL;
   screen->authPasswdFirstViewOnly = 1;
   
   screen->width = width;
   screen->height = height;
   screen->bitsPerPixel = screen->depth = 8*bytesPerPixel;

   screen->passwordCheck = rfbDefaultPasswordCheck;

   screen->ignoreSIGPIPE = TRUE;

   /* disable progressive updating per default */
   screen->progressiveSliceHeight = 0;

   screen->listenInterface = htonl(INADDR_ANY);

   screen->deferUpdateTime=5;
   screen->maxRectsPerUpdate=50;

   screen->handleEventsEagerly = FALSE;

   screen->protocolMajorVersion = rfbProtocolMajorVersion;
   screen->protocolMinorVersion = rfbProtocolMinorVersion;

   screen->permitFileTransfer = FALSE;

   if(!rfbProcessArguments(screen,argc,argv)) {
     free(screen);
     return NULL;
   }

#ifdef WIN32
   {
	   DWORD dummy=255;
	   GetComputerName(screen->thisHost,&dummy);
   }
#else
   gethostname(screen->thisHost, 255);
#endif

   screen->paddedWidthInBytes = width*bytesPerPixel;

   /* format */

   rfbInitServerFormat(screen, bitsPerSample);

   /* cursor */

   screen->cursorX=screen->cursorY=screen->underCursorBufferLen=0;
   screen->underCursorBuffer=NULL;
   screen->dontConvertRichCursorToXCursor = FALSE;
   screen->cursor = &myCursor;
   INIT_MUTEX(screen->cursorMutex);

   IF_PTHREADS(screen->backgroundLoop = FALSE);

   /* proc's and hook's */

   screen->kbdAddEvent = rfbDefaultKbdAddEvent;
   screen->kbdReleaseAllKeys = rfbDoNothingWithClient;
   screen->ptrAddEvent = rfbDefaultPtrAddEvent;
   screen->setXCutText = rfbDefaultSetXCutText;
   screen->getCursorPtr = rfbDefaultGetCursorPtr;
   screen->setTranslateFunction = rfbSetTranslateFunction;
   screen->newClientHook = rfbDefaultNewClientHook;
   screen->displayHook = NULL;
   screen->getKeyboardLedStateHook = NULL;

   /* initialize client list and iterator mutex */
   rfbClientListInit(screen);

   return(screen);
}

/*
 * Switch to another framebuffer (maybe of different size and color
 * format). Clients supporting NewFBSize pseudo-encoding will change
 * their local framebuffer dimensions if necessary.
 * NOTE: Rich cursor data should be converted to new pixel format by
 * the caller.
 */

void rfbNewFramebuffer(rfbScreenInfoPtr screen, char *framebuffer,
                       int width, int height,
                       int bitsPerSample, int samplesPerPixel,
                       int bytesPerPixel)
{
  rfbPixelFormat old_format;
  rfbBool format_changed = FALSE;
  rfbClientIteratorPtr iterator;
  rfbClientPtr cl;

  /* Update information in the screenInfo structure */

  old_format = screen->serverFormat;

  if (width & 3)
    rfbErr("WARNING: New width (%d) is not a multiple of 4.\n", width);

  screen->width = width;
  screen->height = height;
  screen->bitsPerPixel = screen->depth = 8*bytesPerPixel;
  screen->paddedWidthInBytes = width*bytesPerPixel;

  rfbInitServerFormat(screen, bitsPerSample);

  if (memcmp(&screen->serverFormat, &old_format,
             sizeof(rfbPixelFormat)) != 0) {
    format_changed = TRUE;
  }

  screen->frameBuffer = framebuffer;

  /* Adjust pointer position if necessary */

  if (screen->cursorX >= width)
    screen->cursorX = width - 1;
  if (screen->cursorY >= height)
    screen->cursorY = height - 1;

  /* For each client: */
  iterator = rfbGetClientIterator(screen);
  while ((cl = rfbClientIteratorNext(iterator)) != NULL) {

    /* Re-install color translation tables if necessary */

    if (format_changed)
      screen->setTranslateFunction(cl);

    /* Mark the screen contents as changed, and schedule sending
       NewFBSize message if supported by this client. */

    LOCK(cl->updateMutex);
    sraRgnDestroy(cl->modifiedRegion);
    cl->modifiedRegion = sraRgnCreateRect(0, 0, width, height);
    sraRgnMakeEmpty(cl->copyRegion);
    cl->copyDX = 0;
    cl->copyDY = 0;

    if (cl->useNewFBSize)
      cl->newFBSizePending = TRUE;

    TSIGNAL(cl->updateCond);
    UNLOCK(cl->updateMutex);
  }
  rfbReleaseClientIterator(iterator);
}

/* hang up on all clients and free all reserved memory */

void rfbScreenCleanup(rfbScreenInfoPtr screen)
{
  rfbClientIteratorPtr i=rfbGetClientIterator(screen);
  rfbClientPtr cl,cl1=rfbClientIteratorNext(i);
  while(cl1) {
    cl=rfbClientIteratorNext(i);
    rfbClientConnectionGone(cl1);
    cl1=cl;
  }
  rfbReleaseClientIterator(i);
    
#define FREE_IF(x) if(screen->x) free(screen->x)
  FREE_IF(colourMap.data.bytes);
  FREE_IF(underCursorBuffer);
  TINI_MUTEX(screen->cursorMutex);
  if(screen->cursor && screen->cursor->cleanup)
    rfbFreeCursor(screen->cursor);

  rfbRRECleanup(screen);
  rfbCoRRECleanup(screen);
  rfbUltraCleanup(screen);
#ifdef LIBVNCSERVER_HAVE_LIBZ
  rfbZlibCleanup(screen);
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
  rfbTightCleanup(screen);
#endif

  /* free all 'scaled' versions of this screen */
  while (screen->scaledScreenNext!=NULL)
  {
      rfbScreenInfoPtr ptr;
      ptr = screen->scaledScreenNext;
      screen->scaledScreenNext = ptr->scaledScreenNext;
      free(ptr->frameBuffer);
      free(ptr);
  }

#endif
  free(screen);
}

void rfbInitServer(rfbScreenInfoPtr screen)
{
#ifdef WIN32
  WSADATA trash;
  WSAStartup(MAKEWORD(2,2),&trash);
#endif
  rfbInitSockets(screen);
  rfbHttpInitSockets(screen);
#ifndef __MINGW32__
  if(screen->ignoreSIGPIPE)
    signal(SIGPIPE,SIG_IGN);
#endif
}

void rfbShutdownServer(rfbScreenInfoPtr screen,rfbBool disconnectClients) {
  if(disconnectClients) {
    rfbClientPtr cl;
    rfbClientIteratorPtr iter = rfbGetClientIterator(screen);
    while( (cl = rfbClientIteratorNext(iter)) )
      if (cl->sock > -1)
	/* we don't care about maxfd here, because the server goes away */
	rfbCloseClient(cl);
    rfbReleaseClientIterator(iter);
  }

  rfbShutdownSockets(screen);
  rfbHttpShutdownSockets(screen);
}

#ifndef LIBVNCSERVER_HAVE_GETTIMEOFDAY
#include <fcntl.h>
#include <conio.h>
#include <sys/timeb.h>

void gettimeofday(struct timeval* tv,char* dummy)
{
   SYSTEMTIME t;
   GetSystemTime(&t);
   tv->tv_sec=t.wHour*3600+t.wMinute*60+t.wSecond;
   tv->tv_usec=t.wMilliseconds*1000;
}
#endif

rfbBool
rfbProcessEvents(rfbScreenInfoPtr screen,long usec)
{
  rfbClientIteratorPtr i;
  rfbClientPtr cl,clPrev;
  struct timeval tv;
  rfbBool result=FALSE;
  extern rfbClientIteratorPtr
    rfbGetClientIteratorWithClosed(rfbScreenInfoPtr rfbScreen);

  if(usec<0)
    usec=screen->deferUpdateTime*1000;

  rfbCheckFds(screen,usec);
  rfbHttpCheckFds(screen);
#ifdef CORBA
  corbaCheckFds(screen);
#endif

  i = rfbGetClientIteratorWithClosed(screen);
  cl=rfbClientIteratorHead(i);
  while(cl) {
    if (cl->sock >= 0 && !cl->onHold && FB_UPDATE_PENDING(cl) &&
        !sraRgnEmpty(cl->requestedRegion)) {
      result=TRUE;
      if(screen->deferUpdateTime == 0) {
	  rfbSendFramebufferUpdate(cl,cl->modifiedRegion);
      } else if(cl->startDeferring.tv_usec == 0) {
	gettimeofday(&cl->startDeferring,NULL);
	if(cl->startDeferring.tv_usec == 0)
	  cl->startDeferring.tv_usec++;
      } else {
	gettimeofday(&tv,NULL);
	if(tv.tv_sec < cl->startDeferring.tv_sec /* at midnight */
	   || ((tv.tv_sec-cl->startDeferring.tv_sec)*1000
	       +(tv.tv_usec-cl->startDeferring.tv_usec)/1000)
	     > screen->deferUpdateTime) {
	  cl->startDeferring.tv_usec = 0;
	  rfbSendFramebufferUpdate(cl,cl->modifiedRegion);
	}
      }
    }

    if (!cl->viewOnly && cl->lastPtrX >= 0) {
      if(cl->startPtrDeferring.tv_usec == 0) {
        gettimeofday(&cl->startPtrDeferring,NULL);
        if(cl->startPtrDeferring.tv_usec == 0)
          cl->startPtrDeferring.tv_usec++;
      } else {
        struct timeval tv;
        gettimeofday(&tv,NULL);
        if(tv.tv_sec < cl->startPtrDeferring.tv_sec /* at midnight */
           || ((tv.tv_sec-cl->startPtrDeferring.tv_sec)*1000
           +(tv.tv_usec-cl->startPtrDeferring.tv_usec)/1000)
           > cl->screen->deferPtrUpdateTime) {
          cl->startPtrDeferring.tv_usec = 0;
          cl->screen->ptrAddEvent(cl->lastPtrButtons, 
                                  cl->lastPtrX, 
                                  cl->lastPtrY, cl);
	  cl->lastPtrX = -1;
        }
      }
    }
    clPrev=cl;
    cl=rfbClientIteratorNext(i);
    if(clPrev->sock==-1) {
      rfbClientConnectionGone(clPrev);
      result=TRUE;
    }
  }
  rfbReleaseClientIterator(i);

  return result;
}

rfbBool rfbIsActive(rfbScreenInfoPtr screenInfo) {
  return screenInfo->socketState!=RFB_SOCKET_SHUTDOWN || screenInfo->clientHead!=NULL;
}

void rfbRunEventLoop(rfbScreenInfoPtr screen, long usec, rfbBool runInBackground)
{
  if(runInBackground) {
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
       pthread_t listener_thread;

       screen->backgroundLoop = TRUE;

       pthread_create(&listener_thread, NULL, listenerRun, screen);
    return;
#else
    rfbErr("Can't run in background, because I don't have PThreads!\n");
    return;
#endif
  }

  if(usec<0)
    usec=screen->deferUpdateTime*1000;

  while(rfbIsActive(screen))
    rfbProcessEvents(screen,usec);
}
///////////////////////////////////////////end main.c //////////////////////////////////////////////////


///////////////////////////////////////////begin rfbserver.c //////////////////////////////////////////////////
/*
 * rfbserver.c - deal with server-side of the RFB protocol.
 */

/*
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  Copyright (C) 2002 RealVNC Ltd.
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#endif
#include <string.h>
#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#include "private.h"

#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef WIN32
#define write(sock,buf,len) send(sock,buf,len,0)
#else
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>
#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#endif

#ifdef CORBA
#include <vncserverctrl.h>
#endif

#ifdef DEBUGPROTO
#undef DEBUGPROTO
#define DEBUGPROTO(x) x
#else
#define DEBUGPROTO(x)
#endif
#include <stdarg.h>
#include <scale.h>
/* stst() */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
/* readdir() */
#include <dirent.h>
/* errno */
#include <errno.h>
/* strftime() */
#include <time.h>

#ifdef __MINGW32__
static int compat_mkdir(const char *path, int mode)
{
	return mkdir(path);
}
#define mkdir compat_mkdir
#endif

static void rfbProcessClientProtocolVersion(rfbClientPtr cl);
static void rfbProcessClientNormalMessage(rfbClientPtr cl);
static void rfbProcessClientInitMessage(rfbClientPtr cl);

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
void rfbIncrClientRef(rfbClientPtr cl)
{
  LOCK(cl->refCountMutex);
  cl->refCount++;
  UNLOCK(cl->refCountMutex);
}

void rfbDecrClientRef(rfbClientPtr cl)
{
  LOCK(cl->refCountMutex);
  cl->refCount--;
  if(cl->refCount<=0) /* just to be sure also < 0 */
    TSIGNAL(cl->deleteCond);
  UNLOCK(cl->refCountMutex);
}
#else
void rfbIncrClientRef(rfbClientPtr cl) {}
void rfbDecrClientRef(rfbClientPtr cl) {}
#endif

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
static MUTEX(rfbClientListMutex);
#endif

struct rfbClientIterator {
  rfbClientPtr next;
  rfbScreenInfoPtr screen;
  rfbBool closedToo;
};

void
rfbClientListInit(rfbScreenInfoPtr rfbScreen)
{
    if(sizeof(rfbBool)!=1) {
        /* a sanity check */
        fprintf(stderr,"rfbBool's size is not 1 (%d)!\n",(int)sizeof(rfbBool));
	/* we cannot continue, because rfbBool is supposed to be char everywhere */
	exit(1);
    }
    rfbScreen->clientHead = NULL;
    INIT_MUTEX(rfbClientListMutex);
}

rfbClientIteratorPtr
rfbGetClientIterator(rfbScreenInfoPtr rfbScreen)
{
  rfbClientIteratorPtr i =
    (rfbClientIteratorPtr)malloc(sizeof(struct rfbClientIterator));
  i->next = NULL;
  i->screen = rfbScreen;
  i->closedToo = FALSE;
  return i;
}

rfbClientIteratorPtr
rfbGetClientIteratorWithClosed(rfbScreenInfoPtr rfbScreen)
{
  rfbClientIteratorPtr i =
    (rfbClientIteratorPtr)malloc(sizeof(struct rfbClientIterator));
  i->next = NULL;
  i->screen = rfbScreen;
  i->closedToo = TRUE;
  return i;
}

rfbClientPtr
rfbClientIteratorHead(rfbClientIteratorPtr i)
{
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
  if(i->next != 0) {
    rfbDecrClientRef(i->next);
    rfbIncrClientRef(i->screen->clientHead);
  }
#endif
  LOCK(rfbClientListMutex);
  i->next = i->screen->clientHead;
  UNLOCK(rfbClientListMutex);
  return i->next;
}

rfbClientPtr
rfbClientIteratorNext(rfbClientIteratorPtr i)
{
  if(i->next == 0) {
    LOCK(rfbClientListMutex);
    i->next = i->screen->clientHead;
    UNLOCK(rfbClientListMutex);
  } else {
    IF_PTHREADS(rfbClientPtr cl = i->next);
    i->next = i->next->next;
    IF_PTHREADS(rfbDecrClientRef(cl));
  }

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
    if(!i->closedToo)
      while(i->next && i->next->sock<0)
        i->next = i->next->next;
    if(i->next)
      rfbIncrClientRef(i->next);
#endif

    return i->next;
}

void
rfbReleaseClientIterator(rfbClientIteratorPtr iterator)
{
  IF_PTHREADS(if(iterator->next) rfbDecrClientRef(iterator->next));
  free(iterator);
}


/*
 * rfbNewClientConnection is called from sockets.c when a new connection
 * comes in.
 */

void
rfbNewClientConnection(rfbScreenInfoPtr rfbScreen,
                       int sock)
{
    rfbClientPtr cl;

    cl = rfbNewClient(rfbScreen,sock);
#ifdef CORBA
    if(cl!=NULL)
      newConnection(cl, (KEYBOARD_DEVICE|POINTER_DEVICE),1,1,1);
#endif
}


/*
 * rfbReverseConnection is called by the CORBA stuff to make an outward
 * connection to a "listening" RFB client.
 */

rfbClientPtr
rfbReverseConnection(rfbScreenInfoPtr rfbScreen,
                     char *host,
                     int port)
{
    int sock;
    rfbClientPtr cl;

    if ((sock = rfbConnect(rfbScreen, host, port)) < 0)
        return (rfbClientPtr)NULL;

    cl = rfbNewClient(rfbScreen, sock);

    if (cl) {
        cl->reverseConnection = TRUE;
    }

    return cl;
}


void
rfbSetProtocolVersion(rfbScreenInfoPtr rfbScreen, int major_, int minor_)
{
    /* Permit the server to set the version to report */
    /* TODO: sanity checking */
    if ((major_==3) && (minor_ > 2 && minor_ < 9))
    {
      rfbScreen->protocolMajorVersion = major_;
      rfbScreen->protocolMinorVersion = minor_;
    }
    else
        rfbLog("rfbSetProtocolVersion(%d,%d) set to invalid values\n", major_, minor_);
}

/*
 * rfbNewClient is called when a new connection has been made by whatever
 * means.
 */

static rfbClientPtr
rfbNewTCPOrUDPClient(rfbScreenInfoPtr rfbScreen,
                     int sock,
                     rfbBool isUDP)
{
    rfbProtocolVersionMsg pv;
    rfbClientIteratorPtr iterator;
    rfbClientPtr cl,cl_;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    rfbProtocolExtension* extension;

    cl = (rfbClientPtr)calloc(sizeof(rfbClientRec),1);

    cl->screen = rfbScreen;
    cl->sock = sock;
    cl->viewOnly = FALSE;
    /* setup pseudo scaling */
    cl->scaledScreen = rfbScreen;
    cl->scaledScreen->scaledScreenRefCount++;

    rfbResetStats(cl);

    cl->clientData = NULL;
    cl->clientGoneHook = rfbDoNothingWithClient;

    if(isUDP) {
      rfbLog(" accepted UDP client\n");
    } else {
      int one=1;

      getpeername(sock, (struct sockaddr *)&addr, &addrlen);
      cl->host = strdup(inet_ntoa(addr.sin_addr));

      rfbLog("  other clients:\n");
      iterator = rfbGetClientIterator(rfbScreen);
      while ((cl_ = rfbClientIteratorNext(iterator)) != NULL) {
        rfbLog("     %s\n",cl_->host);
      }
      rfbReleaseClientIterator(iterator);

#ifndef WIN32
      if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	rfbLogPerror("fcntl failed");
	close(sock);
	return NULL;
      }
#endif

      if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		     (char *)&one, sizeof(one)) < 0) {
	rfbLogPerror("setsockopt failed");
	close(sock);
	return NULL;
      }

      FD_SET(sock,&(rfbScreen->allFds));
		rfbScreen->maxFd = max(sock,rfbScreen->maxFd);

      INIT_MUTEX(cl->outputMutex);
      INIT_MUTEX(cl->refCountMutex);
      INIT_COND(cl->deleteCond);

      cl->state = RFB_PROTOCOL_VERSION;

      cl->reverseConnection = FALSE;
      cl->readyForSetColourMapEntries = FALSE;
      cl->useCopyRect = FALSE;
      cl->preferredEncoding = -1;
      cl->correMaxWidth = 48;
      cl->correMaxHeight = 48;
#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->zrleData = NULL;
#endif

      cl->copyRegion = sraRgnCreate();
      cl->copyDX = 0;
      cl->copyDY = 0;
   
      cl->modifiedRegion =
	sraRgnCreateRect(0,0,rfbScreen->width,rfbScreen->height);

      INIT_MUTEX(cl->updateMutex);
      INIT_COND(cl->updateCond);

      cl->requestedRegion = sraRgnCreate();

      cl->format = cl->screen->serverFormat;
      cl->translateFn = rfbTranslateNone;
      cl->translateLookupTable = NULL;

      LOCK(rfbClientListMutex);

      IF_PTHREADS(cl->refCount = 0);
      cl->next = rfbScreen->clientHead;
      cl->prev = NULL;
      if (rfbScreen->clientHead)
        rfbScreen->clientHead->prev = cl;

      rfbScreen->clientHead = cl;
      UNLOCK(rfbClientListMutex);

#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->tightQualityLevel = -1;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
      cl->tightCompressLevel = TIGHT_DEFAULT_COMPRESSION;
      {
	int i;
	for (i = 0; i < 4; i++)
          cl->zsActive[i] = FALSE;
      }
#endif
#endif

      cl->fileTransfer.fd = -1;

      cl->enableCursorShapeUpdates = FALSE;
      cl->enableCursorPosUpdates = FALSE;
      cl->useRichCursorEncoding = FALSE;
      cl->enableLastRectEncoding = FALSE;
      cl->enableKeyboardLedState = FALSE;
      cl->enableSupportedMessages = FALSE;
      cl->enableSupportedEncodings = FALSE;
      cl->enableServerIdentity = FALSE;
      cl->lastKeyboardLedState = -1;
      cl->cursorX = rfbScreen->cursorX;
      cl->cursorY = rfbScreen->cursorY;
      cl->useNewFBSize = FALSE;

#ifdef LIBVNCSERVER_HAVE_LIBZ
      cl->compStreamInited = FALSE;
      cl->compStream.total_in = 0;
      cl->compStream.total_out = 0;
      cl->compStream.zalloc = Z_NULL;
      cl->compStream.zfree = Z_NULL;
      cl->compStream.opaque = Z_NULL;

      cl->zlibCompressLevel = 5;
#endif

      cl->progressiveSliceY = 0;

      cl->extensions = NULL;

      cl->lastPtrX = -1;

      sprintf(pv,rfbProtocolVersionFormat,rfbScreen->protocolMajorVersion, 
              rfbScreen->protocolMinorVersion);

      if (rfbWriteExact(cl, pv, sz_rfbProtocolVersionMsg) < 0) {
        rfbLogPerror("rfbNewClient: write");
        rfbCloseClient(cl);
	rfbClientConnectionGone(cl);
        return NULL;
      }
    }

    for(extension = rfbGetExtensionIterator(); extension;
	    extension=extension->next) {
	void* data = NULL;
	/* if the extension does not have a newClient method, it wants
	 * to be initialized later. */
	if(extension->newClient && extension->newClient(cl, &data))
		rfbEnableExtension(cl, extension, data);
    }
    rfbReleaseExtensionIterator();

    switch (cl->screen->newClientHook(cl)) {
    case RFB_CLIENT_ON_HOLD:
	    cl->onHold = TRUE;
	    break;
    case RFB_CLIENT_ACCEPT:
	    cl->onHold = FALSE;
	    break;
    case RFB_CLIENT_REFUSE:
	    rfbCloseClient(cl);
	    rfbClientConnectionGone(cl);
	    cl = NULL;
	    break;
    }
    return cl;
}

rfbClientPtr
rfbNewClient(rfbScreenInfoPtr rfbScreen,
             int sock)
{
  return(rfbNewTCPOrUDPClient(rfbScreen,sock,FALSE));
}

rfbClientPtr
rfbNewUDPClient(rfbScreenInfoPtr rfbScreen)
{
  return((rfbScreen->udpClient=
	  rfbNewTCPOrUDPClient(rfbScreen,rfbScreen->udpSock,TRUE)));
}

/*
 * rfbClientConnectionGone is called from sockets.c just after a connection
 * has gone away.
 */

void
rfbClientConnectionGone(rfbClientPtr cl)
{
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
    int i;
#endif

    LOCK(rfbClientListMutex);

    if (cl->prev)
        cl->prev->next = cl->next;
    else
        cl->screen->clientHead = cl->next;
    if (cl->next)
        cl->next->prev = cl->prev;

    if(cl->sock>0)
	close(cl->sock);

    if (cl->scaledScreen!=NULL)
        cl->scaledScreen->scaledScreenRefCount--;

#ifdef LIBVNCSERVER_HAVE_LIBZ
    rfbFreeZrleData(cl);
#endif

    rfbFreeUltraData(cl);

#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
    if(cl->screen->backgroundLoop != FALSE) {
      int i;
      do {
	LOCK(cl->refCountMutex);
	i=cl->refCount;
	if(i>0)
	  WAIT(cl->deleteCond,cl->refCountMutex);
	UNLOCK(cl->refCountMutex);
      } while(i>0);
    }
#endif

    UNLOCK(rfbClientListMutex);

    if(cl->sock>=0)
       FD_CLR(cl->sock,&(cl->screen->allFds));

    cl->clientGoneHook(cl);

    rfbLog("Client %s gone\n",cl->host);
    free(cl->host);

#ifdef LIBVNCSERVER_HAVE_LIBZ
    /* Release the compression state structures if any. */
    if ( cl->compStreamInited ) {
	deflateEnd( &(cl->compStream) );
    }

#ifdef LIBVNCSERVER_HAVE_LIBJPEG
    for (i = 0; i < 4; i++) {
	if (cl->zsActive[i])
	    deflateEnd(&cl->zsStruct[i]);
    }
#endif
#endif

    if (cl->screen->pointerClient == cl)
        cl->screen->pointerClient = NULL;

    sraRgnDestroy(cl->modifiedRegion);
    sraRgnDestroy(cl->requestedRegion);
    sraRgnDestroy(cl->copyRegion);

    if (cl->translateLookupTable) free(cl->translateLookupTable);

    TINI_COND(cl->updateCond);
    TINI_MUTEX(cl->updateMutex);

    /* make sure outputMutex is unlocked before destroying */
    LOCK(cl->outputMutex);
    UNLOCK(cl->outputMutex);
    TINI_MUTEX(cl->outputMutex);

#ifdef CORBA
    destroyConnection(cl);
#endif

    rfbPrintStats(cl);

    free(cl);
}


/*
 * rfbProcessClientMessage is called when there is data to read from a client.
 */

void
rfbProcessClientMessage(rfbClientPtr cl)
{
    switch (cl->state) {
    case RFB_PROTOCOL_VERSION:
        rfbProcessClientProtocolVersion(cl);
        return;
    case RFB_SECURITY_TYPE:
        rfbProcessClientSecurityType(cl);
        return;
    case RFB_AUTHENTICATION:
        rfbAuthProcessClientMessage(cl);
        return;
    case RFB_INITIALISATION:
        rfbProcessClientInitMessage(cl);
        return;
    default:
        rfbProcessClientNormalMessage(cl);
        return;
    }
}


/*
 * rfbProcessClientProtocolVersion is called when the client sends its
 * protocol version.
 */

static void
rfbProcessClientProtocolVersion(rfbClientPtr cl)
{
    rfbProtocolVersionMsg pv;
    int n, major_, minor_;

    if ((n = rfbReadExact(cl, pv, sz_rfbProtocolVersionMsg)) <= 0) {
        if (n == 0)
            rfbLog("rfbProcessClientProtocolVersion: client gone\n");
        else
            rfbLogPerror("rfbProcessClientProtocolVersion: read");
        rfbCloseClient(cl);
        return;
    }

    pv[sz_rfbProtocolVersionMsg] = 0;
    if (sscanf(pv,rfbProtocolVersionFormat,&major_,&minor_) != 2) {
        char name[1024]; 
	if(sscanf(pv,"RFB %03d.%03d %1023s\n",&major_,&minor_,name) != 3) {
	    rfbErr("rfbProcessClientProtocolVersion: not a valid RFB client: %s\n", pv);
	    rfbCloseClient(cl);
	    return;
	}
	free(cl->host);
	cl->host=strdup(name);
    }
    rfbLog("Client Protocol Version %d.%d\n", major_, minor_);

    if (major_ != rfbProtocolMajorVersion) {
        rfbErr("RFB protocol version mismatch - server %d.%d, client %d.%d",
                cl->screen->protocolMajorVersion, cl->screen->protocolMinorVersion,
                major_,minor_);
        rfbCloseClient(cl);
        return;
    }

    /* Check for the minor version use either of the two standard version of RFB */
    /*
     * UltraVNC Viewer detects FileTransfer compatible servers via rfb versions
     * 3.4, 3.6, 3.14, 3.16
     * It's a bad method, but it is what they use to enable features...
     * maintaining RFB version compatibility across multiple servers is a pain
     * Should use something like ServerIdentity encoding
     */
    cl->protocolMajorVersion = major_;
    cl->protocolMinorVersion = minor_;
    
    rfbLog("Protocol version sent %d.%d, using %d.%d\n",
              major_, minor_, rfbProtocolMajorVersion, cl->protocolMinorVersion);

    rfbAuthNewClient(cl);
}


void
rfbClientSendString(rfbClientPtr cl, char *reason)
{
    char *buf;
    int len = strlen(reason);

    rfbLog("rfbClientSendString(\"%s\")\n", reason);

    buf = (char *)malloc(4 + len);
    ((uint32_t *)buf)[0] = Swap32IfLE(len);
    memcpy(buf + 4, reason, len);

    if (rfbWriteExact(cl, buf, 4 + len) < 0)
        rfbLogPerror("rfbClientSendString: write");
    free(buf);

    rfbCloseClient(cl);
}

/*
 * rfbClientConnFailed is called when a client connection has failed either
 * because it talks the wrong protocol or it has failed authentication.
 */

void
rfbClientConnFailed(rfbClientPtr cl,
                    char *reason)
{
    char *buf;
    int len = strlen(reason);

    rfbLog("rfbClientConnFailed(\"%s\")\n", reason);

    buf = (char *)malloc(8 + len);
    ((uint32_t *)buf)[0] = Swap32IfLE(rfbConnFailed);
    ((uint32_t *)buf)[1] = Swap32IfLE(len);
    memcpy(buf + 8, reason, len);

    if (rfbWriteExact(cl, buf, 8 + len) < 0)
        rfbLogPerror("rfbClientConnFailed: write");
    free(buf);

    rfbCloseClient(cl);
}


/*
 * rfbProcessClientInitMessage is called when the client sends its
 * initialisation message.
 */

static void
rfbProcessClientInitMessage(rfbClientPtr cl)
{
    rfbClientInitMsg ci;
    union {
        char buf[256];
        rfbServerInitMsg si;
    } u;
    int len, n;
    rfbClientIteratorPtr iterator;
    rfbClientPtr otherCl;
    rfbExtensionData* extension;

    if ((n = rfbReadExact(cl, (char *)&ci,sz_rfbClientInitMsg)) <= 0) {
        if (n == 0)
            rfbLog("rfbProcessClientInitMessage: client gone\n");
        else
            rfbLogPerror("rfbProcessClientInitMessage: read");
        rfbCloseClient(cl);
        return;
    }

    memset(u.buf,0,sizeof(u.buf));

    u.si.framebufferWidth = Swap16IfLE(cl->screen->width);
    u.si.framebufferHeight = Swap16IfLE(cl->screen->height);
    u.si.format = cl->screen->serverFormat;
    u.si.format.redMax = Swap16IfLE(u.si.format.redMax);
    u.si.format.greenMax = Swap16IfLE(u.si.format.greenMax);
    u.si.format.blueMax = Swap16IfLE(u.si.format.blueMax);

    strncpy(u.buf + sz_rfbServerInitMsg, cl->screen->desktopName, 127);
    len = strlen(u.buf + sz_rfbServerInitMsg);
    u.si.nameLength = Swap32IfLE(len);

    if (rfbWriteExact(cl, u.buf, sz_rfbServerInitMsg + len) < 0) {
        rfbLogPerror("rfbProcessClientInitMessage: write");
        rfbCloseClient(cl);
        return;
    }

    for(extension = cl->extensions; extension;) {
	rfbExtensionData* next = extension->next;
	if(extension->extension->init &&
		!extension->extension->init(cl, extension->data))
	    /* extension requested that it be removed */
	    rfbDisableExtension(cl, extension->extension);
	extension = next;
    }

    cl->state = RFB_NORMAL;

    if (!cl->reverseConnection &&
                        (cl->screen->neverShared || (!cl->screen->alwaysShared && !ci.shared))) {

        if (cl->screen->dontDisconnect) {
            iterator = rfbGetClientIterator(cl->screen);
            while ((otherCl = rfbClientIteratorNext(iterator)) != NULL) {
                if ((otherCl != cl) && (otherCl->state == RFB_NORMAL)) {
                    rfbLog("-dontdisconnect: Not shared & existing client\n");
                    rfbLog("  refusing new client %s\n", cl->host);
                    rfbCloseClient(cl);
                    rfbReleaseClientIterator(iterator);
                    return;
                }
            }
            rfbReleaseClientIterator(iterator);
        } else {
            iterator = rfbGetClientIterator(cl->screen);
            while ((otherCl = rfbClientIteratorNext(iterator)) != NULL) {
                if ((otherCl != cl) && (otherCl->state == RFB_NORMAL)) {
                    rfbLog("Not shared - closing connection to client %s\n",
                           otherCl->host);
                    rfbCloseClient(otherCl);
                }
            }
            rfbReleaseClientIterator(iterator);
        }
    }
}

/* The values come in based on the scaled screen, we need to convert them to
 * values based on the man screen's coordinate system
 */
static rfbBool rectSwapIfLEAndClip(uint16_t* x,uint16_t* y,uint16_t* w,uint16_t* h,
		rfbClientPtr cl)
{
	int x1=Swap16IfLE(*x);
	int y1=Swap16IfLE(*y);
	int w1=Swap16IfLE(*w);
	int h1=Swap16IfLE(*h);

	rfbScaledCorrection(cl->scaledScreen, cl->screen, &x1, &y1, &w1, &h1, "rectSwapIfLEAndClip");
	*x = x1;
	*y = y1;
	*w = w1;
	*h = h1;

	if(*w>cl->screen->width-*x)
		*w=cl->screen->width-*x;
	/* possible underflow */
	if(*w>cl->screen->width-*x)
		return FALSE;
	if(*h>cl->screen->height-*y)
		*h=cl->screen->height-*y;
	if(*h>cl->screen->height-*y)
		return FALSE;

	return TRUE;
}

/*
 * Send keyboard state (PointerPos pseudo-encoding).
 */

rfbBool
rfbSendKeyboardLedState(rfbClientPtr cl)
{
    rfbFramebufferUpdateRectHeader rect;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingKeyboardLedState);
    rect.r.x = Swap16IfLE(cl->lastKeyboardLedState);
    rect.r.y = 0;
    rect.r.w = 0;
    rect.r.h = 0;

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
        sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, rfbEncodingKeyboardLedState, sz_rfbFramebufferUpdateRectHeader, sz_rfbFramebufferUpdateRectHeader);

    if (!rfbSendUpdateBuf(cl))
        return FALSE;

    return TRUE;
}


#define rfbSetBit(buffer, position)  (buffer[(position & 255) / 8] |= (1 << (position % 8)))

/*
 * Send rfbEncodingSupportedMessages.
 */

rfbBool
rfbSendSupportedMessages(rfbClientPtr cl)
{
    rfbFramebufferUpdateRectHeader rect;
    rfbSupportedMessages msgs;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader
                  + sz_rfbSupportedMessages > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingSupportedMessages);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = Swap16IfLE(sz_rfbSupportedMessages);
    rect.r.h = 0;

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
        sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    memset((char *)&msgs, 0, sz_rfbSupportedMessages);
    rfbSetBit(msgs.client2server, rfbSetPixelFormat);
    rfbSetBit(msgs.client2server, rfbFixColourMapEntries);
    rfbSetBit(msgs.client2server, rfbSetEncodings);
    rfbSetBit(msgs.client2server, rfbFramebufferUpdateRequest);
    rfbSetBit(msgs.client2server, rfbKeyEvent);
    rfbSetBit(msgs.client2server, rfbPointerEvent);
    rfbSetBit(msgs.client2server, rfbClientCutText);
    rfbSetBit(msgs.client2server, rfbFileTransfer);
    rfbSetBit(msgs.client2server, rfbSetScale);
    /*rfbSetBit(msgs.client2server, rfbSetServerInput);  */
    /*rfbSetBit(msgs.client2server, rfbSetSW);           */
    /*rfbSetBit(msgs.client2server, rfbTextChat);        */
    /*rfbSetBit(msgs.client2server, rfbKeyFrameRequest); */
    rfbSetBit(msgs.client2server, rfbPalmVNCSetScaleFactor);

    rfbSetBit(msgs.server2client, rfbFramebufferUpdate);
    rfbSetBit(msgs.server2client, rfbSetColourMapEntries);
    rfbSetBit(msgs.server2client, rfbBell);
    rfbSetBit(msgs.server2client, rfbServerCutText);
    rfbSetBit(msgs.server2client, rfbResizeFrameBuffer);
    /*rfbSetBit(msgs.server2client, rfbKeyFrameUpdate);  */
    rfbSetBit(msgs.server2client, rfbPalmVNCReSizeFrameBuffer);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&msgs, sz_rfbSupportedMessages);
    cl->ublen += sz_rfbSupportedMessages;

    rfbStatRecordEncodingSent(cl, rfbEncodingSupportedMessages,
        sz_rfbFramebufferUpdateRectHeader+sz_rfbSupportedMessages,
        sz_rfbFramebufferUpdateRectHeader+sz_rfbSupportedMessages);
    if (!rfbSendUpdateBuf(cl))
        return FALSE;

    return TRUE;
}



/*
 * Send rfbEncodingSupportedEncodings.
 */

rfbBool
rfbSendSupportedEncodings(rfbClientPtr cl)
{
    rfbFramebufferUpdateRectHeader rect;
    static uint32_t supported[] = {
        rfbEncodingRaw,
	rfbEncodingCopyRect,
	rfbEncodingRRE,
	rfbEncodingCoRRE,
	rfbEncodingHextile,
#ifdef LIBVNCSERVER_HAVE_LIBZ
	rfbEncodingZlib,
	rfbEncodingZRLE,
	rfbEncodingZYWRLE,
#endif
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
	rfbEncodingTight,
#endif
	rfbEncodingUltra,
	rfbEncodingUltraZip,
	rfbEncodingXCursor,
	rfbEncodingRichCursor,
	rfbEncodingPointerPos,
	rfbEncodingLastRect,
	rfbEncodingNewFBSize,
	rfbEncodingKeyboardLedState,
	rfbEncodingSupportedMessages,
	rfbEncodingSupportedEncodings,
	rfbEncodingServerIdentity,
    };
    uint32_t nEncodings = sizeof(supported) / sizeof(supported[0]), i;

    /* think rfbSetEncodingsMsg */

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader
                  + (nEncodings * sizeof(uint32_t)) > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingSupportedEncodings);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = Swap16IfLE(nEncodings * sizeof(uint32_t));
    rect.r.h = Swap16IfLE(nEncodings);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
        sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    for (i = 0; i < nEncodings; i++) {
        uint32_t encoding = Swap32IfLE(supported[i]);
	memcpy(&cl->updateBuf[cl->ublen], (char *)&encoding, sizeof(encoding));
	cl->ublen += sizeof(encoding);
    }

    rfbStatRecordEncodingSent(cl, rfbEncodingSupportedEncodings,
        sz_rfbFramebufferUpdateRectHeader+(nEncodings * sizeof(uint32_t)),
        sz_rfbFramebufferUpdateRectHeader+(nEncodings * sizeof(uint32_t)));

    if (!rfbSendUpdateBuf(cl))
        return FALSE;

    return TRUE;
}


void
rfbSetServerVersionIdentity(rfbScreenInfoPtr screen, char *fmt, ...)
{
    char buffer[256];
    va_list ap;
    
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer)-1, fmt, ap);
    va_end(ap);
    
    if (screen->versionString!=NULL) free(screen->versionString);
    screen->versionString = strdup(buffer);
}

/*
 * Send rfbEncodingServerIdentity.
 */

rfbBool
rfbSendServerIdentity(rfbClientPtr cl)
{
    rfbFramebufferUpdateRectHeader rect;
    char buffer[512];

    /* tack on our library version */
    snprintf(buffer,sizeof(buffer)-1, "%s (%s)", 
        (cl->screen->versionString==NULL ? "unknown" : cl->screen->versionString),
        LIBVNCSERVER_PACKAGE_STRING);

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader
                  + (strlen(buffer)+1) > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingServerIdentity);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = Swap16IfLE(strlen(buffer)+1);
    rect.r.h = 0;

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
        sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    memcpy(&cl->updateBuf[cl->ublen], buffer, strlen(buffer)+1);
    cl->ublen += strlen(buffer)+1;

    rfbStatRecordEncodingSent(cl, rfbEncodingServerIdentity,
        sz_rfbFramebufferUpdateRectHeader+strlen(buffer)+1,
        sz_rfbFramebufferUpdateRectHeader+strlen(buffer)+1);
    

    if (!rfbSendUpdateBuf(cl))
        return FALSE;

    return TRUE;
}

rfbBool rfbSendTextChatMessage(rfbClientPtr cl, uint32_t length, char *buffer)
{
    rfbTextChatMsg tc;
    int bytesToSend=0;

    memset((char *)&tc, 0, sizeof(tc)); 
    tc.type = rfbTextChat;
    tc.length = Swap32IfLE(length);
    
    switch(length) {
    case rfbTextChatOpen:
    case rfbTextChatClose:
    case rfbTextChatFinished:
        bytesToSend=0;
        break;
    default:
        bytesToSend=length;
        if (bytesToSend>rfbTextMaxSize)
            bytesToSend=rfbTextMaxSize;
    }

    if (cl->ublen + sz_rfbTextChatMsg + bytesToSend > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }
    
    memcpy(&cl->updateBuf[cl->ublen], (char *)&tc, sz_rfbTextChatMsg);
    cl->ublen += sz_rfbTextChatMsg;
    if (bytesToSend>0) {
        memcpy(&cl->updateBuf[cl->ublen], buffer, bytesToSend);
        cl->ublen += bytesToSend;    
    }
    rfbStatRecordMessageSent(cl, rfbTextChat, sz_rfbTextChatMsg+bytesToSend, sz_rfbTextChatMsg+bytesToSend);

    if (!rfbSendUpdateBuf(cl))
        return FALSE;
        
    return TRUE;
}

#define FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN(msg, cl, ret) \
	if ((cl->screen->getFileTransferPermission != NULL \
	    && cl->screen->getFileTransferPermission(cl) != TRUE) \
	    || cl->screen->permitFileTransfer != TRUE) { \
		rfbLog("%sUltra File Transfer is disabled, dropping client: %s\n", msg, cl->host); \
		rfbCloseClient(cl); \
		return ret; \
	}

int DB = 1;

rfbBool rfbSendFileTransferMessage(rfbClientPtr cl, uint8_t contentType, uint8_t contentParam, uint32_t size, uint32_t length, char *buffer)
{
    rfbFileTransferMsg ft;
    ft.type = rfbFileTransfer;
    ft.contentType = contentType;
    ft.contentParam = contentParam;
    ft.pad          = 0; /* UltraVNC did not Swap16LE(ft.contentParam) (Looks like it might be BigEndian) */
    ft.size         = Swap32IfLE(size);
    ft.length       = Swap32IfLE(length);
    
    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);
    /*
    rfbLog("rfbSendFileTransferMessage( %dtype, %dparam, %dsize, %dlen, %p)\n", contentType, contentParam, size, length, buffer);
    */
    if (rfbWriteExact(cl, (char *)&ft, sz_rfbFileTransferMsg) < 0) {
        rfbLogPerror("rfbSendFileTransferMessage: write");
        rfbCloseClient(cl);
        return FALSE;
    }

    if (length>0)
    {
        if (rfbWriteExact(cl, buffer, length) < 0) {
            rfbLogPerror("rfbSendFileTransferMessage: write");
            rfbCloseClient(cl);
            return FALSE;
        }
    }

    rfbStatRecordMessageSent(cl, rfbFileTransfer, sz_rfbFileTransferMsg+length, sz_rfbFileTransferMsg+length);

    return TRUE;
}


/*
 * UltraVNC uses Windows Structures
 */
#define MAX_PATH 260

typedef struct {
    uint32_t dwLowDateTime;
    uint32_t dwHighDateTime;
} RFB_FILETIME; 

typedef struct {
    uint32_t dwFileAttributes;
    RFB_FILETIME ftCreationTime;
    RFB_FILETIME ftLastAccessTime;
    RFB_FILETIME ftLastWriteTime;
    uint32_t nFileSizeHigh;
    uint32_t nFileSizeLow;
    uint32_t dwReserved0;
    uint32_t dwReserved1;
    uint8_t  cFileName[ MAX_PATH ];
    uint8_t  cAlternateFileName[ 14 ];
} RFB_FIND_DATA;

#define RFB_FILE_ATTRIBUTE_READONLY   0x1
#define RFB_FILE_ATTRIBUTE_HIDDEN     0x2
#define RFB_FILE_ATTRIBUTE_SYSTEM     0x4
#define RFB_FILE_ATTRIBUTE_DIRECTORY  0x10
#define RFB_FILE_ATTRIBUTE_ARCHIVE    0x20
#define RFB_FILE_ATTRIBUTE_NORMAL     0x80
#define RFB_FILE_ATTRIBUTE_TEMPORARY  0x100
#define RFB_FILE_ATTRIBUTE_COMPRESSED 0x800

rfbBool rfbFilenameTranslate2UNIX(rfbClientPtr cl, char *path, char *unixPath)
{
    int x;
    char *home=NULL;

    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);

    /* C: */
    if (path[0]=='C' && path[1]==':')
      strcpy(unixPath, &path[2]);
    else
    {
      home = getenv("HOME");
      if (home!=NULL)
      {
        strcpy(unixPath, home);
        strcat(unixPath,"/");
        strcat(unixPath, path);
      }
      else
        strcpy(unixPath, path);
    }
    for (x=0;x<strlen(unixPath);x++)
      if (unixPath[x]=='\\') unixPath[x]='/';
    return TRUE;
}

rfbBool rfbFilenameTranslate2DOS(rfbClientPtr cl, char *unixPath, char *path)
{
    int x;

    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);

    sprintf(path,"C:%s", unixPath);
    for (x=2;x<strlen(path);x++)
        if (path[x]=='/') path[x]='\\';
    return TRUE;
}

rfbBool rfbSendDirContent(rfbClientPtr cl, int length, char *buffer)
{
    char retfilename[MAX_PATH];
    char path[MAX_PATH];
    struct stat statbuf;
    RFB_FIND_DATA win32filename;
    int nOptLen = 0, retval=0;
    DIR *dirp=NULL;
    struct dirent *direntp=NULL;

    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);

    /* Client thinks we are Winblows */
    rfbFilenameTranslate2UNIX(cl, buffer, path);

    if (DB) rfbLog("rfbProcessFileTransfer() rfbDirContentRequest: rfbRDirContent: \"%s\"->\"%s\"\n",buffer, path);

    dirp=opendir(path);
    if (dirp==NULL)
        return rfbSendFileTransferMessage(cl, rfbDirPacket, rfbADirectory, 0, 0, NULL);
    /* send back the path name (necessary for links) */
    if (rfbSendFileTransferMessage(cl, rfbDirPacket, rfbADirectory, 0, length, buffer)==FALSE) return FALSE;
    for (direntp=readdir(dirp); direntp!=NULL; direntp=readdir(dirp))
    {
        /* get stats */
        snprintf(retfilename,sizeof(retfilename),"%s/%s", path, direntp->d_name);
        retval = stat(retfilename, &statbuf);

        if (retval==0)
        {
            memset((char *)&win32filename, 0, sizeof(win32filename));
            win32filename.dwFileAttributes = Swap32IfBE(RFB_FILE_ATTRIBUTE_NORMAL);
            if (S_ISDIR(statbuf.st_mode))
              win32filename.dwFileAttributes = Swap32IfBE(RFB_FILE_ATTRIBUTE_DIRECTORY);
            win32filename.ftCreationTime.dwLowDateTime = Swap32IfBE(statbuf.st_ctime);   /* Intel Order */
            win32filename.ftCreationTime.dwHighDateTime = 0;
            win32filename.ftLastAccessTime.dwLowDateTime = Swap32IfBE(statbuf.st_atime); /* Intel Order */
            win32filename.ftLastAccessTime.dwHighDateTime = 0;
            win32filename.ftLastWriteTime.dwLowDateTime = Swap32IfBE(statbuf.st_mtime);  /* Intel Order */
            win32filename.ftLastWriteTime.dwHighDateTime = 0;
            win32filename.nFileSizeLow = Swap32IfBE(statbuf.st_size); /* Intel Order */
            win32filename.nFileSizeHigh = 0;
            win32filename.dwReserved0 = 0;
            win32filename.dwReserved1 = 0;

            /* If this had the full path, we would need to translate to DOS format ("C:\") */
            /* rfbFilenameTranslate2DOS(cl, retfilename, win32filename.cFileName); */
            strcpy((char *)win32filename.cFileName, direntp->d_name);
            
            /* Do not show hidden files (but show how to move up the tree) */
            if ((strcmp(direntp->d_name, "..")==0) || (direntp->d_name[0]!='.'))
            {
                nOptLen = sizeof(RFB_FIND_DATA) - MAX_PATH - 14 + strlen((char *)win32filename.cFileName);
                /*
                rfbLog("rfbProcessFileTransfer() rfbDirContentRequest: rfbRDirContent: Sending \"%s\"\n", (char *)win32filename.cFileName);
                */
                if (rfbSendFileTransferMessage(cl, rfbDirPacket, rfbADirectory, 0, nOptLen, (char *)&win32filename)==FALSE) return FALSE;
            }
        }
    }
    closedir(dirp);
    /* End of the transfer */
    return rfbSendFileTransferMessage(cl, rfbDirPacket, 0, 0, 0, NULL);
}


char *rfbProcessFileTransferReadBuffer(rfbClientPtr cl, uint32_t length)
{
    char *buffer=NULL;
    int   n=0;

    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, NULL);
    /*
    rfbLog("rfbProcessFileTransferReadBuffer(%dlen)\n", length);
    */
    if (length>0) {
        buffer=malloc(length+1);
        if (buffer!=NULL) {
            if ((n = rfbReadExact(cl, (char *)buffer, length)) <= 0) {
                if (n != 0)
                    rfbLogPerror("rfbProcessFileTransferReadBuffer: read");
                rfbCloseClient(cl);
                /* NOTE: don't forget to free(buffer) if you return early! */
                if (buffer!=NULL) free(buffer);
                return NULL;
            }
            /* Null Terminate */
            buffer[length]=0;
        }
    }
    return buffer;
}


rfbBool rfbSendFileTransferChunk(rfbClientPtr cl)
{
    /* Allocate buffer for compression */
    unsigned char readBuf[sz_rfbBlockSize];
    int bytesRead=0;
    int retval=0;
    fd_set wfds;
    struct timeval tv;
    int n;
#ifdef LIBVNCSERVER_HAVE_LIBZ
    unsigned char compBuf[sz_rfbBlockSize + 1024];
    unsigned long nMaxCompSize = sizeof(compBuf);
    int nRetC = 0;
#endif

    /*
     * Don't close the client if we get into this one because 
     * it is called from many places to service file transfers.
     * Note that permitFileTransfer is checked first.
     */
    if (cl->screen->permitFileTransfer != TRUE ||
       (cl->screen->getFileTransferPermission != NULL
        && cl->screen->getFileTransferPermission(cl) != TRUE)) { 
		return TRUE;
    }

    /* If not sending, or no file open...   Return as if we sent something! */
    if ((cl->fileTransfer.fd!=-1) && (cl->fileTransfer.sending==1))
    {
	FD_ZERO(&wfds);
        FD_SET(cl->sock, &wfds);

        /* return immediately */
	tv.tv_sec = 0; 
	tv.tv_usec = 0;
	n = select(cl->sock + 1, NULL, &wfds, NULL, &tv);

	if (n<0) {
            rfbLog("rfbSendFileTransferChunk() select failed: %s\n", strerror(errno));
	}
        /* We have space on the transmit queue */
	if (n > 0)
	{
            bytesRead = read(cl->fileTransfer.fd, readBuf, sz_rfbBlockSize);
            switch (bytesRead) {
            case 0:
                /*
                rfbLog("rfbSendFileTransferChunk(): End-Of-File Encountered\n");
                */
                retval = rfbSendFileTransferMessage(cl, rfbEndOfFile, 0, 0, 0, NULL);
                close(cl->fileTransfer.fd);
                cl->fileTransfer.fd = -1;
                cl->fileTransfer.sending   = 0;
                cl->fileTransfer.receiving = 0;
                return retval;
            case -1:
                /* TODO : send an error msg to the client... */
                rfbLog("rfbSendFileTransferChunk(): %s\n",strerror(errno));
                retval = rfbSendFileTransferMessage(cl, rfbAbortFileTransfer, 0, 0, 0, NULL);
                close(cl->fileTransfer.fd);
                cl->fileTransfer.fd = -1;
                cl->fileTransfer.sending   = 0;
                cl->fileTransfer.receiving = 0;
                return retval;
            default:
                /*
                rfbLog("rfbSendFileTransferChunk(): Read %d bytes\n", bytesRead);
                */
                if (!cl->fileTransfer.compressionEnabled)
                    return  rfbSendFileTransferMessage(cl, rfbFilePacket, 0, 0, bytesRead, (char *)readBuf);
                else
                {
#ifdef LIBVNCSERVER_HAVE_LIBZ
                    nRetC = compress(compBuf, &nMaxCompSize, readBuf, bytesRead);
                    /*
                    rfbLog("Compressed the packet from %d -> %d bytes\n", nMaxCompSize, bytesRead);
                    */
                    
                    if ((nRetC==0) && (nMaxCompSize<bytesRead))
                        return  rfbSendFileTransferMessage(cl, rfbFilePacket, 0, 1, nMaxCompSize, (char *)compBuf);
                    else
                        return  rfbSendFileTransferMessage(cl, rfbFilePacket, 0, 0, bytesRead, (char *)readBuf);
#else
                    /* We do not support compression of the data stream */
                    return  rfbSendFileTransferMessage(cl, rfbFilePacket, 0, 0, bytesRead, (char *)readBuf);
#endif
                }
            }
        }
    }
    return TRUE;
}

rfbBool rfbProcessFileTransfer(rfbClientPtr cl, uint8_t contentType, uint8_t contentParam, uint32_t size, uint32_t length)
{
    char *buffer=NULL, *p=NULL;
    int retval=0;
    char filename1[MAX_PATH];
    char filename2[MAX_PATH];
    char szFileTime[MAX_PATH];
    struct stat statbuf;
    uint32_t sizeHtmp=0;
    int n=0;
    char timespec[64];
#ifdef LIBVNCSERVER_HAVE_LIBZ
    unsigned char compBuff[sz_rfbBlockSize];
    unsigned long nRawBytes = sz_rfbBlockSize;
    int nRet = 0;
#endif

    FILEXFER_ALLOWED_OR_CLOSE_AND_RETURN("", cl, FALSE);
        
    /*
    rfbLog("rfbProcessFileTransfer(%dtype, %dparam, %dsize, %dlen)\n", contentType, contentParam, size, length);
    */

    switch (contentType) {
    case rfbDirContentRequest:
        switch (contentParam) {
        case rfbRDrivesList: /* Client requests the List of Local Drives */
            /*
            rfbLog("rfbProcessFileTransfer() rfbDirContentRequest: rfbRDrivesList:\n");
            */
            /* Format when filled : "C:\<NULL>D:\<NULL>....Z:\<NULL><NULL>
             *
             * We replace the "\" char following the drive letter and ":"
             * with a char corresponding to the type of drive
             * We obtain something like "C:l<NULL>D:c<NULL>....Z:n\<NULL><NULL>"
             *  Isn't it ugly ?
             * DRIVE_FIXED = 'l'     (local?)
             * DRIVE_REMOVABLE = 'f' (floppy?)
             * DRIVE_CDROM = 'c'
             * DRIVE_REMOTE = 'n'
             */
            
            /* in unix, there are no 'drives'  (We could list mount points though)
             * We fake the root as a "C:" for the Winblows users
             */
            filename2[0]='C';
            filename2[1]=':';
            filename2[2]='l';
            filename2[3]=0;
            filename2[4]=0;
            retval = rfbSendFileTransferMessage(cl, rfbDirPacket, rfbADrivesList, 0, 5, filename2);
            if (buffer!=NULL) free(buffer);
            return retval;
            break;
        case rfbRDirContent: /* Client requests the content of a directory */
            /*
            rfbLog("rfbProcessFileTransfer() rfbDirContentRequest: rfbRDirContent\n");
            */
            if ((buffer = rfbProcessFileTransferReadBuffer(cl, length))==NULL) return FALSE;
            retval = rfbSendDirContent(cl, length, buffer);
            if (buffer!=NULL) free(buffer);
            return retval;
        }
        break;

    case rfbDirPacket:
        rfbLog("rfbProcessFileTransfer() rfbDirPacket\n");
        break;
    case rfbFileAcceptHeader:
        rfbLog("rfbProcessFileTransfer() rfbFileAcceptHeader\n");
        break;
    case rfbCommandReturn:
        rfbLog("rfbProcessFileTransfer() rfbCommandReturn\n");
        break;
    case rfbFileChecksums:
        /* Destination file already exists - the viewer sends the checksums */
        rfbLog("rfbProcessFileTransfer() rfbFileChecksums\n");
        break;
    case rfbFileTransferAccess:
        rfbLog("rfbProcessFileTransfer() rfbFileTransferAccess\n");
        break;

    /*
     * sending from the server to the viewer
     */

    case rfbFileTransferRequest:
        /*
        rfbLog("rfbProcessFileTransfer() rfbFileTransferRequest:\n");
        */
        /* add some space to the end of the buffer as we will be adding a timespec to it */
        if ((buffer = rfbProcessFileTransferReadBuffer(cl, length))==NULL) return FALSE;
        /* The client requests a File */
        rfbFilenameTranslate2UNIX(cl, buffer, filename1);
        cl->fileTransfer.fd=open(filename1, O_RDONLY, 0744);

        /*
        */
        if (DB) rfbLog("rfbProcessFileTransfer() rfbFileTransferRequest(\"%s\"->\"%s\") Open: %s fd=%d\n", buffer, filename1, (cl->fileTransfer.fd==-1?"Failed":"Success"), cl->fileTransfer.fd);
        
        if (cl->fileTransfer.fd!=-1) {
            if (fstat(cl->fileTransfer.fd, &statbuf)!=0) {
                close(cl->fileTransfer.fd);
                cl->fileTransfer.fd=-1;
            }
            else
            {
              /* Add the File Time Stamp to the filename */
              strftime(timespec, sizeof(timespec), "%m/%d/%Y %H:%M",gmtime(&statbuf.st_ctime));
              buffer=realloc(buffer, length + strlen(timespec) + 2); /* comma, and Null term */
              if (buffer==NULL) {
                  rfbLog("rfbProcessFileTransfer() rfbFileTransferRequest: Failed to malloc %d bytes\n", length + strlen(timespec) + 2);
                  return FALSE;
              }
              strcat(buffer,",");
              strcat(buffer, timespec);
              length = strlen(buffer);
              if (DB) rfbLog("rfbProcessFileTransfer() buffer is now: \"%s\"\n", buffer);
            }
        }

        /* The viewer supports compression if size==1 */
        cl->fileTransfer.compressionEnabled = (size==1);

        /*
        rfbLog("rfbProcessFileTransfer() rfbFileTransferRequest(\"%s\"->\"%s\")%s\n", buffer, filename1, (size==1?" <Compression Enabled>":""));
        */

        /* File Size in bytes, 0xFFFFFFFF (-1) means error */
        retval = rfbSendFileTransferMessage(cl, rfbFileHeader, 0, (cl->fileTransfer.fd==-1 ? -1 : statbuf.st_size), length, buffer);

        if (cl->fileTransfer.fd==-1)
        {
            if (buffer!=NULL) free(buffer);
            return retval;
        }
        /* setup filetransfer stuff */
        cl->fileTransfer.fileSize = statbuf.st_size;
        cl->fileTransfer.numPackets = statbuf.st_size / sz_rfbBlockSize;
        cl->fileTransfer.receiving = 0;
        cl->fileTransfer.sending = 0; /* set when we receive a rfbFileHeader: */

        /* TODO: finish 64-bit file size support */
        sizeHtmp = 0;        
        if (rfbWriteExact(cl, (char *)&sizeHtmp, 4) < 0) {
          rfbLogPerror("rfbProcessFileTransfer: write");
          rfbCloseClient(cl);
          if (buffer!=NULL) free(buffer);
          return FALSE;
        }
        break;

    case rfbFileHeader:
        /* Destination file (viewer side) is ready for reception (size > 0) or not (size = -1) */
        if (size==-1) {
            rfbLog("rfbProcessFileTransfer() rfbFileHeader (error, aborting)\n");
            close(cl->fileTransfer.fd);
            cl->fileTransfer.fd=-1;
            return TRUE;
        }

        /*
        rfbLog("rfbProcessFileTransfer() rfbFileHeader (%d bytes of a file)\n", size);
        */

        /* Starts the transfer! */
        cl->fileTransfer.sending=1;
        return rfbSendFileTransferChunk(cl);
        break;


    /*
     * sending from the viewer to the server
     */

    case rfbFileTransferOffer:
        /* client is sending a file to us */
        /* buffer contains full path name (plus FileTime) */
        /* size contains size of the file */
        /*
        rfbLog("rfbProcessFileTransfer() rfbFileTransferOffer:\n");
        */
        if ((buffer = rfbProcessFileTransferReadBuffer(cl, length))==NULL) return FALSE;

        /* Parse the FileTime */
        p = strrchr(buffer, ',');
        if (p!=NULL) {
            *p = '\0';
            strcpy(szFileTime, p+1);
        } else
            szFileTime[0]=0;



        /* Need to read in sizeHtmp */
        if ((n = rfbReadExact(cl, (char *)&sizeHtmp, 4)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessFileTransfer: read sizeHtmp");
            rfbCloseClient(cl);
            /* NOTE: don't forget to free(buffer) if you return early! */
            if (buffer!=NULL) free(buffer);
            return FALSE;
        }
        sizeHtmp = Swap32IfLE(sizeHtmp);
        
        rfbFilenameTranslate2UNIX(cl, buffer, filename1);

        /* If the file exists... We can send a rfbFileChecksums back to the client before we send an rfbFileAcceptHeader */
        /* TODO: Delta Transfer */

        cl->fileTransfer.fd=open(filename1, O_CREAT|O_WRONLY|O_TRUNC, 0744);
        if (DB) rfbLog("rfbProcessFileTransfer() rfbFileTransferOffer(\"%s\"->\"%s\") %s %s fd=%d\n", buffer, filename1, (cl->fileTransfer.fd==-1?"Failed":"Success"), (cl->fileTransfer.fd==-1?strerror(errno):""), cl->fileTransfer.fd);
        /*
        */
        
        /* File Size in bytes, 0xFFFFFFFF (-1) means error */
        retval = rfbSendFileTransferMessage(cl, rfbFileAcceptHeader, 0, (cl->fileTransfer.fd==-1 ? -1 : 0), length, buffer);
        if (cl->fileTransfer.fd==-1) {
            free(buffer);
            return retval;
        }
        
        /* setup filetransfer stuff */
        cl->fileTransfer.fileSize = size;
        cl->fileTransfer.numPackets = size / sz_rfbBlockSize;
        cl->fileTransfer.receiving = 1;
        cl->fileTransfer.sending = 0;
        break;

    case rfbFilePacket:
        /*
        rfbLog("rfbProcessFileTransfer() rfbFilePacket:\n");
        */
        if ((buffer = rfbProcessFileTransferReadBuffer(cl, length))==NULL) return FALSE;
        if (cl->fileTransfer.fd!=-1) {
            /* buffer contains the contents of the file */
            if (size==0)
                retval=write(cl->fileTransfer.fd, buffer, length);
            else
            {
#ifdef LIBVNCSERVER_HAVE_LIBZ
                /* compressed packet */
                nRet = uncompress(compBuff,&nRawBytes,(const unsigned char*)buffer, length);
                retval=write(cl->fileTransfer.fd, compBuff, nRawBytes);
#else
                /* Write the file out as received... */
                retval=write(cl->fileTransfer.fd, buffer, length);
#endif
            }
            if (retval==-1)
            {
                close(cl->fileTransfer.fd);
                cl->fileTransfer.fd=-1;
                cl->fileTransfer.sending   = 0;
                cl->fileTransfer.receiving = 0;
            }
        }
        break;

    case rfbEndOfFile:
        if (DB) rfbLog("rfbProcessFileTransfer() rfbEndOfFile\n");
        /*
        */
        if (cl->fileTransfer.fd!=-1)
            close(cl->fileTransfer.fd);
        cl->fileTransfer.fd=-1;
        cl->fileTransfer.sending   = 0;
        cl->fileTransfer.receiving = 0;
        break;

    case rfbAbortFileTransfer:
        if (DB) rfbLog("rfbProcessFileTransfer() rfbAbortFileTransfer\n");
        /*
        */
        if (cl->fileTransfer.fd!=-1)
        {
            close(cl->fileTransfer.fd);
            cl->fileTransfer.fd=-1;
            cl->fileTransfer.sending   = 0;
            cl->fileTransfer.receiving = 0;
        }
        else
        {
            /* We use this message for FileTransfer rights (<=RC18 versions)
             * The client asks for FileTransfer permission
             */
            if (contentParam == 0)
            {
                rfbLog("rfbProcessFileTransfer() File Transfer Permission DENIED! (Client Version <=RC18)\n");
                /* Old method for FileTransfer handshake perimssion (<=RC18) (Deny it)*/
                return rfbSendFileTransferMessage(cl, rfbAbortFileTransfer, 0, -1, 0, "");
            }
            /* New method is allowed */
            if (cl->screen->getFileTransferPermission!=NULL)
            {
                if (cl->screen->getFileTransferPermission(cl)==TRUE)
                {
                    rfbLog("rfbProcessFileTransfer() File Transfer Permission Granted!\n");
                    return rfbSendFileTransferMessage(cl, rfbFileTransferAccess, 0, 1 , 0, ""); /* Permit */
                }
                else
                {
                    rfbLog("rfbProcessFileTransfer() File Transfer Permission DENIED!\n");
                    return rfbSendFileTransferMessage(cl, rfbFileTransferAccess, 0, -1 , 0, ""); /* Deny */
                }
            }
            else
            {
                if (cl->screen->permitFileTransfer)
                {
                    rfbLog("rfbProcessFileTransfer() File Transfer Permission Granted!\n");
                    return rfbSendFileTransferMessage(cl, rfbFileTransferAccess, 0, 1 , 0, ""); /* Permit */
                }
                else
                {
                    rfbLog("rfbProcessFileTransfer() File Transfer Permission DENIED by default!\n");
                    return rfbSendFileTransferMessage(cl, rfbFileTransferAccess, 0, -1 , 0, ""); /* DEFAULT: DENY (for security) */
                }
                
            }
        }
        break;


    case rfbCommand:
        /*
        rfbLog("rfbProcessFileTransfer() rfbCommand:\n");
        */
        if ((buffer = rfbProcessFileTransferReadBuffer(cl, length))==NULL) return FALSE;
        switch (contentParam) {
        case rfbCDirCreate:  /* Client requests the creation of a directory */
            rfbFilenameTranslate2UNIX(cl, buffer, filename1);
            retval = mkdir(filename1, 0755);
            if (DB) rfbLog("rfbProcessFileTransfer() rfbCommand: rfbCDirCreate(\"%s\"->\"%s\") %s\n", buffer, filename1, (retval==-1?"Failed":"Success"));
            /*
            */
            retval = rfbSendFileTransferMessage(cl, rfbCommandReturn, rfbADirCreate, retval, length, buffer);
            if (buffer!=NULL) free(buffer);
            return retval;
        case rfbCFileDelete: /* Client requests the deletion of a file */
            rfbFilenameTranslate2UNIX(cl, buffer, filename1);
            if (stat(filename1,&statbuf)==0)
            {
                if (S_ISDIR(statbuf.st_mode))
                    retval = rmdir(filename1);
                else
                    retval = unlink(filename1);
            }
            else retval=-1;
            retval = rfbSendFileTransferMessage(cl, rfbCommandReturn, rfbAFileDelete, retval, length, buffer);
            if (buffer!=NULL) free(buffer);
            return retval;
        case rfbCFileRename: /* Client requests the Renaming of a file/directory */
            p = strrchr(buffer, '*');
            if (p != NULL)
            {
                /* Split into 2 filenames ('*' is a seperator) */
                *p = '\0';
                rfbFilenameTranslate2UNIX(cl, buffer, filename1);
                rfbFilenameTranslate2UNIX(cl, p+1,    filename2);
                retval = rename(filename1,filename2);
                if (DB) rfbLog("rfbProcessFileTransfer() rfbCommand: rfbCFileRename(\"%s\"->\"%s\" -->> \"%s\"->\"%s\") %s\n", buffer, filename1, p+1, filename2, (retval==-1?"Failed":"Success"));
                /*
                */
                /* Restore the buffer so the reply is good */
                *p = '*';
                retval = rfbSendFileTransferMessage(cl, rfbCommandReturn, rfbAFileRename, retval, length, buffer);
                if (buffer!=NULL) free(buffer);
                return retval;
            }
            break;
        }
    
        break;
    }

    /* NOTE: don't forget to free(buffer) if you return early! */
    if (buffer!=NULL) free(buffer);
    return TRUE;
}

/*
 * rfbProcessClientNormalMessage is called when the client has sent a normal
 * protocol message.
 */

static void
rfbProcessClientNormalMessage(rfbClientPtr cl)
{
    int n=0;
    rfbClientToServerMsg msg;
    char *str;
    int i;
    uint32_t enc=0;
    uint32_t lastPreferredEncoding = -1;
    char encBuf[64];
    char encBuf2[64];

    if ((n = rfbReadExact(cl, (char *)&msg, 1)) <= 0) {
        if (n != 0)
            rfbLogPerror("rfbProcessClientNormalMessage: read");
        rfbCloseClient(cl);
        return;
    }

    switch (msg.type) {

    case rfbSetPixelFormat:

        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                           sz_rfbSetPixelFormatMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }

        cl->format.bitsPerPixel = msg.spf.format.bitsPerPixel;
        cl->format.depth = msg.spf.format.depth;
        cl->format.bigEndian = (msg.spf.format.bigEndian ? TRUE : FALSE);
        cl->format.trueColour = (msg.spf.format.trueColour ? TRUE : FALSE);
        cl->format.redMax = Swap16IfLE(msg.spf.format.redMax);
        cl->format.greenMax = Swap16IfLE(msg.spf.format.greenMax);
        cl->format.blueMax = Swap16IfLE(msg.spf.format.blueMax);
        cl->format.redShift = msg.spf.format.redShift;
        cl->format.greenShift = msg.spf.format.greenShift;
        cl->format.blueShift = msg.spf.format.blueShift;

	cl->readyForSetColourMapEntries = TRUE;
        cl->screen->setTranslateFunction(cl);

        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetPixelFormatMsg, sz_rfbSetPixelFormatMsg);

        return;


    case rfbFixColourMapEntries:
        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                           sz_rfbFixColourMapEntriesMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }
        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetPixelFormatMsg, sz_rfbSetPixelFormatMsg);
        rfbLog("rfbProcessClientNormalMessage: %s",
                "FixColourMapEntries unsupported\n");
        rfbCloseClient(cl);
        return;


    /* NOTE: Some clients send us a set of encodings (ie: PointerPos) designed to enable/disable features...
     * We may want to look into this...
     * Example:
     *     case rfbEncodingXCursor:
     *         cl->enableCursorShapeUpdates = TRUE;
     *
     * Currently: cl->enableCursorShapeUpdates can *never* be turned off...
     */
    case rfbSetEncodings:
    {

        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                           sz_rfbSetEncodingsMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }

        msg.se.nEncodings = Swap16IfLE(msg.se.nEncodings);

        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetEncodingsMsg+(msg.se.nEncodings*4),sz_rfbSetEncodingsMsg+(msg.se.nEncodings*4));

        /*
         * UltraVNC Client has the ability to adapt to changing network environments
         * So, let's give it a change to tell us what it wants now!
         */
        if (cl->preferredEncoding!=-1)
            lastPreferredEncoding = cl->preferredEncoding;

        /* Reset all flags to defaults (allows us to switch between PointerPos and Server Drawn Cursors) */
        cl->preferredEncoding=-1;
        cl->useCopyRect              = FALSE;
        cl->useNewFBSize             = FALSE;
        cl->cursorWasChanged         = FALSE;
        cl->useRichCursorEncoding    = FALSE;
        cl->enableCursorPosUpdates   = FALSE;
        cl->enableCursorShapeUpdates = FALSE;
        cl->enableCursorShapeUpdates = FALSE;
        cl->enableLastRectEncoding   = FALSE;
        cl->enableKeyboardLedState   = FALSE;
        cl->enableSupportedMessages  = FALSE;
        cl->enableSupportedEncodings = FALSE;
        cl->enableServerIdentity     = FALSE;


        for (i = 0; i < msg.se.nEncodings; i++) {
            if ((n = rfbReadExact(cl, (char *)&enc, 4)) <= 0) {
                if (n != 0)
                    rfbLogPerror("rfbProcessClientNormalMessage: read");
                rfbCloseClient(cl);
                return;
            }
            enc = Swap32IfLE(enc);

            switch (enc) {

            case rfbEncodingCopyRect:
		cl->useCopyRect = TRUE;
                break;
            case rfbEncodingRaw:
            case rfbEncodingRRE:
            case rfbEncodingCoRRE:
            case rfbEncodingHextile:
            case rfbEncodingUltra:
#ifdef LIBVNCSERVER_HAVE_LIBZ
	    case rfbEncodingZlib:
            case rfbEncodingZRLE:
            case rfbEncodingZYWRLE:
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
	    case rfbEncodingTight:
#endif
#endif
            /* The first supported encoding is the 'preferred' encoding */
                if (cl->preferredEncoding == -1)
                    cl->preferredEncoding = enc;


                break;
	    case rfbEncodingXCursor:
		if(!cl->screen->dontConvertRichCursorToXCursor) {
		    rfbLog("Enabling X-style cursor updates for client %s\n",
			   cl->host);
		    /* if cursor was drawn, hide the cursor */
		    if(!cl->enableCursorShapeUpdates)
		        rfbRedrawAfterHideCursor(cl,NULL);

		    cl->enableCursorShapeUpdates = TRUE;
		    cl->cursorWasChanged = TRUE;
		}
		break;
	    case rfbEncodingRichCursor:
	        rfbLog("Enabling full-color cursor updates for client %s\n",
		       cl->host);
		/* if cursor was drawn, hide the cursor */
		if(!cl->enableCursorShapeUpdates)
		    rfbRedrawAfterHideCursor(cl,NULL);

	        cl->enableCursorShapeUpdates = TRUE;
	        cl->useRichCursorEncoding = TRUE;
	        cl->cursorWasChanged = TRUE;
	        break;
	    case rfbEncodingPointerPos:
		if (!cl->enableCursorPosUpdates) {
		    rfbLog("Enabling cursor position updates for client %s\n",
			   cl->host);
		    cl->enableCursorPosUpdates = TRUE;
		    cl->cursorWasMoved = TRUE;
		}
	        break;
	    case rfbEncodingLastRect:
		if (!cl->enableLastRectEncoding) {
		    rfbLog("Enabling LastRect protocol extension for client "
			   "%s\n", cl->host);
		    cl->enableLastRectEncoding = TRUE;
		}
		break;
	    case rfbEncodingNewFBSize:
		if (!cl->useNewFBSize) {
		    rfbLog("Enabling NewFBSize protocol extension for client "
			   "%s\n", cl->host);
		    cl->useNewFBSize = TRUE;
		}
		break;
            case rfbEncodingKeyboardLedState:
                if (!cl->enableKeyboardLedState) {
                  rfbLog("Enabling KeyboardLedState protocol extension for client "
                          "%s\n", cl->host);
                  cl->enableKeyboardLedState = TRUE;
                }
                break;           
            case rfbEncodingSupportedMessages:
                if (!cl->enableSupportedMessages) {
                  rfbLog("Enabling SupportedMessages protocol extension for client "
                          "%s\n", cl->host);
                  cl->enableSupportedMessages = TRUE;
                }
                break;           
            case rfbEncodingSupportedEncodings:
                if (!cl->enableSupportedEncodings) {
                  rfbLog("Enabling SupportedEncodings protocol extension for client "
                          "%s\n", cl->host);
                  cl->enableSupportedEncodings = TRUE;
                }
                break;           
            case rfbEncodingServerIdentity:
                if (!cl->enableServerIdentity) {
                  rfbLog("Enabling ServerIdentity protocol extension for client "
                          "%s\n", cl->host);
                  cl->enableServerIdentity = TRUE;
                }
                break;           
            default:
#ifdef LIBVNCSERVER_HAVE_LIBZ
		if ( enc >= (uint32_t)rfbEncodingCompressLevel0 &&
		     enc <= (uint32_t)rfbEncodingCompressLevel9 ) {
		    cl->zlibCompressLevel = enc & 0x0F;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
		    cl->tightCompressLevel = enc & 0x0F;
		    rfbLog("Using compression level %d for client %s\n",
			   cl->tightCompressLevel, cl->host);
#endif
		} else if ( enc >= (uint32_t)rfbEncodingQualityLevel0 &&
			    enc <= (uint32_t)rfbEncodingQualityLevel9 ) {
		    cl->tightQualityLevel = enc & 0x0F;
		    rfbLog("Using image quality level %d for client %s\n",
			   cl->tightQualityLevel, cl->host);
		} else
#endif
		{
			rfbExtensionData* e;
			for(e = cl->extensions; e;) {
				rfbExtensionData* next = e->next;
				if(e->extension->enablePseudoEncoding &&
					e->extension->enablePseudoEncoding(cl,
						&e->data, (int)enc))
					/* ext handles this encoding */
					break;
				e = next;
			}
			if(e == NULL) {
				rfbBool handled = FALSE;
				/* if the pseudo encoding is not handled by the
				   enabled extensions, search through all
				   extensions. */
				rfbProtocolExtension* e;

				for(e = rfbGetExtensionIterator(); e;) {
					int* encs = e->pseudoEncodings;
					while(encs && *encs!=0) {
						if(*encs==(int)enc) {
							void* data = NULL;
							if(!e->enablePseudoEncoding(cl, &data, (int)enc)) {
								rfbLog("Installed extension pretends to handle pseudo encoding 0x%x, but does not!\n",(int)enc);
							} else {
								rfbEnableExtension(cl, e, data);
								handled = TRUE;
								e = NULL;
								break;
							}
						}
						encs++;
					}

					if(e)
						e = e->next;
				}
				rfbReleaseExtensionIterator();

				if(!handled)
					rfbLog("rfbProcessClientNormalMessage: "
					    "ignoring unsupported encoding type %s\n",
					    encodingName(enc,encBuf,sizeof(encBuf)));
			}
		}
            }
        }



        if (cl->preferredEncoding == -1) {
            if (lastPreferredEncoding==-1) {
                cl->preferredEncoding = rfbEncodingRaw;
                rfbLog("Defaulting to %s encoding for client %s\n", encodingName(cl->preferredEncoding,encBuf,sizeof(encBuf)),cl->host);
            }
            else {
                cl->preferredEncoding = lastPreferredEncoding;
                rfbLog("Sticking with %s encoding for client %s\n", encodingName(cl->preferredEncoding,encBuf,sizeof(encBuf)),cl->host);
            }
        }
        else
        {
          if (lastPreferredEncoding==-1) {
              rfbLog("Using %s encoding for client %s\n", encodingName(cl->preferredEncoding,encBuf,sizeof(encBuf)),cl->host);
          } else {
              rfbLog("Switching from %s to %s Encoding for client %s\n", 
                  encodingName(lastPreferredEncoding,encBuf2,sizeof(encBuf2)),
                  encodingName(cl->preferredEncoding,encBuf,sizeof(encBuf)), cl->host);
          }
        }
        
	if (cl->enableCursorPosUpdates && !cl->enableCursorShapeUpdates) {
	  rfbLog("Disabling cursor position updates for client %s\n",
		 cl->host);
	  cl->enableCursorPosUpdates = FALSE;
	}

        return;
    }


    case rfbFramebufferUpdateRequest:
    {
        sraRegionPtr tmpRegion;

        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                           sz_rfbFramebufferUpdateRequestMsg-1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }

        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbFramebufferUpdateRequestMsg,sz_rfbFramebufferUpdateRequestMsg);

        /* The values come in based on the scaled screen, we need to convert them to
         * values based on the main screen's coordinate system
         */
	if(!rectSwapIfLEAndClip(&msg.fur.x,&msg.fur.y,&msg.fur.w,&msg.fur.h,cl))
	{
	        rfbLog("Warning, ignoring rfbFramebufferUpdateRequest: %dXx%dY-%dWx%dH\n",msg.fur.x, msg.fur.y, msg.fur.w, msg.fur.h);
		return;
        }
 
        
	tmpRegion =
	  sraRgnCreateRect(msg.fur.x,
			   msg.fur.y,
			   msg.fur.x+msg.fur.w,
			   msg.fur.y+msg.fur.h);

        LOCK(cl->updateMutex);
	sraRgnOr(cl->requestedRegion,tmpRegion);

	if (!cl->readyForSetColourMapEntries) {
	    /* client hasn't sent a SetPixelFormat so is using server's */
	    cl->readyForSetColourMapEntries = TRUE;
	    if (!cl->format.trueColour) {
		if (!rfbSetClientColourMap(cl, 0, 0)) {
		    sraRgnDestroy(tmpRegion);
		    UNLOCK(cl->updateMutex);
		    return;
		}
	    }
	}

       if (!msg.fur.incremental) {
	    sraRgnOr(cl->modifiedRegion,tmpRegion);
	    sraRgnSubtract(cl->copyRegion,tmpRegion);
       }
       TSIGNAL(cl->updateCond);
       UNLOCK(cl->updateMutex);

       sraRgnDestroy(tmpRegion);

       return;
    }

    case rfbKeyEvent:

	if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
			   sz_rfbKeyEventMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseClient(cl);
	    return;
	}

	rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbKeyEventMsg, sz_rfbKeyEventMsg);

	if(!cl->viewOnly) {
	    cl->screen->kbdAddEvent(msg.ke.down, (rfbKeySym)Swap32IfLE(msg.ke.key), cl);
	}

        return;


    case rfbPointerEvent:

	if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
			   sz_rfbPointerEventMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseClient(cl);
	    return;
	}

	rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbPointerEventMsg, sz_rfbPointerEventMsg);
	
	if (cl->screen->pointerClient && cl->screen->pointerClient != cl)
	    return;

	if (msg.pe.buttonMask == 0)
	    cl->screen->pointerClient = NULL;
	else
	    cl->screen->pointerClient = cl;

	if(!cl->viewOnly) {
	    if (msg.pe.buttonMask != cl->lastPtrButtons ||
		    cl->screen->deferPtrUpdateTime == 0) {
		cl->screen->ptrAddEvent(msg.pe.buttonMask,
			ScaleX(cl->scaledScreen, cl->screen, Swap16IfLE(msg.pe.x)), 
			ScaleY(cl->scaledScreen, cl->screen, Swap16IfLE(msg.pe.y)),
			cl);
		cl->lastPtrButtons = msg.pe.buttonMask;
	    } else {
		cl->lastPtrX = ScaleX(cl->scaledScreen, cl->screen, Swap16IfLE(msg.pe.x));
		cl->lastPtrY = ScaleY(cl->scaledScreen, cl->screen, Swap16IfLE(msg.pe.y));
		cl->lastPtrButtons = msg.pe.buttonMask;
	    }
      }      
      return;


    case rfbFileTransfer:
        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                              sz_rfbFileTransferMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }
        msg.ft.size         = Swap32IfLE(msg.ft.size);
        msg.ft.length       = Swap32IfLE(msg.ft.length);
        /* record statistics in rfbProcessFileTransfer as length is filled with garbage when it is not valid */
        rfbProcessFileTransfer(cl, msg.ft.contentType, msg.ft.contentParam, msg.ft.size, msg.ft.length);
        return;

    case rfbSetSW:
        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                              sz_rfbSetSWMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }
        msg.sw.x = Swap16IfLE(msg.sw.x);
        msg.sw.y = Swap16IfLE(msg.sw.y);
        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetSWMsg, sz_rfbSetSWMsg);
        /* msg.sw.status is not initialized in the ultraVNC viewer and contains random numbers (why???) */

        rfbLog("Received a rfbSetSingleWindow(%d x, %d y)\n", msg.sw.x, msg.sw.y);
        if (cl->screen->setSingleWindow!=NULL)
            cl->screen->setSingleWindow(cl, msg.sw.x, msg.sw.y);
        return;

    case rfbSetServerInput:
        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                              sz_rfbSetServerInputMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }
        rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetServerInputMsg, sz_rfbSetServerInputMsg);

        /* msg.sim.pad is not initialized in the ultraVNC viewer and contains random numbers (why???) */
        /* msg.sim.pad = Swap16IfLE(msg.sim.pad); */

        rfbLog("Received a rfbSetServerInput(%d status)\n", msg.sim.status);
        if (cl->screen->setServerInput!=NULL)
            cl->screen->setServerInput(cl, msg.sim.status);
        return;
        
    case rfbTextChat:
        if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
                              sz_rfbTextChatMsg - 1)) <= 0) {
            if (n != 0)
                rfbLogPerror("rfbProcessClientNormalMessage: read");
            rfbCloseClient(cl);
            return;
        }
        
        msg.tc.pad2   = Swap16IfLE(msg.tc.pad2);
        msg.tc.length = Swap32IfLE(msg.tc.length);

        switch (msg.tc.length) {
        case rfbTextChatOpen:
        case rfbTextChatClose:
        case rfbTextChatFinished:
            /* commands do not have text following */
            /* Why couldn't they have used the pad byte??? */
            str=NULL;
            rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbTextChatMsg, sz_rfbTextChatMsg);
            break;
        default:
            if ((msg.tc.length>0) && (msg.tc.length<rfbTextMaxSize))
            {
                str = (char *)malloc(msg.tc.length);
                if (str==NULL)
                {
                    rfbLog("Unable to malloc %d bytes for a TextChat Message\n", msg.tc.length);
                    rfbCloseClient(cl);
                    return;
                }
                if ((n = rfbReadExact(cl, str, msg.tc.length)) <= 0) {
                    if (n != 0)
                        rfbLogPerror("rfbProcessClientNormalMessage: read");
                    free(str);
                    rfbCloseClient(cl);
                    return;
                }
                rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbTextChatMsg+msg.tc.length, sz_rfbTextChatMsg+msg.tc.length);
            }
            else
            {
                /* This should never happen */
                rfbLog("client sent us a Text Message that is too big %d>%d\n", msg.tc.length, rfbTextMaxSize);
                rfbCloseClient(cl);
                return;
            }
        }

        /* Note: length can be commands: rfbTextChatOpen, rfbTextChatClose, and rfbTextChatFinished
         * at which point, the str is NULL (as it is not sent)
         */
        if (cl->screen->setTextChat!=NULL)
            cl->screen->setTextChat(cl, msg.tc.length, str);

        free(str);
        return;


    case rfbClientCutText:

	if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
			   sz_rfbClientCutTextMsg - 1)) <= 0) {
	    if (n != 0)
		rfbLogPerror("rfbProcessClientNormalMessage: read");
	    rfbCloseClient(cl);
	    return;
	}

	msg.cct.length = Swap32IfLE(msg.cct.length);

	str = (char *)malloc(msg.cct.length);

	if ((n = rfbReadExact(cl, str, msg.cct.length)) <= 0) {
	    if (n != 0)
	        rfbLogPerror("rfbProcessClientNormalMessage: read");
	    free(str);
	    rfbCloseClient(cl);
	    return;
	}
	rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbClientCutTextMsg+msg.cct.length, sz_rfbClientCutTextMsg+msg.cct.length);
	if(!cl->viewOnly) {
	    cl->screen->setXCutText(str, msg.cct.length, cl);
	}
	free(str);

        return;

    case rfbPalmVNCSetScaleFactor:
      cl->PalmVNC = TRUE;
      if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
          sz_rfbSetScaleMsg - 1)) <= 0) {
          if (n != 0)
            rfbLogPerror("rfbProcessClientNormalMessage: read");
          rfbCloseClient(cl);
          return;
      }
      rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetScaleMsg, sz_rfbSetScaleMsg);
      rfbLog("rfbSetScale(%d)\n", msg.ssc.scale);
      rfbScalingSetup(cl,cl->screen->width/msg.ssc.scale, cl->screen->height/msg.ssc.scale);

      rfbSendNewScaleSize(cl);
      return;
      
    case rfbSetScale:

      if ((n = rfbReadExact(cl, ((char *)&msg) + 1,
          sz_rfbSetScaleMsg - 1)) <= 0) {
          if (n != 0)
            rfbLogPerror("rfbProcessClientNormalMessage: read");
          rfbCloseClient(cl);
          return;
      }
      rfbStatRecordMessageRcvd(cl, msg.type, sz_rfbSetScaleMsg, sz_rfbSetScaleMsg);
      rfbLog("rfbSetScale(%d)\n", msg.ssc.scale);
      rfbScalingSetup(cl,cl->screen->width/msg.ssc.scale, cl->screen->height/msg.ssc.scale);

      rfbSendNewScaleSize(cl);
      return;

    default:
	{
	    rfbExtensionData *e,*next;

	    for(e=cl->extensions; e;) {
		next = e->next;
		if(e->extension->handleMessage &&
			e->extension->handleMessage(cl, e->data, &msg))
                {
                    rfbStatRecordMessageRcvd(cl, msg.type, 0, 0); /* Extension should handle this */
		    return;
                }
		e = next;
	    }

	    rfbLog("rfbProcessClientNormalMessage: unknown message type %d\n",
		    msg.type);
	    rfbLog(" ... closing connection\n");
	    rfbCloseClient(cl);
	    return;
	}
    }
}



/*
 * rfbSendFramebufferUpdate - send the currently pending framebuffer update to
 * the RFB client.
 * givenUpdateRegion is not changed.
 */

rfbBool
rfbSendFramebufferUpdate(rfbClientPtr cl,
                         sraRegionPtr givenUpdateRegion)
{
    sraRectangleIterator* i=NULL;
    sraRect rect;
    int nUpdateRegionRects;
    rfbFramebufferUpdateMsg *fu = (rfbFramebufferUpdateMsg *)cl->updateBuf;
    sraRegionPtr updateRegion,updateCopyRegion,tmpRegion;
    int dx, dy;
    rfbBool sendCursorShape = FALSE;
    rfbBool sendCursorPos = FALSE;
    rfbBool sendKeyboardLedState = FALSE;
    rfbBool sendSupportedMessages = FALSE;
    rfbBool sendSupportedEncodings = FALSE;
    rfbBool sendServerIdentity = FALSE;
    rfbBool result = TRUE;
    

    if(cl->screen->displayHook)
      cl->screen->displayHook(cl);

    /*
     * If framebuffer size was changed and the client supports NewFBSize
     * encoding, just send NewFBSize marker and return.
     */

    if (cl->useNewFBSize && cl->newFBSizePending) {
      LOCK(cl->updateMutex);
      cl->newFBSizePending = FALSE;
      UNLOCK(cl->updateMutex);
      fu->type = rfbFramebufferUpdate;
      fu->nRects = Swap16IfLE(1);
      cl->ublen = sz_rfbFramebufferUpdateMsg;
      if (!rfbSendNewFBSize(cl, cl->scaledScreen->width, cl->scaledScreen->height)) {
        return FALSE;
      }
      return rfbSendUpdateBuf(cl);
    }
    
    /*
     * If this client understands cursor shape updates, cursor should be
     * removed from the framebuffer. Otherwise, make sure it's put up.
     */

    if (cl->enableCursorShapeUpdates) {
      if (cl->cursorWasChanged && cl->readyForSetColourMapEntries)
	  sendCursorShape = TRUE;
    }

    /*
     * Do we plan to send cursor position update?
     */

    if (cl->enableCursorPosUpdates && cl->cursorWasMoved)
      sendCursorPos = TRUE;

    /*
     * Do we plan to send a keyboard state update?
     */
    if ((cl->enableKeyboardLedState) &&
	(cl->screen->getKeyboardLedStateHook!=NULL))
    {
        int x;
        x=cl->screen->getKeyboardLedStateHook(cl->screen);
        if (x!=cl->lastKeyboardLedState)
        {
            sendKeyboardLedState = TRUE;
            cl->lastKeyboardLedState=x;
        }
    }

    /*
     * Do we plan to send a rfbEncodingSupportedMessages?
     */
    if (cl->enableSupportedMessages)
    {
        sendSupportedMessages = TRUE;
        /* We only send this message ONCE <per setEncodings message received>
         * (We disable it here)
         */
        cl->enableSupportedMessages = FALSE;
    }
    /*
     * Do we plan to send a rfbEncodingSupportedEncodings?
     */
    if (cl->enableSupportedEncodings)
    {
        sendSupportedEncodings = TRUE;
        /* We only send this message ONCE <per setEncodings message received>
         * (We disable it here)
         */
        cl->enableSupportedEncodings = FALSE;
    }
    /*
     * Do we plan to send a rfbEncodingServerIdentity?
     */
    if (cl->enableServerIdentity)
    {
        sendServerIdentity = TRUE;
        /* We only send this message ONCE <per setEncodings message received>
         * (We disable it here)
         */
        cl->enableServerIdentity = FALSE;
    }

    LOCK(cl->updateMutex);

    /*
     * The modifiedRegion may overlap the destination copyRegion.  We remove
     * any overlapping bits from the copyRegion (since they'd only be
     * overwritten anyway).
     */
    
    sraRgnSubtract(cl->copyRegion,cl->modifiedRegion);

    /*
     * The client is interested in the region requestedRegion.  The region
     * which should be updated now is the intersection of requestedRegion
     * and the union of modifiedRegion and copyRegion.  If it's empty then
     * no update is needed.
     */

    updateRegion = sraRgnCreateRgn(givenUpdateRegion);
    if(cl->screen->progressiveSliceHeight>0) {
	    int height=cl->screen->progressiveSliceHeight,
	    	y=cl->progressiveSliceY;
	    sraRegionPtr bbox=sraRgnBBox(updateRegion);
	    sraRect rect;
	    if(sraRgnPopRect(bbox,&rect,0)) {
		sraRegionPtr slice;
		if(y<rect.y1 || y>=rect.y2)
		    y=rect.y1;
	    	slice=sraRgnCreateRect(0,y,cl->screen->width,y+height);
		sraRgnAnd(updateRegion,slice);
		sraRgnDestroy(slice);
	    }
	    sraRgnDestroy(bbox);
	    y+=height;
	    if(y>=cl->screen->height)
		    y=0;
	    cl->progressiveSliceY=y;
    }

    sraRgnOr(updateRegion,cl->copyRegion);
    if(!sraRgnAnd(updateRegion,cl->requestedRegion) &&
       sraRgnEmpty(updateRegion) &&
       (cl->enableCursorShapeUpdates ||
	(cl->cursorX == cl->screen->cursorX && cl->cursorY == cl->screen->cursorY)) &&
       !sendCursorShape && !sendCursorPos && !sendKeyboardLedState &&
       !sendSupportedMessages && !sendSupportedEncodings && !sendServerIdentity) {
      sraRgnDestroy(updateRegion);
      UNLOCK(cl->updateMutex);
      return TRUE;
    }

    /*
     * We assume that the client doesn't have any pixel data outside the
     * requestedRegion.  In other words, both the source and destination of a
     * copy must lie within requestedRegion.  So the region we can send as a
     * copy is the intersection of the copyRegion with both the requestedRegion
     * and the requestedRegion translated by the amount of the copy.  We set
     * updateCopyRegion to this.
     */

    updateCopyRegion = sraRgnCreateRgn(cl->copyRegion);
    sraRgnAnd(updateCopyRegion,cl->requestedRegion);
    tmpRegion = sraRgnCreateRgn(cl->requestedRegion);
    sraRgnOffset(tmpRegion,cl->copyDX,cl->copyDY);
    sraRgnAnd(updateCopyRegion,tmpRegion);
    sraRgnDestroy(tmpRegion);
    dx = cl->copyDX;
    dy = cl->copyDY;

    /*
     * Next we remove updateCopyRegion from updateRegion so that updateRegion
     * is the part of this update which is sent as ordinary pixel data (i.e not
     * a copy).
     */

    sraRgnSubtract(updateRegion,updateCopyRegion);

    /*
     * Finally we leave modifiedRegion to be the remainder (if any) of parts of
     * the screen which are modified but outside the requestedRegion.  We also
     * empty both the requestedRegion and the copyRegion - note that we never
     * carry over a copyRegion for a future update.
     */

     sraRgnOr(cl->modifiedRegion,cl->copyRegion);
     sraRgnSubtract(cl->modifiedRegion,updateRegion);
     sraRgnSubtract(cl->modifiedRegion,updateCopyRegion);

     sraRgnMakeEmpty(cl->requestedRegion);
     sraRgnMakeEmpty(cl->copyRegion);
     cl->copyDX = 0;
     cl->copyDY = 0;
   
     UNLOCK(cl->updateMutex);
   
    if (!cl->enableCursorShapeUpdates) {
      if(cl->cursorX != cl->screen->cursorX || cl->cursorY != cl->screen->cursorY) {
	rfbRedrawAfterHideCursor(cl,updateRegion);
	LOCK(cl->screen->cursorMutex);
	cl->cursorX = cl->screen->cursorX;
	cl->cursorY = cl->screen->cursorY;
	UNLOCK(cl->screen->cursorMutex);
	rfbRedrawAfterHideCursor(cl,updateRegion);
      }
      rfbShowCursor(cl);
    }

    /*
     * Now send the update.
     */
    
    rfbStatRecordMessageSent(cl, rfbFramebufferUpdate, 0, 0);
    if (cl->preferredEncoding == rfbEncodingCoRRE) {
        nUpdateRegionRects = 0;

        for(i = sraRgnGetIterator(updateRegion); sraRgnIteratorNext(i,&rect);){
            int x = rect.x1;
            int y = rect.y1;
            int w = rect.x2 - x;
            int h = rect.y2 - y;
	    int rectsPerRow, rows;
            /* We need to count the number of rects in the scaled screen */
            if (cl->screen!=cl->scaledScreen)
                rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "rfbSendFramebufferUpdate");
	    rectsPerRow = (w-1)/cl->correMaxWidth+1;
	    rows = (h-1)/cl->correMaxHeight+1;
	    nUpdateRegionRects += rectsPerRow*rows;
        }
	sraRgnReleaseIterator(i); i=NULL;
    } else if (cl->preferredEncoding == rfbEncodingUltra) {
        nUpdateRegionRects = 0;
        
        for(i = sraRgnGetIterator(updateRegion); sraRgnIteratorNext(i,&rect);){
            int x = rect.x1;
            int y = rect.y1;
            int w = rect.x2 - x;
            int h = rect.y2 - y;
            /* We need to count the number of rects in the scaled screen */
            if (cl->screen!=cl->scaledScreen)
                rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "rfbSendFramebufferUpdate");
            nUpdateRegionRects += (((h-1) / (ULTRA_MAX_SIZE( w ) / w)) + 1);
          }
        sraRgnReleaseIterator(i); i=NULL;
#ifdef LIBVNCSERVER_HAVE_LIBZ
    } else if (cl->preferredEncoding == rfbEncodingZlib) {
	nUpdateRegionRects = 0;

        for(i = sraRgnGetIterator(updateRegion); sraRgnIteratorNext(i,&rect);){
            int x = rect.x1;
            int y = rect.y1;
            int w = rect.x2 - x;
            int h = rect.y2 - y;
            /* We need to count the number of rects in the scaled screen */
            if (cl->screen!=cl->scaledScreen)
                rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "rfbSendFramebufferUpdate");
	    nUpdateRegionRects += (((h-1) / (ZLIB_MAX_SIZE( w ) / w)) + 1);
	}
	sraRgnReleaseIterator(i); i=NULL;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
    } else if (cl->preferredEncoding == rfbEncodingTight) {
	nUpdateRegionRects = 0;

        for(i = sraRgnGetIterator(updateRegion); sraRgnIteratorNext(i,&rect);){
            int x = rect.x1;
            int y = rect.y1;
            int w = rect.x2 - x;
            int h = rect.y2 - y;
            int n;
            /* We need to count the number of rects in the scaled screen */
            if (cl->screen!=cl->scaledScreen)
                rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "rfbSendFramebufferUpdate");
	    n = rfbNumCodedRectsTight(cl, x, y, w, h);
	    if (n == 0) {
		nUpdateRegionRects = 0xFFFF;
		break;
	    }
	    nUpdateRegionRects += n;
	}
	sraRgnReleaseIterator(i); i=NULL;
#endif
#endif
    } else {
        nUpdateRegionRects = sraRgnCountRects(updateRegion);
    }

    fu->type = rfbFramebufferUpdate;
    if (nUpdateRegionRects != 0xFFFF) {
	if(cl->screen->maxRectsPerUpdate>0
	   /* CoRRE splits the screen into smaller squares */
	   && cl->preferredEncoding != rfbEncodingCoRRE
	   /* Ultra encoding splits rectangles up into smaller chunks */
           && cl->preferredEncoding != rfbEncodingUltra
#ifdef LIBVNCSERVER_HAVE_LIBZ
	   /* Zlib encoding splits rectangles up into smaller chunks */
	   && cl->preferredEncoding != rfbEncodingZlib
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
	   /* Tight encoding counts the rectangles differently */
	   && cl->preferredEncoding != rfbEncodingTight
#endif
#endif
	   && nUpdateRegionRects>cl->screen->maxRectsPerUpdate) {
	    sraRegion* newUpdateRegion = sraRgnBBox(updateRegion);
	    sraRgnDestroy(updateRegion);
	    updateRegion = newUpdateRegion;
	    nUpdateRegionRects = sraRgnCountRects(updateRegion);
	}
	fu->nRects = Swap16IfLE((uint16_t)(sraRgnCountRects(updateCopyRegion) +
					   nUpdateRegionRects +
					   !!sendCursorShape + !!sendCursorPos + !!sendKeyboardLedState +
					   !!sendSupportedMessages + !!sendSupportedEncodings + !!sendServerIdentity));
    } else {
	fu->nRects = 0xFFFF;
    }
    cl->ublen = sz_rfbFramebufferUpdateMsg;

   if (sendCursorShape) {
	cl->cursorWasChanged = FALSE;
	if (!rfbSendCursorShape(cl))
	    goto updateFailed;
    }
   
   if (sendCursorPos) {
	cl->cursorWasMoved = FALSE;
	if (!rfbSendCursorPos(cl))
	        goto updateFailed;
   }
   
   if (sendKeyboardLedState) {
       if (!rfbSendKeyboardLedState(cl))
           goto updateFailed;
   }

   if (sendSupportedMessages) {
       if (!rfbSendSupportedMessages(cl))
           goto updateFailed;
   }
   if (sendSupportedEncodings) {
       if (!rfbSendSupportedEncodings(cl))
           goto updateFailed;
   }
   if (sendServerIdentity) {
       if (!rfbSendServerIdentity(cl))
           goto updateFailed;
   }
   
    if (!sraRgnEmpty(updateCopyRegion)) {
	if (!rfbSendCopyRegion(cl,updateCopyRegion,dx,dy))
	        goto updateFailed;
    }

    for(i = sraRgnGetIterator(updateRegion); sraRgnIteratorNext(i,&rect);){
        int x = rect.x1;
        int y = rect.y1;
        int w = rect.x2 - x;
        int h = rect.y2 - y;

        /* We need to count the number of rects in the scaled screen */
        if (cl->screen!=cl->scaledScreen)
            rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "rfbSendFramebufferUpdate");

        switch (cl->preferredEncoding) {
	case -1:
        case rfbEncodingRaw:
            if (!rfbSendRectEncodingRaw(cl, x, y, w, h))
	        goto updateFailed;
            break;
        case rfbEncodingRRE:
            if (!rfbSendRectEncodingRRE(cl, x, y, w, h))
	        goto updateFailed;
            break;
        case rfbEncodingCoRRE:
            if (!rfbSendRectEncodingCoRRE(cl, x, y, w, h))
	        goto updateFailed;
	    break;
        case rfbEncodingHextile:
            if (!rfbSendRectEncodingHextile(cl, x, y, w, h))
	        goto updateFailed;
            break;
        case rfbEncodingUltra:
            if (!rfbSendRectEncodingUltra(cl, x, y, w, h))
                goto updateFailed;
            break;
#ifdef LIBVNCSERVER_HAVE_LIBZ
	case rfbEncodingZlib:
	    if (!rfbSendRectEncodingZlib(cl, x, y, w, h))
	        goto updateFailed;
	    break;
#ifdef LIBVNCSERVER_HAVE_LIBJPEG
	case rfbEncodingTight:
	    if (!rfbSendRectEncodingTight(cl, x, y, w, h))
	        goto updateFailed;
	    break;
#endif
#endif
#ifdef LIBVNCSERVER_HAVE_LIBZ
       case rfbEncodingZRLE:
       case rfbEncodingZYWRLE:
           if (!rfbSendRectEncodingZRLE(cl, x, y, w, h))
	       goto updateFailed;
           break;
#endif
        }
    }
    if (i) {
        sraRgnReleaseIterator(i);
        i = NULL;
    }

    if ( nUpdateRegionRects == 0xFFFF &&
	 !rfbSendLastRectMarker(cl) )
	    goto updateFailed;

    if (!rfbSendUpdateBuf(cl)) {
updateFailed:
	result = FALSE;
    }

    if (!cl->enableCursorShapeUpdates) {
      rfbHideCursor(cl);
    }

    if(i)
        sraRgnReleaseIterator(i);
    sraRgnDestroy(updateRegion);
    sraRgnDestroy(updateCopyRegion);
    return result;
}


/*
 * Send the copy region as a string of CopyRect encoded rectangles.
 * The only slightly tricky thing is that we should send the messages in
 * the correct order so that an earlier CopyRect will not corrupt the source
 * of a later one.
 */

rfbBool
rfbSendCopyRegion(rfbClientPtr cl,
                  sraRegionPtr reg,
                  int dx,
                  int dy)
{
    int x, y, w, h;
    rfbFramebufferUpdateRectHeader rect;
    rfbCopyRect cr;
    sraRectangleIterator* i;
    sraRect rect1;

    /* printf("copyrect: "); sraRgnPrint(reg); putchar('\n');fflush(stdout); */
    i = sraRgnGetReverseIterator(reg,dx>0,dy>0);

    /* correct for the scale of the screen */
    dx = ScaleX(cl->screen, cl->scaledScreen, dx);
    dy = ScaleX(cl->screen, cl->scaledScreen, dy);

    while(sraRgnIteratorNext(i,&rect1)) {
      x = rect1.x1;
      y = rect1.y1;
      w = rect1.x2 - x;
      h = rect1.y2 - y;

      /* correct for scaling (if necessary) */
      rfbScaledCorrection(cl->screen, cl->scaledScreen, &x, &y, &w, &h, "copyrect");

      rect.r.x = Swap16IfLE(x);
      rect.r.y = Swap16IfLE(y);
      rect.r.w = Swap16IfLE(w);
      rect.r.h = Swap16IfLE(h);
      rect.encoding = Swap32IfLE(rfbEncodingCopyRect);

      memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
	     sz_rfbFramebufferUpdateRectHeader);
      cl->ublen += sz_rfbFramebufferUpdateRectHeader;

      cr.srcX = Swap16IfLE(x - dx);
      cr.srcY = Swap16IfLE(y - dy);

      memcpy(&cl->updateBuf[cl->ublen], (char *)&cr, sz_rfbCopyRect);
      cl->ublen += sz_rfbCopyRect;

      rfbStatRecordEncodingSent(cl, rfbEncodingCopyRect, sz_rfbFramebufferUpdateRectHeader + sz_rfbCopyRect,
          w * h  * (cl->scaledScreen->bitsPerPixel / 8));
    }
    sraRgnReleaseIterator(i);

    return TRUE;
}

/*
 * Send a given rectangle in raw encoding (rfbEncodingRaw).
 */

rfbBool
rfbSendRectEncodingRaw(rfbClientPtr cl,
                       int x,
                       int y,
                       int w,
                       int h)
{
    rfbFramebufferUpdateRectHeader rect;
    int nlines;
    int bytesPerLine = w * (cl->format.bitsPerPixel / 8);
    char *fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
                   + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    /* Flush the buffer to guarantee correct alignment for translateFn(). */
    if (cl->ublen > 0) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingRaw);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;


    rfbStatRecordEncodingSent(cl, rfbEncodingRaw, sz_rfbFramebufferUpdateRectHeader + bytesPerLine * h,
        sz_rfbFramebufferUpdateRectHeader + bytesPerLine * h);

    nlines = (UPDATE_BUF_SIZE - cl->ublen) / bytesPerLine;

    while (TRUE) {
        if (nlines > h)
            nlines = h;

        (*cl->translateFn)(cl->translateLookupTable,
			   &(cl->screen->serverFormat),
                           &cl->format, fbptr, &cl->updateBuf[cl->ublen],
                           cl->scaledScreen->paddedWidthInBytes, w, nlines);

        cl->ublen += nlines * bytesPerLine;
        h -= nlines;

        if (h == 0)     /* rect fitted in buffer, do next one */
            return TRUE;

        /* buffer full - flush partial rect and do another nlines */

        if (!rfbSendUpdateBuf(cl))
            return FALSE;

        fbptr += (cl->scaledScreen->paddedWidthInBytes * nlines);

        nlines = (UPDATE_BUF_SIZE - cl->ublen) / bytesPerLine;
        if (nlines == 0) {
            rfbErr("rfbSendRectEncodingRaw: send buffer too small for %d "
                   "bytes per line\n", bytesPerLine);
            rfbCloseClient(cl);
            return FALSE;
        }
    }
}



/*
 * Send an empty rectangle with encoding field set to value of
 * rfbEncodingLastRect to notify client that this is the last
 * rectangle in framebuffer update ("LastRect" extension of RFB
 * protocol).
 */

rfbBool
rfbSendLastRectMarker(rfbClientPtr cl)
{
    rfbFramebufferUpdateRectHeader rect;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    rect.encoding = Swap32IfLE(rfbEncodingLastRect);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = 0;
    rect.r.h = 0;

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;


    rfbStatRecordEncodingSent(cl, rfbEncodingLastRect, sz_rfbFramebufferUpdateRectHeader, sz_rfbFramebufferUpdateRectHeader);

    return TRUE;
}


/*
 * Send NewFBSize pseudo-rectangle. This tells the client to change
 * its framebuffer size.
 */

rfbBool
rfbSendNewFBSize(rfbClientPtr cl,
                 int w,
                 int h)
{
    rfbFramebufferUpdateRectHeader rect;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    if (cl->PalmVNC==TRUE)
        rfbLog("Sending rfbEncodingNewFBSize in response to a PalmVNC style framebuffer resize (%dx%d)\n", w, h);
    else
        rfbLog("Sending rfbEncodingNewFBSize for resize to (%dx%d)\n", w, h);

    rect.encoding = Swap32IfLE(rfbEncodingNewFBSize);
    rect.r.x = 0;
    rect.r.y = 0;
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, rfbEncodingNewFBSize, sz_rfbFramebufferUpdateRectHeader, sz_rfbFramebufferUpdateRectHeader);

    return TRUE;
}


/*
 * Send the contents of cl->updateBuf.  Returns 1 if successful, -1 if
 * not (errno should be set).
 */

rfbBool
rfbSendUpdateBuf(rfbClientPtr cl)
{
    if(cl->sock<0)
      return FALSE;

    if (rfbWriteExact(cl, cl->updateBuf, cl->ublen) < 0) {
        rfbLogPerror("rfbSendUpdateBuf: write");
        rfbCloseClient(cl);
        return FALSE;
    }

    cl->ublen = 0;
    return TRUE;
}

/*
 * rfbSendSetColourMapEntries sends a SetColourMapEntries message to the
 * client, using values from the currently installed colormap.
 */

rfbBool
rfbSendSetColourMapEntries(rfbClientPtr cl,
                           int firstColour,
                           int nColours)
{
    char buf[sz_rfbSetColourMapEntriesMsg + 256 * 3 * 2];
    char *wbuf = buf;
    rfbSetColourMapEntriesMsg *scme;
    uint16_t *rgb;
    rfbColourMap* cm = &cl->screen->colourMap;
    int i, len;

    if (nColours > 256) {
	/* some rare hardware has, e.g., 4096 colors cells: PseudoColor:12 */
    	wbuf = (char *) malloc(sz_rfbSetColourMapEntriesMsg + nColours * 3 * 2);
    }

    scme = (rfbSetColourMapEntriesMsg *)wbuf;
    rgb = (uint16_t *)(&wbuf[sz_rfbSetColourMapEntriesMsg]);

    scme->type = rfbSetColourMapEntries;

    scme->firstColour = Swap16IfLE(firstColour);
    scme->nColours = Swap16IfLE(nColours);

    len = sz_rfbSetColourMapEntriesMsg;

    for (i = 0; i < nColours; i++) {
      if(i<(int)cm->count) {
	if(cm->is16) {
	  rgb[i*3] = Swap16IfLE(cm->data.shorts[i*3]);
	  rgb[i*3+1] = Swap16IfLE(cm->data.shorts[i*3+1]);
	  rgb[i*3+2] = Swap16IfLE(cm->data.shorts[i*3+2]);
	} else {
	  rgb[i*3] = Swap16IfLE((unsigned short)cm->data.bytes[i*3]);
	  rgb[i*3+1] = Swap16IfLE((unsigned short)cm->data.bytes[i*3+1]);
	  rgb[i*3+2] = Swap16IfLE((unsigned short)cm->data.bytes[i*3+2]);
	}
      }
    }

    len += nColours * 3 * 2;

    if (rfbWriteExact(cl, wbuf, len) < 0) {
	rfbLogPerror("rfbSendSetColourMapEntries: write");
	rfbCloseClient(cl);
        if (wbuf != buf) free(wbuf);
	return FALSE;
    }

    rfbStatRecordMessageSent(cl, rfbSetColourMapEntries, len, len);
    if (wbuf != buf) free(wbuf);
    return TRUE;
}

/*
 * rfbSendBell sends a Bell message to all the clients.
 */

void
rfbSendBell(rfbScreenInfoPtr rfbScreen)
{
    rfbClientIteratorPtr i;
    rfbClientPtr cl;
    rfbBellMsg b;

    i = rfbGetClientIterator(rfbScreen);
    while((cl=rfbClientIteratorNext(i))) {
	b.type = rfbBell;
	if (rfbWriteExact(cl, (char *)&b, sz_rfbBellMsg) < 0) {
	    rfbLogPerror("rfbSendBell: write");
	    rfbCloseClient(cl);
	}
    }
    rfbStatRecordMessageSent(cl, rfbBell, sz_rfbBellMsg, sz_rfbBellMsg);
    rfbReleaseClientIterator(i);
}


/*
 * rfbSendServerCutText sends a ServerCutText message to all the clients.
 */

void
rfbSendServerCutText(rfbScreenInfoPtr rfbScreen,char *str, int len)
{
    rfbClientPtr cl;
    rfbServerCutTextMsg sct;
    rfbClientIteratorPtr iterator;

    iterator = rfbGetClientIterator(rfbScreen);
    while ((cl = rfbClientIteratorNext(iterator)) != NULL) {
        sct.type = rfbServerCutText;
        sct.length = Swap32IfLE(len);
        if (rfbWriteExact(cl, (char *)&sct,
                       sz_rfbServerCutTextMsg) < 0) {
            rfbLogPerror("rfbSendServerCutText: write");
            rfbCloseClient(cl);
            continue;
        }
        if (rfbWriteExact(cl, str, len) < 0) {
            rfbLogPerror("rfbSendServerCutText: write");
            rfbCloseClient(cl);
        }
        rfbStatRecordMessageSent(cl, rfbServerCutText, sz_rfbServerCutTextMsg+len, sz_rfbServerCutTextMsg+len);
    }
    rfbReleaseClientIterator(iterator);
}

/*****************************************************************************
 *
 * UDP can be used for keyboard and pointer events when the underlying
 * network is highly reliable.  This is really here to support ORL's
 * videotile, whose TCP implementation doesn't like sending lots of small
 * packets (such as 100s of pen readings per second!).
 */

static unsigned char ptrAcceleration = 50;

void
rfbNewUDPConnection(rfbScreenInfoPtr rfbScreen,
                    int sock)
{
    if (write(sock, &ptrAcceleration, 1) < 0) {
	rfbLogPerror("rfbNewUDPConnection: write");
    }
}

/*
 * Because UDP is a message based service, we can't read the first byte and
 * then the rest of the packet separately like we do with TCP.  We will always
 * get a whole packet delivered in one go, so we ask read() for the maximum
 * number of bytes we can possibly get.
 */

void
rfbProcessUDPInput(rfbScreenInfoPtr rfbScreen)
{
    int n;
    rfbClientPtr cl=rfbScreen->udpClient;
    rfbClientToServerMsg msg;

    if((!cl) || cl->onHold)
      return;

    if ((n = read(rfbScreen->udpSock, (char *)&msg, sizeof(msg))) <= 0) {
	if (n < 0) {
	    rfbLogPerror("rfbProcessUDPInput: read");
	}
	rfbDisconnectUDPSock(rfbScreen);
	return;
    }

    switch (msg.type) {

    case rfbKeyEvent:
	if (n != sz_rfbKeyEventMsg) {
	    rfbErr("rfbProcessUDPInput: key event incorrect length\n");
	    rfbDisconnectUDPSock(rfbScreen);
	    return;
	}
	cl->screen->kbdAddEvent(msg.ke.down, (rfbKeySym)Swap32IfLE(msg.ke.key), cl);
	break;

    case rfbPointerEvent:
	if (n != sz_rfbPointerEventMsg) {
	    rfbErr("rfbProcessUDPInput: ptr event incorrect length\n");
	    rfbDisconnectUDPSock(rfbScreen);
	    return;
	}
	cl->screen->ptrAddEvent(msg.pe.buttonMask,
		    Swap16IfLE(msg.pe.x), Swap16IfLE(msg.pe.y), cl);
	break;

    default:
	rfbErr("rfbProcessUDPInput: unknown message type %d\n",
	       msg.type);
	rfbDisconnectUDPSock(rfbScreen);
    }
}


///////////////////////////////////////////end rfbserver.c //////////////////////////////////////////////////

///////////////////////////////////////////begin rfbregion.c //////////////////////////////////////////////////
/* -=- sraRegion.c
 * Copyright (c) 2001 James "Wez" Weatherall, Johannes E. Schindelin
 *
 * A general purpose region clipping library
 * Only deals with rectangular regions, though.
 */

#include <rfb/rfb.h>
#include <rfb/rfbregion.h>

/* -=- Internal Span structure */

struct sraRegion;

typedef struct sraSpan {
  struct sraSpan *_next;
  struct sraSpan *_prev;
  int start;
  int end;
  struct sraRegion *subspan;
} sraSpan;

typedef struct sraRegion {
  sraSpan front;
  sraSpan back;
} sraSpanList;

/* -=- Span routines */

sraSpanList *sraSpanListDup(const sraSpanList *src);
void sraSpanListDestroy(sraSpanList *list);

static sraSpan *
sraSpanCreate(int start, int end, const sraSpanList *subspan) {
  sraSpan *item = (sraSpan*)malloc(sizeof(sraSpan));
  item->_next = item->_prev = NULL;
  item->start = start;
  item->end = end;
  item->subspan = sraSpanListDup(subspan);
  return item;
}

static sraSpan *
sraSpanDup(const sraSpan *src) {
  sraSpan *span;
  if (!src) return NULL;
  span = sraSpanCreate(src->start, src->end, src->subspan);
  return span;
}

static void
sraSpanInsertAfter(sraSpan *newspan, sraSpan *after) {
  newspan->_next = after->_next;
  newspan->_prev = after;
  after->_next->_prev = newspan;
  after->_next = newspan;
}

static void
sraSpanInsertBefore(sraSpan *newspan, sraSpan *before) {
  newspan->_next = before;
  newspan->_prev = before->_prev;
  before->_prev->_next = newspan;
  before->_prev = newspan;
}

static void
sraSpanRemove(sraSpan *span) {
  span->_prev->_next = span->_next;
  span->_next->_prev = span->_prev;
}

static void
sraSpanDestroy(sraSpan *span) {
  if (span->subspan) sraSpanListDestroy(span->subspan);
  free(span);
}

#ifdef DEBUG
static void
sraSpanCheck(const sraSpan *span, const char *text) {
  /* Check the span is valid! */
  if (span->start == span->end) {
    printf(text); 
    printf(":%d-%d\n", span->start, span->end);
  }
}
#endif

/* -=- SpanList routines */

static void sraSpanPrint(const sraSpan *s);

static void
sraSpanListPrint(const sraSpanList *l) {
  sraSpan *curr;
  if (!l) {
	  printf("NULL");
	  return;
  }
  curr = l->front._next;
  printf("[");
  while (curr != &(l->back)) {
    sraSpanPrint(curr);
    curr = curr->_next;
  }
  printf("]");
}

void
sraSpanPrint(const sraSpan *s) {
  printf("(%d-%d)", (s->start), (s->end));
  if (s->subspan)
    sraSpanListPrint(s->subspan);
}

static sraSpanList *
sraSpanListCreate(void) {
  sraSpanList *item = (sraSpanList*)malloc(sizeof(sraSpanList));
  item->front._next = &(item->back);
  item->front._prev = NULL;
  item->back._prev = &(item->front);
  item->back._next = NULL;
  return item;
}

sraSpanList *
sraSpanListDup(const sraSpanList *src) {
  sraSpanList *newlist;
  sraSpan *newspan, *curr;

  if (!src) return NULL;
  newlist = sraSpanListCreate();
  curr = src->front._next;
  while (curr != &(src->back)) {
    newspan = sraSpanDup(curr);
    sraSpanInsertBefore(newspan, &(newlist->back));
    curr = curr->_next;
  }

  return newlist;
}

void
sraSpanListDestroy(sraSpanList *list) {
  sraSpan *curr, *next;
  while (list->front._next != &(list->back)) {
    curr = list->front._next;
    next = curr->_next;
    sraSpanRemove(curr);
    sraSpanDestroy(curr);
    curr = next;
  }
  free(list);
}

static void
sraSpanListMakeEmpty(sraSpanList *list) {
  sraSpan *curr, *next;
  while (list->front._next != &(list->back)) {
    curr = list->front._next;
    next = curr->_next;
    sraSpanRemove(curr);
    sraSpanDestroy(curr);
    curr = next;
  }
  list->front._next = &(list->back);
  list->front._prev = NULL;
  list->back._prev = &(list->front);
  list->back._next = NULL;
}

static rfbBool
sraSpanListEqual(const sraSpanList *s1, const sraSpanList *s2) {
  sraSpan *sp1, *sp2;

  if (!s1) {
    if (!s2) {
      return 1;
    } else {
      rfbErr("sraSpanListEqual:incompatible spans (only one NULL!)\n");
      return FALSE;
    }
  }

  sp1 = s1->front._next;
  sp2 = s2->front._next;
  while ((sp1 != &(s1->back)) &&
	 (sp2 != &(s2->back))) {
    if ((sp1->start != sp2->start) ||
	(sp1->end != sp2->end) ||
	(!sraSpanListEqual(sp1->subspan, sp2->subspan))) {
      return 0;
    }
    sp1 = sp1->_next;
    sp2 = sp2->_next;
  }

  if ((sp1 == &(s1->back)) && (sp2 == &(s2->back))) {
    return 1;
  } else {
    return 0;
  }    
}

static rfbBool
sraSpanListEmpty(const sraSpanList *list) {
  return (list->front._next == &(list->back));
}

static unsigned long
sraSpanListCount(const sraSpanList *list) {
  sraSpan *curr = list->front._next;
  unsigned long count = 0;
  while (curr != &(list->back)) {
    if (curr->subspan) {
      count += sraSpanListCount(curr->subspan);
    } else {
      count += 1;
    }
    curr = curr->_next;
  }
  return count;
}

static void
sraSpanMergePrevious(sraSpan *dest) {
  sraSpan *prev = dest->_prev;
 
  while ((prev->_prev) &&
	 (prev->end == dest->start) &&
	 (sraSpanListEqual(prev->subspan, dest->subspan))) {
    /*
    printf("merge_prev:");
    sraSpanPrint(prev);
    printf(" & ");
    sraSpanPrint(dest);
    printf("\n");
    */
    dest->start = prev->start;
    sraSpanRemove(prev);
    sraSpanDestroy(prev);
    prev = dest->_prev;
  }
}    

static void
sraSpanMergeNext(sraSpan *dest) {
  sraSpan *next = dest->_next;
  while ((next->_next) &&
	 (next->start == dest->end) &&
	 (sraSpanListEqual(next->subspan, dest->subspan))) {
/*
	  printf("merge_next:");
    sraSpanPrint(dest);
    printf(" & ");
    sraSpanPrint(next);
    printf("\n");
	*/
    dest->end = next->end;
    sraSpanRemove(next);
    sraSpanDestroy(next);
    next = dest->_next;
  }
}

static void
sraSpanListOr(sraSpanList *dest, const sraSpanList *src) {
  sraSpan *d_curr, *s_curr;
  int s_start, s_end;

  if (!dest) {
    if (!src) {
      return;
    } else {
      rfbErr("sraSpanListOr:incompatible spans (only one NULL!)\n");
      return;
    }
  }

  d_curr = dest->front._next;
  s_curr = src->front._next;
  s_start = s_curr->start;
  s_end = s_curr->end;
  while (s_curr != &(src->back)) {

    /* - If we are at end of destination list OR
       If the new span comes before the next destination one */
    if ((d_curr == &(dest->back)) ||
		(d_curr->start >= s_end)) {
      /* - Add the span */
      sraSpanInsertBefore(sraSpanCreate(s_start, s_end,
					s_curr->subspan),
			  d_curr);
      if (d_curr != &(dest->back))
	sraSpanMergePrevious(d_curr);
      s_curr = s_curr->_next;
      s_start = s_curr->start;
      s_end = s_curr->end;
    } else {

      /* - If the new span overlaps the existing one */
      if ((s_start < d_curr->end) &&
	  (s_end > d_curr->start)) {

	/* - Insert new span before the existing destination one? */
	if (s_start < d_curr->start) {
	  sraSpanInsertBefore(sraSpanCreate(s_start,
					    d_curr->start,
					    s_curr->subspan),
			      d_curr);
	  sraSpanMergePrevious(d_curr);
	}

	/* Split the existing span if necessary */
	if (s_end < d_curr->end) {
	  sraSpanInsertAfter(sraSpanCreate(s_end,
					   d_curr->end,
					   d_curr->subspan),
			     d_curr);
	  d_curr->end = s_end;
	}
	if (s_start > d_curr->start) {
	  sraSpanInsertBefore(sraSpanCreate(d_curr->start,
					    s_start,
					    d_curr->subspan),
			      d_curr);
	  d_curr->start = s_start;
	}

	/* Recursively OR subspans */
	sraSpanListOr(d_curr->subspan, s_curr->subspan);

	/* Merge this span with previous or next? */
	if (d_curr->_prev != &(dest->front))
	  sraSpanMergePrevious(d_curr);
	if (d_curr->_next != &(dest->back))
	  sraSpanMergeNext(d_curr);

	/* Move onto the next pair to compare */
	if (s_end > d_curr->end) {
	  s_start = d_curr->end;
	  d_curr = d_curr->_next;
	} else {
	  s_curr = s_curr->_next;
	  s_start = s_curr->start;
	  s_end = s_curr->end;
	}
      } else {
	/* - No overlap.  Move to the next destination span */
	d_curr = d_curr->_next;
      }
    }
  }
}

static rfbBool
sraSpanListAnd(sraSpanList *dest, const sraSpanList *src) {
  sraSpan *d_curr, *s_curr, *d_next;

  if (!dest) {
    if (!src) {
      return 1;
    } else {
      rfbErr("sraSpanListAnd:incompatible spans (only one NULL!)\n");
      return FALSE;
    }
  }

  d_curr = dest->front._next;
  s_curr = src->front._next;
  while ((s_curr != &(src->back)) && (d_curr != &(dest->back))) {

    /* - If we haven't reached a destination span yet then move on */
    if (d_curr->start >= s_curr->end) {
      s_curr = s_curr->_next;
      continue;
    }

    /* - If we are beyond the current destination span then remove it */
    if (d_curr->end <= s_curr->start) {
      sraSpan *next = d_curr->_next;
      sraSpanRemove(d_curr);
      sraSpanDestroy(d_curr);
      d_curr = next;
      continue;
    }

    /* - If we partially overlap a span then split it up or remove bits */
    if (s_curr->start > d_curr->start) {
      /* - The top bit of the span does not match */
      d_curr->start = s_curr->start;
    }
    if (s_curr->end < d_curr->end) {
      /* - The end of the span does not match */
      sraSpanInsertAfter(sraSpanCreate(s_curr->end,
				       d_curr->end,
				       d_curr->subspan),
			 d_curr);
      d_curr->end = s_curr->end;
    }

    /* - Now recursively process the affected span */
    if (!sraSpanListAnd(d_curr->subspan, s_curr->subspan)) {
      /* - The destination subspan is now empty, so we should remove it */
		sraSpan *next = d_curr->_next;
      sraSpanRemove(d_curr);
      sraSpanDestroy(d_curr);
      d_curr = next;
    } else {
      /* Merge this span with previous or next? */
      if (d_curr->_prev != &(dest->front))
	sraSpanMergePrevious(d_curr);

      /* - Move on to the next span */
      d_next = d_curr;
      if (s_curr->end >= d_curr->end) {
	d_next = d_curr->_next;
      }
      if (s_curr->end <= d_curr->end) {
	s_curr = s_curr->_next;
      }
      d_curr = d_next;
    }
  }

  while (d_curr != &(dest->back)) {
    sraSpan *next = d_curr->_next;
    sraSpanRemove(d_curr);
    sraSpanDestroy(d_curr);
    d_curr=next;
  }

  return !sraSpanListEmpty(dest);
}

static rfbBool
sraSpanListSubtract(sraSpanList *dest, const sraSpanList *src) {
  sraSpan *d_curr, *s_curr;

  if (!dest) {
    if (!src) {
      return 1;
    } else {
      rfbErr("sraSpanListSubtract:incompatible spans (only one NULL!)\n");
      return FALSE;
    }
  }

  d_curr = dest->front._next;
  s_curr = src->front._next;
  while ((s_curr != &(src->back)) && (d_curr != &(dest->back))) {

    /* - If we haven't reached a destination span yet then move on */
    if (d_curr->start >= s_curr->end) {
      s_curr = s_curr->_next;
      continue;
    }

    /* - If we are beyond the current destination span then skip it */
    if (d_curr->end <= s_curr->start) {
      d_curr = d_curr->_next;
      continue;
    }

    /* - If we partially overlap the current span then split it up */
    if (s_curr->start > d_curr->start) {
      sraSpanInsertBefore(sraSpanCreate(d_curr->start,
					s_curr->start,
					d_curr->subspan),
			  d_curr);
      d_curr->start = s_curr->start;
    }
    if (s_curr->end < d_curr->end) {
      sraSpanInsertAfter(sraSpanCreate(s_curr->end,
				       d_curr->end,
				       d_curr->subspan),
			 d_curr);
      d_curr->end = s_curr->end;
    }

    /* - Now recursively process the affected span */
    if ((!d_curr->subspan) || !sraSpanListSubtract(d_curr->subspan, s_curr->subspan)) {
      /* - The destination subspan is now empty, so we should remove it */
      sraSpan *next = d_curr->_next;
      sraSpanRemove(d_curr);
      sraSpanDestroy(d_curr);
      d_curr = next;
    } else {
      /* Merge this span with previous or next? */
      if (d_curr->_prev != &(dest->front))
	sraSpanMergePrevious(d_curr);
      if (d_curr->_next != &(dest->back))
	sraSpanMergeNext(d_curr);

      /* - Move on to the next span */
      if (s_curr->end > d_curr->end) {
	d_curr = d_curr->_next;
      } else {
	s_curr = s_curr->_next;
      }
    }
  }

  return !sraSpanListEmpty(dest);
}

/* -=- Region routines */

sraRegion *
sraRgnCreate(void) {
  return (sraRegion*)sraSpanListCreate();
}

sraRegion *
sraRgnCreateRect(int x1, int y1, int x2, int y2) {
  sraSpanList *vlist, *hlist;
  sraSpan *vspan, *hspan;

  /* - Build the horizontal portion of the span */
  hlist = sraSpanListCreate();
  hspan = sraSpanCreate(x1, x2, NULL);
  sraSpanInsertAfter(hspan, &(hlist->front));

  /* - Build the vertical portion of the span */
  vlist = sraSpanListCreate();
  vspan = sraSpanCreate(y1, y2, hlist);
  sraSpanInsertAfter(vspan, &(vlist->front));

  sraSpanListDestroy(hlist);

  return (sraRegion*)vlist;
}

sraRegion *
sraRgnCreateRgn(const sraRegion *src) {
  return (sraRegion*)sraSpanListDup((sraSpanList*)src);
}

void
sraRgnDestroy(sraRegion *rgn) {
  sraSpanListDestroy((sraSpanList*)rgn);
}

void
sraRgnMakeEmpty(sraRegion *rgn) {
  sraSpanListMakeEmpty((sraSpanList*)rgn);
}

/* -=- Boolean Region ops */

rfbBool
sraRgnAnd(sraRegion *dst, const sraRegion *src) {
  return sraSpanListAnd((sraSpanList*)dst, (sraSpanList*)src);
}

void
sraRgnOr(sraRegion *dst, const sraRegion *src) {
  sraSpanListOr((sraSpanList*)dst, (sraSpanList*)src);
}

rfbBool
sraRgnSubtract(sraRegion *dst, const sraRegion *src) {
  return sraSpanListSubtract((sraSpanList*)dst, (sraSpanList*)src);
}

void
sraRgnOffset(sraRegion *dst, int dx, int dy) {
  sraSpan *vcurr, *hcurr;

  vcurr = ((sraSpanList*)dst)->front._next;
  while (vcurr != &(((sraSpanList*)dst)->back)) {
    vcurr->start += dy;
    vcurr->end += dy;
    
    hcurr = vcurr->subspan->front._next;
    while (hcurr != &(vcurr->subspan->back)) {
      hcurr->start += dx;
      hcurr->end += dx;
      hcurr = hcurr->_next;
    }

    vcurr = vcurr->_next;
  }
}

sraRegion *sraRgnBBox(const sraRegion *src) {
  int xmin=((unsigned int)(int)-1)>>1,ymin=xmin,xmax=1-xmin,ymax=xmax;
  sraSpan *vcurr, *hcurr;

  if(!src)
    return sraRgnCreate();

  vcurr = ((sraSpanList*)src)->front._next;
  while (vcurr != &(((sraSpanList*)src)->back)) {
    if(vcurr->start<ymin)
      ymin=vcurr->start;
    if(vcurr->end>ymax)
      ymax=vcurr->end;
    
    hcurr = vcurr->subspan->front._next;
    while (hcurr != &(vcurr->subspan->back)) {
      if(hcurr->start<xmin)
	xmin=hcurr->start;
      if(hcurr->end>xmax)
	xmax=hcurr->end;
      hcurr = hcurr->_next;
    }

    vcurr = vcurr->_next;
  }

  if(xmax<xmin || ymax<ymin)
    return sraRgnCreate();

  return sraRgnCreateRect(xmin,ymin,xmax,ymax);
}

rfbBool
sraRgnPopRect(sraRegion *rgn, sraRect *rect, unsigned long flags) {
  sraSpan *vcurr, *hcurr;
  sraSpan *vend, *hend;
  rfbBool right2left = (flags & 2) == 2;
  rfbBool bottom2top = (flags & 1) == 1;

  /* - Pick correct order */
  if (bottom2top) {
    vcurr = ((sraSpanList*)rgn)->back._prev;
    vend = &(((sraSpanList*)rgn)->front);
  } else {
    vcurr = ((sraSpanList*)rgn)->front._next;
    vend = &(((sraSpanList*)rgn)->back);
  }

  if (vcurr != vend) {
    rect->y1 = vcurr->start;
    rect->y2 = vcurr->end;

    /* - Pick correct order */
    if (right2left) {
      hcurr = vcurr->subspan->back._prev;
      hend = &(vcurr->subspan->front);
    } else {
      hcurr = vcurr->subspan->front._next;
      hend = &(vcurr->subspan->back);
    }

    if (hcurr != hend) {
      rect->x1 = hcurr->start;
      rect->x2 = hcurr->end;

      sraSpanRemove(hcurr);
      sraSpanDestroy(hcurr);
      
      if (sraSpanListEmpty(vcurr->subspan)) {
	sraSpanRemove(vcurr);
	sraSpanDestroy(vcurr);
      }

#if 0
      printf("poprect:(%dx%d)-(%dx%d)\n",
	     rect->x1, rect->y1, rect->x2, rect->y2);
#endif
      return 1;
    }
  }

  return 0;
}

unsigned long
sraRgnCountRects(const sraRegion *rgn) {
  unsigned long count = sraSpanListCount((sraSpanList*)rgn);
  return count;
}

rfbBool
sraRgnEmpty(const sraRegion *rgn) {
  return sraSpanListEmpty((sraSpanList*)rgn);
}

/* iterator stuff */
sraRectangleIterator *sraRgnGetIterator(sraRegion *s)
{
  /* these values have to be multiples of 4 */
#define DEFSIZE 4
#define DEFSTEP 8
  sraRectangleIterator *i =
    (sraRectangleIterator*)malloc(sizeof(sraRectangleIterator));
  if(!i)
    return NULL;

  /* we have to recurse eventually. So, the first sPtr is the pointer to
     the sraSpan in the first level. the second sPtr is the pointer to
     the sraRegion.back. The third and fourth sPtr are for the second
     recursion level and so on. */
  i->sPtrs = (sraSpan**)malloc(sizeof(sraSpan*)*DEFSIZE);
  if(!i->sPtrs) {
    free(i);
    return NULL;
  }
  i->ptrSize = DEFSIZE;
  i->sPtrs[0] = &(s->front);
  i->sPtrs[1] = &(s->back);
  i->ptrPos = 0;
  i->reverseX = 0;
  i->reverseY = 0;
  return i;
}

sraRectangleIterator *sraRgnGetReverseIterator(sraRegion *s,rfbBool reverseX,rfbBool reverseY)
{
  sraRectangleIterator *i = sraRgnGetIterator(s);
  if(reverseY) {
    i->sPtrs[1] = &(s->front);
    i->sPtrs[0] = &(s->back);
  }
  i->reverseX = reverseX;
  i->reverseY = reverseY;
  return(i);
}

static rfbBool sraReverse(sraRectangleIterator *i)
{
  return( ((i->ptrPos&2) && i->reverseX) ||
     (!(i->ptrPos&2) && i->reverseY));
}

static sraSpan* sraNextSpan(sraRectangleIterator *i)
{
  if(sraReverse(i))
    return(i->sPtrs[i->ptrPos]->_prev);
  else
    return(i->sPtrs[i->ptrPos]->_next);
}

rfbBool sraRgnIteratorNext(sraRectangleIterator* i,sraRect* r)
{
  /* is the subspan finished? */
  while(sraNextSpan(i) == i->sPtrs[i->ptrPos+1]) {
    i->ptrPos -= 2;
    if(i->ptrPos < 0) /* the end */
      return(0);
  }

  i->sPtrs[i->ptrPos] = sraNextSpan(i);

  /* is this a new subspan? */
  while(i->sPtrs[i->ptrPos]->subspan) {
    if(i->ptrPos+2 > i->ptrSize) { /* array is too small */
      i->ptrSize += DEFSTEP;
      i->sPtrs = (sraSpan**)realloc(i->sPtrs, sizeof(sraSpan*)*i->ptrSize);
    }
    i->ptrPos =+ 2;
    if(sraReverse(i)) {
      i->sPtrs[i->ptrPos]   =   i->sPtrs[i->ptrPos-2]->subspan->back._prev;
      i->sPtrs[i->ptrPos+1] = &(i->sPtrs[i->ptrPos-2]->subspan->front);
    } else {
      i->sPtrs[i->ptrPos]   =   i->sPtrs[i->ptrPos-2]->subspan->front._next;
      i->sPtrs[i->ptrPos+1] = &(i->sPtrs[i->ptrPos-2]->subspan->back);
    }
  }

  if((i->ptrPos%4)!=2) {
    rfbErr("sraRgnIteratorNext: offset is wrong (%d%%4!=2)\n",i->ptrPos);
    return FALSE;
  }

  r->y1 = i->sPtrs[i->ptrPos-2]->start;
  r->y2 = i->sPtrs[i->ptrPos-2]->end;
  r->x1 = i->sPtrs[i->ptrPos]->start;
  r->x2 = i->sPtrs[i->ptrPos]->end;

  return(-1);
}

void sraRgnReleaseIterator(sraRectangleIterator* i)
{
  free(i->sPtrs);
  free(i);
}

void
sraRgnPrint(const sraRegion *rgn) {
	sraSpanListPrint((sraSpanList*)rgn);
}

rfbBool
sraClipRect(int *x, int *y, int *w, int *h,
	    int cx, int cy, int cw, int ch) {
  if (*x < cx) {
    *w -= (cx-*x);
    *x = cx;
  }
  if (*y < cy) {
    *h -= (cy-*y);
    *y = cy;
  }
  if (*x+*w > cx+cw) {
    *w = (cx+cw)-*x;
  }
  if (*y+*h > cy+ch) {
    *h = (cy+ch)-*y;
  }
  return (*w>0) && (*h>0);
}

rfbBool
sraClipRect2(int *x, int *y, int *x2, int *y2,
	    int cx, int cy, int cx2, int cy2) {
  if (*x < cx)
    *x = cx;
  if (*y < cy)
    *y = cy;
  if (*x >= cx2)
    *x = cx2-1;
  if (*y >= cy2)
    *y = cy2-1;
  if (*x2 <= cx)
    *x2 = cx+1;
  if (*y2 <= cy)
    *y2 = cy+1;
  if (*x2 > cx2)
    *x2 = cx2;
  if (*y2 > cy2)
    *y2 = cy2;
  return (*x2>*x) && (*y2>*y);
}

/* test */

#ifdef SRA_TEST
/* pipe the output to sort|uniq -u and you'll get the errors. */
int main(int argc, char** argv)
{
  sraRegionPtr region, region1, region2;
  sraRectangleIterator* i;
  sraRect rect;
  rfbBool b;

  region = sraRgnCreateRect(10, 10, 600, 300);
  region1 = sraRgnCreateRect(40, 50, 350, 200);
  region2 = sraRgnCreateRect(0, 0, 20, 40);

  sraRgnPrint(region);
  printf("\n[(10-300)[(10-600)]]\n\n");

  b = sraRgnSubtract(region, region1);
  printf("%s ",b?"true":"false");
  sraRgnPrint(region);
  printf("\ntrue [(10-50)[(10-600)](50-200)[(10-40)(350-600)](200-300)[(10-600)]]\n\n");

  sraRgnOr(region, region2);
  printf("%ld\n6\n\n", sraRgnCountRects(region));

  i = sraRgnGetIterator(region);
  while(sraRgnIteratorNext(i, &rect))
    printf("%dx%d+%d+%d ",
	   rect.x2-rect.x1,rect.y2-rect.y1,
	   rect.x1,rect.y1);
  sraRgnReleaseIterator(i);
  printf("\n20x10+0+0 600x30+0+10 590x10+10+40 30x150+10+50 250x150+350+50 590x100+10+200 \n\n");

  i = sraRgnGetReverseIterator(region,1,0);
  while(sraRgnIteratorNext(i, &rect))
    printf("%dx%d+%d+%d ",
	   rect.x2-rect.x1,rect.y2-rect.y1,
	   rect.x1,rect.y1);
  sraRgnReleaseIterator(i);
  printf("\n20x10+0+0 600x30+0+10 590x10+10+40 250x150+350+50 30x150+10+50 590x100+10+200 \n\n");

  i = sraRgnGetReverseIterator(region,1,1);
  while(sraRgnIteratorNext(i, &rect))
    printf("%dx%d+%d+%d ",
	   rect.x2-rect.x1,rect.y2-rect.y1,
	   rect.x1,rect.y1);
  sraRgnReleaseIterator(i);
  printf("\n590x100+10+200 250x150+350+50 30x150+10+50 590x10+10+40 600x30+0+10 20x10+0+0 \n\n");

  sraRgnDestroy(region);
  sraRgnDestroy(region1);
  sraRgnDestroy(region2);

  return(0);
}
#endif
///////////////////////////////////////////end rfbregion.c //////////////////////////////////////////////////

///////////////////////////////////////////begin auth.c //////////////////////////////////////////////////
/*
 * auth.c - deal with authentication.
 *
 * This file implements the VNC authentication protocol when setting up an RFB
 * connection.
 */

/*
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

/* RFB 3.8 clients are well informed */
void rfbClientSendString(rfbClientPtr cl, char *reason);


/*
 * Handle security types
 */

static rfbSecurityHandler* securityHandlers = NULL;

/*
 * This method registers a list of new security types.  
 * It avoids same security type getting registered multiple times. 
 * The order is not preserved if multiple security types are
 * registered at one-go.
 */
void
rfbRegisterSecurityHandler(rfbSecurityHandler* handler)
{
	rfbSecurityHandler *head = securityHandlers, *next = NULL;

	if(handler == NULL)
		return;

	next = handler->next;

	while(head != NULL) {
		if(head == handler) {
			rfbRegisterSecurityHandler(next);
			return;
		}

		head = head->next;
	}

	handler->next = securityHandlers;
	securityHandlers = handler;

	rfbRegisterSecurityHandler(next);
}

/*
 * This method unregisters a list of security types. 
 * These security types won't be available for any new
 * client connection. 
 */
void
rfbUnregisterSecurityHandler(rfbSecurityHandler* handler)
{
	rfbSecurityHandler *cur = NULL, *pre = NULL;

	if(handler == NULL)
		return;

	if(securityHandlers == handler) {
		securityHandlers = securityHandlers->next;
		rfbUnregisterSecurityHandler(handler->next);
		return;
	}

	cur = pre = securityHandlers;

	while(cur) {
		if(cur == handler) {
			pre->next = cur->next;
			break;
		}
		pre = cur;
		cur = cur->next;
	}
	rfbUnregisterSecurityHandler(handler->next);
}

/*
 * Send the authentication challenge.
 */

static void
rfbVncAuthSendChallenge(rfbClientPtr cl)
{
	
    /* 4 byte header is alreay sent. Which is rfbSecTypeVncAuth 
       (same as rfbVncAuth). Just send the challenge. */
    rfbRandomBytes(cl->authChallenge);
    if (rfbWriteExact(cl, (char *)cl->authChallenge, CHALLENGESIZE) < 0) {
        rfbLogPerror("rfbAuthNewClient: write");
        rfbCloseClient(cl);
        return;
    }
    
    /* Dispatch client input to rfbVncAuthProcessResponse. */
    cl->state = RFB_AUTHENTICATION;
}

/*
 * Send the NO AUTHENTICATION. SCARR
 */

static void
rfbVncAuthNone(rfbClientPtr cl)
{
    uint32_t authResult;

    if (cl->protocolMajorVersion==3 && cl->protocolMinorVersion > 7) {
        rfbLog("rfbProcessClientSecurityType: returning securityResult for client rfb version >= 3.8\n");
        authResult = Swap32IfLE(rfbVncAuthOK);
        if (rfbWriteExact(cl, (char *)&authResult, 4) < 0) {
            rfbLogPerror("rfbAuthProcessClientMessage: write");
            rfbCloseClient(cl);
            return;
        }
    }
    cl->state = RFB_INITIALISATION;
    return;
}


/*
 * Advertise the supported security types (protocol 3.7). Here before sending 
 * the list of security types to the client one more security type is added 
 * to the list if primaryType is not set to rfbSecTypeInvalid. This security
 * type is the standard vnc security type which does the vnc authentication
 * or it will be security type for no authentication.
 * Different security types will be added by applications using this library.
 */

static rfbSecurityHandler VncSecurityHandlerVncAuth = {
    rfbSecTypeVncAuth,
    rfbVncAuthSendChallenge,
    NULL
};

static rfbSecurityHandler VncSecurityHandlerNone = {
    rfbSecTypeNone,
    rfbVncAuthNone,
    NULL
};
                        

static void
rfbSendSecurityTypeList(rfbClientPtr cl, int primaryType)
{
    /* The size of the message is the count of security types +1,
     * since the first byte is the number of types. */
    int size = 1;
    rfbSecurityHandler* handler;
#define MAX_SECURITY_TYPES 255
    uint8_t buffer[MAX_SECURITY_TYPES+1];


    /* Fill in the list of security types in the client structure. (NOTE: Not really in the client structure) */
    switch (primaryType) {
    case rfbSecTypeNone:
        rfbRegisterSecurityHandler(&VncSecurityHandlerNone);
        break;
    case rfbSecTypeVncAuth:
        rfbRegisterSecurityHandler(&VncSecurityHandlerVncAuth);
        break;
    }

    for (handler = securityHandlers;
	    handler && size<MAX_SECURITY_TYPES; handler = handler->next) {
	buffer[size] = handler->type;
	size++;
    }
    buffer[0] = (unsigned char)size-1;

    /* Send the list. */
    if (rfbWriteExact(cl, (char *)buffer, size) < 0) {
	rfbLogPerror("rfbSendSecurityTypeList: write");
	rfbCloseClient(cl);
	return;
    }

    /*
      * if count is 0, we need to send the reason and close the connection.
      */
    if(size <= 1) {
	/* This means total count is Zero and so reason msg should be sent */
	/* The execution should never reach here */
	char* reason = "No authentication mode is registered!";

	rfbClientSendString(cl, reason);
	return;
    }

    /* Dispatch client input to rfbProcessClientSecurityType. */
    cl->state = RFB_SECURITY_TYPE;
}




/*
 * Tell the client what security type will be used (protocol 3.3).
 */
static void
rfbSendSecurityType(rfbClientPtr cl, int32_t securityType)
{
    uint32_t value32;

    /* Send the value. */
    value32 = Swap32IfLE(securityType);
    if (rfbWriteExact(cl, (char *)&value32, 4) < 0) {
	rfbLogPerror("rfbSendSecurityType: write");
	rfbCloseClient(cl);
	return;
    }

    /* Decide what to do next. */
    switch (securityType) {
    case rfbSecTypeNone:
	/* Dispatch client input to rfbProcessClientInitMessage. */
	cl->state = RFB_INITIALISATION;
	break;
    case rfbSecTypeVncAuth:
	/* Begin the standard VNC authentication procedure. */
	rfbVncAuthSendChallenge(cl);
	break;
    default:
	/* Impossible case (hopefully). */
	rfbLogPerror("rfbSendSecurityType: assertion failed");
	rfbCloseClient(cl);
    }
}



/*
 * rfbAuthNewClient is called right after negotiating the protocol
 * version. Depending on the protocol version, we send either a code
 * for authentication scheme to be used (protocol 3.3), or a list of
 * possible "security types" (protocol 3.7).
 */

void
rfbAuthNewClient(rfbClientPtr cl)
{
    int32_t securityType = rfbSecTypeInvalid;

    if (!cl->screen->authPasswdData || cl->reverseConnection) {
	/* chk if this condition is valid or not. */
	securityType = rfbSecTypeNone;
    } else if (cl->screen->authPasswdData) {
 	    securityType = rfbSecTypeVncAuth;
    }

    if (cl->protocolMajorVersion==3 && cl->protocolMinorVersion < 7)
    {
	/* Make sure we use only RFB 3.3 compatible security types. */
	if (securityType == rfbSecTypeInvalid) {
	    rfbLog("VNC authentication disabled - RFB 3.3 client rejected\n");
	    rfbClientConnFailed(cl, "Your viewer cannot handle required "
				"authentication methods");
	    return;
	}
	rfbSendSecurityType(cl, securityType);
    } else {
	/* Here it's ok when securityType is set to rfbSecTypeInvalid. */
	rfbSendSecurityTypeList(cl, securityType);
    }
}

/*
 * Read the security type chosen by the client (protocol 3.7).
 */

void
rfbProcessClientSecurityType(rfbClientPtr cl)
{
    int n;
    uint8_t chosenType;
    rfbSecurityHandler* handler;
    
    /* Read the security type. */
    n = rfbReadExact(cl, (char *)&chosenType, 1);
    if (n <= 0) {
	if (n == 0)
	    rfbLog("rfbProcessClientSecurityType: client gone\n");
	else
	    rfbLogPerror("rfbProcessClientSecurityType: read");
	rfbCloseClient(cl);
	return;
    }

    /* Make sure it was present in the list sent by the server. */
    for (handler = securityHandlers; handler; handler = handler->next) {
	if (chosenType == handler->type) {
	      rfbLog("rfbProcessClientSecurityType: executing handler for type %d\n", chosenType);
	      handler->handler(cl);
	      return;
	}
    }

    rfbLog("rfbProcessClientSecurityType: wrong security type (%d) requested\n", chosenType);
    rfbCloseClient(cl);
}



/*
 * rfbAuthProcessClientMessage is called when the client sends its
 * authentication response.
 */

void
rfbAuthProcessClientMessage(rfbClientPtr cl)
{
    int n;
    uint8_t response[CHALLENGESIZE];
    uint32_t authResult;

    if ((n = rfbReadExact(cl, (char *)response, CHALLENGESIZE)) <= 0) {
        if (n != 0)
            rfbLogPerror("rfbAuthProcessClientMessage: read");
        rfbCloseClient(cl);
        return;
    }

    if(!cl->screen->passwordCheck(cl,(const char*)response,CHALLENGESIZE)) {
        rfbErr("rfbAuthProcessClientMessage: password check failed\n");
        authResult = Swap32IfLE(rfbVncAuthFailed);
        if (rfbWriteExact(cl, (char *)&authResult, 4) < 0) {
            rfbLogPerror("rfbAuthProcessClientMessage: write");
        }
	/* support RFB 3.8 clients, they expect a reason *why* it was disconnected */
        if (cl->protocolMinorVersion > 7) {
            rfbClientSendString(cl, "password check failed!");
	}
	else
            rfbCloseClient(cl);
        return;
    }

    authResult = Swap32IfLE(rfbVncAuthOK);

    if (rfbWriteExact(cl, (char *)&authResult, 4) < 0) {
        rfbLogPerror("rfbAuthProcessClientMessage: write");
        rfbCloseClient(cl);
        return;
    }

    cl->state = RFB_INITIALISATION;
}
///////////////////////////////////////////end auth.c //////////////////////////////////////////////////

///////////////////////////////////////////begin sockets.c //////////////////////////////////////////////////
/*
 * sockets.c - deal with TCP & UDP sockets.
 *
 * This code should be independent of any changes in the RFB protocol.  It just
 * deals with the X server scheduling stuff, calling rfbNewClientConnection and
 * rfbProcessClientMessage to actually deal with the protocol.  If a socket
 * needs to be closed for any reason then rfbCloseClient should be called, and
 * this in turn will call rfbClientConnectionGone.  To make an active
 * connection out, call rfbConnect - note that this does _not_ call
 * rfbNewClientConnection.
 *
 * This file is divided into two types of function.  Those beginning with
 * "rfb" are specific to sockets using the RFB protocol.  Those without the
 * "rfb" prefix are more general socket routines (which are used by the http
 * code).
 *
 * Thanks to Karl Hakimian for pointing out that some platforms return EAGAIN
 * not EWOULDBLOCK.
 */

/*
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef LIBVNCSERVER_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(__linux__) && defined(NEED_TIMEVAL)
struct timeval 
{
   long int tv_sec,tv_usec;
}
;
#endif

#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include <errno.h>

#ifdef USE_LIBWRAP
#include <syslog.h>
#include <tcpd.h>
int allow_severity=LOG_INFO;
int deny_severity=LOG_WARNING;
#endif

#if defined(WIN32)
#ifndef __MINGW32__
#pragma warning (disable: 4018 4761)
#endif
#define read(sock,buf,len) recv(sock,buf,len,0)
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ETIMEDOUT WSAETIMEDOUT
#define write(sock,buf,len) send(sock,buf,len,0)
#else
#define closesocket close
#endif

int rfbMaxClientWait = 20000;   /* time (ms) after which we decide client has
                                   gone away - needed to stop us hanging */

/*
 * rfbInitSockets sets up the TCP and UDP sockets to listen for RFB
 * connections.  It does nothing if called again.
 */

void
rfbInitSockets(rfbScreenInfoPtr rfbScreen)
{
    in_addr_t iface = rfbScreen->listenInterface;

    if (rfbScreen->socketState!=RFB_SOCKET_INIT)
	return;

    rfbScreen->socketState = RFB_SOCKET_READY;

    if (rfbScreen->inetdSock != -1) {
	const int one = 1;

#ifndef WIN32
	if (fcntl(rfbScreen->inetdSock, F_SETFL, O_NONBLOCK) < 0) {
	    rfbLogPerror("fcntl");
	    return;
	}
#endif

	if (setsockopt(rfbScreen->inetdSock, IPPROTO_TCP, TCP_NODELAY,
		       (char *)&one, sizeof(one)) < 0) {
	    rfbLogPerror("setsockopt");
	    return;
	}

    	FD_ZERO(&(rfbScreen->allFds));
    	FD_SET(rfbScreen->inetdSock, &(rfbScreen->allFds));
    	rfbScreen->maxFd = rfbScreen->inetdSock;
	return;
    }

    if(rfbScreen->autoPort) {
        int i;
        rfbLog("Autoprobing TCP port \n");
        for (i = 5900; i < 6000; i++) {
            if ((rfbScreen->listenSock = rfbListenOnTCPPort(i, iface)) >= 0) {
		rfbScreen->port = i;
		break;
	    }
        }

        if (i >= 6000) {
	    rfbLogPerror("Failure autoprobing");
	    return;
        }

        rfbLog("Autoprobing selected port %d\n", rfbScreen->port);
        FD_ZERO(&(rfbScreen->allFds));
        FD_SET(rfbScreen->listenSock, &(rfbScreen->allFds));
        rfbScreen->maxFd = rfbScreen->listenSock;
    }
    else if(rfbScreen->port>0) {
      rfbLog("Listening for VNC connections on TCP port %d\n", rfbScreen->port);

      if ((rfbScreen->listenSock = rfbListenOnTCPPort(rfbScreen->port, iface)) < 0) {
	rfbLogPerror("ListenOnTCPPort");
	return;
      }

      FD_ZERO(&(rfbScreen->allFds));
      FD_SET(rfbScreen->listenSock, &(rfbScreen->allFds));
      rfbScreen->maxFd = rfbScreen->listenSock;
    }

    if (rfbScreen->udpPort != 0) {
	rfbLog("rfbInitSockets: listening for input on UDP port %d\n",rfbScreen->udpPort);

	if ((rfbScreen->udpSock = rfbListenOnUDPPort(rfbScreen->udpPort, iface)) < 0) {
	    rfbLogPerror("ListenOnUDPPort");
	    return;
	}
	FD_SET(rfbScreen->udpSock, &(rfbScreen->allFds));
	rfbScreen->maxFd = max((int)rfbScreen->udpSock,rfbScreen->maxFd);
    }
}

void rfbShutdownSockets(rfbScreenInfoPtr rfbScreen)
{
    if (rfbScreen->socketState!=RFB_SOCKET_READY)
	return;

    rfbScreen->socketState = RFB_SOCKET_SHUTDOWN;

    if(rfbScreen->inetdSock>-1) {
	closesocket(rfbScreen->inetdSock);
	FD_CLR(rfbScreen->inetdSock,&rfbScreen->allFds);
	rfbScreen->inetdSock=-1;
    }

    if(rfbScreen->listenSock>-1) {
	closesocket(rfbScreen->listenSock);
	FD_CLR(rfbScreen->listenSock,&rfbScreen->allFds);
	rfbScreen->listenSock=-1;
    }

    if(rfbScreen->udpSock>-1) {
	closesocket(rfbScreen->udpSock);
	FD_CLR(rfbScreen->udpSock,&rfbScreen->allFds);
	rfbScreen->udpSock=-1;
    }
}

/*
 * rfbCheckFds is called from ProcessInputEvents to check for input on the RFB
 * socket(s).  If there is input to process, the appropriate function in the
 * RFB server code will be called (rfbNewClientConnection,
 * rfbProcessClientMessage, etc).
 */

int
rfbCheckFds(rfbScreenInfoPtr rfbScreen,long usec)
{
    int nfds;
    fd_set fds;
    struct timeval tv;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char buf[6];
    const int one = 1;
    int sock;
    rfbClientIteratorPtr i;
    rfbClientPtr cl;
    int result = 0;

    if (!rfbScreen->inetdInitDone && rfbScreen->inetdSock != -1) {
	rfbNewClientConnection(rfbScreen,rfbScreen->inetdSock); 
	rfbScreen->inetdInitDone = TRUE;
    }

    do {
	memcpy((char *)&fds, (char *)&(rfbScreen->allFds), sizeof(fd_set));
	tv.tv_sec = 0;
	tv.tv_usec = usec;
	nfds = select(rfbScreen->maxFd + 1, &fds, NULL, NULL /* &fds */, &tv);
	if (nfds == 0) {
	    /* timed out, check for async events */
            i = rfbGetClientIterator(rfbScreen);
            while((cl = rfbClientIteratorNext(i))) {
                if (cl->onHold)
                    continue;
                if (FD_ISSET(cl->sock, &(rfbScreen->allFds)))
                    rfbSendFileTransferChunk(cl);
            }
            rfbReleaseClientIterator(i);
	    return result;
	}

	if (nfds < 0) {
#ifdef WIN32
	    errno = WSAGetLastError();
#endif
	    if (errno != EINTR)
		rfbLogPerror("rfbCheckFds: select");
	    return -1;
	}

	result += nfds;

	if (rfbScreen->listenSock != -1 && FD_ISSET(rfbScreen->listenSock, &fds)) {

	    if ((sock = accept(rfbScreen->listenSock,
			    (struct sockaddr *)&addr, &addrlen)) < 0) {
		rfbLogPerror("rfbCheckFds: accept");
		return -1;
	    }

#ifndef WIN32
	    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
		rfbLogPerror("rfbCheckFds: fcntl");
		closesocket(sock);
		return -1;
	    }
#endif

	    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
			(char *)&one, sizeof(one)) < 0) {
		rfbLogPerror("rfbCheckFds: setsockopt");
		closesocket(sock);
		return -1;
	    }

#ifdef USE_LIBWRAP
	    if(!hosts_ctl("vnc",STRING_UNKNOWN,inet_ntoa(addr.sin_addr),
			STRING_UNKNOWN)) {
		rfbLog("Rejected connection from client %s\n",
			inet_ntoa(addr.sin_addr));
		closesocket(sock);
		return -1;
	    }
#endif

	    rfbLog("Got connection from client %s\n", inet_ntoa(addr.sin_addr));

	    rfbNewClient(rfbScreen,sock);

	    FD_CLR(rfbScreen->listenSock, &fds);
	    if (--nfds == 0)
		return result;
	}

	if ((rfbScreen->udpSock != -1) && FD_ISSET(rfbScreen->udpSock, &fds)) {
	    if(!rfbScreen->udpClient)
		rfbNewUDPClient(rfbScreen);
	    if (recvfrom(rfbScreen->udpSock, buf, 1, MSG_PEEK,
			(struct sockaddr *)&addr, &addrlen) < 0) {
		rfbLogPerror("rfbCheckFds: UDP: recvfrom");
		rfbDisconnectUDPSock(rfbScreen);
		rfbScreen->udpSockConnected = FALSE;
	    } else {
		if (!rfbScreen->udpSockConnected ||
			(memcmp(&addr, &rfbScreen->udpRemoteAddr, addrlen) != 0))
		{
		    /* new remote end */
		    rfbLog("rfbCheckFds: UDP: got connection\n");

		    memcpy(&rfbScreen->udpRemoteAddr, &addr, addrlen);
		    rfbScreen->udpSockConnected = TRUE;

		    if (connect(rfbScreen->udpSock,
				(struct sockaddr *)&addr, addrlen) < 0) {
			rfbLogPerror("rfbCheckFds: UDP: connect");
			rfbDisconnectUDPSock(rfbScreen);
			return -1;
		    }

		    rfbNewUDPConnection(rfbScreen,rfbScreen->udpSock);
		}

		rfbProcessUDPInput(rfbScreen);
	    }

	    FD_CLR(rfbScreen->udpSock, &fds);
	    if (--nfds == 0)
		return result;
	}

	i = rfbGetClientIterator(rfbScreen);
	while((cl = rfbClientIteratorNext(i))) {

	    if (cl->onHold)
		continue;

            if (FD_ISSET(cl->sock, &(rfbScreen->allFds)))
            {
                if (FD_ISSET(cl->sock, &fds))
                    rfbProcessClientMessage(cl);
                else
                    rfbSendFileTransferChunk(cl);
            }
	}
	rfbReleaseClientIterator(i);
    } while(rfbScreen->handleEventsEagerly);
    return result;
}


void
rfbDisconnectUDPSock(rfbScreenInfoPtr rfbScreen)
{
  rfbScreen->udpSockConnected = FALSE;
}



void
rfbCloseClient(rfbClientPtr cl)
{
    rfbExtensionData* extension;

    for(extension=cl->extensions; extension; extension=extension->next)
	if(extension->extension->close)
	    extension->extension->close(cl, extension->data);

    LOCK(cl->updateMutex);
#ifdef LIBVNCSERVER_HAVE_LIBPTHREAD
    if (cl->sock != -1)
#endif
      {
	FD_CLR(cl->sock,&(cl->screen->allFds));
	if(cl->sock==cl->screen->maxFd)
	  while(cl->screen->maxFd>0
		&& !FD_ISSET(cl->screen->maxFd,&(cl->screen->allFds)))
	    cl->screen->maxFd--;
#ifndef __MINGW32__
	shutdown(cl->sock,SHUT_RDWR);
#endif
	closesocket(cl->sock);
	cl->sock = -1;
      }
    TSIGNAL(cl->updateCond);
    UNLOCK(cl->updateMutex);
}


/*
 * rfbConnect is called to make a connection out to a given TCP address.
 */

int
rfbConnect(rfbScreenInfoPtr rfbScreen,
           char *host,
           int port)
{
    int sock;
    int one = 1;

    rfbLog("Making connection to client on host %s port %d\n",
	   host,port);

    if ((sock = rfbConnectToTcpAddr(host, port)) < 0) {
	rfbLogPerror("connection failed");
	return -1;
    }

#ifndef WIN32
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
	rfbLogPerror("fcntl failed");
	closesocket(sock);
	return -1;
    }
#endif

    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		   (char *)&one, sizeof(one)) < 0) {
	rfbLogPerror("setsockopt failed");
	closesocket(sock);
	return -1;
    }

    /* AddEnabledDevice(sock); */
    FD_SET(sock, &rfbScreen->allFds);
    rfbScreen->maxFd = max(sock,rfbScreen->maxFd);

    return sock;
}

/*
 * ReadExact reads an exact number of bytes from a client.  Returns 1 if
 * those bytes have been read, 0 if the other end has closed, or -1 if an error
 * occurred (errno is set to ETIMEDOUT if it timed out).
 */

int
rfbReadExactTimeout(rfbClientPtr cl, char* buf, int len, int timeout)
{
    int sock = cl->sock;
    int n;
    fd_set fds;
    struct timeval tv;

    while (len > 0) {
        n = read(sock, buf, len);

        if (n > 0) {

            buf += n;
            len -= n;

        } else if (n == 0) {

            return 0;

        } else {
#ifdef WIN32
	    errno = WSAGetLastError();
#endif
	    if (errno == EINTR)
		continue;

#ifdef LIBVNCSERVER_ENOENT_WORKAROUND
	    if (errno != ENOENT)
#endif
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                return n;
            }

            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            tv.tv_sec = timeout / 1000;
            tv.tv_usec = (timeout % 1000) * 1000;
            n = select(sock+1, &fds, NULL, &fds, &tv);
            if (n < 0) {
                rfbLogPerror("ReadExact: select");
                return n;
            }
            if (n == 0) {
                errno = ETIMEDOUT;
                return -1;
            }
        }
    }
#undef DEBUG_READ_EXACT
#ifdef DEBUG_READ_EXACT
    rfbLog("ReadExact %d bytes\n",len);
    for(n=0;n<len;n++)
	    fprintf(stderr,"%02x ",(unsigned char)buf[n]);
    fprintf(stderr,"\n");
#endif

    return 1;
}

int rfbReadExact(rfbClientPtr cl,char* buf,int len)
{
  return(rfbReadExactTimeout(cl,buf,len,rfbMaxClientWait));
}

/*
 * WriteExact writes an exact number of bytes to a client.  Returns 1 if
 * those bytes have been written, or -1 if an error occurred (errno is set to
 * ETIMEDOUT if it timed out).
 */

int
rfbWriteExact(rfbClientPtr cl,
              const char *buf,
              int len)
{
    int sock = cl->sock;
    int n;
    fd_set fds;
    struct timeval tv;
    int totalTimeWaited = 0;

#undef DEBUG_WRITE_EXACT
#ifdef DEBUG_WRITE_EXACT
    rfbLog("WriteExact %d bytes\n",len);
    for(n=0;n<len;n++)
	    fprintf(stderr,"%02x ",(unsigned char)buf[n]);
    fprintf(stderr,"\n");
#endif

    LOCK(cl->outputMutex);
    while (len > 0) {
        n = write(sock, buf, len);

        if (n > 0) {

            buf += n;
            len -= n;

        } else if (n == 0) {

            rfbErr("WriteExact: write returned 0?\n");
            return 0;

        } else {
#ifdef WIN32
			errno = WSAGetLastError();
#endif
	    if (errno == EINTR)
		continue;

            if (errno != EWOULDBLOCK && errno != EAGAIN) {
	        UNLOCK(cl->outputMutex);
                return n;
            }

            /* Retry every 5 seconds until we exceed rfbMaxClientWait.  We
               need to do this because select doesn't necessarily return
               immediately when the other end has gone away */

            FD_ZERO(&fds);
            FD_SET(sock, &fds);
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            n = select(sock+1, NULL, &fds, NULL /* &fds */, &tv);
	    if (n < 0) {
       	        if(errno==EINTR)
		    continue;
                rfbLogPerror("WriteExact: select");
                UNLOCK(cl->outputMutex);
                return n;
            }
            if (n == 0) {
                totalTimeWaited += 5000;
                if (totalTimeWaited >= rfbMaxClientWait) {
                    errno = ETIMEDOUT;
                    UNLOCK(cl->outputMutex);
                    return -1;
                }
            } else {
                totalTimeWaited = 0;
            }
        }
    }
    UNLOCK(cl->outputMutex);
    return 1;
}

/* currently private, called by rfbProcessArguments() */
int
rfbStringToAddr(char *str, in_addr_t *addr)  {
    if (str == NULL || *str == '\0' || strcmp(str, "any") == 0) {
        *addr = htonl(INADDR_ANY);
    } else if (strcmp(str, "localhost") == 0) {
        *addr = htonl(INADDR_LOOPBACK);
    } else {
        struct hostent *hp;
        if ((*addr = inet_addr(str)) == htonl(INADDR_NONE)) {
            if (!(hp = gethostbyname(str))) {
                return 0;
            }
            *addr = *(unsigned long *)hp->h_addr;
        }
    }
    return 1;
}

int
rfbListenOnTCPPort(int port,
                   in_addr_t iface)
{
    struct sockaddr_in addr;
    int sock;
    int one = 1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = iface;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof(one)) < 0) {
	closesocket(sock);
	return -1;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	closesocket(sock);
	return -1;
    }
    if (listen(sock, 5) < 0) {
	closesocket(sock);
	return -1;
    }

    return sock;
}

int
rfbConnectToTcpAddr(char *host,
                    int port)
{
    struct hostent *hp;
    int sock;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if ((addr.sin_addr.s_addr = inet_addr(host)) == htonl(INADDR_NONE))
    {
	if (!(hp = gethostbyname(host))) {
	    errno = EINVAL;
	    return -1;
	}
	addr.sin_addr.s_addr = *(unsigned long *)hp->h_addr;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, (sizeof(addr))) < 0) {
	closesocket(sock);
	return -1;
    }

    return sock;
}

int
rfbListenOnUDPPort(int port,
                   in_addr_t iface)
{
    struct sockaddr_in addr;
    int sock;
    int one = 1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = iface;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	return -1;
    }
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		   (char *)&one, sizeof(one)) < 0) {
	return -1;
    }
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
	return -1;
    }

    return sock;
}

///////////////////////////////////////////end sockets.c //////////////////////////////////////////////////

///////////////////////////////////////////begin stats.c //////////////////////////////////////////////////
/*
 * stats.c
 */

/*
 *  Copyright (C) 2002 RealVNC Ltd.
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

char *messageNameServer2Client(uint32_t type, char *buf, int len);
char *messageNameClient2Server(uint32_t type, char *buf, int len);
char *encodingName(uint32_t enc, char *buf, int len);

rfbStatList *rfbStatLookupEncoding(rfbClientPtr cl, uint32_t type);
rfbStatList *rfbStatLookupMessage(rfbClientPtr cl, uint32_t type);

void  rfbStatRecordEncodingSent(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw);
void  rfbStatRecordEncodingRcvd(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw);
void  rfbStatRecordMessageSent(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw);
void  rfbStatRecordMessageRcvd(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw);
void rfbResetStats(rfbClientPtr cl);
void rfbPrintStats(rfbClientPtr cl);




char *messageNameServer2Client(uint32_t type, char *buf, int len) {
    if (buf==NULL) return "error";
    switch (type) {
    case rfbFramebufferUpdate:        snprintf(buf, len, "FramebufferUpdate"); break;
    case rfbSetColourMapEntries:      snprintf(buf, len, "SetColourMapEntries"); break;
    case rfbBell:                     snprintf(buf, len, "Bell"); break;
    case rfbServerCutText:            snprintf(buf, len, "ServerCutText"); break;
    case rfbResizeFrameBuffer:        snprintf(buf, len, "ResizeFrameBuffer"); break;
    case rfbKeyFrameUpdate:           snprintf(buf, len, "KeyFrameUpdate"); break;
    case rfbFileTransfer:             snprintf(buf, len, "FileTransfer"); break;
    case rfbTextChat:                 snprintf(buf, len, "TextChat"); break;
    case rfbPalmVNCReSizeFrameBuffer: snprintf(buf, len, "PalmVNCReSize"); break;
    default:
        snprintf(buf, len, "svr2cli-0x%08X", 0xFF);
    }
    return buf;
}

char *messageNameClient2Server(uint32_t type, char *buf, int len) {
    if (buf==NULL) return "error";
    switch (type) {
    case rfbSetPixelFormat:           snprintf(buf, len, "SetPixelFormat"); break;
    case rfbFixColourMapEntries:      snprintf(buf, len, "FixColourMapEntries"); break;
    case rfbSetEncodings:             snprintf(buf, len, "SetEncodings"); break;
    case rfbFramebufferUpdateRequest: snprintf(buf, len, "FramebufferUpdate"); break;
    case rfbKeyEvent:                 snprintf(buf, len, "KeyEvent"); break;
    case rfbPointerEvent:             snprintf(buf, len, "PointerEvent"); break;
    case rfbClientCutText:            snprintf(buf, len, "ClientCutText"); break;
    case rfbFileTransfer:             snprintf(buf, len, "FileTransfer"); break;
    case rfbSetScale:                 snprintf(buf, len, "SetScale"); break;
    case rfbSetServerInput:           snprintf(buf, len, "SetServerInput"); break;
    case rfbSetSW:                    snprintf(buf, len, "SetSingleWindow"); break;
    case rfbTextChat:                 snprintf(buf, len, "TextChat"); break;
    case rfbKeyFrameRequest:          snprintf(buf, len, "KeyFrameRequest"); break;
    case rfbPalmVNCSetScaleFactor:    snprintf(buf, len, "PalmVNCSetScale"); break;
    default:
        snprintf(buf, len, "cli2svr-0x%08X", type);


    }
    return buf;
}

/* Encoding name must be <=16 characters to fit nicely on the status output in
 * an 80 column terminal window
 */
char *encodingName(uint32_t type, char *buf, int len) {
    if (buf==NULL) return "error";
    
    switch (type) {
    case rfbEncodingRaw:                snprintf(buf, len, "raw");         break;
    case rfbEncodingCopyRect:           snprintf(buf, len, "copyRect");    break;
    case rfbEncodingRRE:                snprintf(buf, len, "RRE");         break;
    case rfbEncodingCoRRE:              snprintf(buf, len, "CoRRE");       break;
    case rfbEncodingHextile:            snprintf(buf, len, "hextile");     break;
    case rfbEncodingZlib:               snprintf(buf, len, "zlib");        break;
    case rfbEncodingTight:              snprintf(buf, len, "tight");       break;
    case rfbEncodingZlibHex:            snprintf(buf, len, "zlibhex");     break;
    case rfbEncodingUltra:              snprintf(buf, len, "ultra");       break;
    case rfbEncodingZRLE:               snprintf(buf, len, "ZRLE");        break;
    case rfbEncodingZYWRLE:             snprintf(buf, len, "ZYWRLE");      break;
    case rfbEncodingCache:              snprintf(buf, len, "cache");       break;
    case rfbEncodingCacheEnable:        snprintf(buf, len, "cacheEnable"); break;
    case rfbEncodingXOR_Zlib:           snprintf(buf, len, "xorZlib");     break;
    case rfbEncodingXORMonoColor_Zlib:  snprintf(buf, len, "xorMonoZlib");  break;
    case rfbEncodingXORMultiColor_Zlib: snprintf(buf, len, "xorColorZlib"); break;
    case rfbEncodingSolidColor:         snprintf(buf, len, "solidColor");  break;
    case rfbEncodingXOREnable:          snprintf(buf, len, "xorEnable");   break;
    case rfbEncodingCacheZip:           snprintf(buf, len, "cacheZip");    break;
    case rfbEncodingSolMonoZip:         snprintf(buf, len, "monoZip");     break;
    case rfbEncodingUltraZip:           snprintf(buf, len, "ultraZip");    break;

    case rfbEncodingXCursor:            snprintf(buf, len, "Xcursor");     break;
    case rfbEncodingRichCursor:         snprintf(buf, len, "RichCursor");  break;
    case rfbEncodingPointerPos:         snprintf(buf, len, "PointerPos");  break;

    case rfbEncodingLastRect:           snprintf(buf, len, "LastRect");    break;
    case rfbEncodingNewFBSize:          snprintf(buf, len, "NewFBSize");   break;
    case rfbEncodingKeyboardLedState:   snprintf(buf, len, "LedState");    break;
    case rfbEncodingSupportedMessages:  snprintf(buf, len, "SupportedMessage");  break;
    case rfbEncodingSupportedEncodings: snprintf(buf, len, "SupportedEncoding"); break;
    case rfbEncodingServerIdentity:     snprintf(buf, len, "ServerIdentify");    break;

    /* The following lookups do not report in stats */
    case rfbEncodingCompressLevel0: snprintf(buf, len, "CompressLevel0");  break;
    case rfbEncodingCompressLevel1: snprintf(buf, len, "CompressLevel1");  break;
    case rfbEncodingCompressLevel2: snprintf(buf, len, "CompressLevel2");  break;
    case rfbEncodingCompressLevel3: snprintf(buf, len, "CompressLevel3");  break;
    case rfbEncodingCompressLevel4: snprintf(buf, len, "CompressLevel4");  break;
    case rfbEncodingCompressLevel5: snprintf(buf, len, "CompressLevel5");  break;
    case rfbEncodingCompressLevel6: snprintf(buf, len, "CompressLevel6");  break;
    case rfbEncodingCompressLevel7: snprintf(buf, len, "CompressLevel7");  break;
    case rfbEncodingCompressLevel8: snprintf(buf, len, "CompressLevel8");  break;
    case rfbEncodingCompressLevel9: snprintf(buf, len, "CompressLevel9");  break;
    
    case rfbEncodingQualityLevel0:  snprintf(buf, len, "QualityLevel0");   break;
    case rfbEncodingQualityLevel1:  snprintf(buf, len, "QualityLevel1");   break;
    case rfbEncodingQualityLevel2:  snprintf(buf, len, "QualityLevel2");   break;
    case rfbEncodingQualityLevel3:  snprintf(buf, len, "QualityLevel3");   break;
    case rfbEncodingQualityLevel4:  snprintf(buf, len, "QualityLevel4");   break;
    case rfbEncodingQualityLevel5:  snprintf(buf, len, "QualityLevel5");   break;
    case rfbEncodingQualityLevel6:  snprintf(buf, len, "QualityLevel6");   break;
    case rfbEncodingQualityLevel7:  snprintf(buf, len, "QualityLevel7");   break;
    case rfbEncodingQualityLevel8:  snprintf(buf, len, "QualityLevel8");   break;
    case rfbEncodingQualityLevel9:  snprintf(buf, len, "QualityLevel9");   break;


    default:
        snprintf(buf, len, "Enc(0x%08X)", type);
    }

    return buf;
}





rfbStatList *rfbStatLookupEncoding(rfbClientPtr cl, uint32_t type)
{
    rfbStatList *ptr;
    if (cl==NULL) return NULL;
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
    {
        if (ptr->type==type) return ptr;
    }
    /* Well, we are here... need to *CREATE* an entry */
    ptr = (rfbStatList *)malloc(sizeof(rfbStatList));
    if (ptr!=NULL)
    {
        memset((char *)ptr, 0, sizeof(rfbStatList));
        ptr->type = type;
        /* add to the top of the list */
        ptr->Next = cl->statEncList;
        cl->statEncList = ptr;
    }
    return ptr;
}


rfbStatList *rfbStatLookupMessage(rfbClientPtr cl, uint32_t type)
{
    rfbStatList *ptr;
    if (cl==NULL) return NULL;
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
    {
        if (ptr->type==type) return ptr;
    }
    /* Well, we are here... need to *CREATE* an entry */
    ptr = (rfbStatList *)malloc(sizeof(rfbStatList));
    if (ptr!=NULL)
    {
        memset((char *)ptr, 0, sizeof(rfbStatList));
        ptr->type = type;
        /* add to the top of the list */
        ptr->Next = cl->statMsgList;
        cl->statMsgList = ptr;
    }
    return ptr;
}

void rfbStatRecordEncodingSentAdd(rfbClientPtr cl, uint32_t type, int byteCount) /* Specifically for tight encoding */
{
    rfbStatList *ptr;

    ptr = rfbStatLookupEncoding(cl, type);
    if (ptr!=NULL)
        ptr->bytesSent      += byteCount;
}


void  rfbStatRecordEncodingSent(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw)
{
    rfbStatList *ptr;

    ptr = rfbStatLookupEncoding(cl, type);
    if (ptr!=NULL)
    {
        ptr->sentCount++;
        ptr->bytesSent      += byteCount;
        ptr->bytesSentIfRaw += byteIfRaw;
    }
}

void  rfbStatRecordEncodingRcvd(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw)
{
    rfbStatList *ptr;

    ptr = rfbStatLookupEncoding(cl, type);
    if (ptr!=NULL)
    {
        ptr->rcvdCount++;
        ptr->bytesRcvd      += byteCount;
        ptr->bytesRcvdIfRaw += byteIfRaw;
    }
}

void  rfbStatRecordMessageSent(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw)
{
    rfbStatList *ptr;

    ptr = rfbStatLookupMessage(cl, type);
    if (ptr!=NULL)
    {
        ptr->sentCount++;
        ptr->bytesSent      += byteCount;
        ptr->bytesSentIfRaw += byteIfRaw;
    }
}

void  rfbStatRecordMessageRcvd(rfbClientPtr cl, uint32_t type, int byteCount, int byteIfRaw)
{
    rfbStatList *ptr;

    ptr = rfbStatLookupMessage(cl, type);
    if (ptr!=NULL)
    {
        ptr->rcvdCount++;
        ptr->bytesRcvd      += byteCount;
        ptr->bytesRcvdIfRaw += byteIfRaw;
    }
}


int rfbStatGetSentBytes(rfbClientPtr cl)
{
    rfbStatList *ptr=NULL;
    int bytes=0;
    if (cl==NULL) return 0;
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesSent;
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesSent;
    return bytes;
}

int rfbStatGetSentBytesIfRaw(rfbClientPtr cl)
{
    rfbStatList *ptr=NULL;
    int bytes=0;
    if (cl==NULL) return 0;
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesSentIfRaw;
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesSentIfRaw;
    return bytes;
}

int rfbStatGetRcvdBytes(rfbClientPtr cl)
{
    rfbStatList *ptr=NULL;
    int bytes=0;
    if (cl==NULL) return 0;
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesRcvd;
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesRcvd;
    return bytes;
}

int rfbStatGetRcvdBytesIfRaw(rfbClientPtr cl)
{
    rfbStatList *ptr=NULL;
    int bytes=0;
    if (cl==NULL) return 0;
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesRcvdIfRaw;
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
        bytes += ptr->bytesRcvdIfRaw;
    return bytes;
}

int rfbStatGetMessageCountSent(rfbClientPtr cl, uint32_t type)
{
  rfbStatList *ptr=NULL;
    if (cl==NULL) return 0;
  for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
      if (ptr->type==type) return ptr->sentCount;
  return 0;
}
int rfbStatGetMessageCountRcvd(rfbClientPtr cl, uint32_t type)
{
  rfbStatList *ptr=NULL;
    if (cl==NULL) return 0;
  for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
      if (ptr->type==type) return ptr->rcvdCount;
  return 0;
}

int rfbStatGetEncodingCountSent(rfbClientPtr cl, uint32_t type)
{
  rfbStatList *ptr=NULL;
    if (cl==NULL) return 0;
  for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
      if (ptr->type==type) return ptr->sentCount;
  return 0;
}
int rfbStatGetEncodingCountRcvd(rfbClientPtr cl, uint32_t type)
{
  rfbStatList *ptr=NULL;
    if (cl==NULL) return 0;
  for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
      if (ptr->type==type) return ptr->rcvdCount;
  return 0;
}




void rfbResetStats(rfbClientPtr cl)
{
    rfbStatList *ptr;
    if (cl==NULL) return;
    while (cl->statEncList!=NULL)
    {
        ptr = cl->statEncList;
        cl->statEncList = ptr->Next;
        free(ptr);
    }
    while (cl->statMsgList!=NULL)
    {
        ptr = cl->statMsgList;
        cl->statMsgList = ptr->Next;
        free(ptr);
    }
}


void rfbPrintStats(rfbClientPtr cl)
{
    rfbStatList *ptr=NULL;
    char encBuf[64];
    double savings=0.0;
    int    totalRects=0;
    double totalBytes=0.0;
    double totalBytesIfRaw=0.0;

    char *name=NULL;
    int bytes=0;
    int bytesIfRaw=0;
    int count=0;

    if (cl==NULL) return;
    
    rfbLog("%-21.21s  %-6.6s   %9.9s/%9.9s (%6.6s)\n", "Statistics", "events", "Transmit","RawEquiv","saved");
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
    {
        name       = messageNameServer2Client(ptr->type, encBuf, sizeof(encBuf));
        count      = ptr->sentCount;
        bytes      = ptr->bytesSent;
        bytesIfRaw = ptr->bytesSentIfRaw;
        
        savings = 0.0;
        if (bytesIfRaw>0.0)
            savings = 100.0 - (((double)bytes / (double)bytesIfRaw) * 100.0);
        if ((bytes>0) || (count>0) || (bytesIfRaw>0))
            rfbLog(" %-20.20s: %6d | %9d/%9d (%5.1f%%)\n",
	        name, count, bytes, bytesIfRaw, savings);
        totalRects += count;
        totalBytes += bytes;
        totalBytesIfRaw += bytesIfRaw;
    }

    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
    {
        name       = encodingName(ptr->type, encBuf, sizeof(encBuf));
        count      = ptr->sentCount;
        bytes      = ptr->bytesSent;
        bytesIfRaw = ptr->bytesSentIfRaw;
        savings    = 0.0;

        if (bytesIfRaw>0.0)
            savings = 100.0 - (((double)bytes / (double)bytesIfRaw) * 100.0);
        if ((bytes>0) || (count>0) || (bytesIfRaw>0))
            rfbLog(" %-20.20s: %6d | %9d/%9d (%5.1f%%)\n",
	        name, count, bytes, bytesIfRaw, savings);
        totalRects += count;
        totalBytes += bytes;
        totalBytesIfRaw += bytesIfRaw;
    }
    savings=0.0;
    if (totalBytesIfRaw>0.0)
        savings = 100.0 - ((totalBytes/totalBytesIfRaw)*100.0);
    rfbLog(" %-20.20s: %6d | %9.0f/%9.0f (%5.1f%%)\n",
            "TOTALS", totalRects, totalBytes,totalBytesIfRaw, savings);

    totalRects=0.0;
    totalBytes=0.0;
    totalBytesIfRaw=0.0;

    rfbLog("%-21.21s  %-6.6s   %9.9s/%9.9s (%6.6s)\n", "Statistics", "events", "Received","RawEquiv","saved");
    for (ptr = cl->statMsgList; ptr!=NULL; ptr=ptr->Next)
    {
        name       = messageNameClient2Server(ptr->type, encBuf, sizeof(encBuf));
        count      = ptr->rcvdCount;
        bytes      = ptr->bytesRcvd;
        bytesIfRaw = ptr->bytesRcvdIfRaw;
        savings    = 0.0;

        if (bytesIfRaw>0.0)
            savings = 100.0 - (((double)bytes / (double)bytesIfRaw) * 100.0);
        if ((bytes>0) || (count>0) || (bytesIfRaw>0))
            rfbLog(" %-20.20s: %6d | %9d/%9d (%5.1f%%)\n",
	        name, count, bytes, bytesIfRaw, savings);
        totalRects += count;
        totalBytes += bytes;
        totalBytesIfRaw += bytesIfRaw;
    }
    for (ptr = cl->statEncList; ptr!=NULL; ptr=ptr->Next)
    {
        name       = encodingName(ptr->type, encBuf, sizeof(encBuf));
        count      = ptr->rcvdCount;
        bytes      = ptr->bytesRcvd;
        bytesIfRaw = ptr->bytesRcvdIfRaw;
        savings    = 0.0;

        if (bytesIfRaw>0.0)
            savings = 100.0 - (((double)bytes / (double)bytesIfRaw) * 100.0);
        if ((bytes>0) || (count>0) || (bytesIfRaw>0))
            rfbLog(" %-20.20s: %6d | %9d/%9d (%5.1f%%)\n",
	        name, count, bytes, bytesIfRaw, savings);
        totalRects += count;
        totalBytes += bytes;
        totalBytesIfRaw += bytesIfRaw;
    }
    savings=0.0;
    if (totalBytesIfRaw>0.0)
        savings = 100.0 - ((totalBytes/totalBytesIfRaw)*100.0);
    rfbLog(" %-20.20s: %6d | %9.0f/%9.0f (%5.1f%%)\n",
            "TOTALS", totalRects, totalBytes,totalBytesIfRaw, savings);
      
} 

///////////////////////////////////////////end stats.c //////////////////////////////////////////////////

///////////////////////////////////////////begin corre.c //////////////////////////////////////////////////
/*
 * corre.c
 *
 * Routines to implement Compact Rise-and-Run-length Encoding (CoRRE).  This
 * code is based on krw's original javatel rfbserver.
 */

/*
 *  Copyright (C) 2002 RealVNC Ltd.
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

/*
 * rreBeforeBuf contains pixel data in the client's format.
 * rreAfterBuf contains the RRE encoded version.  If the RRE encoded version is
 * larger than the raw data or if it exceeds rreAfterBufSize then
 * raw encoding is used instead.
 */

static int rreBeforeBufSize = 0;
static char *rreBeforeBuf = NULL;

static int rreAfterBufSize = 0;
static char *rreAfterBuf = NULL;
static int rreAfterBufLen = 0;

static int subrectEncode8(uint8_t *data, int w, int h);
static int subrectEncode16(uint16_t *data, int w, int h);
static int subrectEncode32(uint32_t *data, int w, int h);
static uint32_t getBgColour(char *data, int size, int bpp);
static rfbBool rfbSendSmallRectEncodingCoRRE(rfbClientPtr cl, int x, int y,
                                          int w, int h);

void rfbCoRRECleanup(rfbScreenInfoPtr screen)
{
  if (rreBeforeBufSize) {
    free(rreBeforeBuf);
    rreBeforeBufSize=0;
  }
  if (rreAfterBufSize) {
    free(rreAfterBuf);
    rreAfterBufSize=0;
  }
}

/*
 * rfbSendRectEncodingCoRRE - send an arbitrary size rectangle using CoRRE
 * encoding.
 */

rfbBool
rfbSendRectEncodingCoRRE(rfbClientPtr cl,
                         int x,
                         int y,
                         int w,
                         int h)
{
    if (h > cl->correMaxHeight) {
        return (rfbSendRectEncodingCoRRE(cl, x, y, w, cl->correMaxHeight) &&
		rfbSendRectEncodingCoRRE(cl, x, y + cl->correMaxHeight, w,
					 h - cl->correMaxHeight));
    }

    if (w > cl->correMaxWidth) {
        return (rfbSendRectEncodingCoRRE(cl, x, y, cl->correMaxWidth, h) &&
		rfbSendRectEncodingCoRRE(cl, x + cl->correMaxWidth, y,
					 w - cl->correMaxWidth, h));
    }

    rfbSendSmallRectEncodingCoRRE(cl, x, y, w, h);
    return TRUE;
}



/*
 * rfbSendSmallRectEncodingCoRRE - send a small (guaranteed < 256x256)
 * rectangle using CoRRE encoding.
 */

static rfbBool
rfbSendSmallRectEncodingCoRRE(rfbClientPtr cl,
                              int x,
                              int y,
                              int w,
                              int h)
{
    rfbFramebufferUpdateRectHeader rect;
    rfbRREHeader hdr;
    int nSubrects;
    int i;
    char *fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
                   + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    int maxRawSize = (cl->scaledScreen->width * cl->scaledScreen->height
                      * (cl->format.bitsPerPixel / 8));

    if (rreBeforeBufSize < maxRawSize) {
        rreBeforeBufSize = maxRawSize;
        if (rreBeforeBuf == NULL)
            rreBeforeBuf = (char *)malloc(rreBeforeBufSize);
        else
            rreBeforeBuf = (char *)realloc(rreBeforeBuf, rreBeforeBufSize);
    }

    if (rreAfterBufSize < maxRawSize) {
        rreAfterBufSize = maxRawSize;
        if (rreAfterBuf == NULL)
            rreAfterBuf = (char *)malloc(rreAfterBufSize);
        else
            rreAfterBuf = (char *)realloc(rreAfterBuf, rreAfterBufSize);
    }

    (*cl->translateFn)(cl->translateLookupTable,&(cl->screen->serverFormat),
                       &cl->format, fbptr, rreBeforeBuf,
                       cl->scaledScreen->paddedWidthInBytes, w, h);

    switch (cl->format.bitsPerPixel) {
    case 8:
        nSubrects = subrectEncode8((uint8_t *)rreBeforeBuf, w, h);
        break;
    case 16:
        nSubrects = subrectEncode16((uint16_t *)rreBeforeBuf, w, h);
        break;
    case 32:
        nSubrects = subrectEncode32((uint32_t *)rreBeforeBuf, w, h);
        break;
    default:
        rfbLog("getBgColour: bpp %d?\n",cl->format.bitsPerPixel);
        return FALSE;
    }
        
    if (nSubrects < 0) {

        /* RRE encoding was too large, use raw */

        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    rfbStatRecordEncodingSent(cl,rfbEncodingCoRRE,
        sz_rfbFramebufferUpdateRectHeader + sz_rfbRREHeader + rreAfterBufLen,
        sz_rfbFramebufferUpdateRectHeader + w * h * (cl->format.bitsPerPixel / 8));

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbRREHeader
        > UPDATE_BUF_SIZE)
    {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingCoRRE);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    hdr.nSubrects = Swap32IfLE(nSubrects);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&hdr, sz_rfbRREHeader);
    cl->ublen += sz_rfbRREHeader;

    for (i = 0; i < rreAfterBufLen;) {

        int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

        if (i + bytesToCopy > rreAfterBufLen) {
            bytesToCopy = rreAfterBufLen - i;
        }

        memcpy(&cl->updateBuf[cl->ublen], &rreAfterBuf[i], bytesToCopy);

        cl->ublen += bytesToCopy;
        i += bytesToCopy;

        if (cl->ublen == UPDATE_BUF_SIZE) {
            if (!rfbSendUpdateBuf(cl))
                return FALSE;
        }
    }

    return TRUE;
}



/*
 * subrectEncode() encodes the given multicoloured rectangle as a background 
 * colour overwritten by single-coloured rectangles.  It returns the number 
 * of subrectangles in the encoded buffer, or -1 if subrect encoding won't
 * fit in the buffer.  It puts the encoded rectangles in rreAfterBuf.  The
 * single-colour rectangle partition is not optimal, but does find the biggest
 * horizontal or vertical rectangle top-left anchored to each consecutive 
 * coordinate position.
 *
 * The coding scheme is simply [<bgcolour><subrect><subrect>...] where each 
 * <subrect> is [<colour><x><y><w><h>].
 */

#define DEFINE_SUBRECT_ENCODE(bpp)                                            \
static int                                                                    \
subrectEncode##bpp(uint##bpp##_t *data, int w, int h) {                       \
    uint##bpp##_t cl;                                                         \
    rfbCoRRERectangle subrect;                                                \
    int x,y;                                                                  \
    int i,j;                                                                  \
    int hx=0,hy,vx=0,vy;                                                      \
    int hyflag;                                                               \
    uint##bpp##_t *seg;                                                       \
    uint##bpp##_t *line;                                                      \
    int hw,hh,vw,vh;                                                          \
    int thex,they,thew,theh;                                                  \
    int numsubs = 0;                                                          \
    int newLen;                                                               \
    uint##bpp##_t bg = (uint##bpp##_t)getBgColour((char*)data,w*h,bpp);       \
                                                                              \
    *((uint##bpp##_t*)rreAfterBuf) = bg;                                      \
                                                                              \
    rreAfterBufLen = (bpp/8);                                                 \
                                                                              \
    for (y=0; y<h; y++) {                                                     \
      line = data+(y*w);                                                      \
      for (x=0; x<w; x++) {                                                   \
        if (line[x] != bg) {                                                  \
          cl = line[x];                                                       \
          hy = y-1;                                                           \
          hyflag = 1;                                                         \
          for (j=y; j<h; j++) {                                               \
            seg = data+(j*w);                                                 \
            if (seg[x] != cl) {break;}                                        \
            i = x;                                                            \
            while ((seg[i] == cl) && (i < w)) i += 1;                         \
            i -= 1;                                                           \
            if (j == y) vx = hx = i;                                          \
            if (i < vx) vx = i;                                               \
            if ((hyflag > 0) && (i >= hx)) {hy += 1;} else {hyflag = 0;}      \
          }                                                                   \
          vy = j-1;                                                           \
                                                                              \
          /*  We now have two possible subrects: (x,y,hx,hy) and (x,y,vx,vy)  \
           *  We'll choose the bigger of the two.                             \
           */                                                                 \
          hw = hx-x+1;                                                        \
          hh = hy-y+1;                                                        \
          vw = vx-x+1;                                                        \
          vh = vy-y+1;                                                        \
                                                                              \
          thex = x;                                                           \
          they = y;                                                           \
                                                                              \
          if ((hw*hh) > (vw*vh)) {                                            \
            thew = hw;                                                        \
            theh = hh;                                                        \
          } else {                                                            \
            thew = vw;                                                        \
            theh = vh;                                                        \
          }                                                                   \
                                                                              \
          subrect.x = thex;                                                   \
          subrect.y = they;                                                   \
          subrect.w = thew;                                                   \
          subrect.h = theh;                                                   \
                                                                              \
          newLen = rreAfterBufLen + (bpp/8) + sz_rfbCoRRERectangle;           \
          if ((newLen > (w * h * (bpp/8))) || (newLen > rreAfterBufSize))     \
            return -1;                                                        \
                                                                              \
          numsubs += 1;                                                       \
          *((uint##bpp##_t*)(rreAfterBuf + rreAfterBufLen)) = cl;             \
          rreAfterBufLen += (bpp/8);                                          \
          memcpy(&rreAfterBuf[rreAfterBufLen],&subrect,sz_rfbCoRRERectangle); \
          rreAfterBufLen += sz_rfbCoRRERectangle;                             \
                                                                              \
          /*                                                                  \
           * Now mark the subrect as done.                                    \
           */                                                                 \
          for (j=they; j < (they+theh); j++) {                                \
            for (i=thex; i < (thex+thew); i++) {                              \
              data[j*w+i] = bg;                                               \
            }                                                                 \
          }                                                                   \
        }                                                                     \
      }                                                                       \
    }                                                                         \
                                                                              \
    return numsubs;                                                           \
}

DEFINE_SUBRECT_ENCODE(8)
DEFINE_SUBRECT_ENCODE(16)
DEFINE_SUBRECT_ENCODE(32)


/*
 * getBgColour() gets the most prevalent colour in a byte array.
 */
static uint32_t
getBgColour(char *data, int size, int bpp)
{

#define NUMCLRS 256
  
  static int counts[NUMCLRS];
  int i,j,k;

  int maxcount = 0;
  uint8_t maxclr = 0;

  if (bpp != 8) {
    if (bpp == 16) {
      return ((uint16_t *)data)[0];
    } else if (bpp == 32) {
      return ((uint32_t *)data)[0];
    } else {
      rfbLog("getBgColour: bpp %d?\n",bpp);
      return 0;
    }
  }

  for (i=0; i<NUMCLRS; i++) {
    counts[i] = 0;
  }

  for (j=0; j<size; j++) {
    k = (int)(((uint8_t *)data)[j]);
    if (k >= NUMCLRS) {
      rfbLog("getBgColour: unusual colour = %d\n", k);
      return 0;
    }
    counts[k] += 1;
    if (counts[k] > maxcount) {
      maxcount = counts[k];
      maxclr = ((uint8_t *)data)[j];
    }
  }
  
  return maxclr;
}
///////////////////////////////////////////end corre.c //////////////////////////////////////////////////

///////////////////////////////////////////begin hextile.c //////////////////////////////////////////////////
/*
 * hextile.c
 *
 * Routines to implement Hextile Encoding
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

static rfbBool sendHextiles8(rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool sendHextiles16(rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool sendHextiles32(rfbClientPtr cl, int x, int y, int w, int h);


/*
 * rfbSendRectEncodingHextile - send a rectangle using hextile encoding.
 */

rfbBool
rfbSendRectEncodingHextile(rfbClientPtr cl,
                           int x,
                           int y,
                           int w,
                           int h)
{
    rfbFramebufferUpdateRectHeader rect;
    
    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingHextile);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, rfbEncodingHextile,
          sz_rfbFramebufferUpdateRectHeader,
          sz_rfbFramebufferUpdateRectHeader + w * (cl->format.bitsPerPixel / 8) * h);

    switch (cl->format.bitsPerPixel) {
    case 8:
        return sendHextiles8(cl, x, y, w, h);
    case 16:
        return sendHextiles16(cl, x, y, w, h);
    case 32:
        return sendHextiles32(cl, x, y, w, h);
    }

    rfbLog("rfbSendRectEncodingHextile: bpp %d?\n", cl->format.bitsPerPixel);
    return FALSE;
}


#define PUT_PIXEL8(pix) (cl->updateBuf[cl->ublen++] = (pix))

#define PUT_PIXEL16(pix) (cl->updateBuf[cl->ublen++] = ((char*)&(pix))[0], \
                          cl->updateBuf[cl->ublen++] = ((char*)&(pix))[1])

#define PUT_PIXEL32(pix) (cl->updateBuf[cl->ublen++] = ((char*)&(pix))[0], \
                          cl->updateBuf[cl->ublen++] = ((char*)&(pix))[1], \
                          cl->updateBuf[cl->ublen++] = ((char*)&(pix))[2], \
                          cl->updateBuf[cl->ublen++] = ((char*)&(pix))[3])


#define DEFINE_SEND_HEXTILES(bpp)                                               \
                                                                                \
                                                                                \
static rfbBool subrectEncode##bpp(rfbClientPtr cli, uint##bpp##_t *data,        \
		int w, int h, uint##bpp##_t bg, uint##bpp##_t fg, rfbBool mono);\
static void testColours##bpp(uint##bpp##_t *data, int size, rfbBool *mono,      \
                  rfbBool *solid, uint##bpp##_t *bg, uint##bpp##_t *fg);        \
                                                                                \
                                                                                \
/*                                                                              \
 * rfbSendHextiles                                                              \
 */                                                                             \
                                                                                \
static rfbBool                                                                  \
sendHextiles##bpp(rfbClientPtr cl, int rx, int ry, int rw, int rh) {            \
    int x, y, w, h;                                                             \
    int startUblen;                                                             \
    char *fbptr;                                                                \
    uint##bpp##_t bg = 0, fg = 0, newBg, newFg;                                 \
    rfbBool mono, solid;                                                        \
    rfbBool validBg = FALSE;                                                    \
    rfbBool validFg = FALSE;                                                    \
    uint##bpp##_t clientPixelData[16*16*(bpp/8)];                               \
                                                                                \
    for (y = ry; y < ry+rh; y += 16) {                                          \
        for (x = rx; x < rx+rw; x += 16) {                                      \
            w = h = 16;                                                         \
            if (rx+rw - x < 16)                                                 \
                w = rx+rw - x;                                                  \
            if (ry+rh - y < 16)                                                 \
                h = ry+rh - y;                                                  \
                                                                                \
            if ((cl->ublen + 1 + (2 + 16 * 16) * (bpp/8)) >                     \
                UPDATE_BUF_SIZE) {                                              \
                if (!rfbSendUpdateBuf(cl))                                      \
                    return FALSE;                                               \
            }                                                                   \
                                                                                \
            fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)   \
                     + (x * (cl->scaledScreen->bitsPerPixel / 8)));                   \
                                                                                \
            (*cl->translateFn)(cl->translateLookupTable, &(cl->screen->serverFormat),      \
                               &cl->format, fbptr, (char *)clientPixelData,     \
                               cl->scaledScreen->paddedWidthInBytes, w, h);           \
                                                                                \
            startUblen = cl->ublen;                                             \
            cl->updateBuf[startUblen] = 0;                                      \
            cl->ublen++;                                                        \
            rfbStatRecordEncodingSentAdd(cl, rfbEncodingHextile, 1);            \
                                                                                \
            testColours##bpp(clientPixelData, w * h,                            \
                             &mono, &solid, &newBg, &newFg);                    \
                                                                                \
            if (!validBg || (newBg != bg)) {                                    \
                validBg = TRUE;                                                 \
                bg = newBg;                                                     \
                cl->updateBuf[startUblen] |= rfbHextileBackgroundSpecified;     \
                PUT_PIXEL##bpp(bg);                                             \
            }                                                                   \
                                                                                \
            if (solid) {                                                        \
                continue;                                                       \
            }                                                                   \
                                                                                \
            cl->updateBuf[startUblen] |= rfbHextileAnySubrects;                 \
                                                                                \
            if (mono) {                                                         \
                if (!validFg || (newFg != fg)) {                                \
                    validFg = TRUE;                                             \
                    fg = newFg;                                                 \
                    cl->updateBuf[startUblen] |= rfbHextileForegroundSpecified; \
                    PUT_PIXEL##bpp(fg);                                         \
                }                                                               \
            } else {                                                            \
                validFg = FALSE;                                                \
                cl->updateBuf[startUblen] |= rfbHextileSubrectsColoured;        \
            }                                                                   \
                                                                                \
            if (!subrectEncode##bpp(cl, clientPixelData, w, h, bg, fg, mono)) { \
                /* encoding was too large, use raw */                           \
                validBg = FALSE;                                                \
                validFg = FALSE;                                                \
                cl->ublen = startUblen;                                         \
                cl->updateBuf[cl->ublen++] = rfbHextileRaw;                     \
                (*cl->translateFn)(cl->translateLookupTable,                    \
                                   &(cl->screen->serverFormat), &cl->format, fbptr,        \
                                   (char *)clientPixelData,                     \
                                   cl->scaledScreen->paddedWidthInBytes, w, h); \
                                                                                \
                memcpy(&cl->updateBuf[cl->ublen], (char *)clientPixelData,      \
                       w * h * (bpp/8));                                        \
                                                                                \
                cl->ublen += w * h * (bpp/8);                                   \
                rfbStatRecordEncodingSentAdd(cl, rfbEncodingHextile,            \
                             w * h * (bpp/8));                                  \
            }                                                                   \
        }                                                                       \
    }                                                                           \
                                                                                \
    return TRUE;                                                                \
}                                                                               \
                                                                                \
                                                                                \
static rfbBool                                                                  \
subrectEncode##bpp(rfbClientPtr cl, uint##bpp##_t *data, int w, int h,          \
                   uint##bpp##_t bg, uint##bpp##_t fg, rfbBool mono)            \
{                                                                               \
    uint##bpp##_t cl2;                                                          \
    int x,y;                                                                    \
    int i,j;                                                                    \
    int hx=0,hy,vx=0,vy;                                                        \
    int hyflag;                                                                 \
    uint##bpp##_t *seg;                                                         \
    uint##bpp##_t *line;                                                        \
    int hw,hh,vw,vh;                                                            \
    int thex,they,thew,theh;                                                    \
    int numsubs = 0;                                                            \
    int newLen;                                                                 \
    int nSubrectsUblen;                                                         \
                                                                                \
    nSubrectsUblen = cl->ublen;                                                 \
    cl->ublen++;                                                                \
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingHextile, 1);                    \
                                                                                \
    for (y=0; y<h; y++) {                                                       \
        line = data+(y*w);                                                      \
        for (x=0; x<w; x++) {                                                   \
            if (line[x] != bg) {                                                \
                cl2 = line[x];                                                  \
                hy = y-1;                                                       \
                hyflag = 1;                                                     \
                for (j=y; j<h; j++) {                                           \
                    seg = data+(j*w);                                           \
                    if (seg[x] != cl2) {break;}                                 \
                    i = x;                                                      \
                    while ((seg[i] == cl2) && (i < w)) i += 1;                  \
                    i -= 1;                                                     \
                    if (j == y) vx = hx = i;                                    \
                    if (i < vx) vx = i;                                         \
                    if ((hyflag > 0) && (i >= hx)) {                            \
                        hy += 1;                                                \
                    } else {                                                    \
                        hyflag = 0;                                             \
                    }                                                           \
                }                                                               \
                vy = j-1;                                                       \
                                                                                \
                /* We now have two possible subrects: (x,y,hx,hy) and           \
                 * (x,y,vx,vy).  We'll choose the bigger of the two.            \
                 */                                                             \
                hw = hx-x+1;                                                    \
                hh = hy-y+1;                                                    \
                vw = vx-x+1;                                                    \
                vh = vy-y+1;                                                    \
                                                                                \
                thex = x;                                                       \
                they = y;                                                       \
                                                                                \
                if ((hw*hh) > (vw*vh)) {                                        \
                    thew = hw;                                                  \
                    theh = hh;                                                  \
                } else {                                                        \
                    thew = vw;                                                  \
                    theh = vh;                                                  \
                }                                                               \
                                                                                \
                if (mono) {                                                     \
                    newLen = cl->ublen - nSubrectsUblen + 2;                    \
                } else {                                                        \
                    newLen = cl->ublen - nSubrectsUblen + bpp/8 + 2;            \
                }                                                               \
                                                                                \
                if (newLen > (w * h * (bpp/8)))                                 \
                    return FALSE;                                               \
                                                                                \
                numsubs += 1;                                                   \
                                                                                \
                if (!mono) PUT_PIXEL##bpp(cl2);                                 \
                                                                                \
                cl->updateBuf[cl->ublen++] = rfbHextilePackXY(thex,they);       \
                cl->updateBuf[cl->ublen++] = rfbHextilePackWH(thew,theh);       \
                rfbStatRecordEncodingSentAdd(cl, rfbEncodingHextile, 1);        \
                                                                                \
                /*                                                              \
                 * Now mark the subrect as done.                                \
                 */                                                             \
                for (j=they; j < (they+theh); j++) {                            \
                    for (i=thex; i < (thex+thew); i++) {                        \
                        data[j*w+i] = bg;                                       \
                    }                                                           \
                }                                                               \
            }                                                                   \
        }                                                                       \
    }                                                                           \
                                                                                \
    cl->updateBuf[nSubrectsUblen] = numsubs;                                    \
                                                                                \
    return TRUE;                                                                \
}                                                                               \
                                                                                \
                                                                                \
/*                                                                              \
 * testColours() tests if there are one (solid), two (mono) or more             \
 * colours in a tile and gets a reasonable guess at the best background         \
 * pixel, and the foreground pixel for mono.                                    \
 */                                                                             \
                                                                                \
static void                                                                     \
testColours##bpp(uint##bpp##_t *data, int size, rfbBool *mono, rfbBool *solid,  \
                 uint##bpp##_t *bg, uint##bpp##_t *fg) {                        \
    uint##bpp##_t colour1 = 0, colour2 = 0;                                     \
    int n1 = 0, n2 = 0;                                                         \
    *mono = TRUE;                                                               \
    *solid = TRUE;                                                              \
                                                                                \
    for (; size > 0; size--, data++) {                                          \
                                                                                \
        if (n1 == 0)                                                            \
            colour1 = *data;                                                    \
                                                                                \
        if (*data == colour1) {                                                 \
            n1++;                                                               \
            continue;                                                           \
        }                                                                       \
                                                                                \
        if (n2 == 0) {                                                          \
            *solid = FALSE;                                                     \
            colour2 = *data;                                                    \
        }                                                                       \
                                                                                \
        if (*data == colour2) {                                                 \
            n2++;                                                               \
            continue;                                                           \
        }                                                                       \
                                                                                \
        *mono = FALSE;                                                          \
        break;                                                                  \
    }                                                                           \
                                                                                \
    if (n1 > n2) {                                                              \
        *bg = colour1;                                                          \
        *fg = colour2;                                                          \
    } else {                                                                    \
        *bg = colour2;                                                          \
        *fg = colour1;                                                          \
    }                                                                           \
}

DEFINE_SEND_HEXTILES(8)
DEFINE_SEND_HEXTILES(16)
DEFINE_SEND_HEXTILES(32)
///////////////////////////////////////////end hextile.c //////////////////////////////////////////////////

///////////////////////////////////////////begin rre.c //////////////////////////////////////////////////
/*
 * rre.c
 *
 * Routines to implement Rise-and-Run-length Encoding (RRE).  This
 * code is based on krw's original javatel rfbserver.
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

/*
 * rreBeforeBuf contains pixel data in the client's format.
 * rreAfterBuf contains the RRE encoded version.  If the RRE encoded version is
 * larger than the raw data or if it exceeds rreAfterBufSize then
 * raw encoding is used instead.
 */

static int rreBeforeBufSize = 0;
static char *rreBeforeBuf = NULL;

static int rreAfterBufSize = 0;
static char *rreAfterBuf = NULL;
static int rreAfterBufLen=0;

static int subrectEncode8(uint8_t *data, int w, int h);
static int subrectEncode16(uint16_t *data, int w, int h);
static int subrectEncode32(uint32_t *data, int w, int h);
static uint32_t getBgColour(char *data, int size, int bpp);


void rfbRRECleanup(rfbScreenInfoPtr screen)
{
  if (rreBeforeBufSize) {
    free(rreBeforeBuf);
    rreBeforeBufSize=0;
  }
  if (rreAfterBufSize) {
    free(rreAfterBuf);
    rreAfterBufSize=0;
  }
}


/*
 * rfbSendRectEncodingRRE - send a given rectangle using RRE encoding.
 */

rfbBool
rfbSendRectEncodingRRE(rfbClientPtr cl,
                       int x,
                       int y,
                       int w,
                       int h)
{
    rfbFramebufferUpdateRectHeader rect;
    rfbRREHeader hdr;
    int nSubrects;
    int i;
    char *fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
                   + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    int maxRawSize = (cl->scaledScreen->width * cl->scaledScreen->height
                      * (cl->format.bitsPerPixel / 8));

    if (rreBeforeBufSize < maxRawSize) {
        rreBeforeBufSize = maxRawSize;
        if (rreBeforeBuf == NULL)
            rreBeforeBuf = (char *)malloc(rreBeforeBufSize);
        else
            rreBeforeBuf = (char *)realloc(rreBeforeBuf, rreBeforeBufSize);
    }

    if (rreAfterBufSize < maxRawSize) {
        rreAfterBufSize = maxRawSize;
        if (rreAfterBuf == NULL)
            rreAfterBuf = (char *)malloc(rreAfterBufSize);
        else
            rreAfterBuf = (char *)realloc(rreAfterBuf, rreAfterBufSize);
    }

    (*cl->translateFn)(cl->translateLookupTable,
		       &(cl->screen->serverFormat),
                       &cl->format, fbptr, rreBeforeBuf,
                       cl->scaledScreen->paddedWidthInBytes, w, h);

    switch (cl->format.bitsPerPixel) {
    case 8:
        nSubrects = subrectEncode8((uint8_t *)rreBeforeBuf, w, h);
        break;
    case 16:
        nSubrects = subrectEncode16((uint16_t *)rreBeforeBuf, w, h);
        break;
    case 32:
        nSubrects = subrectEncode32((uint32_t *)rreBeforeBuf, w, h);
        break;
    default:
        rfbLog("getBgColour: bpp %d?\n",cl->format.bitsPerPixel);
        return FALSE;
    }
        
    if (nSubrects < 0) {

        /* RRE encoding was too large, use raw */

        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    rfbStatRecordEncodingSent(cl, rfbEncodingRRE,
                              sz_rfbFramebufferUpdateRectHeader + sz_rfbRREHeader + rreAfterBufLen,
                              sz_rfbFramebufferUpdateRectHeader + w * h * (cl->format.bitsPerPixel / 8));

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbRREHeader
        > UPDATE_BUF_SIZE)
    {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingRRE);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    hdr.nSubrects = Swap32IfLE(nSubrects);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&hdr, sz_rfbRREHeader);
    cl->ublen += sz_rfbRREHeader;

    for (i = 0; i < rreAfterBufLen;) {

        int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

        if (i + bytesToCopy > rreAfterBufLen) {
            bytesToCopy = rreAfterBufLen - i;
        }

        memcpy(&cl->updateBuf[cl->ublen], &rreAfterBuf[i], bytesToCopy);

        cl->ublen += bytesToCopy;
        i += bytesToCopy;

        if (cl->ublen == UPDATE_BUF_SIZE) {
            if (!rfbSendUpdateBuf(cl))
                return FALSE;
        }
    }

    return TRUE;
}



/*
 * subrectEncode() encodes the given multicoloured rectangle as a background 
 * colour overwritten by single-coloured rectangles.  It returns the number 
 * of subrectangles in the encoded buffer, or -1 if subrect encoding won't
 * fit in the buffer.  It puts the encoded rectangles in rreAfterBuf.  The
 * single-colour rectangle partition is not optimal, but does find the biggest
 * horizontal or vertical rectangle top-left anchored to each consecutive 
 * coordinate position.
 *
 * The coding scheme is simply [<bgcolour><subrect><subrect>...] where each 
 * <subrect> is [<colour><x><y><w><h>].
 */

#define DEFINE_SUBRECT_ENCODE(bpp)                                            \
static int                                                                    \
subrectEncode##bpp(uint##bpp##_t *data, int w, int h) {                       \
    uint##bpp##_t cl;                                                         \
    rfbRectangle subrect;                                                     \
    int x,y;                                                                  \
    int i,j;                                                                  \
    int hx=0,hy,vx=0,vy;                                                      \
    int hyflag;                                                               \
    uint##bpp##_t *seg;                                                       \
    uint##bpp##_t *line;                                                      \
    int hw,hh,vw,vh;                                                          \
    int thex,they,thew,theh;                                                  \
    int numsubs = 0;                                                          \
    int newLen;                                                               \
    uint##bpp##_t bg = (uint##bpp##_t)getBgColour((char*)data,w*h,bpp);       \
                                                                              \
    *((uint##bpp##_t*)rreAfterBuf) = bg;                                      \
                                                                              \
    rreAfterBufLen = (bpp/8);                                                 \
                                                                              \
    for (y=0; y<h; y++) {                                                     \
      line = data+(y*w);                                                      \
      for (x=0; x<w; x++) {                                                   \
        if (line[x] != bg) {                                                  \
          cl = line[x];                                                       \
          hy = y-1;                                                           \
          hyflag = 1;                                                         \
          for (j=y; j<h; j++) {                                               \
            seg = data+(j*w);                                                 \
            if (seg[x] != cl) {break;}                                        \
            i = x;                                                            \
            while ((seg[i] == cl) && (i < w)) i += 1;                         \
            i -= 1;                                                           \
            if (j == y) vx = hx = i;                                          \
            if (i < vx) vx = i;                                               \
            if ((hyflag > 0) && (i >= hx)) {hy += 1;} else {hyflag = 0;}      \
          }                                                                   \
          vy = j-1;                                                           \
                                                                              \
          /*  We now have two possible subrects: (x,y,hx,hy) and (x,y,vx,vy)  \
           *  We'll choose the bigger of the two.                             \
           */                                                                 \
          hw = hx-x+1;                                                        \
          hh = hy-y+1;                                                        \
          vw = vx-x+1;                                                        \
          vh = vy-y+1;                                                        \
                                                                              \
          thex = x;                                                           \
          they = y;                                                           \
                                                                              \
          if ((hw*hh) > (vw*vh)) {                                            \
            thew = hw;                                                        \
            theh = hh;                                                        \
          } else {                                                            \
            thew = vw;                                                        \
            theh = vh;                                                        \
          }                                                                   \
                                                                              \
          subrect.x = Swap16IfLE(thex);                                       \
          subrect.y = Swap16IfLE(they);                                       \
          subrect.w = Swap16IfLE(thew);                                       \
          subrect.h = Swap16IfLE(theh);                                       \
                                                                              \
          newLen = rreAfterBufLen + (bpp/8) + sz_rfbRectangle;                \
          if ((newLen > (w * h * (bpp/8))) || (newLen > rreAfterBufSize))     \
            return -1;                                                        \
                                                                              \
          numsubs += 1;                                                       \
          *((uint##bpp##_t*)(rreAfterBuf + rreAfterBufLen)) = cl;             \
          rreAfterBufLen += (bpp/8);                                          \
          memcpy(&rreAfterBuf[rreAfterBufLen],&subrect,sz_rfbRectangle);      \
          rreAfterBufLen += sz_rfbRectangle;                                  \
                                                                              \
          /*                                                                  \
           * Now mark the subrect as done.                                    \
           */                                                                 \
          for (j=they; j < (they+theh); j++) {                                \
            for (i=thex; i < (thex+thew); i++) {                              \
              data[j*w+i] = bg;                                               \
            }                                                                 \
          }                                                                   \
        }                                                                     \
      }                                                                       \
    }                                                                         \
                                                                              \
    return numsubs;                                                           \
}

DEFINE_SUBRECT_ENCODE(8)
DEFINE_SUBRECT_ENCODE(16)
DEFINE_SUBRECT_ENCODE(32)


/*
 * getBgColour() gets the most prevalent colour in a byte array.
 */
static uint32_t
getBgColour(char *data, int size, int bpp)
{
    
#define NUMCLRS 256
  
  static int counts[NUMCLRS];
  int i,j,k;

  int maxcount = 0;
  uint8_t maxclr = 0;

  if (bpp != 8) {
    if (bpp == 16) {
      return ((uint16_t *)data)[0];
    } else if (bpp == 32) {
      return ((uint32_t *)data)[0];
    } else {
      rfbLog("getBgColour: bpp %d?\n",bpp);
      return 0;
    }
  }

  for (i=0; i<NUMCLRS; i++) {
    counts[i] = 0;
  }

  for (j=0; j<size; j++) {
    k = (int)(((uint8_t *)data)[j]);
    if (k >= NUMCLRS) {
      rfbErr("getBgColour: unusual colour = %d\n", k);
      return 0;
    }
    counts[k] += 1;
    if (counts[k] > maxcount) {
      maxcount = counts[k];
      maxclr = ((uint8_t *)data)[j];
    }
  }
  
  return maxclr;
}
///////////////////////////////////////////end rre.c //////////////////////////////////////////////////

///////////////////////////////////////////begin translate.c //////////////////////////////////////////////////
/*
 * translate.c - translate between different pixel formats
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>
#include <rfb/rfbregion.h>

static void PrintPixelFormat(rfbPixelFormat *pf);
static rfbBool rfbSetClientColourMapBGR233(rfbClientPtr cl);

rfbBool rfbEconomicTranslate = FALSE;

/*
 * Some standard pixel formats.
 */

static const rfbPixelFormat BGR233Format = {
    8, 8, 0, 1, 7, 7, 3, 0, 3, 6, 0, 0
};


/*
 * Macro to compare pixel formats.
 */

#define PF_EQ(x,y)                                                      \
        ((x.bitsPerPixel == y.bitsPerPixel) &&                          \
         (x.depth == y.depth) &&                                        \
         ((x.bigEndian == y.bigEndian) || (x.bitsPerPixel == 8)) &&     \
         (x.trueColour == y.trueColour) &&                              \
         (!x.trueColour || ((x.redMax == y.redMax) &&                   \
                            (x.greenMax == y.greenMax) &&               \
                            (x.blueMax == y.blueMax) &&                 \
                            (x.redShift == y.redShift) &&               \
                            (x.greenShift == y.greenShift) &&           \
                            (x.blueShift == y.blueShift))))

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#define CONCAT3(a,b,c) a##b##c
#define CONCAT3E(a,b,c) CONCAT3(a,b,c)
#define CONCAT4(a,b,c,d) a##b##c##d
#define CONCAT4E(a,b,c,d) CONCAT4(a,b,c,d)

#undef OUT
#undef IN

#define OUT 8
#include "tableinitcmtemplate.c"
#include "tableinittctemplate.c"
#define IN 8
#include "tabletranstemplate.c"
#undef IN
#define IN 16
#include "tabletranstemplate.c"
#undef IN
#define IN 32
#include "tabletranstemplate.c"
#undef IN
#undef OUT

#define OUT 16
#include "tableinitcmtemplate.c"
#include "tableinittctemplate.c"
#define IN 8
#include "tabletranstemplate.c"
#undef IN
#define IN 16
#include "tabletranstemplate.c"
#undef IN
#define IN 32
#include "tabletranstemplate.c"
#undef IN
#undef OUT

#define OUT 32
#include "tableinitcmtemplate.c"
#include "tableinittctemplate.c"
#define IN 8
#include "tabletranstemplate.c"
#undef IN
#define IN 16
#include "tabletranstemplate.c"
#undef IN
#define IN 32
#include "tabletranstemplate.c"
#undef IN
#undef OUT

#ifdef LIBVNCSERVER_ALLOW24BPP
#define COUNT_OFFSETS 4
#define BPP2OFFSET(bpp) ((bpp)/8-1)
#include "tableinit24.c"
#define BPP 8
#include "tabletrans24template.c"
#undef BPP
#define BPP 16
#include "tabletrans24template.c"
#undef BPP
#define BPP 24
#include "tabletrans24template.c"
#undef BPP
#define BPP 32
#include "tabletrans24template.c"
#undef BPP
#else
#define COUNT_OFFSETS 3
#define BPP2OFFSET(bpp) ((int)(bpp)/16)
#endif

typedef void (*rfbInitCMTableFnType)(char **table, rfbPixelFormat *in,
                                   rfbPixelFormat *out,rfbColourMap* cm);
typedef void (*rfbInitTableFnType)(char **table, rfbPixelFormat *in,
                                   rfbPixelFormat *out);

static rfbInitCMTableFnType rfbInitColourMapSingleTableFns[COUNT_OFFSETS] = {
    rfbInitColourMapSingleTable8,
    rfbInitColourMapSingleTable16,
#ifdef LIBVNCSERVER_ALLOW24BPP
    rfbInitColourMapSingleTable24,
#endif
    rfbInitColourMapSingleTable32
};

static rfbInitTableFnType rfbInitTrueColourSingleTableFns[COUNT_OFFSETS] = {
    rfbInitTrueColourSingleTable8,
    rfbInitTrueColourSingleTable16,
#ifdef LIBVNCSERVER_ALLOW24BPP
    rfbInitTrueColourSingleTable24,
#endif
    rfbInitTrueColourSingleTable32
};

static rfbInitTableFnType rfbInitTrueColourRGBTablesFns[COUNT_OFFSETS] = {
    rfbInitTrueColourRGBTables8,
    rfbInitTrueColourRGBTables16,
#ifdef LIBVNCSERVER_ALLOW24BPP
    rfbInitTrueColourRGBTables24,
#endif
    rfbInitTrueColourRGBTables32
};

static rfbTranslateFnType rfbTranslateWithSingleTableFns[COUNT_OFFSETS][COUNT_OFFSETS] = {
    { rfbTranslateWithSingleTable8to8,
      rfbTranslateWithSingleTable8to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithSingleTable8to24,
#endif
      rfbTranslateWithSingleTable8to32 },
    { rfbTranslateWithSingleTable16to8,
      rfbTranslateWithSingleTable16to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithSingleTable16to24,
#endif
      rfbTranslateWithSingleTable16to32 },
#ifdef LIBVNCSERVER_ALLOW24BPP
    { rfbTranslateWithSingleTable24to8,
      rfbTranslateWithSingleTable24to16,
      rfbTranslateWithSingleTable24to24,
      rfbTranslateWithSingleTable24to32 },
#endif
    { rfbTranslateWithSingleTable32to8,
      rfbTranslateWithSingleTable32to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithSingleTable32to24,
#endif
      rfbTranslateWithSingleTable32to32 }
};

static rfbTranslateFnType rfbTranslateWithRGBTablesFns[COUNT_OFFSETS][COUNT_OFFSETS] = {
    { rfbTranslateWithRGBTables8to8,
      rfbTranslateWithRGBTables8to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithRGBTables8to24,
#endif
      rfbTranslateWithRGBTables8to32 },
    { rfbTranslateWithRGBTables16to8,
      rfbTranslateWithRGBTables16to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithRGBTables16to24,
#endif
      rfbTranslateWithRGBTables16to32 },
#ifdef LIBVNCSERVER_ALLOW24BPP
    { rfbTranslateWithRGBTables24to8,
      rfbTranslateWithRGBTables24to16,
      rfbTranslateWithRGBTables24to24,
      rfbTranslateWithRGBTables24to32 },
#endif
    { rfbTranslateWithRGBTables32to8,
      rfbTranslateWithRGBTables32to16,
#ifdef LIBVNCSERVER_ALLOW24BPP
      rfbTranslateWithRGBTables32to24,
#endif
      rfbTranslateWithRGBTables32to32 }
};



/*
 * rfbTranslateNone is used when no translation is required.
 */

void
rfbTranslateNone(char *table, rfbPixelFormat *in, rfbPixelFormat *out,
                 char *iptr, char *optr, int bytesBetweenInputLines,
                 int width, int height)
{
    int bytesPerOutputLine = width * (out->bitsPerPixel / 8);

    while (height > 0) {
        memcpy(optr, iptr, bytesPerOutputLine);
        iptr += bytesBetweenInputLines;
        optr += bytesPerOutputLine;
        height--;
    }
}


/*
 * rfbSetTranslateFunction sets the translation function.
 */

rfbBool
rfbSetTranslateFunction(rfbClientPtr cl)
{
    rfbLog("Pixel format for client %s:\n",cl->host);
    PrintPixelFormat(&cl->format);

    /*
     * Check that bits per pixel values are valid
     */

    if ((cl->screen->serverFormat.bitsPerPixel != 8) &&
        (cl->screen->serverFormat.bitsPerPixel != 16) &&
#ifdef LIBVNCSERVER_ALLOW24BPP
	(cl->screen->serverFormat.bitsPerPixel != 24) &&
#endif
        (cl->screen->serverFormat.bitsPerPixel != 32))
    {
        rfbErr("%s: server bits per pixel not 8, 16 or 32 (is %d)\n",
	       "rfbSetTranslateFunction", 
	       cl->screen->serverFormat.bitsPerPixel);
        rfbCloseClient(cl);
        return FALSE;
    }

    if ((cl->format.bitsPerPixel != 8) &&
        (cl->format.bitsPerPixel != 16) &&
#ifdef LIBVNCSERVER_ALLOW24BPP
	(cl->format.bitsPerPixel != 24) &&
#endif
        (cl->format.bitsPerPixel != 32))
    {
        rfbErr("%s: client bits per pixel not 8, 16 or 32\n",
                "rfbSetTranslateFunction");
        rfbCloseClient(cl);
        return FALSE;
    }

    if (!cl->format.trueColour && (cl->format.bitsPerPixel != 8)) {
        rfbErr("rfbSetTranslateFunction: client has colour map "
                "but %d-bit - can only cope with 8-bit colour maps\n",
                cl->format.bitsPerPixel);
        rfbCloseClient(cl);
        return FALSE;
    }

    /*
     * bpp is valid, now work out how to translate
     */

    if (!cl->format.trueColour) {
        /*
         * truecolour -> colour map
         *
         * Set client's colour map to BGR233, then effectively it's
         * truecolour as well
         */

        if (!rfbSetClientColourMapBGR233(cl))
            return FALSE;

        cl->format = BGR233Format;
    }

    /* truecolour -> truecolour */

    if (PF_EQ(cl->format,cl->screen->serverFormat)) {

        /* client & server the same */

        rfbLog("no translation needed\n");
        cl->translateFn = rfbTranslateNone;
        return TRUE;
    }

    if ((cl->screen->serverFormat.bitsPerPixel < 16) ||
        ((!cl->screen->serverFormat.trueColour || !rfbEconomicTranslate) &&
	   (cl->screen->serverFormat.bitsPerPixel == 16))) {

        /* we can use a single lookup table for <= 16 bpp */

        cl->translateFn = rfbTranslateWithSingleTableFns
                              [BPP2OFFSET(cl->screen->serverFormat.bitsPerPixel)]
                                  [BPP2OFFSET(cl->format.bitsPerPixel)];

	if(cl->screen->serverFormat.trueColour)
	  (*rfbInitTrueColourSingleTableFns
	   [BPP2OFFSET(cl->format.bitsPerPixel)]) (&cl->translateLookupTable,
						   &(cl->screen->serverFormat), &cl->format);
	else
	  (*rfbInitColourMapSingleTableFns
	   [BPP2OFFSET(cl->format.bitsPerPixel)]) (&cl->translateLookupTable,
						   &(cl->screen->serverFormat), &cl->format,&cl->screen->colourMap);

    } else {

        /* otherwise we use three separate tables for red, green and blue */

        cl->translateFn = rfbTranslateWithRGBTablesFns
                              [BPP2OFFSET(cl->screen->serverFormat.bitsPerPixel)]
                                  [BPP2OFFSET(cl->format.bitsPerPixel)];

        (*rfbInitTrueColourRGBTablesFns
            [BPP2OFFSET(cl->format.bitsPerPixel)]) (&cl->translateLookupTable,
                                             &(cl->screen->serverFormat), &cl->format);
    }

    return TRUE;
}



/*
 * rfbSetClientColourMapBGR233 sets the client's colour map so that it's
 * just like an 8-bit BGR233 true colour client.
 */

static rfbBool
rfbSetClientColourMapBGR233(rfbClientPtr cl)
{
    char buf[sz_rfbSetColourMapEntriesMsg + 256 * 3 * 2];
    rfbSetColourMapEntriesMsg *scme = (rfbSetColourMapEntriesMsg *)buf;
    uint16_t *rgb = (uint16_t *)(&buf[sz_rfbSetColourMapEntriesMsg]);
    int i, len;
    int r, g, b;

    if (cl->format.bitsPerPixel != 8 ) {
        rfbErr("%s: client not 8 bits per pixel\n",
                "rfbSetClientColourMapBGR233");
        rfbCloseClient(cl);
        return FALSE;
    }
    
    scme->type = rfbSetColourMapEntries;

    scme->firstColour = Swap16IfLE(0);
    scme->nColours = Swap16IfLE(256);

    len = sz_rfbSetColourMapEntriesMsg;

    i = 0;

    for (b = 0; b < 4; b++) {
        for (g = 0; g < 8; g++) {
            for (r = 0; r < 8; r++) {
                rgb[i++] = Swap16IfLE(r * 65535 / 7);
                rgb[i++] = Swap16IfLE(g * 65535 / 7);
                rgb[i++] = Swap16IfLE(b * 65535 / 3);
            }
        }
    }

    len += 256 * 3 * 2;

    if (rfbWriteExact(cl, buf, len) < 0) {
        rfbLogPerror("rfbSetClientColourMapBGR233: write");
        rfbCloseClient(cl);
        return FALSE;
    }
    return TRUE;
}

/* this function is not called very often, so it needn't be
   efficient. */

/*
 * rfbSetClientColourMap is called to set the client's colour map.  If the
 * client is a true colour client, we simply update our own translation table
 * and mark the whole screen as having been modified.
 */

rfbBool
rfbSetClientColourMap(rfbClientPtr cl, int firstColour, int nColours)
{
    if (cl->screen->serverFormat.trueColour || !cl->readyForSetColourMapEntries) {
	return TRUE;
    }

    if (nColours == 0) {
	nColours = cl->screen->colourMap.count;
    }

    if (cl->format.trueColour) {
	(*rfbInitColourMapSingleTableFns
	    [BPP2OFFSET(cl->format.bitsPerPixel)]) (&cl->translateLookupTable,
					     &cl->screen->serverFormat, &cl->format,&cl->screen->colourMap);

	sraRgnDestroy(cl->modifiedRegion);
	cl->modifiedRegion =
	  sraRgnCreateRect(0,0,cl->screen->width,cl->screen->height);

	return TRUE;
    }

    return rfbSendSetColourMapEntries(cl, firstColour, nColours);
}


/*
 * rfbSetClientColourMaps sets the colour map for each RFB client.
 */

void
rfbSetClientColourMaps(rfbScreenInfoPtr rfbScreen, int firstColour, int nColours)
{
    rfbClientIteratorPtr i;
    rfbClientPtr cl;

    i = rfbGetClientIterator(rfbScreen);
    while((cl = rfbClientIteratorNext(i)))
      rfbSetClientColourMap(cl, firstColour, nColours);
    rfbReleaseClientIterator(i);
}

static void
PrintPixelFormat(rfbPixelFormat *pf)
{
    if (pf->bitsPerPixel == 1) {
        rfbLog("  1 bpp, %s sig bit in each byte is leftmost on the screen.\n",
               (pf->bigEndian ? "most" : "least"));
    } else {
        rfbLog("  %d bpp, depth %d%s\n",pf->bitsPerPixel,pf->depth,
               ((pf->bitsPerPixel == 8) ? ""
                : (pf->bigEndian ? ", big endian" : ", little endian")));
        if (pf->trueColour) {
            rfbLog("  true colour: max r %d g %d b %d, shift r %d g %d b %d\n",
                   pf->redMax, pf->greenMax, pf->blueMax,
                   pf->redShift, pf->greenShift, pf->blueShift);
        } else {
            rfbLog("  uses a colour map (not true colour).\n");
        }
    }
}

///////////////////////////////////////////end translate.c //////////////////////////////////////////////////

///////////////////////////////////////////begin cutpaste.c //////////////////////////////////////////////////
/*
 * cutpaste.c - routines to deal with cut & paste buffers / selection.
 */

/*
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>


/*
 * rfbSetXCutText sets the cut buffer to be the given string.  We also clear
 * the primary selection.  Ideally we'd like to set it to the same thing, but I
 * can't work out how to do that without some kind of helper X client.
 */

void rfbGotXCutText(rfbScreenInfoPtr rfbScreen, char *str, int len)
{
   rfbSendServerCutText(rfbScreen, str, len);
}
///////////////////////////////////////////end cutpaste.c //////////////////////////////////////////////////

///////////////////////////////////////////begin httpd.c //////////////////////////////////////////////////
/*
 * httpd.c - a simple HTTP server
 */

/*
 *  Copyright (C) 2002 RealVNC Ltd.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>

#include <ctype.h>
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <errno.h>

#ifdef WIN32
#include <winsock.h>
#define close closesocket
#else
#ifdef LIBVNCSERVER_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif
#include <pwd.h>
#endif

#ifdef USE_LIBWRAP
#include <tcpd.h>
#endif

#define connection_close
#ifndef connection_close

#define NOT_FOUND_STR "HTTP/1.0 404 Not found\r\n\r\n" \
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n" \
    "<BODY><H1>File Not Found</H1></BODY>\n"

#define INVALID_REQUEST_STR "HTTP/1.0 400 Invalid Request\r\n\r\n" \
    "<HEAD><TITLE>Invalid Request</TITLE></HEAD>\n" \
    "<BODY><H1>Invalid request</H1></BODY>\n"

#define OK_STR "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"

#else

#define NOT_FOUND_STR "HTTP/1.0 404 Not found\r\nConnection: close\r\n\r\n" \
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n" \
    "<BODY><H1>File Not Found</H1></BODY>\n"

#define INVALID_REQUEST_STR "HTTP/1.0 400 Invalid Request\r\nConnection: close\r\n\r\n" \
    "<HEAD><TITLE>Invalid Request</TITLE></HEAD>\n" \
    "<BODY><H1>Invalid request</H1></BODY>\n"

#define OK_STR "HTTP/1.0 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"

#endif

static void httpProcessInput(rfbScreenInfoPtr screen);
static rfbBool compareAndSkip(char **ptr, const char *str);
static rfbBool parseParams(const char *request, char *result, int max_bytes);
static rfbBool validateString(char *str);

#define BUF_SIZE 32768

static char buf[BUF_SIZE];
static size_t buf_filled=0;

/*
 * httpInitSockets sets up the TCP socket to listen for HTTP connections.
 */

void
rfbHttpInitSockets(rfbScreenInfoPtr rfbScreen)
{
    if (rfbScreen->httpInitDone)
	return;

    rfbScreen->httpInitDone = TRUE;

    if (!rfbScreen->httpDir)
	return;

    if (rfbScreen->httpPort == 0) {
	rfbScreen->httpPort = rfbScreen->port-100;
    }

    rfbLog("Listening for HTTP connections on TCP port %d\n", rfbScreen->httpPort);

    rfbLog("  URL http://%s:%d\n",rfbScreen->thisHost,rfbScreen->httpPort);

    if ((rfbScreen->httpListenSock =
      rfbListenOnTCPPort(rfbScreen->httpPort, rfbScreen->listenInterface)) < 0) {
	rfbLogPerror("ListenOnTCPPort");
	return;
    }

   /*AddEnabledDevice(httpListenSock);*/
}

void rfbHttpShutdownSockets(rfbScreenInfoPtr rfbScreen) {
    if(rfbScreen->httpSock>-1) {
	close(rfbScreen->httpSock);
	FD_CLR(rfbScreen->httpSock,&rfbScreen->allFds);
	rfbScreen->httpSock=-1;
    }
}

/*
 * httpCheckFds is called from ProcessInputEvents to check for input on the
 * HTTP socket(s).  If there is input to process, httpProcessInput is called.
 */

void
rfbHttpCheckFds(rfbScreenInfoPtr rfbScreen)
{
    int nfds;
    fd_set fds;
    struct timeval tv;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    if (!rfbScreen->httpDir)
	return;

    if (rfbScreen->httpListenSock < 0)
	return;

    FD_ZERO(&fds);
    FD_SET(rfbScreen->httpListenSock, &fds);
    if (rfbScreen->httpSock >= 0) {
	FD_SET(rfbScreen->httpSock, &fds);
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    nfds = select(max(rfbScreen->httpSock,rfbScreen->httpListenSock) + 1, &fds, NULL, NULL, &tv);
    if (nfds == 0) {
	return;
    }
    if (nfds < 0) {
#ifdef WIN32
		errno = WSAGetLastError();
#endif
	if (errno != EINTR)
		rfbLogPerror("httpCheckFds: select");
	return;
    }

    if ((rfbScreen->httpSock >= 0) && FD_ISSET(rfbScreen->httpSock, &fds)) {
	httpProcessInput(rfbScreen);
    }

    if (FD_ISSET(rfbScreen->httpListenSock, &fds)) {
        int flags;
	if (rfbScreen->httpSock >= 0) close(rfbScreen->httpSock);

	if ((rfbScreen->httpSock = accept(rfbScreen->httpListenSock,
			       (struct sockaddr *)&addr, &addrlen)) < 0) {
	    rfbLogPerror("httpCheckFds: accept");
	    return;
	}
#ifdef __MINGW32__
	rfbErr("O_NONBLOCK on MinGW32 NOT IMPLEMENTED");
#else
#ifdef USE_LIBWRAP
	if(!hosts_ctl("vnc",STRING_UNKNOWN,inet_ntoa(addr.sin_addr),
		      STRING_UNKNOWN)) {
	  rfbLog("Rejected HTTP connection from client %s\n",
		 inet_ntoa(addr.sin_addr));
#else
	flags = fcntl(rfbScreen->httpSock, F_GETFL);

	if (flags < 0 || fcntl(rfbScreen->httpSock, F_SETFL, flags | O_NONBLOCK) == -1) {
	    rfbLogPerror("httpCheckFds: fcntl");
#endif
	    close(rfbScreen->httpSock);
	    rfbScreen->httpSock = -1;
	    return;
	}

	flags=fcntl(rfbScreen->httpSock,F_GETFL);
	if(flags==-1 ||
	   fcntl(rfbScreen->httpSock,F_SETFL,flags|O_NONBLOCK)==-1) {
	  rfbLogPerror("httpCheckFds: fcntl");
	  close(rfbScreen->httpSock);
	  rfbScreen->httpSock=-1;
	  return;
	}
#endif

	/*AddEnabledDevice(httpSock);*/
    }
}


static void
httpCloseSock(rfbScreenInfoPtr rfbScreen)
{
    close(rfbScreen->httpSock);
    rfbScreen->httpSock = -1;
    buf_filled = 0;
}

static rfbClientRec cl;

/*
 * httpProcessInput is called when input is received on the HTTP socket.
 */

static void
httpProcessInput(rfbScreenInfoPtr rfbScreen)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    char fullFname[512];
    char params[1024];
    char *ptr;
    char *fname;
    unsigned int maxFnameLen;
    FILE* fd;
    rfbBool performSubstitutions = FALSE;
    char str[256+32];
#ifndef WIN32
    char* user=getenv("USER");
#endif
   
    cl.sock=rfbScreen->httpSock;

    if (strlen(rfbScreen->httpDir) > 255) {
	rfbErr("-httpd directory too long\n");
	httpCloseSock(rfbScreen);
	return;
    }
    strcpy(fullFname, rfbScreen->httpDir);
    fname = &fullFname[strlen(fullFname)];
    maxFnameLen = 511 - strlen(fullFname);

    buf_filled=0;

    /* Read data from the HTTP client until we get a complete request. */
    while (1) {
	ssize_t got;

        if (buf_filled > sizeof (buf)) {
	    rfbErr("httpProcessInput: HTTP request is too long\n");
	    httpCloseSock(rfbScreen);
	    return;
	}

	got = read (rfbScreen->httpSock, buf + buf_filled,
			    sizeof (buf) - buf_filled - 1);

	if (got <= 0) {
	    if (got == 0) {
		rfbErr("httpd: premature connection close\n");
	    } else {
		if (errno == EAGAIN) {
		    return;
		}
		rfbLogPerror("httpProcessInput: read");
	    }
	    httpCloseSock(rfbScreen);
	    return;
	}

	buf_filled += got;
	buf[buf_filled] = '\0';

	/* Is it complete yet (is there a blank line)? */
	if (strstr (buf, "\r\r") || strstr (buf, "\n\n") ||
	    strstr (buf, "\r\n\r\n") || strstr (buf, "\n\r\n\r"))
	    break;
    }


    /* Process the request. */
    if(rfbScreen->httpEnableProxyConnect) {
	const static char* PROXY_OK_STR = "HTTP/1.0 200 OK\r\nContent-Type: octet-stream\r\nPragma: no-cache\r\n\r\n";
	if(!strncmp(buf, "CONNECT ", 8)) {
	    if(atoi(strchr(buf, ':')+1)!=rfbScreen->port) {
		rfbErr("httpd: CONNECT format invalid.\n");
		rfbWriteExact(&cl,INVALID_REQUEST_STR, strlen(INVALID_REQUEST_STR));
		httpCloseSock(rfbScreen);
		return;
	    }
	    /* proxy connection */
	    rfbLog("httpd: client asked for CONNECT\n");
	    rfbWriteExact(&cl,PROXY_OK_STR,strlen(PROXY_OK_STR));
	    rfbNewClientConnection(rfbScreen,rfbScreen->httpSock);
	    rfbScreen->httpSock = -1;
	    return;
	}
	if (!strncmp(buf, "GET ",4) && !strncmp(strchr(buf,'/'),"/proxied.connection HTTP/1.", 27)) {
	    /* proxy connection */
	    rfbLog("httpd: client asked for /proxied.connection\n");
	    rfbWriteExact(&cl,PROXY_OK_STR,strlen(PROXY_OK_STR));
	    rfbNewClientConnection(rfbScreen,rfbScreen->httpSock);
	    rfbScreen->httpSock = -1;
	    return;
	}	   
    }

    if (strncmp(buf, "GET ", 4)) {
	rfbErr("httpd: no GET line\n");
	httpCloseSock(rfbScreen);
	return;
    } else {
	/* Only use the first line. */
	buf[strcspn(buf, "\n\r")] = '\0';
    }

    if (strlen(buf) > maxFnameLen) {
	rfbErr("httpd: GET line too long\n");
	httpCloseSock(rfbScreen);
	return;
    }

    if (sscanf(buf, "GET %s HTTP/1.", fname) != 1) {
	rfbErr("httpd: couldn't parse GET line\n");
	httpCloseSock(rfbScreen);
	return;
    }

    if (fname[0] != '/') {
	rfbErr("httpd: filename didn't begin with '/'\n");
	rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock(rfbScreen);
	return;
    }

    if (strchr(fname+1, '/') != NULL) {
	rfbErr("httpd: asking for file in other directory\n");
	rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock(rfbScreen);
	return;
    }

    getpeername(rfbScreen->httpSock, (struct sockaddr *)&addr, &addrlen);
    rfbLog("httpd: get '%s' for %s\n", fname+1,
	   inet_ntoa(addr.sin_addr));

    /* Extract parameters from the URL string if necessary */

    params[0] = '\0';
    ptr = strchr(fname, '?');
    if (ptr != NULL) {
       *ptr = '\0';
       if (!parseParams(&ptr[1], params, 1024)) {
           params[0] = '\0';
           rfbErr("httpd: bad parameters in the URL\n");
       }
    }


    /* If we were asked for '/', actually read the file index.vnc */

    if (strcmp(fname, "/") == 0) {
	strcpy(fname, "/index.vnc");
	rfbLog("httpd: defaulting to '%s'\n", fname+1);
    }

    /* Substitutions are performed on files ending .vnc */

    if (strlen(fname) >= 4 && strcmp(&fname[strlen(fname)-4], ".vnc") == 0) {
	performSubstitutions = TRUE;
    }

    /* Open the file */

    if ((fd = fopen(fullFname, "r")) == 0) {
        rfbLogPerror("httpProcessInput: open");
        rfbWriteExact(&cl, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
        httpCloseSock(rfbScreen);
        return;
    }

    rfbWriteExact(&cl, OK_STR, strlen(OK_STR));

    while (1) {
	int n = fread(buf, 1, BUF_SIZE-1, fd);
	if (n < 0) {
	    rfbLogPerror("httpProcessInput: read");
	    fclose(fd);
	    httpCloseSock(rfbScreen);
	    return;
	}

	if (n == 0)
	    break;

	if (performSubstitutions) {

	    /* Substitute $WIDTH, $HEIGHT, etc with the appropriate values.
	       This won't quite work properly if the .vnc file is longer than
	       BUF_SIZE, but it's reasonable to assume that .vnc files will
	       always be short. */

	    char *ptr = buf;
	    char *dollar;
	    buf[n] = 0; /* make sure it's null-terminated */

	    while ((dollar = strchr(ptr, '$'))!=NULL) {
		rfbWriteExact(&cl, ptr, (dollar - ptr));

		ptr = dollar;

		if (compareAndSkip(&ptr, "$WIDTH")) {

		    sprintf(str, "%d", rfbScreen->width);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$HEIGHT")) {

		    sprintf(str, "%d", rfbScreen->height);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETWIDTH")) {

		    sprintf(str, "%d", rfbScreen->width);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETHEIGHT")) {

		    sprintf(str, "%d", rfbScreen->height + 32);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$PORT")) {

		    sprintf(str, "%d", rfbScreen->port);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$DESKTOP")) {

		    rfbWriteExact(&cl, rfbScreen->desktopName, strlen(rfbScreen->desktopName));

		} else if (compareAndSkip(&ptr, "$DISPLAY")) {

		    sprintf(str, "%s:%d", rfbScreen->thisHost, rfbScreen->port-5900);
		    rfbWriteExact(&cl, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$USER")) {
#ifndef WIN32
		    if (user) {
			rfbWriteExact(&cl, user,
				   strlen(user));
		    } else
#endif
			rfbWriteExact(&cl, "?", 1);
		} else if (compareAndSkip(&ptr, "$PARAMS")) {
		    if (params[0] != '\0')
			rfbWriteExact(&cl, params, strlen(params));
		} else {
		    if (!compareAndSkip(&ptr, "$$"))
			ptr++;

		    if (rfbWriteExact(&cl, "$", 1) < 0) {
			fclose(fd);
			httpCloseSock(rfbScreen);
			return;
		    }
		}
	    }
	    if (rfbWriteExact(&cl, ptr, (&buf[n] - ptr)) < 0)
		break;

	} else {

	    /* For files not ending .vnc, just write out the buffer */

	    if (rfbWriteExact(&cl, buf, n) < 0)
		break;
	}
    }

    fclose(fd);
    httpCloseSock(rfbScreen);
}


static rfbBool
compareAndSkip(char **ptr, const char *str)
{
    if (strncmp(*ptr, str, strlen(str)) == 0) {
	*ptr += strlen(str);
	return TRUE;
    }

    return FALSE;
}

/*
 * Parse the request tail after the '?' character, and format a sequence
 * of <param> tags for inclusion into an HTML page with embedded applet.
 */

static rfbBool
parseParams(const char *request, char *result, int max_bytes)
{
    char param_request[128];
    char param_formatted[196];
    const char *tail;
    char *delim_ptr;
    char *value_str;
    int cur_bytes, len;

    result[0] = '\0';
    cur_bytes = 0;

    tail = request;
    for (;;) {
	/* Copy individual "name=value" string into a buffer */
	delim_ptr = strchr((char *)tail, '&');
	if (delim_ptr == NULL) {
	    if (strlen(tail) >= sizeof(param_request)) {
		return FALSE;
	    }
	    strcpy(param_request, tail);
	} else {
	    len = delim_ptr - tail;
	    if (len >= sizeof(param_request)) {
		return FALSE;
	    }
	    memcpy(param_request, tail, len);
	    param_request[len] = '\0';
	}

	/* Split the request into parameter name and value */
	value_str = strchr(&param_request[1], '=');
	if (value_str == NULL) {
	    return FALSE;
	}
	*value_str++ = '\0';
	if (strlen(value_str) == 0) {
	    return FALSE;
	}

	/* Validate both parameter name and value */
	if (!validateString(param_request) || !validateString(value_str)) {
	    return FALSE;
	}

	/* Prepare HTML-formatted representation of the name=value pair */
	len = sprintf(param_formatted,
		      "<PARAM NAME=\"%s\" VALUE=\"%s\">\n",
		      param_request, value_str);
	if (cur_bytes + len + 1 > max_bytes) {
	    return FALSE;
	}
	strcat(result, param_formatted);
	cur_bytes += len;

	/* Go to the next parameter */
	if (delim_ptr == NULL) {
	    break;
	}
	tail = delim_ptr + 1;
    }
    return TRUE;
}

/*
 * Check if the string consists only of alphanumeric characters, '+'
 * signs, underscores, and dots. Replace all '+' signs with spaces.
 */

static rfbBool
validateString(char *str)
{
    char *ptr;

    for (ptr = str; *ptr != '\0'; ptr++) {
	if (!isalnum(*ptr) && *ptr != '_' && *ptr != '.') {
	    if (*ptr == '+') {
		*ptr = ' ';
	    } else {
		return FALSE;
	    }
	}
    }
    return TRUE;
}


///////////////////////////////////////////end httpd.c //////////////////////////////////////////////////

///////////////////////////////////////////begin cursor.c //////////////////////////////////////////////////
/*
 * cursor.c - support for cursor shape updates.
 */

/*
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#include "private.h"

void rfbScaledScreenUpdate(rfbScreenInfoPtr screen, int x1, int y1, int x2, int y2);

/*
 * Send cursor shape either in X-style format or in client pixel format.
 */

rfbBool
rfbSendCursorShape(rfbClientPtr cl)
{
    rfbCursorPtr pCursor;
    rfbFramebufferUpdateRectHeader rect;
    rfbXCursorColors colors;
    int saved_ublen;
    int bitmapRowBytes, maskBytes, dataBytes;
    int i, j;
    uint8_t *bitmapData;
    uint8_t bitmapByte;

    /* TODO: scale the cursor data to the correct size */

    pCursor = cl->screen->getCursorPtr(cl);
    /*if(!pCursor) return TRUE;*/

    if (cl->useRichCursorEncoding) {
      if(pCursor && !pCursor->richSource)
	rfbMakeRichCursorFromXCursor(cl->screen,pCursor);
      rect.encoding = Swap32IfLE(rfbEncodingRichCursor);
    } else {
       if(pCursor && !pCursor->source)
	 rfbMakeXCursorFromRichCursor(cl->screen,pCursor);
       rect.encoding = Swap32IfLE(rfbEncodingXCursor);
    }

    /* If there is no cursor, send update with empty cursor data. */

    if ( pCursor && pCursor->width == 1 &&
	 pCursor->height == 1 &&
	 pCursor->mask[0] == 0 ) {
	pCursor = NULL;
    }

    if (pCursor == NULL) {
	if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE ) {
	    if (!rfbSendUpdateBuf(cl))
		return FALSE;
	}
	rect.r.x = rect.r.y = 0;
	rect.r.w = rect.r.h = 0;
	memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
	       sz_rfbFramebufferUpdateRectHeader);
	cl->ublen += sz_rfbFramebufferUpdateRectHeader;

	if (!rfbSendUpdateBuf(cl))
	    return FALSE;

	return TRUE;
    }

    /* Calculate data sizes. */

    bitmapRowBytes = (pCursor->width + 7) / 8;
    maskBytes = bitmapRowBytes * pCursor->height;
    dataBytes = (cl->useRichCursorEncoding) ?
	(pCursor->width * pCursor->height *
	 (cl->format.bitsPerPixel / 8)) : maskBytes;

    /* Send buffer contents if needed. */

    if ( cl->ublen + sz_rfbFramebufferUpdateRectHeader +
	 sz_rfbXCursorColors + maskBytes + dataBytes > UPDATE_BUF_SIZE ) {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    if ( cl->ublen + sz_rfbFramebufferUpdateRectHeader +
	 sz_rfbXCursorColors + maskBytes + dataBytes > UPDATE_BUF_SIZE ) {
	return FALSE;		/* FIXME. */
    }

    saved_ublen = cl->ublen;

    /* Prepare rectangle header. */

    rect.r.x = Swap16IfLE(pCursor->xhot);
    rect.r.y = Swap16IfLE(pCursor->yhot);
    rect.r.w = Swap16IfLE(pCursor->width);
    rect.r.h = Swap16IfLE(pCursor->height);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    /* Prepare actual cursor data (depends on encoding used). */

    if (!cl->useRichCursorEncoding) {
	/* XCursor encoding. */
	colors.foreRed   = (char)(pCursor->foreRed   >> 8);
	colors.foreGreen = (char)(pCursor->foreGreen >> 8);
	colors.foreBlue  = (char)(pCursor->foreBlue  >> 8);
	colors.backRed   = (char)(pCursor->backRed   >> 8);
	colors.backGreen = (char)(pCursor->backGreen >> 8);
	colors.backBlue  = (char)(pCursor->backBlue  >> 8);

	memcpy(&cl->updateBuf[cl->ublen], (char *)&colors, sz_rfbXCursorColors);
	cl->ublen += sz_rfbXCursorColors;

	bitmapData = (uint8_t *)pCursor->source;

	for (i = 0; i < pCursor->height; i++) {
	    for (j = 0; j < bitmapRowBytes; j++) {
		bitmapByte = bitmapData[i * bitmapRowBytes + j];
		cl->updateBuf[cl->ublen++] = (char)bitmapByte;
	    }
	}
    } else {
	/* RichCursor encoding. */
       int bpp1=cl->screen->serverFormat.bitsPerPixel/8,
	 bpp2=cl->format.bitsPerPixel/8;
       (*cl->translateFn)(cl->translateLookupTable,
		       &(cl->screen->serverFormat),
                       &cl->format, (char*)pCursor->richSource,
		       &cl->updateBuf[cl->ublen],
                       pCursor->width*bpp1, pCursor->width, pCursor->height);

       cl->ublen += pCursor->width*bpp2*pCursor->height;
    }

    /* Prepare transparency mask. */

    bitmapData = (uint8_t *)pCursor->mask;

    for (i = 0; i < pCursor->height; i++) {
	for (j = 0; j < bitmapRowBytes; j++) {
	    bitmapByte = bitmapData[i * bitmapRowBytes + j];
	    cl->updateBuf[cl->ublen++] = (char)bitmapByte;
	}
    }

    /* Send everything we have prepared in the cl->updateBuf[]. */
    rfbStatRecordEncodingSent(cl, (cl->useRichCursorEncoding ? rfbEncodingRichCursor : rfbEncodingXCursor), 
        sz_rfbFramebufferUpdateRectHeader + (cl->ublen - saved_ublen), sz_rfbFramebufferUpdateRectHeader + (cl->ublen - saved_ublen));

    if (!rfbSendUpdateBuf(cl))
	return FALSE;

    return TRUE;
}

/*
 * Send cursor position (PointerPos pseudo-encoding).
 */

rfbBool
rfbSendCursorPos(rfbClientPtr cl)
{
  rfbFramebufferUpdateRectHeader rect;

  if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
    if (!rfbSendUpdateBuf(cl))
      return FALSE;
  }

  rect.encoding = Swap32IfLE(rfbEncodingPointerPos);
  rect.r.x = Swap16IfLE(cl->screen->cursorX);
  rect.r.y = Swap16IfLE(cl->screen->cursorY);
  rect.r.w = 0;
  rect.r.h = 0;

  memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
	 sz_rfbFramebufferUpdateRectHeader);
  cl->ublen += sz_rfbFramebufferUpdateRectHeader;

  rfbStatRecordEncodingSent(cl, rfbEncodingPointerPos, sz_rfbFramebufferUpdateRectHeader, sz_rfbFramebufferUpdateRectHeader);

  if (!rfbSendUpdateBuf(cl))
    return FALSE;

  return TRUE;
}

/* conversion routine for predefined cursors in LSB order */
unsigned char rfbReverseByte[0x100] = {
  /* copied from Xvnc/lib/font/util/utilbitmap.c */
	0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

void rfbConvertLSBCursorBitmapOrMask(int width,int height,unsigned char* bitmap)
{
   int i,t=(width+7)/8*height;
   for(i=0;i<t;i++)
     bitmap[i]=rfbReverseByte[(int)bitmap[i]];
}

/* Cursor creation. You "paint" a cursor and let these routines do the work */

rfbCursorPtr rfbMakeXCursor(int width,int height,char* cursorString,char* maskString)
{
   int i,j,w=(width+7)/8;
   rfbCursorPtr cursor = (rfbCursorPtr)calloc(1,sizeof(rfbCursor));
   char* cp;
   unsigned char bit;

   cursor->cleanup=TRUE;
   cursor->width=width;
   cursor->height=height;
   /*cursor->backRed=cursor->backGreen=cursor->backBlue=0xffff;*/
   cursor->foreRed=cursor->foreGreen=cursor->foreBlue=0xffff;
   
   cursor->source = (unsigned char*)calloc(w,height);
   cursor->cleanupSource = TRUE;
   for(j=0,cp=cursorString;j<height;j++)
      for(i=0,bit=0x80;i<width;i++,bit=(bit&1)?0x80:bit>>1,cp++)
	if(*cp!=' ') cursor->source[j*w+i/8]|=bit;

   if(maskString) {
      cursor->mask = (unsigned char*)calloc(w,height);
      for(j=0,cp=maskString;j<height;j++)
	for(i=0,bit=0x80;i<width;i++,bit=(bit&1)?0x80:bit>>1,cp++)
	  if(*cp!=' ') cursor->mask[j*w+i/8]|=bit;
   } else
     cursor->mask = (unsigned char*)rfbMakeMaskForXCursor(width,height,(char*)cursor->source);
   cursor->cleanupMask = TRUE;

   return(cursor);
}

char* rfbMakeMaskForXCursor(int width,int height,char* source)
{
   int i,j,w=(width+7)/8;
   char* mask=(char*)calloc(w,height);
   unsigned char c;
   
   for(j=0;j<height;j++)
     for(i=w-1;i>=0;i--) {
	c=source[j*w+i];
	if(j>0) c|=source[(j-1)*w+i];
	if(j<height-1) c|=source[(j+1)*w+i];
	
	if(i>0 && (c&0x80))
	  mask[j*w+i-1]|=0x01;
	if(i<w-1 && (c&0x01))
	  mask[j*w+i+1]|=0x80;
	
	mask[j*w+i]|=(c<<1)|c|(c>>1);
     }
   
   return(mask);
}

/* this function dithers the alpha using Floyd-Steinberg */

char* rfbMakeMaskFromAlphaSource(int width,int height,unsigned char* alphaSource)
{
	int* error=(int*)calloc(sizeof(int),width);
	int i,j,currentError=0,maskStride=(width+7)/8;
	unsigned char* result=(unsigned char*)calloc(maskStride,height);
	
	for(j=0;j<height;j++)
		for(i=0;i<width;i++) {
			int right,middle,left;
			currentError+=alphaSource[i+width*j]+error[i];
		
			if(currentError<0x80) {
				/* set to transparent */
				/* alpha was treated as 0 */
			} else {
				/* set to solid */
				result[i/8+j*maskStride]|=(0x100>>(i&7));
				/* alpha was treated as 0xff */
				currentError-=0xff;
			}
			/* propagate to next row */
			right=currentError/16;
			middle=currentError*5/16;
			left=currentError*3/16;
			currentError-=right+middle+left;
			error[i]=right;
			if(i>0) {
				error[i-1]=middle;
				if(i>1)
					error[i-2]=left;
			}
		}
	free(error);
	return (char *) result;
}

void rfbFreeCursor(rfbCursorPtr cursor)
{
   if(cursor) {
       if(cursor->cleanupRichSource && cursor->richSource)
	   free(cursor->richSource);
       if(cursor->cleanupRichSource && cursor->alphaSource)
	   free(cursor->alphaSource);
       if(cursor->cleanupSource && cursor->source)
	   free(cursor->source);
       if(cursor->cleanupMask && cursor->mask)
	   free(cursor->mask);
       if(cursor->cleanup)
	   free(cursor);
       else {
	   cursor->cleanup=cursor->cleanupSource=cursor->cleanupMask
	       =cursor->cleanupRichSource=FALSE;
	   cursor->source=cursor->mask=cursor->richSource=NULL;
	   cursor->alphaSource=NULL;
       }
   }
   
}

/* background and foregroud colour have to be set beforehand */
void rfbMakeXCursorFromRichCursor(rfbScreenInfoPtr rfbScreen,rfbCursorPtr cursor)
{
   rfbPixelFormat* format=&rfbScreen->serverFormat;
   int i,j,w=(cursor->width+7)/8,bpp=format->bitsPerPixel/8,
     width=cursor->width*bpp;
   uint32_t background;
   char *back=(char*)&background;
   unsigned char bit;
   int interp = 0, db = 0;

   if(cursor->source && cursor->cleanupSource)
       free(cursor->source);
   cursor->source=(unsigned char*)calloc(w,cursor->height);
   cursor->cleanupSource=TRUE;
   
   if(format->bigEndian) {
      back+=4-bpp;
   }

	/* all zeros means we should interpolate to black+white ourselves */
	if (!cursor->backRed && !cursor->backGreen && !cursor->backBlue &&
	    !cursor->foreRed && !cursor->foreGreen && !cursor->foreBlue) {
		if (format->trueColour && (bpp == 1 || bpp == 2 || bpp == 4)) {
			interp = 1;
			cursor->foreRed = cursor->foreGreen = cursor->foreBlue = 0xffff;
		}
	}

   background = ((format->redMax   * cursor->backRed)   / 0xffff) << format->redShift   |
                ((format->greenMax * cursor->backGreen) / 0xffff) << format->greenShift |
                ((format->blueMax  * cursor->backBlue)  / 0xffff) << format->blueShift;

#define SETRGB(u) \
   r = (255 * (((format->redMax   << format->redShift)   & (*u)) >> format->redShift))   / format->redMax; \
   g = (255 * (((format->greenMax << format->greenShift) & (*u)) >> format->greenShift)) / format->greenMax; \
   b = (255 * (((format->blueMax  << format->blueShift)  & (*u)) >> format->blueShift))  / format->blueMax;

   if (db) fprintf(stderr, "interp: %d\n", interp);

   for(j=0;j<cursor->height;j++) {
	for(i=0,bit=0x80;i<cursor->width;i++,bit=(bit&1)?0x80:bit>>1) {
		if (interp) {
			int r = 0, g = 0, b = 0, grey;
			char *p = cursor->richSource+j*width+i*bpp;
			if (bpp == 1) {
				unsigned char*  uc = (unsigned char*)  p;
				SETRGB(uc);
			} else if (bpp == 2) {
				unsigned short* us = (unsigned short*) p;
				SETRGB(us);
			} else if (bpp == 4) {
				unsigned int*   ui = (unsigned int*)   p;
				SETRGB(ui);
			}
			grey = (r + g + b) / 3;
			if (grey >= 128) {
				cursor->source[j*w+i/8]|=bit;
				if (db) fprintf(stderr, "1");
			} else {
				if (db) fprintf(stderr, "0");
			}
			
		} else if(memcmp(cursor->richSource+j*width+i*bpp, back, bpp)) {
			cursor->source[j*w+i/8]|=bit;
		}
	}
	if (db) fprintf(stderr, "\n");
   }
}

void rfbMakeRichCursorFromXCursor(rfbScreenInfoPtr rfbScreen,rfbCursorPtr cursor)
{
   rfbPixelFormat* format=&rfbScreen->serverFormat;
   int i,j,w=(cursor->width+7)/8,bpp=format->bitsPerPixel/8;
   uint32_t background,foreground;
   char *back=(char*)&background,*fore=(char*)&foreground;
   unsigned char *cp;
   unsigned char bit;

   if(cursor->richSource && cursor->cleanupRichSource)
       free(cursor->richSource);
   cp=cursor->richSource=(unsigned char*)calloc(cursor->width*bpp,cursor->height);
   cursor->cleanupRichSource=TRUE;
   
   if(format->bigEndian) {
      back+=4-bpp;
      fore+=4-bpp;
   }

   background=cursor->backRed<<format->redShift|
     cursor->backGreen<<format->greenShift|cursor->backBlue<<format->blueShift;
   foreground=cursor->foreRed<<format->redShift|
     cursor->foreGreen<<format->greenShift|cursor->foreBlue<<format->blueShift;
   
   for(j=0;j<cursor->height;j++)
     for(i=0,bit=0x80;i<cursor->width;i++,bit=(bit&1)?0x80:bit>>1,cp+=bpp)
       if(cursor->source[j*w+i/8]&bit) memcpy(cp,fore,bpp);
       else memcpy(cp,back,bpp);
}

/* functions to draw/hide cursor directly in the frame buffer */

void rfbHideCursor(rfbClientPtr cl)
{
   rfbScreenInfoPtr s=cl->screen;
   rfbCursorPtr c=s->cursor;
   int j,x1,x2,y1,y2,bpp=s->serverFormat.bitsPerPixel/8,
     rowstride=s->paddedWidthInBytes;
   LOCK(s->cursorMutex);
   if(!c) {
     UNLOCK(s->cursorMutex);
     return;
   }
   
   /* restore what is under the cursor */
   x1=cl->cursorX-c->xhot;
   x2=x1+c->width;
   if(x1<0) x1=0;
   if(x2>=s->width) x2=s->width-1;
   x2-=x1; if(x2<=0) {
     UNLOCK(s->cursorMutex);
     return;
   }
   y1=cl->cursorY-c->yhot;
   y2=y1+c->height;
   if(y1<0) y1=0;
   if(y2>=s->height) y2=s->height-1;
   y2-=y1; if(y2<=0) {
     UNLOCK(s->cursorMutex);
     return;
   }

   /* get saved data */
   for(j=0;j<y2;j++)
     memcpy(s->frameBuffer+(y1+j)*rowstride+x1*bpp,
	    s->underCursorBuffer+j*x2*bpp,
	    x2*bpp);

   /* Copy to all scaled versions */
   rfbScaledScreenUpdate(s, x1, y1, x1+x2, y1+y2);
   
   UNLOCK(s->cursorMutex);
}

void rfbShowCursor(rfbClientPtr cl)
{
   rfbScreenInfoPtr s=cl->screen;
   rfbCursorPtr c=s->cursor;
   int i,j,x1,x2,y1,y2,i1,j1,bpp=s->serverFormat.bitsPerPixel/8,
     rowstride=s->paddedWidthInBytes,
     bufSize,w;
   rfbBool wasChanged=FALSE;

   if(!c) return;
   LOCK(s->cursorMutex);

   bufSize=c->width*c->height*bpp;
   w=(c->width+7)/8;
   if(s->underCursorBufferLen<bufSize) {
      if(s->underCursorBuffer!=NULL)
	free(s->underCursorBuffer);
      s->underCursorBuffer=malloc(bufSize);
      s->underCursorBufferLen=bufSize;
   }

   /* save what is under the cursor */
   i1=j1=0; /* offset in cursor */
   x1=cl->cursorX-c->xhot;
   x2=x1+c->width;
   if(x1<0) { i1=-x1; x1=0; }
   if(x2>=s->width) x2=s->width-1;
   x2-=x1; if(x2<=0) {
     UNLOCK(s->cursorMutex);
     return; /* nothing to do */
   }

   y1=cl->cursorY-c->yhot;
   y2=y1+c->height;
   if(y1<0) { j1=-y1; y1=0; }
   if(y2>=s->height) y2=s->height-1;
   y2-=y1; if(y2<=0) {
     UNLOCK(s->cursorMutex);
     return; /* nothing to do */
   }

   /* save data */
   for(j=0;j<y2;j++) {
     char* dest=s->underCursorBuffer+j*x2*bpp;
     const char* src=s->frameBuffer+(y1+j)*rowstride+x1*bpp;
     unsigned int count=x2*bpp;
     if(wasChanged || memcmp(dest,src,count)) {
       wasChanged=TRUE;
       memcpy(dest,src,count);
     }
   }
   
   if(!c->richSource)
     rfbMakeRichCursorFromXCursor(s,c);
  
   if (c->alphaSource) {
	int rmax, rshift;
	int gmax, gshift;
	int bmax, bshift;
	int amax = 255;	/* alphaSource is always 8bits of info per pixel */
	unsigned int rmask, gmask, bmask;

	rmax   = s->serverFormat.redMax;
	gmax   = s->serverFormat.greenMax;
	bmax   = s->serverFormat.blueMax;
	rshift = s->serverFormat.redShift;
	gshift = s->serverFormat.greenShift;
	bshift = s->serverFormat.blueShift;

	rmask = (rmax << rshift);
	gmask = (gmax << gshift);
	bmask = (bmax << bshift);

	for(j=0;j<y2;j++) {
		for(i=0;i<x2;i++) {
			/*
			 * we loop over the whole cursor ignoring c->mask[],
			 * using the extracted alpha value instead.
			 */
			char *dest;
			unsigned char *src, *aptr;
			unsigned int val, dval, sval;
			int rdst, gdst, bdst;		/* fb RGB */
			int asrc, rsrc, gsrc, bsrc;	/* rich source ARGB */

			dest = s->frameBuffer + (j+y1)*rowstride + (i+x1)*bpp;
			src  = c->richSource  + (j+j1)*c->width*bpp + (i+i1)*bpp;
			aptr = c->alphaSource + (j+j1)*c->width + (i+i1);

			asrc = *aptr;
			if (!asrc) {
				continue;
			}

			if (bpp == 1) {
				dval = *((unsigned char*) dest);
				sval = *((unsigned char*) src);
			} else if (bpp == 2) {
				dval = *((unsigned short*) dest);
				sval = *((unsigned short*) src);
			} else if (bpp == 3) {
				unsigned char *dst = (unsigned char *) dest;
				dval = 0;
				dval |= ((*(dst+0)) << 0);
				dval |= ((*(dst+1)) << 8);
				dval |= ((*(dst+2)) << 16);
				sval = 0;
				sval |= ((*(src+0)) << 0);
				sval |= ((*(src+1)) << 8);
				sval |= ((*(src+2)) << 16);
			} else if (bpp == 4) {
				dval = *((unsigned int*) dest);
				sval = *((unsigned int*) src);
			} else {
				continue;
			}

			/* extract dest and src RGB */
			rdst = (dval & rmask) >> rshift;	/* fb */
			gdst = (dval & gmask) >> gshift;
			bdst = (dval & bmask) >> bshift;

			rsrc = (sval & rmask) >> rshift;	/* richcursor */
			gsrc = (sval & gmask) >> gshift;
			bsrc = (sval & bmask) >> bshift;

			/* blend in fb data. */
			if (! c->alphaPreMultiplied) {
				rsrc = (asrc * rsrc)/amax;
				gsrc = (asrc * gsrc)/amax;
				bsrc = (asrc * bsrc)/amax;
			}
			rdst = rsrc + ((amax - asrc) * rdst)/amax;
			gdst = gsrc + ((amax - asrc) * gdst)/amax;
			bdst = bsrc + ((amax - asrc) * bdst)/amax;

			val = 0;
			val |= (rdst << rshift);
			val |= (gdst << gshift);
			val |= (bdst << bshift);

			/* insert the cooked pixel into the fb */
			memcpy(dest, &val, bpp);
		}
	}
   } else {
      /* now the cursor has to be drawn */
      for(j=0;j<y2;j++)
        for(i=0;i<x2;i++)
          if((c->mask[(j+j1)*w+(i+i1)/8]<<((i+i1)&7))&0x80)
   	 memcpy(s->frameBuffer+(j+y1)*rowstride+(i+x1)*bpp,
   		c->richSource+(j+j1)*c->width*bpp+(i+i1)*bpp,bpp);
   }

   /* Copy to all scaled versions */
   rfbScaledScreenUpdate(s, x1, y1, x1+x2, y1+y2);

   UNLOCK(s->cursorMutex);
}

/* 
 * If enableCursorShapeUpdates is FALSE, and the cursor is hidden, make sure
 * that if the frameBuffer was transmitted with a cursor drawn, then that
 * region gets redrawn.
 */

void rfbRedrawAfterHideCursor(rfbClientPtr cl,sraRegionPtr updateRegion)
{
    rfbScreenInfoPtr s = cl->screen;
    rfbCursorPtr c = s->cursor;
    
    if(c) {
	int x,y,x2,y2;

	x = cl->cursorX-c->xhot;
	y = cl->cursorY-c->yhot;
	x2 = x+c->width;
	y2 = y+c->height;

	if(sraClipRect2(&x,&y,&x2,&y2,0,0,s->width,s->height)) {
	    sraRegionPtr rect;
	    rect = sraRgnCreateRect(x,y,x2,y2);
	    if(updateRegion)
	    	sraRgnOr(updateRegion,rect);
	    else
		    sraRgnOr(cl->modifiedRegion,rect);
	    sraRgnDestroy(rect);
	}
    }
}

#ifdef DEBUG

static void rfbPrintXCursor(rfbCursorPtr cursor)
{
   int i,i1,j,w=(cursor->width+7)/8;
   unsigned char bit;
   for(j=0;j<cursor->height;j++) {
      for(i=0,i1=0,bit=0x80;i1<cursor->width;i1++,i+=(bit&1)?1:0,bit=(bit&1)?0x80:bit>>1)
	if(cursor->source[j*w+i]&bit) putchar('#'); else putchar(' ');
      putchar(':');
      for(i=0,i1=0,bit=0x80;i1<cursor->width;i1++,i+=(bit&1)?1:0,bit=(bit&1)?0x80:bit>>1)
	if(cursor->mask[j*w+i]&bit) putchar('#'); else putchar(' ');
      putchar('\n');
   }
}

#endif

void rfbSetCursor(rfbScreenInfoPtr rfbScreen,rfbCursorPtr c)
{
  rfbClientIteratorPtr iterator;
  rfbClientPtr cl;

  LOCK(rfbScreen->cursorMutex);

  if(rfbScreen->cursor) {
    iterator=rfbGetClientIterator(rfbScreen);
    while((cl=rfbClientIteratorNext(iterator)))
	if(!cl->enableCursorShapeUpdates)
	  rfbRedrawAfterHideCursor(cl,NULL);
    rfbReleaseClientIterator(iterator);

    if(rfbScreen->cursor->cleanup)
	 rfbFreeCursor(rfbScreen->cursor);
  }

  rfbScreen->cursor = c;

  iterator=rfbGetClientIterator(rfbScreen);
  while((cl=rfbClientIteratorNext(iterator))) {
    cl->cursorWasChanged = TRUE;
    if(!cl->enableCursorShapeUpdates)
      rfbRedrawAfterHideCursor(cl,NULL);
  }
  rfbReleaseClientIterator(iterator);

  UNLOCK(rfbScreen->cursorMutex);
}

///////////////////////////////////////////end cursor.c //////////////////////////////////////////////////

///////////////////////////////////////////begin font.c //////////////////////////////////////////////////
#include <rfb/rfb.h>

int rfbDrawChar(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
		 int x,int y,unsigned char c,rfbPixel col)
{
  int i,j,width,height;
  unsigned char* data=font->data+font->metaData[c*5];
  unsigned char d=*data;
  int rowstride=rfbScreen->paddedWidthInBytes;
  int bpp=rfbScreen->serverFormat.bitsPerPixel/8;
  char *colour=(char*)&col;

  if(!rfbEndianTest)
    colour += 4-bpp;

  width=font->metaData[c*5+1];
  height=font->metaData[c*5+2];
  x+=font->metaData[c*5+3];
  y+=-font->metaData[c*5+4]-height+1;

  for(j=0;j<height;j++) {
    for(i=0;i<width;i++) {
      if((i&7)==0) {
	d=*data;
	data++;
      }
      if(d&0x80)
	memcpy(rfbScreen->frameBuffer+(y+j)*rowstride+(x+i)*bpp,colour,bpp);
      d<<=1;
    }
    /* if((i&7)!=0) data++; */
  }
  return(width);
}

void rfbDrawString(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
		   int x,int y,const char* string,rfbPixel colour)
{
  while(*string) {
    x+=rfbDrawChar(rfbScreen,font,x,y,*string,colour);
    string++;
  }
}

/* TODO: these two functions need to be more efficient */
/* if col==bcol, assume transparent background */
int rfbDrawCharWithClip(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
			int x,int y,unsigned char c,
			int x1,int y1,int x2,int y2,
			rfbPixel col,rfbPixel bcol)
{
  int i,j,width,height;
  unsigned char* data=font->data+font->metaData[c*5];
  unsigned char d;
  int rowstride=rfbScreen->paddedWidthInBytes;
  int bpp=rfbScreen->serverFormat.bitsPerPixel/8,extra_bytes=0;
  char* colour=(char*)&col;
  char* bcolour=(char*)&bcol;

  if(!rfbEndianTest) {
    colour+=4-bpp;
    bcolour+=4-bpp;
  }

  width=font->metaData[c*5+1];
  height=font->metaData[c*5+2];
  x+=font->metaData[c*5+3];
  y+=-font->metaData[c*5+4]-height+1;

  /* after clipping, x2 will be count of bytes between rows,
   * x1 start of i, y1 start of j, width and height will be adjusted. */
  if(y1>y) { y1-=y; data+=(width+7)/8; height-=y1; y+=y1; } else y1=0;
  if(x1>x) { x1-=x; data+=x1; width-=x1; x+=x1; extra_bytes+=x1/8; } else x1=0;
  if(y2<y+height) height-=y+height-y2;
  if(x2<x+width) { extra_bytes+=(x1+width)/8-(x+width-x2+7)/8; width-=x+width-x2; }

  d=*data;
  for(j=y1;j<height;j++) {
    if((x1&7)!=0)
      d=data[-1]; /* TODO: check if in this case extra_bytes is correct! */
    for(i=x1;i<width;i++) {
      if((i&7)==0) {
	d=*data;
	data++;
      }
      /* if(x+i>=x1 && x+i<x2 && y+j>=y1 && y+j<y2) */ {
	 if(d&0x80) {
	   memcpy(rfbScreen->frameBuffer+(y+j)*rowstride+(x+i)*bpp,
		  colour,bpp);
	 } else if(bcol!=col) {
	   memcpy(rfbScreen->frameBuffer+(y+j)*rowstride+(x+i)*bpp,
		  bcolour,bpp);
	 }
      }
      d<<=1;
    }
    /* if((i&7)==0) data++; */
    data += extra_bytes;
  }
  return(width);
}

void rfbDrawStringWithClip(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
			   int x,int y,const char* string,
			   int x1,int y1,int x2,int y2,
			   rfbPixel colour,rfbPixel backColour)
{
  while(*string) {
    x+=rfbDrawCharWithClip(rfbScreen,font,x,y,*string,x1,y1,x2,y2,
			   colour,backColour);
    string++;
  }
}

int rfbWidthOfString(rfbFontDataPtr font,const char* string)
{
  int i=0;
  while(*string) {
    i+=font->metaData[*string*5+1];
    string++;
  }
  return(i);
}

int rfbWidthOfChar(rfbFontDataPtr font,unsigned char c)
{
  return(font->metaData[c*5+1]+font->metaData[c*5+3]);
}

void rfbFontBBox(rfbFontDataPtr font,unsigned char c,int* x1,int* y1,int* x2,int* y2)
{
  *x1+=font->metaData[c*5+3];
  *y1+=-font->metaData[c*5+4]-font->metaData[c*5+2]+1;
  *x2=*x1+font->metaData[c*5+1]+1;
  *y2=*y1+font->metaData[c*5+2]+1;
}

#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif

void rfbWholeFontBBox(rfbFontDataPtr font,
		      int *x1, int *y1, int *x2, int *y2)
{
   int i;
   int* m=font->metaData;
   
   (*x1)=(*y1)=INT_MAX; (*x2)=(*y2)=1-(INT_MAX);
   for(i=0;i<256;i++) {
      if(m[i*5+1]-m[i*5+3]>(*x2))
	(*x2)=m[i*5+1]-m[i*5+3];
      if(-m[i*5+2]+m[i*5+4]<(*y1))
	(*y1)=-m[i*5+2]+m[i*5+4];
      if(m[i*5+3]<(*x1))
	(*x1)=m[i*5+3];
      if(-m[i*5+4]>(*y2))
	(*y2)=-m[i*5+4];
   }
   (*x2)++;
   (*y2)++;
}

rfbFontDataPtr rfbLoadConsoleFont(char *filename)
{
  FILE *f=fopen(filename,"rb");
  rfbFontDataPtr p;
  int i;

  if(!f) return NULL;

  p=(rfbFontDataPtr)malloc(sizeof(rfbFontData));
  p->data=(unsigned char*)malloc(4096);
  if(1!=fread(p->data,4096,1,f)) {
    free(p->data);
    free(p);
    return NULL;
  }
  fclose(f);
  p->metaData=(int*)malloc(256*5*sizeof(int));
  for(i=0;i<256;i++) {
    p->metaData[i*5+0]=i*16; /* offset */
    p->metaData[i*5+1]=8; /* width */
    p->metaData[i*5+2]=16; /* height */
    p->metaData[i*5+3]=0; /* xhot */
    p->metaData[i*5+4]=0; /* yhot */
  }
  return(p);
}

void rfbFreeFont(rfbFontDataPtr f)
{
  free(f->data);
  free(f->metaData);
  free(f);
}
///////////////////////////////////////////end font.c //////////////////////////////////////////////////

///////////////////////////////////////////begin draw.c //////////////////////////////////////////////////
#include <rfb/rfb.h>

void rfbFillRect(rfbScreenInfoPtr s,int x1,int y1,int x2,int y2,rfbPixel col)
{
  int rowstride = s->paddedWidthInBytes, bpp = s->bitsPerPixel>>3;
  int i,j;
  char* colour=(char*)&col;

   if(!rfbEndianTest)
    colour += 4-bpp;
  for(j=y1;j<y2;j++)
    for(i=x1;i<x2;i++)
      memcpy(s->frameBuffer+j*rowstride+i*bpp,colour,bpp);
  rfbMarkRectAsModified(s,x1,y1,x2,y2);
}

#define SETPIXEL(x,y) \
  memcpy(s->frameBuffer+(y)*rowstride+(x)*bpp,colour,bpp)

void rfbDrawPixel(rfbScreenInfoPtr s,int x,int y,rfbPixel col)
{
  int rowstride = s->paddedWidthInBytes, bpp = s->bitsPerPixel>>3;
  char* colour=(char*)&col;

  if(!rfbEndianTest)
    colour += 4-bpp;
  SETPIXEL(x,y);
  rfbMarkRectAsModified(s,x,y,x+1,y+1);
}

void rfbDrawLine(rfbScreenInfoPtr s,int x1,int y1,int x2,int y2,rfbPixel col)
{
  int rowstride = s->paddedWidthInBytes, bpp = s->bitsPerPixel>>3;
  int i;
  char* colour=(char*)&col;

  if(!rfbEndianTest)
    colour += 4-bpp;

#define SWAPPOINTS { i=x1; x1=x2; x2=i; i=y1; y1=y2; y2=i; }
  if(abs(x1-x2)<abs(y1-y2)) {
    if(y1>y2)
      SWAPPOINTS
    for(i=y1;i<=y2;i++)
      SETPIXEL(x1+(i-y1)*(x2-x1)/(y2-y1),i);
    /* TODO: Maybe make this more intelligently? */
    if(x2<x1) { i=x1; x1=x2; x2=i; }
    rfbMarkRectAsModified(s,x1,y1,x2+1,y2+1);
  } else {
    if(x1>x2)
      SWAPPOINTS
    else if(x1==x2) {
      rfbDrawPixel(s,x1,y1,col);
      return;
    }
    for(i=x1;i<=x2;i++)
      SETPIXEL(i,y1+(i-x1)*(y2-y1)/(x2-x1));
    if(y2<y1) { i=y1; y1=y2; y2=i; }
    rfbMarkRectAsModified(s,x1,y1,x2+1,y2+1);
  }
}
///////////////////////////////////////////end draw.c //////////////////////////////////////////////////

///////////////////////////////////////////begin selbox.c //////////////////////////////////////////////////
#include <ctype.h>
#include <rfb/rfb.h>
#include <rfb/keysym.h>

typedef struct {
  rfbScreenInfoPtr screen;
  rfbFontDataPtr font;
  char** list;
  int listSize;
  int selected;
  int displayStart;
  int x1,y1,x2,y2,textH,pageH;
  int xhot,yhot;
  int buttonWidth,okBX,cancelBX,okX,cancelX,okY;
  rfbBool okInverted,cancelInverted;
  int lastButtons;
  rfbPixel colour,backColour;
  SelectionChangedHookPtr selChangedHook;
  enum { SELECTING, OK, CANCEL } state;
} rfbSelectData;

static const char* okStr="OK";
static const char* cancelStr="Cancel";

static void selPaintButtons(rfbSelectData* m,rfbBool invertOk,rfbBool invertCancel)
{
  rfbScreenInfoPtr s = m->screen;
  rfbPixel bcolour = m->backColour;
  rfbPixel colour = m->colour;

  rfbFillRect(s,m->x1,m->okY-m->textH,m->x2,m->okY,bcolour);

  if(invertOk) {
    rfbFillRect(s,m->okBX,m->okY-m->textH,m->okBX+m->buttonWidth,m->okY,colour);
    rfbDrawStringWithClip(s,m->font,m->okX+m->xhot,m->okY-1+m->yhot,okStr,
			  m->x1,m->okY-m->textH,m->x2,m->okY,
			  bcolour,colour);
  } else
    rfbDrawString(s,m->font,m->okX+m->xhot,m->okY-1+m->yhot,okStr,colour);
    
  if(invertCancel) {
    rfbFillRect(s,m->cancelBX,m->okY-m->textH,
	     m->cancelBX+m->buttonWidth,m->okY,colour);
    rfbDrawStringWithClip(s,m->font,m->cancelX+m->xhot,m->okY-1+m->yhot,
			  cancelStr,m->x1,m->okY-m->textH,m->x2,m->okY,
			  bcolour,colour);
  } else
    rfbDrawString(s,m->font,m->cancelX+m->xhot,m->okY-1+m->yhot,cancelStr,colour);

  m->okInverted = invertOk;
  m->cancelInverted = invertCancel;
}

/* line is relative to displayStart */
static void selPaintLine(rfbSelectData* m,int line,rfbBool invert)
{
  int y1 = m->y1+line*m->textH, y2 = y1+m->textH;
  if(y2>m->y2)
    y2=m->y2;
  rfbFillRect(m->screen,m->x1,y1,m->x2,y2,invert?m->colour:m->backColour);
  if(m->displayStart+line<m->listSize)
    rfbDrawStringWithClip(m->screen,m->font,m->x1+m->xhot,y2-1+m->yhot,
			  m->list[m->displayStart+line],
			  m->x1,y1,m->x2,y2,
			  invert?m->backColour:m->colour,
			  invert?m->backColour:m->colour);
}

static void selSelect(rfbSelectData* m,int _index)
{
  int delta;

  if(_index==m->selected || _index<0 || _index>=m->listSize)
    return;

  if(m->selected>=0)
    selPaintLine(m,m->selected-m->displayStart,FALSE);

  if(_index<m->displayStart || _index>=m->displayStart+m->pageH) {
    /* targetLine is the screen line in which the selected line will
       be displayed.
       targetLine = m->pageH/2 doesn't look so nice */
    int targetLine = m->selected-m->displayStart;
    int lineStart,lineEnd;

    /* scroll */
    if(_index<targetLine)
      targetLine = _index;
    else if(_index+m->pageH-targetLine>=m->listSize)
      targetLine = _index+m->pageH-m->listSize;
    delta = _index-(m->displayStart+targetLine);

    if(delta>-m->pageH && delta<m->pageH) {
      if(delta>0) {
	lineStart = m->pageH-delta;
	lineEnd = m->pageH;
	rfbDoCopyRect(m->screen,m->x1,m->y1,m->x2,m->y1+lineStart*m->textH,
		      0,-delta*m->textH);
      } else {
	lineStart = 0;
	lineEnd = -delta;
	rfbDoCopyRect(m->screen,
		      m->x1,m->y1+lineEnd*m->textH,m->x2,m->y2,
		      0,-delta*m->textH);
      }
    } else {
      lineStart = 0;
      lineEnd = m->pageH;
    }
    m->displayStart += delta;
    for(delta=lineStart;delta<lineEnd;delta++)
      if(delta!=_index)
	selPaintLine(m,delta,FALSE);
  }

  m->selected = _index;
  selPaintLine(m,m->selected-m->displayStart,TRUE);

  if(m->selChangedHook)
    m->selChangedHook(_index);

  /* todo: scrollbars */
}

static void selKbdAddEvent(rfbBool down,rfbKeySym keySym,rfbClientPtr cl)
{
  if(down) {
    if(keySym>' ' && keySym<0xff) {
      int i;
      rfbSelectData* m = (rfbSelectData*)cl->screen->screenData;
      char c = tolower(keySym);

      for(i=m->selected+1;m->list[i] && tolower(m->list[i][0])!=c;i++);
      if(!m->list[i])
	for(i=0;i<m->selected && tolower(m->list[i][0])!=c;i++);
      selSelect(m,i);
    } else if(keySym==XK_Escape) {
      rfbSelectData* m = (rfbSelectData*)cl->screen->screenData;
      m->state = CANCEL;
    } else if(keySym==XK_Return) {
      rfbSelectData* m = (rfbSelectData*)cl->screen->screenData;
      m->state = OK;
    } else {
      rfbSelectData* m = (rfbSelectData*)cl->screen->screenData;
      int curSel=m->selected;
      if(keySym==XK_Up) {
	if(curSel>0)
	  selSelect(m,curSel-1);
      } else if(keySym==XK_Down) {
	if(curSel+1<m->listSize)
	  selSelect(m,curSel+1);
      } else {
	if(keySym==XK_Page_Down) {
	  if(curSel+m->pageH<m->listSize)
	    selSelect(m,curSel+m->pageH);
	  else
	    selSelect(m,m->listSize-1);
	} else if(keySym==XK_Page_Up) {
	  if(curSel-m->pageH>=0)
	    selSelect(m,curSel-m->pageH);
	  else
	    selSelect(m,0);
	}
      }
    }
  }
}

static void selPtrAddEvent(int buttonMask,int x,int y,rfbClientPtr cl)
{
  rfbSelectData* m = (rfbSelectData*)cl->screen->screenData;
  if(y<m->okY && y>=m->okY-m->textH) {
    if(x>=m->okBX && x<m->okBX+m->buttonWidth) {
      if(!m->okInverted)
	selPaintButtons(m,TRUE,FALSE);
      if(buttonMask)
	m->state = OK;
    } else if(x>=m->cancelBX && x<m->cancelBX+m->buttonWidth) {
      if(!m->cancelInverted)
	selPaintButtons(m,FALSE,TRUE);
      if(buttonMask)
	m->state = CANCEL;
    } else if(m->okInverted || m->cancelInverted)
      selPaintButtons(m,FALSE,FALSE);
  } else {
    if(m->okInverted || m->cancelInverted)
      selPaintButtons(m,FALSE,FALSE);
    if(!m->lastButtons && buttonMask) {
      if(x>=m->x1 && x<m->x2 && y>=m->y1 && y<m->y2)
	selSelect(m,m->displayStart+(y-m->y1)/m->textH);
    }
  }
  m->lastButtons = buttonMask;

  /* todo: scrollbars */
}

static rfbCursorPtr selGetCursorPtr(rfbClientPtr cl)
{
  return NULL;
}

int rfbSelectBox(rfbScreenInfoPtr rfbScreen,rfbFontDataPtr font,
		 char** list,
		 int x1,int y1,int x2,int y2,
		 rfbPixel colour,rfbPixel backColour,
		 int border,SelectionChangedHookPtr selChangedHook)
{
   int bpp = rfbScreen->bitsPerPixel/8;
   char* frameBufferBackup;
   void* screenDataBackup = rfbScreen->screenData;
   rfbKbdAddEventProcPtr kbdAddEventBackup = rfbScreen->kbdAddEvent;
   rfbPtrAddEventProcPtr ptrAddEventBackup = rfbScreen->ptrAddEvent;
   rfbGetCursorProcPtr getCursorPtrBackup = rfbScreen->getCursorPtr;
   rfbDisplayHookPtr displayHookBackup = rfbScreen->displayHook;
   rfbSelectData selData;
   int i,j,k;
   int fx1,fy1,fx2,fy2; /* for font bbox */

   if(list==0 || *list==0)
     return(-1);
   
   rfbWholeFontBBox(font, &fx1, &fy1, &fx2, &fy2);
   selData.textH = fy2-fy1;
   /* I need at least one line for the choice and one for the buttons */
   if(y2-y1<selData.textH*2+3*border)
     return(-1);
   selData.xhot = -fx1;
   selData.yhot = -fy2;
   selData.x1 = x1+border;
   selData.y1 = y1+border;
   selData.y2 = y2-selData.textH-3*border;
   selData.x2 = x2-2*border;
   selData.pageH = (selData.y2-selData.y1)/selData.textH;

   i = rfbWidthOfString(font,okStr);
   j = rfbWidthOfString(font,cancelStr);
   selData.buttonWidth= k = 4*border+(i<j)?j:i;
   selData.okBX = x1+(x2-x1-2*k)/3;
   if(selData.okBX<x1+border) /* too narrow! */
     return(-1);
   selData.cancelBX = x1+k+(x2-x1-2*k)*2/3;
   selData.okX = selData.okBX+(k-i)/2;
   selData.cancelX = selData.cancelBX+(k-j)/2;
   selData.okY = y2-border;

   frameBufferBackup = (char*)malloc(bpp*(x2-x1)*(y2-y1));

   selData.state = SELECTING;
   selData.screen = rfbScreen;
   selData.font = font;
   selData.list = list;
   selData.colour = colour;
   selData.backColour = backColour;
   for(i=0;list[i];i++);
   selData.selected = i;
   selData.listSize = i;
   selData.displayStart = i;
   selData.lastButtons = 0;
   selData.selChangedHook = selChangedHook;
   
   rfbScreen->screenData = &selData;
   rfbScreen->kbdAddEvent = selKbdAddEvent;
   rfbScreen->ptrAddEvent = selPtrAddEvent;
   rfbScreen->getCursorPtr = selGetCursorPtr;
   rfbScreen->displayHook = NULL;
   
   /* backup screen */
   for(j=0;j<y2-y1;j++)
     memcpy(frameBufferBackup+j*(x2-x1)*bpp,
	    rfbScreen->frameBuffer+j*rfbScreen->paddedWidthInBytes+x1*bpp,
	    (x2-x1)*bpp);

   /* paint list and buttons */
   rfbFillRect(rfbScreen,x1,y1,x2,y2,colour);
   selPaintButtons(&selData,FALSE,FALSE);
   selSelect(&selData,0);

   /* modal loop */
   while(selData.state == SELECTING)
     rfbProcessEvents(rfbScreen,20000);

   /* copy back screen data */
   for(j=0;j<y2-y1;j++)
     memcpy(rfbScreen->frameBuffer+j*rfbScreen->paddedWidthInBytes+x1*bpp,
	    frameBufferBackup+j*(x2-x1)*bpp,
	    (x2-x1)*bpp);
   free(frameBufferBackup);
   rfbMarkRectAsModified(rfbScreen,x1,y1,x2,y2);
   rfbScreen->screenData = screenDataBackup;
   rfbScreen->kbdAddEvent = kbdAddEventBackup;
   rfbScreen->ptrAddEvent = ptrAddEventBackup;
   rfbScreen->getCursorPtr = getCursorPtrBackup;
   rfbScreen->displayHook = displayHookBackup;

   if(selData.state==CANCEL)
     selData.selected=-1;
   return(selData.selected);
}

///////////////////////////////////////////end selbox.c //////////////////////////////////////////////////

///////////////////////////////////////////begin d3des.c //////////////////////////////////////////////////
/*
 * This is D3DES (V5.09) by Richard Outerbridge with the double and
 * triple-length support removed for use in VNC.  Also the bytebit[] array
 * has been reversed so that the most significant bit in each byte of the
 * key is ignored, not the least significant.
 *
 * These changes are:
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

/* D3DES (V5.09) -
 *
 * A portable, public domain, version of the Data Encryption Standard.
 *
 * Written with Symantec's THINK (Lightspeed) C by Richard Outerbridge.
 * Thanks to: Dan Hoey for his excellent Initial and Inverse permutation
 * code;  Jim Gillogly & Phil Karn for the DES key schedule code; Dennis
 * Ferguson, Eric Young and Dana How for comparing notes; and Ray Lau,
 * for humouring me on.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge.
 * (GEnie : OUTER; CIS : [71755,204]) Graven Imagery, 1992.
 */

#include "d3des.h"

static void scrunch(unsigned char *, unsigned long *);
static void unscrun(unsigned long *, unsigned char *);
static void desfunc(unsigned long *, unsigned long *);
static void cookey(unsigned long *);

static unsigned long KnL[32] = { 0L };
/*
static unsigned long KnR[32] = { 0L };
static unsigned long Kn3[32] = { 0L };
static unsigned char Df_Key[24] = {
	0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
	0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
	0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67 };
*/

static unsigned short bytebit[8]	= {
	01, 02, 04, 010, 020, 040, 0100, 0200 };

static unsigned long bigbyte[24] = {
	0x800000L,	0x400000L,	0x200000L,	0x100000L,
	0x80000L,	0x40000L,	0x20000L,	0x10000L,
	0x8000L,	0x4000L,	0x2000L,	0x1000L,
	0x800L, 	0x400L, 	0x200L, 	0x100L,
	0x80L,		0x40L,		0x20L,		0x10L,
	0x8L,		0x4L,		0x2L,		0x1L	};

/* Use the key schedule specified in the Standard (ANSI X3.92-1981). */

static unsigned char pc1[56] = {
	56, 48, 40, 32, 24, 16,  8,	 0, 57, 49, 41, 33, 25, 17,
	 9,  1, 58, 50, 42, 34, 26,	18, 10,  2, 59, 51, 43, 35,
	62, 54, 46, 38, 30, 22, 14,	 6, 61, 53, 45, 37, 29, 21,
	13,  5, 60, 52, 44, 36, 28,	20, 12,  4, 27, 19, 11,  3 };

static unsigned char totrot[16] = {
	1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28 };

static unsigned char pc2[48] = {
	13, 16, 10, 23,  0,  4,  2, 27, 14,  5, 20,  9,
	22, 18, 11,  3, 25,  7, 15,  6, 26, 19, 12,  1,
	40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47,
	43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31 };

void rfbDesKey(unsigned char *key,
               int edf)
{
	register int i, j, l, m, n;
	unsigned char pc1m[56], pcr[56];
	unsigned long kn[32];

	for ( j = 0; j < 56; j++ ) {
		l = pc1[j];
		m = l & 07;
		pc1m[j] = (key[l >> 3] & bytebit[m]) ? 1 : 0;
		}
	for( i = 0; i < 16; i++ ) {
		if( edf == DE1 ) m = (15 - i) << 1;
		else m = i << 1;
		n = m + 1;
		kn[m] = kn[n] = 0L;
		for( j = 0; j < 28; j++ ) {
			l = j + totrot[i];
			if( l < 28 ) pcr[j] = pc1m[l];
			else pcr[j] = pc1m[l - 28];
			}
		for( j = 28; j < 56; j++ ) {
		    l = j + totrot[i];
		    if( l < 56 ) pcr[j] = pc1m[l];
		    else pcr[j] = pc1m[l - 28];
		    }
		for( j = 0; j < 24; j++ ) {
			if( pcr[pc2[j]] ) kn[m] |= bigbyte[j];
			if( pcr[pc2[j+24]] ) kn[n] |= bigbyte[j];
			}
		}
	cookey(kn);
	return;
	}

static void cookey(register unsigned long *raw1)
{
	register unsigned long *cook, *raw0;
	unsigned long dough[32];
	register int i;

	cook = dough;
	for( i = 0; i < 16; i++, raw1++ ) {
		raw0 = raw1++;
		*cook	 = (*raw0 & 0x00fc0000L) << 6;
		*cook	|= (*raw0 & 0x00000fc0L) << 10;
		*cook	|= (*raw1 & 0x00fc0000L) >> 10;
		*cook++ |= (*raw1 & 0x00000fc0L) >> 6;
		*cook	 = (*raw0 & 0x0003f000L) << 12;
		*cook	|= (*raw0 & 0x0000003fL) << 16;
		*cook	|= (*raw1 & 0x0003f000L) >> 4;
		*cook++ |= (*raw1 & 0x0000003fL);
		}
	rfbUseKey(dough);
	return;
	}

void rfbCPKey(register unsigned long *into)
{
	register unsigned long *from, *endp;

	from = KnL, endp = &KnL[32];
	while( from < endp ) *into++ = *from++;
	return;
	}

void rfbUseKey(register unsigned long *from)
{
	register unsigned long *to, *endp;

	to = KnL, endp = &KnL[32];
	while( to < endp ) *to++ = *from++;
	return;
	}

void rfbDes(unsigned char *inblock,
            unsigned char *outblock)
{
	unsigned long work[2];

	scrunch(inblock, work);
	desfunc(work, KnL);
	unscrun(work, outblock);
	return;
	}

static void scrunch(register unsigned char *outof,
                    register unsigned long *into)
{
	*into	 = (*outof++ & 0xffL) << 24;
	*into	|= (*outof++ & 0xffL) << 16;
	*into	|= (*outof++ & 0xffL) << 8;
	*into++ |= (*outof++ & 0xffL);
	*into	 = (*outof++ & 0xffL) << 24;
	*into	|= (*outof++ & 0xffL) << 16;
	*into	|= (*outof++ & 0xffL) << 8;
	*into	|= (*outof   & 0xffL);
	return;
	}

static void unscrun(register unsigned long *outof,
                    register unsigned char *into)
{
	*into++ = (unsigned char)((*outof >> 24) & 0xffL);
	*into++ = (unsigned char)((*outof >> 16) & 0xffL);
	*into++ = (unsigned char)((*outof >>  8) & 0xffL);
	*into++ = (unsigned char)( *outof++	 & 0xffL);
	*into++ = (unsigned char)((*outof >> 24) & 0xffL);
	*into++ = (unsigned char)((*outof >> 16) & 0xffL);
	*into++ = (unsigned char)((*outof >>  8) & 0xffL);
	*into	= (unsigned char)( *outof	 & 0xffL);
	return;
	}

static unsigned long SP1[64] = {
	0x01010400L, 0x00000000L, 0x00010000L, 0x01010404L,
	0x01010004L, 0x00010404L, 0x00000004L, 0x00010000L,
	0x00000400L, 0x01010400L, 0x01010404L, 0x00000400L,
	0x01000404L, 0x01010004L, 0x01000000L, 0x00000004L,
	0x00000404L, 0x01000400L, 0x01000400L, 0x00010400L,
	0x00010400L, 0x01010000L, 0x01010000L, 0x01000404L,
	0x00010004L, 0x01000004L, 0x01000004L, 0x00010004L,
	0x00000000L, 0x00000404L, 0x00010404L, 0x01000000L,
	0x00010000L, 0x01010404L, 0x00000004L, 0x01010000L,
	0x01010400L, 0x01000000L, 0x01000000L, 0x00000400L,
	0x01010004L, 0x00010000L, 0x00010400L, 0x01000004L,
	0x00000400L, 0x00000004L, 0x01000404L, 0x00010404L,
	0x01010404L, 0x00010004L, 0x01010000L, 0x01000404L,
	0x01000004L, 0x00000404L, 0x00010404L, 0x01010400L,
	0x00000404L, 0x01000400L, 0x01000400L, 0x00000000L,
	0x00010004L, 0x00010400L, 0x00000000L, 0x01010004L };

static unsigned long SP2[64] = {
	0x80108020L, 0x80008000L, 0x00008000L, 0x00108020L,
	0x00100000L, 0x00000020L, 0x80100020L, 0x80008020L,
	0x80000020L, 0x80108020L, 0x80108000L, 0x80000000L,
	0x80008000L, 0x00100000L, 0x00000020L, 0x80100020L,
	0x00108000L, 0x00100020L, 0x80008020L, 0x00000000L,
	0x80000000L, 0x00008000L, 0x00108020L, 0x80100000L,
	0x00100020L, 0x80000020L, 0x00000000L, 0x00108000L,
	0x00008020L, 0x80108000L, 0x80100000L, 0x00008020L,
	0x00000000L, 0x00108020L, 0x80100020L, 0x00100000L,
	0x80008020L, 0x80100000L, 0x80108000L, 0x00008000L,
	0x80100000L, 0x80008000L, 0x00000020L, 0x80108020L,
	0x00108020L, 0x00000020L, 0x00008000L, 0x80000000L,
	0x00008020L, 0x80108000L, 0x00100000L, 0x80000020L,
	0x00100020L, 0x80008020L, 0x80000020L, 0x00100020L,
	0x00108000L, 0x00000000L, 0x80008000L, 0x00008020L,
	0x80000000L, 0x80100020L, 0x80108020L, 0x00108000L };

static unsigned long SP3[64] = {
	0x00000208L, 0x08020200L, 0x00000000L, 0x08020008L,
	0x08000200L, 0x00000000L, 0x00020208L, 0x08000200L,
	0x00020008L, 0x08000008L, 0x08000008L, 0x00020000L,
	0x08020208L, 0x00020008L, 0x08020000L, 0x00000208L,
	0x08000000L, 0x00000008L, 0x08020200L, 0x00000200L,
	0x00020200L, 0x08020000L, 0x08020008L, 0x00020208L,
	0x08000208L, 0x00020200L, 0x00020000L, 0x08000208L,
	0x00000008L, 0x08020208L, 0x00000200L, 0x08000000L,
	0x08020200L, 0x08000000L, 0x00020008L, 0x00000208L,
	0x00020000L, 0x08020200L, 0x08000200L, 0x00000000L,
	0x00000200L, 0x00020008L, 0x08020208L, 0x08000200L,
	0x08000008L, 0x00000200L, 0x00000000L, 0x08020008L,
	0x08000208L, 0x00020000L, 0x08000000L, 0x08020208L,
	0x00000008L, 0x00020208L, 0x00020200L, 0x08000008L,
	0x08020000L, 0x08000208L, 0x00000208L, 0x08020000L,
	0x00020208L, 0x00000008L, 0x08020008L, 0x00020200L };

static unsigned long SP4[64] = {
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
	0x00802080L, 0x00800081L, 0x00800001L, 0x00002001L,
	0x00000000L, 0x00802000L, 0x00802000L, 0x00802081L,
	0x00000081L, 0x00000000L, 0x00800080L, 0x00800001L,
	0x00000001L, 0x00002000L, 0x00800000L, 0x00802001L,
	0x00000080L, 0x00800000L, 0x00002001L, 0x00002080L,
	0x00800081L, 0x00000001L, 0x00002080L, 0x00800080L,
	0x00002000L, 0x00802080L, 0x00802081L, 0x00000081L,
	0x00800080L, 0x00800001L, 0x00802000L, 0x00802081L,
	0x00000081L, 0x00000000L, 0x00000000L, 0x00802000L,
	0x00002080L, 0x00800080L, 0x00800081L, 0x00000001L,
	0x00802001L, 0x00002081L, 0x00002081L, 0x00000080L,
	0x00802081L, 0x00000081L, 0x00000001L, 0x00002000L,
	0x00800001L, 0x00002001L, 0x00802080L, 0x00800081L,
	0x00002001L, 0x00002080L, 0x00800000L, 0x00802001L,
	0x00000080L, 0x00800000L, 0x00002000L, 0x00802080L };

static unsigned long SP5[64] = {
	0x00000100L, 0x02080100L, 0x02080000L, 0x42000100L,
	0x00080000L, 0x00000100L, 0x40000000L, 0x02080000L,
	0x40080100L, 0x00080000L, 0x02000100L, 0x40080100L,
	0x42000100L, 0x42080000L, 0x00080100L, 0x40000000L,
	0x02000000L, 0x40080000L, 0x40080000L, 0x00000000L,
	0x40000100L, 0x42080100L, 0x42080100L, 0x02000100L,
	0x42080000L, 0x40000100L, 0x00000000L, 0x42000000L,
	0x02080100L, 0x02000000L, 0x42000000L, 0x00080100L,
	0x00080000L, 0x42000100L, 0x00000100L, 0x02000000L,
	0x40000000L, 0x02080000L, 0x42000100L, 0x40080100L,
	0x02000100L, 0x40000000L, 0x42080000L, 0x02080100L,
	0x40080100L, 0x00000100L, 0x02000000L, 0x42080000L,
	0x42080100L, 0x00080100L, 0x42000000L, 0x42080100L,
	0x02080000L, 0x00000000L, 0x40080000L, 0x42000000L,
	0x00080100L, 0x02000100L, 0x40000100L, 0x00080000L,
	0x00000000L, 0x40080000L, 0x02080100L, 0x40000100L };

static unsigned long SP6[64] = {
	0x20000010L, 0x20400000L, 0x00004000L, 0x20404010L,
	0x20400000L, 0x00000010L, 0x20404010L, 0x00400000L,
	0x20004000L, 0x00404010L, 0x00400000L, 0x20000010L,
	0x00400010L, 0x20004000L, 0x20000000L, 0x00004010L,
	0x00000000L, 0x00400010L, 0x20004010L, 0x00004000L,
	0x00404000L, 0x20004010L, 0x00000010L, 0x20400010L,
	0x20400010L, 0x00000000L, 0x00404010L, 0x20404000L,
	0x00004010L, 0x00404000L, 0x20404000L, 0x20000000L,
	0x20004000L, 0x00000010L, 0x20400010L, 0x00404000L,
	0x20404010L, 0x00400000L, 0x00004010L, 0x20000010L,
	0x00400000L, 0x20004000L, 0x20000000L, 0x00004010L,
	0x20000010L, 0x20404010L, 0x00404000L, 0x20400000L,
	0x00404010L, 0x20404000L, 0x00000000L, 0x20400010L,
	0x00000010L, 0x00004000L, 0x20400000L, 0x00404010L,
	0x00004000L, 0x00400010L, 0x20004010L, 0x00000000L,
	0x20404000L, 0x20000000L, 0x00400010L, 0x20004010L };

static unsigned long SP7[64] = {
	0x00200000L, 0x04200002L, 0x04000802L, 0x00000000L,
	0x00000800L, 0x04000802L, 0x00200802L, 0x04200800L,
	0x04200802L, 0x00200000L, 0x00000000L, 0x04000002L,
	0x00000002L, 0x04000000L, 0x04200002L, 0x00000802L,
	0x04000800L, 0x00200802L, 0x00200002L, 0x04000800L,
	0x04000002L, 0x04200000L, 0x04200800L, 0x00200002L,
	0x04200000L, 0x00000800L, 0x00000802L, 0x04200802L,
	0x00200800L, 0x00000002L, 0x04000000L, 0x00200800L,
	0x04000000L, 0x00200800L, 0x00200000L, 0x04000802L,
	0x04000802L, 0x04200002L, 0x04200002L, 0x00000002L,
	0x00200002L, 0x04000000L, 0x04000800L, 0x00200000L,
	0x04200800L, 0x00000802L, 0x00200802L, 0x04200800L,
	0x00000802L, 0x04000002L, 0x04200802L, 0x04200000L,
	0x00200800L, 0x00000000L, 0x00000002L, 0x04200802L,
	0x00000000L, 0x00200802L, 0x04200000L, 0x00000800L,
	0x04000002L, 0x04000800L, 0x00000800L, 0x00200002L };

static unsigned long SP8[64] = {
	0x10001040L, 0x00001000L, 0x00040000L, 0x10041040L,
	0x10000000L, 0x10001040L, 0x00000040L, 0x10000000L,
	0x00040040L, 0x10040000L, 0x10041040L, 0x00041000L,
	0x10041000L, 0x00041040L, 0x00001000L, 0x00000040L,
	0x10040000L, 0x10000040L, 0x10001000L, 0x00001040L,
	0x00041000L, 0x00040040L, 0x10040040L, 0x10041000L,
	0x00001040L, 0x00000000L, 0x00000000L, 0x10040040L,
	0x10000040L, 0x10001000L, 0x00041040L, 0x00040000L,
	0x00041040L, 0x00040000L, 0x10041000L, 0x00001000L,
	0x00000040L, 0x10040040L, 0x00001000L, 0x00041040L,
	0x10001000L, 0x00000040L, 0x10000040L, 0x10040000L,
	0x10040040L, 0x10000000L, 0x00040000L, 0x10001040L,
	0x00000000L, 0x10041040L, 0x00040040L, 0x10000040L,
	0x10040000L, 0x10001000L, 0x10001040L, 0x00000000L,
	0x10041040L, 0x00041000L, 0x00041000L, 0x00001040L,
	0x00001040L, 0x00040040L, 0x10000000L, 0x10041000L };

static void desfunc(register unsigned long *block,
                    register unsigned long *keys)
{
	register unsigned long fval, work, right, leftt;
	register int round;

	leftt = block[0];
	right = block[1];
	work = ((leftt >> 4) ^ right) & 0x0f0f0f0fL;
	right ^= work;
	leftt ^= (work << 4);
	work = ((leftt >> 16) ^ right) & 0x0000ffffL;
	right ^= work;
	leftt ^= (work << 16);
	work = ((right >> 2) ^ leftt) & 0x33333333L;
	leftt ^= work;
	right ^= (work << 2);
	work = ((right >> 8) ^ leftt) & 0x00ff00ffL;
	leftt ^= work;
	right ^= (work << 8);
	right = ((right << 1) | ((right >> 31) & 1L)) & 0xffffffffL;
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = ((leftt << 1) | ((leftt >> 31) & 1L)) & 0xffffffffL;

	for( round = 0; round < 8; round++ ) {
		work  = (right << 28) | (right >> 4);
		work ^= *keys++;
		fval  = SP7[ work		 & 0x3fL];
		fval |= SP5[(work >>  8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work  = right ^ *keys++;
		fval |= SP8[ work		 & 0x3fL];
		fval |= SP6[(work >>  8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		leftt ^= fval;
		work  = (leftt << 28) | (leftt >> 4);
		work ^= *keys++;
		fval  = SP7[ work		 & 0x3fL];
		fval |= SP5[(work >>  8) & 0x3fL];
		fval |= SP3[(work >> 16) & 0x3fL];
		fval |= SP1[(work >> 24) & 0x3fL];
		work  = leftt ^ *keys++;
		fval |= SP8[ work		 & 0x3fL];
		fval |= SP6[(work >>  8) & 0x3fL];
		fval |= SP4[(work >> 16) & 0x3fL];
		fval |= SP2[(work >> 24) & 0x3fL];
		right ^= fval;
		}

	right = (right << 31) | (right >> 1);
	work = (leftt ^ right) & 0xaaaaaaaaL;
	leftt ^= work;
	right ^= work;
	leftt = (leftt << 31) | (leftt >> 1);
	work = ((leftt >> 8) ^ right) & 0x00ff00ffL;
	right ^= work;
	leftt ^= (work << 8);
	work = ((leftt >> 2) ^ right) & 0x33333333L;
	right ^= work;
	leftt ^= (work << 2);
	work = ((right >> 16) ^ leftt) & 0x0000ffffL;
	leftt ^= work;
	right ^= (work << 16);
	work = ((right >> 4) ^ leftt) & 0x0f0f0f0fL;
	leftt ^= work;
	right ^= (work << 4);
	*block++ = right;
	*block = leftt;
	return;
	}

/* Validation sets:
 *
 * Single-length key, single-length plaintext -
 * Key	  : 0123 4567 89ab cdef
 * Plain  : 0123 4567 89ab cde7
 * Cipher : c957 4425 6a5e d31d
 *
 * Double-length key, single-length plaintext -
 * Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210
 * Plain  : 0123 4567 89ab cde7
 * Cipher : 7f1d 0a77 826b 8aff
 *
 * Double-length key, double-length plaintext -
 * Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210
 * Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
 * Cipher : 27a0 8440 406a df60 278f 47cf 42d6 15d7
 *
 * Triple-length key, single-length plaintext -
 * Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
 * Plain  : 0123 4567 89ab cde7
 * Cipher : de0b 7c06 ae5e 0ed5
 *
 * Triple-length key, double-length plaintext -
 * Key	  : 0123 4567 89ab cdef fedc ba98 7654 3210 89ab cdef 0123 4567
 * Plain  : 0123 4567 89ab cdef 0123 4567 89ab cdff
 * Cipher : ad0d 1b30 ac17 cf07 0ed1 1c63 81e4 4de5
 *
 * d3des V5.0a rwo 9208.07 18:44 Graven Imagery
 **********************************************************************/
///////////////////////////////////////////end d3des.c //////////////////////////////////////////////////

///////////////////////////////////////////begin vncauth.c //////////////////////////////////////////////////
/*
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * vncauth.c - Functions for VNC password management and authentication.
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#define _POSIX_SOURCE
#endif
#ifdef LIBVNCSERVER_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <rfb/rfbproto.h>
#include "d3des.h"

#include <string.h>
#include <math.h>

#ifdef LIBVNCSERVER_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <time.h>

#ifdef WIN32
#define srandom srand
#define random rand
#else
#include <sys/time.h>
#endif


/* libvncclient does not need this */
#ifndef rfbEncryptBytes

/*
 * We use a fixed key to store passwords, since we assume that our local
 * file system is secure but nonetheless don't want to store passwords
 * as plaintext.
 */

static unsigned char fixedkey[8] = {23,82,107,6,35,78,88,7};


/*
 * Encrypt a password and store it in a file.  Returns 0 if successful,
 * 1 if the file could not be written.
 */

int
rfbEncryptAndStorePasswd(char *passwd, char *fname)
{
    FILE *fp;
    unsigned int i;
    unsigned char encryptedPasswd[8];

    if ((fp = fopen(fname,"w")) == NULL) return 1;

	/* windows security sux */
#ifndef WIN32
    fchmod(fileno(fp), S_IRUSR|S_IWUSR);
#endif

    /* pad password with nulls */

    for (i = 0; i < 8; i++) {
	if (i < strlen(passwd)) {
	    encryptedPasswd[i] = passwd[i];
	} else {
	    encryptedPasswd[i] = 0;
	}
    }

    /* Do encryption in-place - this way we overwrite our copy of the plaintext
       password */

    rfbDesKey(fixedkey, EN0);
    rfbDes(encryptedPasswd, encryptedPasswd);

    for (i = 0; i < 8; i++) {
	putc(encryptedPasswd[i], fp);
    }
  
    fclose(fp);
    return 0;
}


/*
 * Decrypt a password from a file.  Returns a pointer to a newly allocated
 * string containing the password or a null pointer if the password could
 * not be retrieved for some reason.
 */

char *
rfbDecryptPasswdFromFile(char *fname)
{
    FILE *fp;
    int i, ch;
    unsigned char *passwd = (unsigned char *)malloc(9);

    if ((fp = fopen(fname,"r")) == NULL) return NULL;

    for (i = 0; i < 8; i++) {
	ch = getc(fp);
	if (ch == EOF) {
	    fclose(fp);
	    return NULL;
	}
	passwd[i] = ch;
    }

    fclose(fp);

    rfbDesKey(fixedkey, DE1);
    rfbDes(passwd, passwd);

    passwd[8] = 0;

    return (char *)passwd;
}


/*
 * Generate CHALLENGESIZE random bytes for use in challenge-response
 * authentication.
 */

void
rfbRandomBytes(unsigned char *bytes)
{
    int i;
    static rfbBool s_srandom_called = FALSE;

    if (!s_srandom_called) {
	srandom((unsigned int)time(NULL) ^ (unsigned int)getpid());
	s_srandom_called = TRUE;
    }

    for (i = 0; i < CHALLENGESIZE; i++) {
	bytes[i] = (unsigned char)(random() & 255);    
    }
}

#endif

/*
 * Encrypt CHALLENGESIZE bytes in memory using a password.
 */

void
rfbEncryptBytes(unsigned char *bytes, char *passwd)
{
    unsigned char key[8];
    unsigned int i;

    /* key is simply password padded with nulls */

    for (i = 0; i < 8; i++) {
	if (i < strlen(passwd)) {
	    key[i] = passwd[i];
	} else {
	    key[i] = 0;
	}
    }

    rfbDesKey(key, EN0);

    for (i = 0; i < CHALLENGESIZE; i += 8) {
	rfbDes(bytes+i, bytes+i);
    }
}

///////////////////////////////////////////end vncauth.c //////////////////////////////////////////////////

///////////////////////////////////////////begin cargs.c //////////////////////////////////////////////////
/*
 *  This parses the command line arguments. It was seperated from main.c by 
 *  Justin Dearing <jdeari01@longisland.poly.edu>.
 */

/*
 *  LibVNCServer (C) 2001 Johannes E. Schindelin <Johannes.Schindelin@gmx.de>
 *  Original OSXvnc (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  see GPL (latest version) for full details
 */

#include <rfb/rfb.h>

extern int rfbStringToAddr(char *str, in_addr_t *iface);

void
rfbUsage(void)
{
    rfbProtocolExtension* extension;

    fprintf(stderr, "-rfbport port          TCP port for RFB protocol\n");
    fprintf(stderr, "-rfbwait time          max time in ms to wait for RFB client\n");
    fprintf(stderr, "-rfbauth passwd-file   use authentication on RFB protocol\n"
                    "                       (use 'storepasswd' to create a password file)\n");
    fprintf(stderr, "-rfbversion 3.x        Set the version of the RFB we choose to advertise\n");
    fprintf(stderr, "-permitfiletransfer    permit file transfer support\n");
    fprintf(stderr, "-passwd plain-password use authentication \n"
                    "                       (use plain-password as password, USE AT YOUR RISK)\n");
    fprintf(stderr, "-deferupdate time      time in ms to defer updates "
                                                             "(default 40)\n");
    fprintf(stderr, "-deferptrupdate time   time in ms to defer pointer updates"
                                                           " (default none)\n");
    fprintf(stderr, "-desktop name          VNC desktop name (default \"LibVNCServer\")\n");
    fprintf(stderr, "-alwaysshared          always treat new clients as shared\n");
    fprintf(stderr, "-nevershared           never treat new clients as shared\n");
    fprintf(stderr, "-dontdisconnect        don't disconnect existing clients when a "
                                                             "new non-shared\n"
                    "                       connection comes in (refuse new connection "
                                                                "instead)\n");
    fprintf(stderr, "-httpdir dir-path      enable http server using dir-path home\n");
    fprintf(stderr, "-httpport portnum      use portnum for http connection\n");
    fprintf(stderr, "-enablehttpproxy       enable http proxy support\n");
    fprintf(stderr, "-progressive height    enable progressive updating for slow links\n");
    fprintf(stderr, "-listen ipaddr         listen for connections only on network interface with\n");
    fprintf(stderr, "                       addr ipaddr. '-listen localhost' and hostname work too.\n");

    for(extension=rfbGetExtensionIterator();extension;extension=extension->next)
	if(extension->usage)
		extension->usage();
    rfbReleaseExtensionIterator();
}

/* purges COUNT arguments from ARGV at POSITION and decrements ARGC.
   POSITION points to the first non purged argument afterwards. */
void rfbPurgeArguments(int* argc,int* position,int count,char *argv[])
{
  int amount=(*argc)-(*position)-count;
  if(amount)
    memmove(argv+(*position),argv+(*position)+count,sizeof(char*)*amount);
  (*argc)-=count;
}

rfbBool 
rfbProcessArguments(rfbScreenInfoPtr rfbScreen,int* argc, char *argv[])
{
    int i,i1;

    if(!argc) return TRUE;
    
    for (i = i1 = 1; i < *argc;) {
        if (strcmp(argv[i], "-help") == 0) {
	    rfbUsage();
	    return FALSE;
	} else if (strcmp(argv[i], "-rfbport") == 0) { /* -rfbport port */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
	    rfbScreen->port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-rfbwait") == 0) {  /* -rfbwait ms */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
	    rfbScreen->maxClientWait = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-rfbauth") == 0) {  /* -rfbauth passwd-file */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->authPasswdData = argv[++i];

        } else if (strcmp(argv[i], "-permitfiletransfer") == 0) {  /* -permitfiletransfer  */
            rfbScreen->permitFileTransfer = TRUE;
        } else if (strcmp(argv[i], "-rfbversion") == 0) {  /* -rfbversion 3.6  */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
	    sscanf(argv[++i],"%d.%d", &rfbScreen->protocolMajorVersion, &rfbScreen->protocolMinorVersion);
	} else if (strcmp(argv[i], "-passwd") == 0) {  /* -passwd password */
	    char **passwds = malloc(sizeof(char**)*2);
	    if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
	    passwds[0] = argv[++i];
	    passwds[1] = NULL;
	    rfbScreen->authPasswdData = (void*)passwds;
	    rfbScreen->passwordCheck = rfbCheckPasswordByList;
        } else if (strcmp(argv[i], "-deferupdate") == 0) {  /* -deferupdate milliseconds */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->deferUpdateTime = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-deferptrupdate") == 0) {  /* -deferptrupdate milliseconds */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->deferPtrUpdateTime = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-desktop") == 0) {  /* -desktop desktop-name */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->desktopName = argv[++i];
        } else if (strcmp(argv[i], "-alwaysshared") == 0) {
	    rfbScreen->alwaysShared = TRUE;
        } else if (strcmp(argv[i], "-nevershared") == 0) {
            rfbScreen->neverShared = TRUE;
        } else if (strcmp(argv[i], "-dontdisconnect") == 0) {
            rfbScreen->dontDisconnect = TRUE;
        } else if (strcmp(argv[i], "-httpdir") == 0) {  /* -httpdir directory-path */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->httpDir = argv[++i];
        } else if (strcmp(argv[i], "-httpport") == 0) {  /* -httpport portnum */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->httpPort = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-enablehttpproxy") == 0) {
            rfbScreen->httpEnableProxyConnect = TRUE;
        } else if (strcmp(argv[i], "-progressive") == 0) {  /* -httpport portnum */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            rfbScreen->progressiveSliceHeight = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-listen") == 0) {  /* -listen ipaddr */
            if (i + 1 >= *argc) {
		rfbUsage();
		return FALSE;
	    }
            if (! rfbStringToAddr(argv[++i], &(rfbScreen->listenInterface))) {
                return FALSE;
            }
        } else {
	    rfbProtocolExtension* extension;
	    int handled=0;

	    for(extension=rfbGetExtensionIterator();handled==0 && extension;
			extension=extension->next)
		if(extension->processArgument)
			handled = extension->processArgument(*argc - i, argv + i);
	    rfbReleaseExtensionIterator();

	    if(handled==0) {
		i++;
		i1=i;
		continue;
	    }
	    i+=handled-1;
	}
	/* we just remove the processed arguments from the list */
	rfbPurgeArguments(argc,&i1,i-i1+1,argv);
	i=i1;
    }
    return TRUE;
}

rfbBool 
rfbProcessSizeArguments(int* width,int* height,int* bpp,int* argc, char *argv[])
{
    int i,i1;

    if(!argc) return TRUE;
    for (i = i1 = 1; i < *argc-1;) {
        if (strcmp(argv[i], "-bpp") == 0) {
               *bpp = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-width") == 0) {
               *width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-height") == 0) {
               *height = atoi(argv[++i]);
        } else {
	    i++;
	    i1=i;
	    continue;
	}
	rfbPurgeArguments(argc,&i1,i-i1,argv);
	i=i1;
    }
    return TRUE;
}

///////////////////////////////////////////end cargs.c //////////////////////////////////////////////////

///////////////////////////////////////////begin minilzo.c //////////////////////////////////////////////////
/* minilzo.c -- mini subset of the LZO real-time data compression library

   This file is part of the LZO real-time data compression library.

   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */

/*
 * NOTE:
 *   the full LZO package can be found at
 *   http://www.oberhumer.com/opensource/lzo/
 */

#define __LZO_IN_MINILZO
#define LZO_BUILD

#ifdef MINILZO_HAVE_CONFIG_H
#  include <config.h>
#endif

#undef LZO_HAVE_CONFIG_H
#include "minilzo.h"

#if !defined(MINILZO_VERSION) || (MINILZO_VERSION != 0x1080)
#  error "version mismatch in miniLZO source files"
#endif

#ifdef MINILZO_HAVE_CONFIG_H
#  define LZO_HAVE_CONFIG_H
#endif

#if !defined(LZO_NO_SYS_TYPES_H)
#  include <sys/types.h>
#endif
#include <stdio.h>

#ifndef __LZO_CONF_H
#define __LZO_CONF_H

#if !defined(__LZO_IN_MINILZO)
#  ifndef __LZOCONF_H
#    include <lzoconf.h>
#  endif
#endif

#if defined(__BOUNDS_CHECKING_ON)
#  include <unchecked.h>
#else
#  define BOUNDS_CHECKING_OFF_DURING(stmt)      stmt
#  define BOUNDS_CHECKING_OFF_IN_EXPR(expr)     (expr)
#endif

#if !defined(LZO_HAVE_CONFIG_H)
#  include <stddef.h>
#  include <string.h>
#  if !defined(NO_STDLIB_H)
#    include <stdlib.h>
#  endif
#  define HAVE_MEMCMP
#  define HAVE_MEMCPY
#  define HAVE_MEMMOVE
#  define HAVE_MEMSET
#else
#  include <sys/types.h>
#  if defined(HAVE_STDDEF_H)
#    include <stddef.h>
#  endif
#  if defined(STDC_HEADERS)
#    include <string.h>
#    include <stdlib.h>
#  endif
#endif

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#  define HAVE_MALLOC_H
#  define HAVE_HALLOC
#endif

#undef NDEBUG
#if !defined(LZO_DEBUG)
#  define NDEBUG
#endif
#if defined(LZO_DEBUG) || !defined(NDEBUG)
#  if !defined(NO_STDIO_H)
#    include <stdio.h>
#  endif
#endif
#include <assert.h>

#if !defined(LZO_COMPILE_TIME_ASSERT)
#  define LZO_COMPILE_TIME_ASSERT(expr) \
	{ typedef int __lzo_compile_time_assert_fail[1 - 2 * !(expr)]; }
#endif

#if !defined(LZO_UNUSED)
#  if 1
#    define LZO_UNUSED(var)     ((void)&var)
#  elif 0
#    define LZO_UNUSED(var)     { typedef int __lzo_unused[sizeof(var) ? 2 : 1]; }
#  else
#    define LZO_UNUSED(parm)    (parm = parm)
#  endif
#endif

#if !defined(__inline__) && !defined(__GNUC__)
#  if defined(__cplusplus)
#    define __inline__      inline
#  else
#    define __inline__
#  endif
#endif

#if defined(NO_MEMCMP)
#  undef HAVE_MEMCMP
#endif

#if !defined(HAVE_MEMCMP)
#  undef memcmp
#  define memcmp    lzo_memcmp
#endif
#if !defined(HAVE_MEMCPY)
#  undef memcpy
#  define memcpy    lzo_memcpy
#endif
#if !defined(HAVE_MEMMOVE)
#  undef memmove
#  define memmove   lzo_memmove
#endif
#if !defined(HAVE_MEMSET)
#  undef memset
#  define memset    lzo_memset
#endif

#if 0
#  define LZO_BYTE(x)       ((unsigned char) (x))
#else
#  define LZO_BYTE(x)       ((unsigned char) ((x) & 0xff))
#endif

#define LZO_MAX(a,b)        ((a) >= (b) ? (a) : (b))
#define LZO_MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define LZO_MAX3(a,b,c)     ((a) >= (b) ? LZO_MAX(a,c) : LZO_MAX(b,c))
#define LZO_MIN3(a,b,c)     ((a) <= (b) ? LZO_MIN(a,c) : LZO_MIN(b,c))

#define lzo_sizeof(type)    ((lzo_uint) (sizeof(type)))

#define LZO_HIGH(array)     ((lzo_uint) (sizeof(array)/sizeof(*(array))))

#define LZO_SIZE(bits)      (1u << (bits))
#define LZO_MASK(bits)      (LZO_SIZE(bits) - 1)

#define LZO_LSIZE(bits)     (1ul << (bits))
#define LZO_LMASK(bits)     (LZO_LSIZE(bits) - 1)

#define LZO_USIZE(bits)     ((lzo_uint) 1 << (bits))
#define LZO_UMASK(bits)     (LZO_USIZE(bits) - 1)

#define LZO_STYPE_MAX(b)    (((1l  << (8*(b)-2)) - 1l)  + (1l  << (8*(b)-2)))
#define LZO_UTYPE_MAX(b)    (((1ul << (8*(b)-1)) - 1ul) + (1ul << (8*(b)-1)))

#if !defined(SIZEOF_UNSIGNED)
#  if (UINT_MAX == 0xffff)
#    define SIZEOF_UNSIGNED         2
#  elif (UINT_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED         4
#  elif (UINT_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED         8
#  else
#    error "SIZEOF_UNSIGNED"
#  endif
#endif

#if !defined(SIZEOF_UNSIGNED_LONG)
#  if (ULONG_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    4
#  elif (ULONG_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    8
#  else
#    error "SIZEOF_UNSIGNED_LONG"
#  endif
#endif

#if !defined(SIZEOF_SIZE_T)
#  define SIZEOF_SIZE_T             SIZEOF_UNSIGNED
#endif
#if !defined(SIZE_T_MAX)
#  define SIZE_T_MAX                LZO_UTYPE_MAX(SIZEOF_SIZE_T)
#endif

#if 1 && defined(__LZO_i386) && (UINT_MAX == LZO_0xffffffffL)
#  if !defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX == 0xffff)
#    define LZO_UNALIGNED_OK_2
#  endif
#  if !defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX == LZO_0xffffffffL)
#    define LZO_UNALIGNED_OK_4
#  endif
#endif

#if defined(LZO_UNALIGNED_OK_2) || defined(LZO_UNALIGNED_OK_4)
#  if !defined(LZO_UNALIGNED_OK)
#    define LZO_UNALIGNED_OK
#  endif
#endif

#if defined(__LZO_NO_UNALIGNED)
#  undef LZO_UNALIGNED_OK
#  undef LZO_UNALIGNED_OK_2
#  undef LZO_UNALIGNED_OK_4
#endif

#if defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX != 0xffff)
#  error "LZO_UNALIGNED_OK_2 must not be defined on this system"
#endif
#if defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_UNALIGNED_OK_4 must not be defined on this system"
#endif

#if defined(__LZO_NO_ALIGNED)
#  undef LZO_ALIGNED_OK_4
#endif

#if defined(LZO_ALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_ALIGNED_OK_4 must not be defined on this system"
#endif

#define LZO_LITTLE_ENDIAN       1234
#define LZO_BIG_ENDIAN          4321
#define LZO_PDP_ENDIAN          3412

#if !defined(LZO_BYTE_ORDER)
#  if defined(MFX_BYTE_ORDER)
#    define LZO_BYTE_ORDER      MFX_BYTE_ORDER
#  elif defined(__LZO_i386)
#    define LZO_BYTE_ORDER      LZO_LITTLE_ENDIAN
#  elif defined(BYTE_ORDER)
#    define LZO_BYTE_ORDER      BYTE_ORDER
#  elif defined(__BYTE_ORDER)
#    define LZO_BYTE_ORDER      __BYTE_ORDER
#  endif
#endif

#if defined(LZO_BYTE_ORDER)
#  if (LZO_BYTE_ORDER != LZO_LITTLE_ENDIAN) && \
      (LZO_BYTE_ORDER != LZO_BIG_ENDIAN)
#    error "invalid LZO_BYTE_ORDER"
#  endif
#endif

#if defined(LZO_UNALIGNED_OK) && !defined(LZO_BYTE_ORDER)
#  error "LZO_BYTE_ORDER is not defined"
#endif

#define LZO_OPTIMIZE_GNUC_i386_IS_BUGGY

#if defined(NDEBUG) && !defined(LZO_DEBUG) && !defined(__LZO_CHECKER)
#  if defined(__GNUC__) && defined(__i386__)
#    if !defined(LZO_OPTIMIZE_GNUC_i386_IS_BUGGY)
#      define LZO_OPTIMIZE_GNUC_i386
#    endif
#  endif
#endif

__LZO_EXTERN_C int __lzo_init_done;
__LZO_EXTERN_C const lzo_byte __lzo_copyright[];
LZO_EXTERN(const lzo_byte *) lzo_copyright(void);
__LZO_EXTERN_C const lzo_uint32 _lzo_crc32_table[256];

#define _LZO_STRINGIZE(x)           #x
#define _LZO_MEXPAND(x)             _LZO_STRINGIZE(x)

#define _LZO_CONCAT2(a,b)           a ## b
#define _LZO_CONCAT3(a,b,c)         a ## b ## c
#define _LZO_CONCAT4(a,b,c,d)       a ## b ## c ## d
#define _LZO_CONCAT5(a,b,c,d,e)     a ## b ## c ## d ## e

#define _LZO_ECONCAT2(a,b)          _LZO_CONCAT2(a,b)
#define _LZO_ECONCAT3(a,b,c)        _LZO_CONCAT3(a,b,c)
#define _LZO_ECONCAT4(a,b,c,d)      _LZO_CONCAT4(a,b,c,d)
#define _LZO_ECONCAT5(a,b,c,d,e)    _LZO_CONCAT5(a,b,c,d,e)

#if 0

#define __LZO_IS_COMPRESS_QUERY(i,il,o,ol,w)    ((lzo_voidp)(o) == (w))
#define __LZO_QUERY_COMPRESS(i,il,o,ol,w,n,s) \
		(*ol = (n)*(s), LZO_E_OK)

#define __LZO_IS_DECOMPRESS_QUERY(i,il,o,ol,w)  ((lzo_voidp)(o) == (w))
#define __LZO_QUERY_DECOMPRESS(i,il,o,ol,w,n,s) \
		(*ol = (n)*(s), LZO_E_OK)

#define __LZO_IS_OPTIMIZE_QUERY(i,il,o,ol,w)    ((lzo_voidp)(o) == (w))
#define __LZO_QUERY_OPTIMIZE(i,il,o,ol,w,n,s) \
		(*ol = (n)*(s), LZO_E_OK)

#endif

#ifndef __LZO_PTR_H
#define __LZO_PTR_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#  include <dos.h>
#  if 1 && defined(__WATCOMC__)
#    include <i86.h>
     __LZO_EXTERN_C unsigned char _HShift;
#    define __LZO_HShift    _HShift
#  elif 1 && defined(_MSC_VER)
     __LZO_EXTERN_C unsigned short __near _AHSHIFT;
#    define __LZO_HShift    ((unsigned) &_AHSHIFT)
#  elif defined(__LZO_WIN16)
#    define __LZO_HShift    3
#  else
#    define __LZO_HShift    12
#  endif
#  if !defined(_FP_SEG) && defined(FP_SEG)
#    define _FP_SEG         FP_SEG
#  endif
#  if !defined(_FP_OFF) && defined(FP_OFF)
#    define _FP_OFF         FP_OFF
#  endif
#endif

#if !defined(lzo_ptrdiff_t)
#  if (UINT_MAX >= LZO_0xffffffffL)
     typedef ptrdiff_t          lzo_ptrdiff_t;
#  else
     typedef long               lzo_ptrdiff_t;
#  endif
#endif

#if !defined(__LZO_HAVE_PTR_T)
#  if defined(lzo_ptr_t)
#    define __LZO_HAVE_PTR_T
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_LONG)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_LONG)
       typedef unsigned long    lzo_ptr_t;
       typedef long             lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED)
       typedef unsigned int     lzo_ptr_t;
       typedef int              lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_SHORT)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_SHORT)
       typedef unsigned short   lzo_ptr_t;
       typedef short            lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(LZO_HAVE_CONFIG_H) || defined(SIZEOF_CHAR_P)
#    error "no suitable type for lzo_ptr_t"
#  else
     typedef unsigned long      lzo_ptr_t;
     typedef long               lzo_sptr_t;
#    define __LZO_HAVE_PTR_T
#  endif
#endif

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
#define PTR(a)              ((lzo_bytep) (a))
#define PTR_ALIGNED_4(a)    ((_FP_OFF(a) & 3) == 0)
#define PTR_ALIGNED2_4(a,b) (((_FP_OFF(a) | _FP_OFF(b)) & 3) == 0)
#else
#define PTR(a)              ((lzo_ptr_t) (a))
#define PTR_LINEAR(a)       PTR(a)
#define PTR_ALIGNED_4(a)    ((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)    ((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)
#endif

#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_GE(a,b)         (PTR(a) >= PTR(b))
#define PTR_DIFF(a,b)       ((lzo_ptrdiff_t) (PTR(a) - PTR(b)))
#define pd(a,b)             ((lzo_uint) ((a)-(b)))

LZO_EXTERN(lzo_ptr_t)
__lzo_ptr_linear(const lzo_voidp ptr);

typedef union
{
    char            a_char;
    unsigned char   a_uchar;
    short           a_short;
    unsigned short  a_ushort;
    int             a_int;
    unsigned int    a_uint;
    long            a_long;
    unsigned long   a_ulong;
    lzo_int         a_lzo_int;
    lzo_uint        a_lzo_uint;
    lzo_int32       a_lzo_int32;
    lzo_uint32      a_lzo_uint32;
    ptrdiff_t       a_ptrdiff_t;
    lzo_ptrdiff_t   a_lzo_ptrdiff_t;
    lzo_ptr_t       a_lzo_ptr_t;
    lzo_voidp       a_lzo_voidp;
    void *          a_void_p;
    lzo_bytep       a_lzo_bytep;
    lzo_bytepp      a_lzo_bytepp;
    lzo_uintp       a_lzo_uintp;
    lzo_uint *      a_lzo_uint_p;
    lzo_uint32p     a_lzo_uint32p;
    lzo_uint32 *    a_lzo_uint32_p;
    unsigned char * a_uchar_p;
    char *          a_char_p;
}
lzo_full_align_t;

#ifdef __cplusplus
}
#endif

#endif

#define LZO_DETERMINISTIC

#define LZO_DICT_USE_PTR
#if defined(__LZO_DOS16) || defined(__LZO_WIN16) || defined(__LZO_STRICT_16BIT)
#  undef LZO_DICT_USE_PTR
#endif

#if defined(LZO_DICT_USE_PTR)
#  define lzo_dict_t    const lzo_bytep
#  define lzo_dict_p    lzo_dict_t __LZO_MMODEL *
#else
#  define lzo_dict_t    lzo_uint
#  define lzo_dict_p    lzo_dict_t __LZO_MMODEL *
#endif

#if !defined(lzo_moff_t)
#define lzo_moff_t      lzo_uint
#endif

#endif

LZO_PUBLIC(lzo_ptr_t)
__lzo_ptr_linear(const lzo_voidp ptr)
{
    lzo_ptr_t p;

#if defined(__LZO_DOS16) || defined(__LZO_WIN16)
    p = (((lzo_ptr_t)(_FP_SEG(ptr))) << (16 - __LZO_HShift)) + (_FP_OFF(ptr));
#else
    p = PTR_LINEAR(ptr);
#endif

    return p;
}

LZO_PUBLIC(unsigned)
__lzo_align_gap(const lzo_voidp ptr, lzo_uint size)
{
    lzo_ptr_t p, s, n;

    assert(size > 0);

    p = __lzo_ptr_linear(ptr);
    s = (lzo_ptr_t) (size - 1);
#if 0
    assert((size & (size - 1)) == 0);
    n = ((p + s) & ~s) - p;
#else
    n = (((p + s) / size) * size) - p;
#endif

    assert((long)n >= 0);
    assert(n <= s);

    return (unsigned)n;
}

#ifndef __LZO_UTIL_H
#define __LZO_UTIL_H

#ifndef __LZO_CONF_H
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if 1 && defined(HAVE_MEMCPY)
#if !defined(__LZO_DOS16) && !defined(__LZO_WIN16)

#define MEMCPY8_DS(dest,src,len) \
    memcpy(dest,src,len); \
    dest += len; \
    src += len

#endif
#endif

#if 0 && !defined(MEMCPY8_DS)

#define MEMCPY8_DS(dest,src,len) \
    { do { \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	len -= 8; \
    } while (len > 0); }

#endif

#if !defined(MEMCPY8_DS)

#define MEMCPY8_DS(dest,src,len) \
    { register lzo_uint __l = (len) / 8; \
    do { \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
    } while (--__l > 0); }

#endif

#define MEMCPY_DS(dest,src,len) \
    do *dest++ = *src++; \
    while (--len > 0)

#define MEMMOVE_DS(dest,src,len) \
    do *dest++ = *src++; \
    while (--len > 0)

#if 0 && defined(LZO_OPTIMIZE_GNUC_i386)

#define BZERO8_PTR(s,l,n) \
__asm__ __volatile__( \
    "movl  %0,%%eax \n"             \
    "movl  %1,%%edi \n"             \
    "movl  %2,%%ecx \n"             \
    "cld \n"                        \
    "rep \n"                        \
    "stosl %%eax,(%%edi) \n"        \
    :               \
    :"g" (0),"g" (s),"g" (n)        \
    :"eax","edi","ecx", "memory", "cc" \
)

#elif (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)

#if 1
#define BZERO8_PTR(s,l,n)   memset((s),0,(lzo_uint)(l)*(n))
#else
#define BZERO8_PTR(s,l,n)   memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))
#endif

#else

#define BZERO8_PTR(s,l,n) \
    lzo_memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))

#endif

#if 0
#if defined(__GNUC__) && defined(__i386__)

unsigned char lzo_rotr8(unsigned char value, int shift);
extern __inline__ unsigned char lzo_rotr8(unsigned char value, int shift)
{
    unsigned char result;

    __asm__ __volatile__ ("movb %b1, %b0; rorb %b2, %b0"
			: "=a"(result) : "g"(value), "c"(shift));
    return result;
}

unsigned short lzo_rotr16(unsigned short value, int shift);
extern __inline__ unsigned short lzo_rotr16(unsigned short value, int shift)
{
    unsigned short result;

    __asm__ __volatile__ ("movw %b1, %b0; rorw %b2, %b0"
			: "=a"(result) : "g"(value), "c"(shift));
    return result;
}

#endif
#endif

#ifdef __cplusplus
}
#endif

#endif

LZO_PUBLIC(lzo_bool)
lzo_assert(int expr)
{
    return (expr) ? 1 : 0;
}

/* If you use the LZO library in a product, you *must* keep this
 * copyright string in the executable of your product.
 */

const lzo_byte __lzo_copyright[] =
#if !defined(__LZO_IN_MINLZO)
    LZO_VERSION_STRING;
#else
    "\n\n\n"
    "LZO real-time data compression library.\n"
    "Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002 Markus Franz Xaver Johannes Oberhumer\n"
    "<markus.oberhumer@jk.uni-linz.ac.at>\n"
    "http://www.oberhumer.com/opensource/lzo/\n"
    "\n"
    "LZO version: v" LZO_VERSION_STRING ", " LZO_VERSION_DATE "\n"
    "LZO build date: " __DATE__ " " __TIME__ "\n\n"
    "LZO special compilation options:\n"
#ifdef __cplusplus
    " __cplusplus\n"
#endif
#if defined(__PIC__)
    " __PIC__\n"
#elif defined(__pic__)
    " __pic__\n"
#endif
#if (UINT_MAX < LZO_0xffffffffL)
    " 16BIT\n"
#endif
#if defined(__LZO_STRICT_16BIT)
    " __LZO_STRICT_16BIT\n"
#endif
#if (UINT_MAX > LZO_0xffffffffL)
    " UINT_MAX=" _LZO_MEXPAND(UINT_MAX) "\n"
#endif
#if (ULONG_MAX > LZO_0xffffffffL)
    " ULONG_MAX=" _LZO_MEXPAND(ULONG_MAX) "\n"
#endif
#if defined(LZO_BYTE_ORDER)
    " LZO_BYTE_ORDER=" _LZO_MEXPAND(LZO_BYTE_ORDER) "\n"
#endif
#if defined(LZO_UNALIGNED_OK_2)
    " LZO_UNALIGNED_OK_2\n"
#endif
#if defined(LZO_UNALIGNED_OK_4)
    " LZO_UNALIGNED_OK_4\n"
#endif
#if defined(LZO_ALIGNED_OK_4)
    " LZO_ALIGNED_OK_4\n"
#endif
#if defined(LZO_DICT_USE_PTR)
    " LZO_DICT_USE_PTR\n"
#endif
#if defined(__LZO_QUERY_COMPRESS)
    " __LZO_QUERY_COMPRESS\n"
#endif
#if defined(__LZO_QUERY_DECOMPRESS)
    " __LZO_QUERY_DECOMPRESS\n"
#endif
#if defined(__LZO_IN_MINILZO)
    " __LZO_IN_MINILZO\n"
#endif
    "\n\n"
    "$Id: LZO " LZO_VERSION_STRING " built " __DATE__ " " __TIME__
#if defined(__GNUC__) && defined(__VERSION__)
    " by gcc " __VERSION__
#elif defined(__BORLANDC__)
    " by Borland C " _LZO_MEXPAND(__BORLANDC__)
#elif defined(_MSC_VER)
    " by Microsoft C " _LZO_MEXPAND(_MSC_VER)
#elif defined(__PUREC__)
    " by Pure C " _LZO_MEXPAND(__PUREC__)
#elif defined(__SC__)
    " by Symantec C " _LZO_MEXPAND(__SC__)
#elif defined(__TURBOC__)
    " by Turbo C " _LZO_MEXPAND(__TURBOC__)
#elif defined(__WATCOMC__)
    " by Watcom C " _LZO_MEXPAND(__WATCOMC__)
#endif
    " $\n"
    "$Copyright: LZO (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002 Markus Franz Xaver Johannes Oberhumer $\n";
#endif

LZO_PUBLIC(const lzo_byte *)
lzo_copyright(void)
{
    return __lzo_copyright;
}

LZO_PUBLIC(unsigned)
lzo_version(void)
{
    return LZO_VERSION;
}

LZO_PUBLIC(const char *)
lzo_version_string(void)
{
    return LZO_VERSION_STRING;
}

LZO_PUBLIC(const char *)
lzo_version_date(void)
{
    return LZO_VERSION_DATE;
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_string(void)
{
    return LZO_VERSION_STRING;
}

LZO_PUBLIC(const lzo_charp)
_lzo_version_date(void)
{
    return LZO_VERSION_DATE;
}

#define LZO_BASE 65521u
#define LZO_NMAX 5552

#define LZO_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define LZO_DO2(buf,i)  LZO_DO1(buf,i); LZO_DO1(buf,i+1);
#define LZO_DO4(buf,i)  LZO_DO2(buf,i); LZO_DO2(buf,i+2);
#define LZO_DO8(buf,i)  LZO_DO4(buf,i); LZO_DO4(buf,i+4);
#define LZO_DO16(buf,i) LZO_DO8(buf,i); LZO_DO8(buf,i+8);

LZO_PUBLIC(lzo_uint32)
lzo_adler32(lzo_uint32 adler, const lzo_byte *buf, lzo_uint len)
{
    lzo_uint32 s1 = adler & 0xffff;
    lzo_uint32 s2 = (adler >> 16) & 0xffff;
    int k;

    if (buf == NULL)
	return 1;

    while (len > 0)
    {
	k = len < LZO_NMAX ? (int) len : LZO_NMAX;
	len -= k;
	if (k >= 16) do
	{
	    LZO_DO16(buf,0);
	    buf += 16;
	    k -= 16;
	} while (k >= 16);
	if (k != 0) do
	{
	    s1 += *buf++;
	    s2 += s1;
	} while (--k > 0);
	s1 %= LZO_BASE;
	s2 %= LZO_BASE;
    }
    return (s2 << 16) | s1;
}

LZO_PUBLIC(int)
lzo_memcmp(const lzo_voidp s1, const lzo_voidp s2, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCMP)
    return memcmp(s1,s2,len);
#else
    const lzo_byte *p1 = (const lzo_byte *) s1;
    const lzo_byte *p2 = (const lzo_byte *) s2;
    int d;

    if (len > 0) do
    {
	d = *p1 - *p2;
	if (d != 0)
	    return d;
	p1++;
	p2++;
    }
    while (--len > 0);
    return 0;
#endif
}

LZO_PUBLIC(lzo_voidp)
lzo_memcpy(lzo_voidp dest, const lzo_voidp src, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMCPY)
    return memcpy(dest,src,len);
#else
    lzo_byte *p1 = (lzo_byte *) dest;
    const lzo_byte *p2 = (const lzo_byte *) src;

    if (len <= 0 || p1 == p2)
	return dest;
    do
	*p1++ = *p2++;
    while (--len > 0);
    return dest;
#endif
}

LZO_PUBLIC(lzo_voidp)
lzo_memmove(lzo_voidp dest, const lzo_voidp src, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMMOVE)
    return memmove(dest,src,len);
#else
    lzo_byte *p1 = (lzo_byte *) dest;
    const lzo_byte *p2 = (const lzo_byte *) src;

    if (len <= 0 || p1 == p2)
	return dest;

    if (p1 < p2)
    {
	do
	    *p1++ = *p2++;
	while (--len > 0);
    }
    else
    {
	p1 += len;
	p2 += len;
	do
	    *--p1 = *--p2;
	while (--len > 0);
    }
    return dest;
#endif
}

LZO_PUBLIC(lzo_voidp)
lzo_memset(lzo_voidp s, int c, lzo_uint len)
{
#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)
    return memset(s,c,len);
#else
    lzo_byte *p = (lzo_byte *) s;

    if (len > 0) do
	*p++ = LZO_BYTE(c);
    while (--len > 0);
    return s;
#endif
}

#if 0
#  define IS_SIGNED(type)       (((type) (1ul << (8 * sizeof(type) - 1))) < 0)
#  define IS_UNSIGNED(type)     (((type) (1ul << (8 * sizeof(type) - 1))) > 0)
#else
#  define IS_SIGNED(type)       (((type) (-1)) < ((type) 0))
#  define IS_UNSIGNED(type)     (((type) (-1)) > ((type) 0))
#endif

#define IS_POWER_OF_2(x)        (((x) & ((x) - 1)) == 0)

static lzo_bool schedule_insns_bug(void);
static lzo_bool strength_reduce_bug(int *);

#if 0 || defined(LZO_DEBUG)
#include <stdio.h>
static lzo_bool __lzo_assert_fail(const char *s, unsigned line)
{
#if defined(__palmos__)
    printf("LZO assertion failed in line %u: '%s'\n",line,s);
#else
    fprintf(stderr,"LZO assertion failed in line %u: '%s'\n",line,s);
#endif
    return 0;
}
#  define __lzo_assert(x)   ((x) ? 1 : __lzo_assert_fail(#x,__LINE__))
#else
#  define __lzo_assert(x)   ((x) ? 1 : 0)
#endif

#undef COMPILE_TIME_ASSERT
#if 0
#  define COMPILE_TIME_ASSERT(expr)     r &= __lzo_assert(expr)
#else
#  define COMPILE_TIME_ASSERT(expr)     LZO_COMPILE_TIME_ASSERT(expr)
#endif

static lzo_bool basic_integral_check(void)
{
    lzo_bool r = 1;

    COMPILE_TIME_ASSERT(CHAR_BIT == 8);
    COMPILE_TIME_ASSERT(sizeof(char) == 1);
    COMPILE_TIME_ASSERT(sizeof(short) >= 2);
    COMPILE_TIME_ASSERT(sizeof(long) >= 4);
    COMPILE_TIME_ASSERT(sizeof(int) >= sizeof(short));
    COMPILE_TIME_ASSERT(sizeof(long) >= sizeof(int));

    COMPILE_TIME_ASSERT(sizeof(lzo_uint) == sizeof(lzo_int));
    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == sizeof(lzo_int32));

    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= 4);
    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= sizeof(unsigned));
#if defined(__LZO_STRICT_16BIT)
    COMPILE_TIME_ASSERT(sizeof(lzo_uint) == 2);
#else
    COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= 4);
    COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= sizeof(unsigned));
#endif

#if (USHRT_MAX == 65535u)
    COMPILE_TIME_ASSERT(sizeof(short) == 2);
#elif (USHRT_MAX == LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(short) == 4);
#elif (USHRT_MAX >= LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(short) > 4);
#endif
#if (UINT_MAX == 65535u)
    COMPILE_TIME_ASSERT(sizeof(int) == 2);
#elif (UINT_MAX == LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(int) == 4);
#elif (UINT_MAX >= LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(int) > 4);
#endif
#if (ULONG_MAX == 65535ul)
    COMPILE_TIME_ASSERT(sizeof(long) == 2);
#elif (ULONG_MAX == LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(long) == 4);
#elif (ULONG_MAX >= LZO_0xffffffffL)
    COMPILE_TIME_ASSERT(sizeof(long) > 4);
#endif

#if defined(SIZEOF_UNSIGNED)
    COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED == sizeof(unsigned));
#endif
#if defined(SIZEOF_UNSIGNED_LONG)
    COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED_LONG == sizeof(unsigned long));
#endif
#if defined(SIZEOF_UNSIGNED_SHORT)
    COMPILE_TIME_ASSERT(SIZEOF_UNSIGNED_SHORT == sizeof(unsigned short));
#endif
#if !defined(__LZO_IN_MINILZO)
#if defined(SIZEOF_SIZE_T)
    COMPILE_TIME_ASSERT(SIZEOF_SIZE_T == sizeof(size_t));
#endif
#endif

    COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned char));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned short));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned long));
    COMPILE_TIME_ASSERT(IS_SIGNED(short));
    COMPILE_TIME_ASSERT(IS_SIGNED(int));
    COMPILE_TIME_ASSERT(IS_SIGNED(long));

    COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint32));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint));
    COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int32));
    COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int));

    COMPILE_TIME_ASSERT(INT_MAX    == LZO_STYPE_MAX(sizeof(int)));
    COMPILE_TIME_ASSERT(UINT_MAX   == LZO_UTYPE_MAX(sizeof(unsigned)));
    COMPILE_TIME_ASSERT(LONG_MAX   == LZO_STYPE_MAX(sizeof(long)));
    COMPILE_TIME_ASSERT(ULONG_MAX  == LZO_UTYPE_MAX(sizeof(unsigned long)));
    COMPILE_TIME_ASSERT(SHRT_MAX   == LZO_STYPE_MAX(sizeof(short)));
    COMPILE_TIME_ASSERT(USHRT_MAX  == LZO_UTYPE_MAX(sizeof(unsigned short)));
    COMPILE_TIME_ASSERT(LZO_UINT32_MAX == LZO_UTYPE_MAX(sizeof(lzo_uint32)));
    COMPILE_TIME_ASSERT(LZO_UINT_MAX   == LZO_UTYPE_MAX(sizeof(lzo_uint)));
#if !defined(__LZO_IN_MINILZO)
    COMPILE_TIME_ASSERT(SIZE_T_MAX     == LZO_UTYPE_MAX(sizeof(size_t)));
#endif

    r &= __lzo_assert(LZO_BYTE(257) == 1);

    return r;
}

static lzo_bool basic_ptr_check(void)
{
    lzo_bool r = 1;

    COMPILE_TIME_ASSERT(sizeof(char *) >= sizeof(int));
    COMPILE_TIME_ASSERT(sizeof(lzo_byte *) >= sizeof(char *));

    COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_byte *));
    COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_voidpp));
    COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_bytepp));
    COMPILE_TIME_ASSERT(sizeof(lzo_voidp) >= sizeof(lzo_uint));

    COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_voidp));
    COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_sptr_t));
    COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) >= sizeof(lzo_uint));

    COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= 4);
    COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(ptrdiff_t));

    COMPILE_TIME_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t));
    COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(lzo_uint));

#if defined(SIZEOF_CHAR_P)
    COMPILE_TIME_ASSERT(SIZEOF_CHAR_P == sizeof(char *));
#endif
#if defined(SIZEOF_PTRDIFF_T)
    COMPILE_TIME_ASSERT(SIZEOF_PTRDIFF_T == sizeof(ptrdiff_t));
#endif

    COMPILE_TIME_ASSERT(IS_SIGNED(ptrdiff_t));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(size_t));
    COMPILE_TIME_ASSERT(IS_SIGNED(lzo_ptrdiff_t));
    COMPILE_TIME_ASSERT(IS_SIGNED(lzo_sptr_t));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_ptr_t));
    COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_moff_t));

    return r;
}

static lzo_bool ptr_check(void)
{
    lzo_bool r = 1;
    int i;
    char _wrkmem[10 * sizeof(lzo_byte *) + sizeof(lzo_full_align_t)];
    lzo_bytep wrkmem;
    lzo_bytepp dict;
    unsigned char x[4 * sizeof(lzo_full_align_t)];
    long d;
    lzo_full_align_t a;
    lzo_full_align_t u;

    for (i = 0; i < (int) sizeof(x); i++)
	x[i] = LZO_BYTE(i);

    wrkmem = LZO_PTR_ALIGN_UP((lzo_byte *)_wrkmem,sizeof(lzo_full_align_t));

#if 0
    dict = (lzo_bytepp) wrkmem;
#else

    u.a_lzo_bytep = wrkmem; dict = u.a_lzo_bytepp;
#endif

    d = (long) ((const lzo_bytep) dict - (const lzo_bytep) _wrkmem);
    r &= __lzo_assert(d >= 0);
    r &= __lzo_assert(d < (long) sizeof(lzo_full_align_t));

    memset(&a,0,sizeof(a));
    r &= __lzo_assert(a.a_lzo_voidp == NULL);

    memset(&a,0xff,sizeof(a));
    r &= __lzo_assert(a.a_ushort == USHRT_MAX);
    r &= __lzo_assert(a.a_uint == UINT_MAX);
    r &= __lzo_assert(a.a_ulong == ULONG_MAX);
    r &= __lzo_assert(a.a_lzo_uint == LZO_UINT_MAX);
    r &= __lzo_assert(a.a_lzo_uint32 == LZO_UINT32_MAX);

    if (r == 1)
    {
	for (i = 0; i < 8; i++)
	    r &= __lzo_assert((const lzo_voidp) (&dict[i]) == (const lzo_voidp) (&wrkmem[i * sizeof(lzo_byte *)]));
    }

    memset(&a,0,sizeof(a));
    r &= __lzo_assert(a.a_char_p == NULL);
    r &= __lzo_assert(a.a_lzo_bytep == NULL);
    r &= __lzo_assert(NULL == (void *)0);
    if (r == 1)
    {
	for (i = 0; i < 10; i++)
	    dict[i] = wrkmem;
	BZERO8_PTR(dict+1,sizeof(dict[0]),8);
	r &= __lzo_assert(dict[0] == wrkmem);
	for (i = 1; i < 9; i++)
	    r &= __lzo_assert(dict[i] == NULL);
	r &= __lzo_assert(dict[9] == wrkmem);
    }

    if (r == 1)
    {
	unsigned k = 1;
	const unsigned n = (unsigned) sizeof(lzo_uint32);
	lzo_byte *p0;
	lzo_byte *p1;

	k += __lzo_align_gap(&x[k],n);
	p0 = (lzo_bytep) &x[k];
#if defined(PTR_LINEAR)
	r &= __lzo_assert((PTR_LINEAR(p0) & (n-1)) == 0);
#else
	r &= __lzo_assert(n == 4);
	r &= __lzo_assert(PTR_ALIGNED_4(p0));
#endif

	r &= __lzo_assert(k >= 1);
	p1 = (lzo_bytep) &x[1];
	r &= __lzo_assert(PTR_GE(p0,p1));

	r &= __lzo_assert(k < 1+n);
	p1 = (lzo_bytep) &x[1+n];
	r &= __lzo_assert(PTR_LT(p0,p1));

	if (r == 1)
	{
	    lzo_uint32 v0, v1;
#if 0
	    v0 = * (lzo_uint32 *) &x[k];
	    v1 = * (lzo_uint32 *) &x[k+n];
#else

	    u.a_uchar_p = &x[k];
	    v0 = *u.a_lzo_uint32_p;
	    u.a_uchar_p = &x[k+n];
	    v1 = *u.a_lzo_uint32_p;
#endif
	    r &= __lzo_assert(v0 > 0);
	    r &= __lzo_assert(v1 > 0);
	}
    }

    return r;
}

LZO_PUBLIC(int)
_lzo_config_check(void)
{
    lzo_bool r = 1;
    int i;
    union {
	lzo_uint32 a;
	unsigned short b;
	lzo_uint32 aa[4];
	unsigned char x[4*sizeof(lzo_full_align_t)];
    } u;

    COMPILE_TIME_ASSERT( (int) ((unsigned char) ((signed char) -1)) == 255);
    COMPILE_TIME_ASSERT( (((unsigned char)128) << (int)(8*sizeof(int)-8)) < 0);

#if 0
    r &= __lzo_assert((const void *)&u == (const void *)&u.a);
    r &= __lzo_assert((const void *)&u == (const void *)&u.b);
    r &= __lzo_assert((const void *)&u == (const void *)&u.x[0]);
    r &= __lzo_assert((const void *)&u == (const void *)&u.aa[0]);
#endif

    r &= basic_integral_check();
    r &= basic_ptr_check();
    if (r != 1)
	return LZO_E_ERROR;

    u.a = 0; u.b = 0;
    for (i = 0; i < (int) sizeof(u.x); i++)
	u.x[i] = LZO_BYTE(i);

#if defined(LZO_BYTE_ORDER)
    if (r == 1)
    {
#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
	lzo_uint32 a = (lzo_uint32) (u.a & LZO_0xffffffffL);
	unsigned short b = (unsigned short) (u.b & 0xffff);
	r &= __lzo_assert(a == 0x03020100L);
	r &= __lzo_assert(b == 0x0100);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
	lzo_uint32 a = u.a >> (8 * sizeof(u.a) - 32);
	unsigned short b = u.b >> (8 * sizeof(u.b) - 16);
	r &= __lzo_assert(a == 0x00010203L);
	r &= __lzo_assert(b == 0x0001);
#  else
#    error "invalid LZO_BYTE_ORDER"
#  endif
    }
#endif

#if defined(LZO_UNALIGNED_OK_2)
    COMPILE_TIME_ASSERT(sizeof(short) == 2);
    if (r == 1)
    {
	unsigned short b[4];

	for (i = 0; i < 4; i++)
	    b[i] = * (const unsigned short *) &u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
	r &= __lzo_assert(b[0] == 0x0100);
	r &= __lzo_assert(b[1] == 0x0201);
	r &= __lzo_assert(b[2] == 0x0302);
	r &= __lzo_assert(b[3] == 0x0403);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
	r &= __lzo_assert(b[0] == 0x0001);
	r &= __lzo_assert(b[1] == 0x0102);
	r &= __lzo_assert(b[2] == 0x0203);
	r &= __lzo_assert(b[3] == 0x0304);
#  endif
    }
#endif

#if defined(LZO_UNALIGNED_OK_4)
    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
    if (r == 1)
    {
	lzo_uint32 a[4];

	for (i = 0; i < 4; i++)
	    a[i] = * (const lzo_uint32 *) &u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
	r &= __lzo_assert(a[0] == 0x03020100L);
	r &= __lzo_assert(a[1] == 0x04030201L);
	r &= __lzo_assert(a[2] == 0x05040302L);
	r &= __lzo_assert(a[3] == 0x06050403L);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
	r &= __lzo_assert(a[0] == 0x00010203L);
	r &= __lzo_assert(a[1] == 0x01020304L);
	r &= __lzo_assert(a[2] == 0x02030405L);
	r &= __lzo_assert(a[3] == 0x03040506L);
#  endif
    }
#endif

#if defined(LZO_ALIGNED_OK_4)
    COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
#endif

    COMPILE_TIME_ASSERT(lzo_sizeof_dict_t == sizeof(lzo_dict_t));

#if defined(__LZO_IN_MINLZO)
    if (r == 1)
    {
	lzo_uint32 adler;
	adler = lzo_adler32(0, NULL, 0);
	adler = lzo_adler32(adler, lzo_copyright(), 200);
	r &= __lzo_assert(adler == 0xc76f1751L);
    }
#endif

    if (r == 1)
    {
	r &= __lzo_assert(!schedule_insns_bug());
    }

    if (r == 1)
    {
	static int x[3];
	static unsigned xn = 3;
	register unsigned j;

	for (j = 0; j < xn; j++)
	    x[j] = (int)j - 3;
	r &= __lzo_assert(!strength_reduce_bug(x));
    }

    if (r == 1)
    {
	r &= ptr_check();
    }

    return r == 1 ? LZO_E_OK : LZO_E_ERROR;
}

static lzo_bool schedule_insns_bug(void)
{
#if defined(__LZO_CHECKER)
    return 0;
#else
    const int clone[] = {1, 2, 0};
    const int *q;
    q = clone;
    return (*q) ? 0 : 1;
#endif
}

static lzo_bool strength_reduce_bug(int *x)
{
    return x[0] != -3 || x[1] != -2 || x[2] != -1;
}

#undef COMPILE_TIME_ASSERT

int __lzo_init_done = 0;

LZO_PUBLIC(int)
__lzo_init2(unsigned v, int s1, int s2, int s3, int s4, int s5,
			int s6, int s7, int s8, int s9)
{
    int r;

    __lzo_init_done = 1;

    if (v == 0)
	return LZO_E_ERROR;

    r = (s1 == -1 || s1 == (int) sizeof(short)) &&
	(s2 == -1 || s2 == (int) sizeof(int)) &&
	(s3 == -1 || s3 == (int) sizeof(long)) &&
	(s4 == -1 || s4 == (int) sizeof(lzo_uint32)) &&
	(s5 == -1 || s5 == (int) sizeof(lzo_uint)) &&
	(s6 == -1 || s6 == (int) lzo_sizeof_dict_t) &&
	(s7 == -1 || s7 == (int) sizeof(char *)) &&
	(s8 == -1 || s8 == (int) sizeof(lzo_voidp)) &&
	(s9 == -1 || s9 == (int) sizeof(lzo_compress_t));
    if (!r)
	return LZO_E_ERROR;

    r = _lzo_config_check();
    if (r != LZO_E_OK)
	return r;

    return r;
}

#if !defined(__LZO_IN_MINILZO)

LZO_EXTERN(int)
__lzo_init(unsigned v,int s1,int s2,int s3,int s4,int s5,int s6,int s7);

LZO_PUBLIC(int)
__lzo_init(unsigned v,int s1,int s2,int s3,int s4,int s5,int s6,int s7)
{
    if (v == 0 || v > 0x1010)
	return LZO_E_ERROR;
    return __lzo_init2(v,s1,s2,s3,s4,s5,-1,-1,s6,s7);
}

#endif

#define do_compress         _lzo1x_1_do_compress

#define LZO_NEED_DICT_H
#define D_BITS          14
#define D_INDEX1(d,p)       d = DM((0x21*DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)       d = (d & (D_MASK & 0x7ff)) ^ (D_HIGH | 0x1f)

#ifndef __LZO_CONFIG1X_H
#define __LZO_CONFIG1X_H

#if !defined(LZO1X) && !defined(LZO1Y) && !defined(LZO1Z)
#  define LZO1X
#endif

#if !defined(__LZO_IN_MINILZO)
#include <lzo1x.h>
#endif

#define LZO_EOF_CODE
#undef LZO_DETERMINISTIC

#define M1_MAX_OFFSET   0x0400
#ifndef M2_MAX_OFFSET
#define M2_MAX_OFFSET   0x0800
#endif
#define M3_MAX_OFFSET   0x4000
#define M4_MAX_OFFSET   0xbfff

#define MX_MAX_OFFSET   (M1_MAX_OFFSET + M2_MAX_OFFSET)

#define M1_MIN_LEN      2
#define M1_MAX_LEN      2
#define M2_MIN_LEN      3
#ifndef M2_MAX_LEN
#define M2_MAX_LEN      8
#endif
#define M3_MIN_LEN      3
#define M3_MAX_LEN      33
#define M4_MIN_LEN      3
#define M4_MAX_LEN      9

#define M1_MARKER       0
#define M2_MARKER       64
#define M3_MARKER       32
#define M4_MARKER       16

#ifndef MIN_LOOKAHEAD
#define MIN_LOOKAHEAD       (M2_MAX_LEN + 1)
#endif

#if defined(LZO_NEED_DICT_H)

#ifndef LZO_HASH
#define LZO_HASH            LZO_HASH_LZO_INCREMENTAL_B
#endif
#define DL_MIN_LEN          M2_MIN_LEN

#ifndef __LZO_DICT_H
#define __LZO_DICT_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(D_BITS) && defined(DBITS)
#  define D_BITS        DBITS
#endif
#if !defined(D_BITS)
#  error "D_BITS is not defined"
#endif
#if (D_BITS < 16)
#  define D_SIZE        LZO_SIZE(D_BITS)
#  define D_MASK        LZO_MASK(D_BITS)
#else
#  define D_SIZE        LZO_USIZE(D_BITS)
#  define D_MASK        LZO_UMASK(D_BITS)
#endif
#define D_HIGH          ((D_MASK >> 1) + 1)

#if !defined(DD_BITS)
#  define DD_BITS       0
#endif
#define DD_SIZE         LZO_SIZE(DD_BITS)
#define DD_MASK         LZO_MASK(DD_BITS)

#if !defined(DL_BITS)
#  define DL_BITS       (D_BITS - DD_BITS)
#endif
#if (DL_BITS < 16)
#  define DL_SIZE       LZO_SIZE(DL_BITS)
#  define DL_MASK       LZO_MASK(DL_BITS)
#else
#  define DL_SIZE       LZO_USIZE(DL_BITS)
#  define DL_MASK       LZO_UMASK(DL_BITS)
#endif

#if (D_BITS != DL_BITS + DD_BITS)
#  error "D_BITS does not match"
#endif
#if (D_BITS < 8 || D_BITS > 18)
#  error "invalid D_BITS"
#endif
#if (DL_BITS < 8 || DL_BITS > 20)
#  error "invalid DL_BITS"
#endif
#if (DD_BITS < 0 || DD_BITS > 6)
#  error "invalid DD_BITS"
#endif

#if !defined(DL_MIN_LEN)
#  define DL_MIN_LEN    3
#endif
#if !defined(DL_SHIFT)
#  define DL_SHIFT      ((DL_BITS + (DL_MIN_LEN - 1)) / DL_MIN_LEN)
#endif

#define LZO_HASH_GZIP                   1
#define LZO_HASH_GZIP_INCREMENTAL       2
#define LZO_HASH_LZO_INCREMENTAL_A      3
#define LZO_HASH_LZO_INCREMENTAL_B      4

#if !defined(LZO_HASH)
#  error "choose a hashing strategy"
#endif

#if (DL_MIN_LEN == 3)
#  define _DV2_A(p,shift1,shift2) \
	(((( (lzo_uint32)((p)[0]) << shift1) ^ (p)[1]) << shift2) ^ (p)[2])
#  define _DV2_B(p,shift1,shift2) \
	(((( (lzo_uint32)((p)[2]) << shift1) ^ (p)[1]) << shift2) ^ (p)[0])
#  define _DV3_B(p,shift1,shift2,shift3) \
	((_DV2_B((p)+1,shift1,shift2) << (shift3)) ^ (p)[0])
#elif (DL_MIN_LEN == 2)
#  define _DV2_A(p,shift1,shift2) \
	(( (lzo_uint32)(p[0]) << shift1) ^ p[1])
#  define _DV2_B(p,shift1,shift2) \
	(( (lzo_uint32)(p[1]) << shift1) ^ p[2])
#else
#  error "invalid DL_MIN_LEN"
#endif
#define _DV_A(p,shift)      _DV2_A(p,shift,shift)
#define _DV_B(p,shift)      _DV2_B(p,shift,shift)
#define DA2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) + (p)[1]) << (s1)) + (p)[0])
#define DS2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) - (p)[1]) << (s1)) - (p)[0])
#define DX2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) ^ (p)[1]) << (s1)) ^ (p)[0])
#define DA3(p,s1,s2,s3) ((DA2((p)+1,s2,s3) << (s1)) + (p)[0])
#define DS3(p,s1,s2,s3) ((DS2((p)+1,s2,s3) << (s1)) - (p)[0])
#define DX3(p,s1,s2,s3) ((DX2((p)+1,s2,s3) << (s1)) ^ (p)[0])
#define DMS(v,s)        ((lzo_uint) (((v) & (D_MASK >> (s))) << (s)))
#define DM(v)           DMS(v,0)

#if (LZO_HASH == LZO_HASH_GZIP)
#  define _DINDEX(dv,p)     (_DV_A((p),DL_SHIFT))

#elif (LZO_HASH == LZO_HASH_GZIP_INCREMENTAL)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),DL_SHIFT)
#  define DVAL_NEXT(dv,p)   dv = (((dv) << DL_SHIFT) ^ p[2])
#  define _DINDEX(dv,p)     (dv)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_A)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),5)
#  define DVAL_NEXT(dv,p) \
		dv ^= (lzo_uint32)(p[-1]) << (2*5); dv = (((dv) << 5) ^ p[2])
#  define _DINDEX(dv,p)     ((0x9f5f * (dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_B)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_B((p),5)
#  define DVAL_NEXT(dv,p) \
		dv ^= p[-1]; dv = (((dv) >> 5) ^ ((lzo_uint32)(p[2]) << (2*5)))
#  define _DINDEX(dv,p)     ((0x9f5f * (dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#else
#  error "choose a hashing strategy"
#endif

#ifndef DINDEX
#define DINDEX(dv,p)        ((lzo_uint)((_DINDEX(dv,p)) & DL_MASK) << DD_BITS)
#endif
#if !defined(DINDEX1) && defined(D_INDEX1)
#define DINDEX1             D_INDEX1
#endif
#if !defined(DINDEX2) && defined(D_INDEX2)
#define DINDEX2             D_INDEX2
#endif

#if !defined(__LZO_HASH_INCREMENTAL)
#  define DVAL_FIRST(dv,p)  ((void) 0)
#  define DVAL_NEXT(dv,p)   ((void) 0)
#  define DVAL_LOOKAHEAD    0
#endif

#if !defined(DVAL_ASSERT)
#if defined(__LZO_HASH_INCREMENTAL) && !defined(NDEBUG)
static void DVAL_ASSERT(lzo_uint32 dv, const lzo_byte *p)
{
    lzo_uint32 df;
    DVAL_FIRST(df,(p));
    assert(DINDEX(dv,p) == DINDEX(df,p));
}
#else
#  define DVAL_ASSERT(dv,p) ((void) 0)
#endif
#endif

#if defined(LZO_DICT_USE_PTR)
#  define DENTRY(p,in)                          (p)
#  define GINDEX(m_pos,m_off,dict,dindex,in)    m_pos = dict[dindex]
#else
#  define DENTRY(p,in)                          ((lzo_uint) ((p)-(in)))
#  define GINDEX(m_pos,m_off,dict,dindex,in)    m_off = dict[dindex]
#endif

#if (DD_BITS == 0)

#  define UPDATE_D(dict,drun,dv,p,in)       dict[ DINDEX(dv,p) ] = DENTRY(p,in)
#  define UPDATE_I(dict,drun,index,p,in)    dict[index] = DENTRY(p,in)
#  define UPDATE_P(ptr,drun,p,in)           (ptr)[0] = DENTRY(p,in)

#else

#  define UPDATE_D(dict,drun,dv,p,in)   \
	dict[ DINDEX(dv,p) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_I(dict,drun,index,p,in)    \
	dict[ (index) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_P(ptr,drun,p,in)   \
	(ptr) [ drun++ ] = DENTRY(p,in); drun &= DD_MASK

#endif

#if defined(LZO_DICT_USE_PTR)

#define LZO_CHECK_MPOS_DET(m_pos,m_off,in,ip,max_offset) \
	(m_pos == NULL || (m_off = (lzo_moff_t) (ip - m_pos)) > max_offset)

#define LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset) \
    (BOUNDS_CHECKING_OFF_IN_EXPR( \
	(PTR_LT(m_pos,in) || \
	 (m_off = (lzo_moff_t) PTR_DIFF(ip,m_pos)) <= 0 || \
	  m_off > max_offset) ))

#else

#define LZO_CHECK_MPOS_DET(m_pos,m_off,in,ip,max_offset) \
	(m_off == 0 || \
	 ((m_off = (lzo_moff_t) ((ip)-(in)) - m_off) > max_offset) || \
	 (m_pos = (ip) - (m_off), 0) )

#define LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset) \
	((lzo_moff_t) ((ip)-(in)) <= m_off || \
	 ((m_off = (lzo_moff_t) ((ip)-(in)) - m_off) > max_offset) || \
	 (m_pos = (ip) - (m_off), 0) )

#endif

#if defined(LZO_DETERMINISTIC)
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_DET
#else
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_NON_DET
#endif

#ifdef __cplusplus
}
#endif

#endif

#endif

#endif

#define DO_COMPRESS     lzo1x_1_compress

static
lzo_uint do_compress     ( const lzo_byte *in , lzo_uint  in_len,
				 lzo_byte *out, lzo_uintp out_len,
				 lzo_voidp wrkmem )
{
#if 0 && defined(__GNUC__) && defined(__i386__)
    register const lzo_byte *ip __asm__("%esi");
#else
    register const lzo_byte *ip;
#endif
    lzo_byte *op;
    const lzo_byte * const in_end = in + in_len;
    const lzo_byte * const ip_end = in + in_len - M2_MAX_LEN - 5;
    const lzo_byte *ii;
    lzo_dict_p const dict = (lzo_dict_p) wrkmem;

    op = out;
    ip = in;
    ii = ip;

    ip += 4;
    for (;;)
    {
#if 0 && defined(__GNUC__) && defined(__i386__)
	register const lzo_byte *m_pos __asm__("%edi");
#else
	register const lzo_byte *m_pos;
#endif
	lzo_moff_t m_off;
	lzo_uint m_len;
	lzo_uint dindex;

	DINDEX1(dindex,ip);
	GINDEX(m_pos,m_off,dict,dindex,in);
	if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M4_MAX_OFFSET))
	    goto literal;
#if 1
	if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
	    goto try_match;
	DINDEX2(dindex,ip);
#endif
	GINDEX(m_pos,m_off,dict,dindex,in);
	if (LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,M4_MAX_OFFSET))
	    goto literal;
	if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
	    goto try_match;
	goto literal;

try_match:
#if 1 && defined(LZO_UNALIGNED_OK_2)
	if (* (const lzo_ushortp) m_pos != * (const lzo_ushortp) ip)
#else
	if (m_pos[0] != ip[0] || m_pos[1] != ip[1])
#endif
	{
	}
	else
	{
	    if (m_pos[2] == ip[2])
	    {
#if 0
		if (m_off <= M2_MAX_OFFSET)
		    goto match;
		if (lit <= 3)
		    goto match;
		if (lit == 3)
		{
		    assert(op - 2 > out); op[-2] |= LZO_BYTE(3);
		    *op++ = *ii++; *op++ = *ii++; *op++ = *ii++;
		    goto code_match;
		}
		if (m_pos[3] == ip[3])
#endif
		    goto match;
	    }
	    else
	    {
#if 0
#if 0
		if (m_off <= M1_MAX_OFFSET && lit > 0 && lit <= 3)
#else
		if (m_off <= M1_MAX_OFFSET && lit == 3)
#endif
		{
		    register lzo_uint t;

		    t = lit;
		    assert(op - 2 > out); op[-2] |= LZO_BYTE(t);
		    do *op++ = *ii++; while (--t > 0);
		    assert(ii == ip);
		    m_off -= 1;
		    *op++ = LZO_BYTE(M1_MARKER | ((m_off & 3) << 2));
		    *op++ = LZO_BYTE(m_off >> 2);
		    ip += 2;
		    goto match_done;
		}
#endif
	    }
	}

literal:
	UPDATE_I(dict,0,dindex,ip,in);
	++ip;
	if (ip >= ip_end)
	    break;
	continue;

match:
	UPDATE_I(dict,0,dindex,ip,in);
	if (pd(ip,ii) > 0)
	{
	    register lzo_uint t = pd(ip,ii);

	    if (t <= 3)
	    {
		assert(op - 2 > out);
		op[-2] |= LZO_BYTE(t);
	    }
	    else if (t <= 18)
		*op++ = LZO_BYTE(t - 3);
	    else
	    {
		register lzo_uint tt = t - 18;

		*op++ = 0;
		while (tt > 255)
		{
		    tt -= 255;
		    *op++ = 0;
		}
		assert(tt > 0);
		*op++ = LZO_BYTE(tt);
	    }
	    do *op++ = *ii++; while (--t > 0);
	}

	assert(ii == ip);
	ip += 3;
	if (m_pos[3] != *ip++ || m_pos[4] != *ip++ || m_pos[5] != *ip++ ||
	    m_pos[6] != *ip++ || m_pos[7] != *ip++ || m_pos[8] != *ip++
#ifdef LZO1Y
	    || m_pos[ 9] != *ip++ || m_pos[10] != *ip++ || m_pos[11] != *ip++
	    || m_pos[12] != *ip++ || m_pos[13] != *ip++ || m_pos[14] != *ip++
#endif
	   )
	{
	    --ip;
	    m_len = ip - ii;
	    assert(m_len >= 3); assert(m_len <= M2_MAX_LEN);

	    if (m_off <= M2_MAX_OFFSET)
	    {
		m_off -= 1;
#if defined(LZO1X)
		*op++ = LZO_BYTE(((m_len - 1) << 5) | ((m_off & 7) << 2));
		*op++ = LZO_BYTE(m_off >> 3);
#elif defined(LZO1Y)
		*op++ = LZO_BYTE(((m_len + 1) << 4) | ((m_off & 3) << 2));
		*op++ = LZO_BYTE(m_off >> 2);
#endif
	    }
	    else if (m_off <= M3_MAX_OFFSET)
	    {
		m_off -= 1;
		*op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
		goto m3_m4_offset;
	    }
	    else
#if defined(LZO1X)
	    {
		m_off -= 0x4000;
		assert(m_off > 0); assert(m_off <= 0x7fff);
		*op++ = LZO_BYTE(M4_MARKER |
				 ((m_off & 0x4000) >> 11) | (m_len - 2));
		goto m3_m4_offset;
	    }
#elif defined(LZO1Y)
		goto m4_match;
#endif
	}
	else
	{
	    {
		const lzo_byte *end = in_end;
		const lzo_byte *m = m_pos + M2_MAX_LEN + 1;
		while (ip < end && *m == *ip)
		    m++, ip++;
		m_len = (ip - ii);
	    }
	    assert(m_len > M2_MAX_LEN);

	    if (m_off <= M3_MAX_OFFSET)
	    {
		m_off -= 1;
		if (m_len <= 33)
		    *op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
		else
		{
		    m_len -= 33;
		    *op++ = M3_MARKER | 0;
		    goto m3_m4_len;
		}
	    }
	    else
	    {
#if defined(LZO1Y)
m4_match:
#endif
		m_off -= 0x4000;
		assert(m_off > 0); assert(m_off <= 0x7fff);
		if (m_len <= M4_MAX_LEN)
		    *op++ = LZO_BYTE(M4_MARKER |
				     ((m_off & 0x4000) >> 11) | (m_len - 2));
		else
		{
		    m_len -= M4_MAX_LEN;
		    *op++ = LZO_BYTE(M4_MARKER | ((m_off & 0x4000) >> 11));
m3_m4_len:
		    while (m_len > 255)
		    {
			m_len -= 255;
			*op++ = 0;
		    }
		    assert(m_len > 0);
		    *op++ = LZO_BYTE(m_len);
		}
	    }

m3_m4_offset:
	    *op++ = LZO_BYTE((m_off & 63) << 2);
	    *op++ = LZO_BYTE(m_off >> 6);
	}

#if 0
match_done:
#endif
	ii = ip;
	if (ip >= ip_end)
	    break;
    }

    *out_len = op - out;
    return pd(in_end,ii);
}

LZO_PUBLIC(int)
DO_COMPRESS      ( const lzo_byte *in , lzo_uint  in_len,
			 lzo_byte *out, lzo_uintp out_len,
			 lzo_voidp wrkmem )
{
    lzo_byte *op = out;
    lzo_uint t;

#if defined(__LZO_QUERY_COMPRESS)
    if (__LZO_IS_COMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
	return __LZO_QUERY_COMPRESS(in,in_len,out,out_len,wrkmem,D_SIZE,lzo_sizeof(lzo_dict_t));
#endif

    if (in_len <= M2_MAX_LEN + 5)
	t = in_len;
    else
    {
	t = do_compress(in,in_len,op,out_len,wrkmem);
	op += *out_len;
    }

    if (t > 0)
    {
	const lzo_byte *ii = in + in_len - t;

	if (op == out && t <= 238)
	    *op++ = LZO_BYTE(17 + t);
	else if (t <= 3)
	    op[-2] |= LZO_BYTE(t);
	else if (t <= 18)
	    *op++ = LZO_BYTE(t - 3);
	else
	{
	    lzo_uint tt = t - 18;

	    *op++ = 0;
	    while (tt > 255)
	    {
		tt -= 255;
		*op++ = 0;
	    }
	    assert(tt > 0);
	    *op++ = LZO_BYTE(tt);
	}
	do *op++ = *ii++; while (--t > 0);
    }

    *op++ = M4_MARKER | 1;
    *op++ = 0;
    *op++ = 0;

    *out_len = op - out;
    return LZO_E_OK;
}

#undef do_compress
#undef DO_COMPRESS
#undef LZO_HASH

#undef LZO_TEST_DECOMPRESS_OVERRUN
#undef LZO_TEST_DECOMPRESS_OVERRUN_INPUT
#undef LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT
#undef LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress

#if defined(LZO_TEST_DECOMPRESS_OVERRUN)
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LOOKBEHIND
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
	    if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
	    if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#  define TEST_LOOKBEHIND(m_pos,out)    if (m_pos < out) goto lookbehind_overrun
#else
#  define TEST_LOOKBEHIND(m_pos,op)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_ptr_t)(dst),(lzo_ptr_t)(src))
#endif

#if defined(DO_DECOMPRESS)
LZO_PUBLIC(int)
DO_DECOMPRESS  ( const lzo_byte *in , lzo_uint  in_len,
		       lzo_byte *out, lzo_uintp out_len,
		       lzo_voidp wrkmem )
#endif
{
    register lzo_byte *op;
    register const lzo_byte *ip;
    register lzo_uint t;
#if defined(COPY_DICT)
    lzo_uint m_off;
    const lzo_byte *dict_end;
#else
    register const lzo_byte *m_pos;
#endif

    const lzo_byte * const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_byte * const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
    lzo_uint last_m_off = 0;
#endif

    LZO_UNUSED(wrkmem);

#if defined(__LZO_QUERY_DECOMPRESS)
    if (__LZO_IS_DECOMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
	return __LZO_QUERY_DECOMPRESS(in,in_len,out,out_len,wrkmem,0,0);
#endif

#if defined(COPY_DICT)
    if (dict)
    {
	if (dict_len > M4_MAX_OFFSET)
	{
	    dict += dict_len - M4_MAX_OFFSET;
	    dict_len = M4_MAX_OFFSET;
	}
	dict_end = dict + dict_len;
    }
    else
    {
	dict_len = 0;
	dict_end = NULL;
    }
#endif

    *out_len = 0;

    op = out;
    ip = in;

    if (*ip > 17)
    {
	t = *ip++ - 17;
	if (t < 4)
	    goto match_next;
	assert(t > 0); NEED_OP(t); NEED_IP(t+1);
	do *op++ = *ip++; while (--t > 0);
	goto first_literal_run;
    }

    while (TEST_IP && TEST_OP)
    {
	t = *ip++;
	if (t >= 16)
	    goto match;
	if (t == 0)
	{
	    NEED_IP(1);
	    while (*ip == 0)
	    {
		t += 255;
		ip++;
		NEED_IP(1);
	    }
	    t += 15 + *ip++;
	}
	assert(t > 0); NEED_OP(t+3); NEED_IP(t+4);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
	if (PTR_ALIGNED2_4(op,ip))
	{
#endif
	COPY4(op,ip);
	op += 4; ip += 4;
	if (--t > 0)
	{
	    if (t >= 4)
	    {
		do {
		    COPY4(op,ip);
		    op += 4; ip += 4; t -= 4;
		} while (t >= 4);
		if (t > 0) do *op++ = *ip++; while (--t > 0);
	    }
	    else
		do *op++ = *ip++; while (--t > 0);
	}
#if !defined(LZO_UNALIGNED_OK_4)
	}
	else
#endif
#endif
#if !defined(LZO_UNALIGNED_OK_4)
	{
	    *op++ = *ip++; *op++ = *ip++; *op++ = *ip++;
	    do *op++ = *ip++; while (--t > 0);
	}
#endif

first_literal_run:

	t = *ip++;
	if (t >= 16)
	    goto match;
#if defined(COPY_DICT)
#if defined(LZO1Z)
	m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
	last_m_off = m_off;
#else
	m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (*ip++ << 2);
#endif
	NEED_OP(3);
	t = 3; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
	t = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
	m_pos = op - t;
	last_m_off = t;
#else
	m_pos = op - (1 + M2_MAX_OFFSET);
	m_pos -= t >> 2;
	m_pos -= *ip++ << 2;
#endif
	TEST_LOOKBEHIND(m_pos,out); NEED_OP(3);
	*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
#endif
	goto match_done;

	while (TEST_IP && TEST_OP)
	{
match:
	    if (t >= 64)
	    {
#if defined(COPY_DICT)
#if defined(LZO1X)
		m_off = 1 + ((t >> 2) & 7) + (*ip++ << 3);
		t = (t >> 5) - 1;
#elif defined(LZO1Y)
		m_off = 1 + ((t >> 2) & 3) + (*ip++ << 2);
		t = (t >> 4) - 3;
#elif defined(LZO1Z)
		m_off = t & 0x1f;
		if (m_off >= 0x1c)
		    m_off = last_m_off;
		else
		{
		    m_off = 1 + (m_off << 6) + (*ip++ >> 2);
		    last_m_off = m_off;
		}
		t = (t >> 5) - 1;
#endif
#else
#if defined(LZO1X)
		m_pos = op - 1;
		m_pos -= (t >> 2) & 7;
		m_pos -= *ip++ << 3;
		t = (t >> 5) - 1;
#elif defined(LZO1Y)
		m_pos = op - 1;
		m_pos -= (t >> 2) & 3;
		m_pos -= *ip++ << 2;
		t = (t >> 4) - 3;
#elif defined(LZO1Z)
		{
		    lzo_uint off = t & 0x1f;
		    m_pos = op;
		    if (off >= 0x1c)
		    {
			assert(last_m_off > 0);
			m_pos -= last_m_off;
		    }
		    else
		    {
			off = 1 + (off << 6) + (*ip++ >> 2);
			m_pos -= off;
			last_m_off = off;
		    }
		}
		t = (t >> 5) - 1;
#endif
		TEST_LOOKBEHIND(m_pos,out); assert(t > 0); NEED_OP(t+3-1);
		goto copy_match;
#endif
	    }
	    else if (t >= 32)
	    {
		t &= 31;
		if (t == 0)
		{
		    NEED_IP(1);
		    while (*ip == 0)
		    {
			t += 255;
			ip++;
			NEED_IP(1);
		    }
		    t += 31 + *ip++;
		}
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off = 1 + (ip[0] << 6) + (ip[1] >> 2);
		last_m_off = m_off;
#else
		m_off = 1 + (ip[0] >> 2) + (ip[1] << 6);
#endif
#else
#if defined(LZO1Z)
		{
		    lzo_uint off = 1 + (ip[0] << 6) + (ip[1] >> 2);
		    m_pos = op - off;
		    last_m_off = off;
		}
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		m_pos = op - 1;
		m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
		m_pos = op - 1;
		m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
#endif
		ip += 2;
	    }
	    else if (t >= 16)
	    {
#if defined(COPY_DICT)
		m_off = (t & 8) << 11;
#else
		m_pos = op;
		m_pos -= (t & 8) << 11;
#endif
		t &= 7;
		if (t == 0)
		{
		    NEED_IP(1);
		    while (*ip == 0)
		    {
			t += 255;
			ip++;
			NEED_IP(1);
		    }
		    t += 7 + *ip++;
		}
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off += (ip[0] << 6) + (ip[1] >> 2);
#else
		m_off += (ip[0] >> 2) + (ip[1] << 6);
#endif
		ip += 2;
		if (m_off == 0)
		    goto eof_found;
		m_off += 0x4000;
#if defined(LZO1Z)
		last_m_off = m_off;
#endif
#else
#if defined(LZO1Z)
		m_pos -= (ip[0] << 6) + (ip[1] >> 2);
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
		m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
		ip += 2;
		if (m_pos == op)
		    goto eof_found;
		m_pos -= 0x4000;
#if defined(LZO1Z)
		last_m_off = op - m_pos;
#endif
#endif
	    }
	    else
	    {
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off = 1 + (t << 6) + (*ip++ >> 2);
		last_m_off = m_off;
#else
		m_off = 1 + (t >> 2) + (*ip++ << 2);
#endif
		NEED_OP(2);
		t = 2; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
		t = 1 + (t << 6) + (*ip++ >> 2);
		m_pos = op - t;
		last_m_off = t;
#else
		m_pos = op - 1;
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;
#endif
		TEST_LOOKBEHIND(m_pos,out); NEED_OP(2);
		*op++ = *m_pos++; *op++ = *m_pos;
#endif
		goto match_done;
	    }

#if defined(COPY_DICT)

	    NEED_OP(t+3-1);
	    t += 3-1; COPY_DICT(t,m_off)

#else

	    TEST_LOOKBEHIND(m_pos,out); assert(t > 0); NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
	    if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op,m_pos))
	    {
		assert((op - m_pos) >= 4);
#else
	    if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
	    {
#endif
		COPY4(op,m_pos);
		op += 4; m_pos += 4; t -= 4 - (3 - 1);
		do {
		    COPY4(op,m_pos);
		    op += 4; m_pos += 4; t -= 4;
		} while (t >= 4);
		if (t > 0) do *op++ = *m_pos++; while (--t > 0);
	    }
	    else
#endif
	    {
copy_match:
		*op++ = *m_pos++; *op++ = *m_pos++;
		do *op++ = *m_pos++; while (--t > 0);
	    }

#endif

match_done:
#if defined(LZO1Z)
	    t = ip[-1] & 3;
#else
	    t = ip[-2] & 3;
#endif
	    if (t == 0)
		break;

match_next:
	    assert(t > 0); NEED_OP(t); NEED_IP(t+1);
	    do *op++ = *ip++; while (--t > 0);
	    t = *ip++;
	}
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    *out_len = op - out;
    return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
    assert(t == 1);
    *out_len = op - out;
    return (ip == ip_end ? LZO_E_OK :
	   (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = op - out;
    return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = op - out;
    return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = op - out;
    return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}

#define LZO_TEST_DECOMPRESS_OVERRUN
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress_safe

#if defined(LZO_TEST_DECOMPRESS_OVERRUN)
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LOOKBEHIND
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
	    if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
	    if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#  define TEST_LOOKBEHIND(m_pos,out)    if (m_pos < out) goto lookbehind_overrun
#else
#  define TEST_LOOKBEHIND(m_pos,op)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_ptr_t)(dst),(lzo_ptr_t)(src))
#endif

#if defined(DO_DECOMPRESS)
LZO_PUBLIC(int)
DO_DECOMPRESS  ( const lzo_byte *in , lzo_uint  in_len,
		       lzo_byte *out, lzo_uintp out_len,
		       lzo_voidp wrkmem )
#endif
{
    register lzo_byte *op;
    register const lzo_byte *ip;
    register lzo_uint t;
#if defined(COPY_DICT)
    lzo_uint m_off;
    const lzo_byte *dict_end;
#else
    register const lzo_byte *m_pos;
#endif

    const lzo_byte * const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
    lzo_byte * const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
    lzo_uint last_m_off = 0;
#endif

    LZO_UNUSED(wrkmem);

#if defined(__LZO_QUERY_DECOMPRESS)
    if (__LZO_IS_DECOMPRESS_QUERY(in,in_len,out,out_len,wrkmem))
	return __LZO_QUERY_DECOMPRESS(in,in_len,out,out_len,wrkmem,0,0);
#endif

#if defined(COPY_DICT)
    if (dict)
    {
	if (dict_len > M4_MAX_OFFSET)
	{
	    dict += dict_len - M4_MAX_OFFSET;
	    dict_len = M4_MAX_OFFSET;
	}
	dict_end = dict + dict_len;
    }
    else
    {
	dict_len = 0;
	dict_end = NULL;
    }
#endif

    *out_len = 0;

    op = out;
    ip = in;

    if (*ip > 17)
    {
	t = *ip++ - 17;
	if (t < 4)
	    goto match_next;
	assert(t > 0); NEED_OP(t); NEED_IP(t+1);
	do *op++ = *ip++; while (--t > 0);
	goto first_literal_run;
    }

    while (TEST_IP && TEST_OP)
    {
	t = *ip++;
	if (t >= 16)
	    goto match;
	if (t == 0)
	{
	    NEED_IP(1);
	    while (*ip == 0)
	    {
		t += 255;
		ip++;
		NEED_IP(1);
	    }
	    t += 15 + *ip++;
	}
	assert(t > 0); NEED_OP(t+3); NEED_IP(t+4);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
	if (PTR_ALIGNED2_4(op,ip))
	{
#endif
	COPY4(op,ip);
	op += 4; ip += 4;
	if (--t > 0)
	{
	    if (t >= 4)
	    {
		do {
		    COPY4(op,ip);
		    op += 4; ip += 4; t -= 4;
		} while (t >= 4);
		if (t > 0) do *op++ = *ip++; while (--t > 0);
	    }
	    else
		do *op++ = *ip++; while (--t > 0);
	}
#if !defined(LZO_UNALIGNED_OK_4)
	}
	else
#endif
#endif
#if !defined(LZO_UNALIGNED_OK_4)
	{
	    *op++ = *ip++; *op++ = *ip++; *op++ = *ip++;
	    do *op++ = *ip++; while (--t > 0);
	}
#endif

first_literal_run:

	t = *ip++;
	if (t >= 16)
	    goto match;
#if defined(COPY_DICT)
#if defined(LZO1Z)
	m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
	last_m_off = m_off;
#else
	m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (*ip++ << 2);
#endif
	NEED_OP(3);
	t = 3; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
	t = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
	m_pos = op - t;
	last_m_off = t;
#else
	m_pos = op - (1 + M2_MAX_OFFSET);
	m_pos -= t >> 2;
	m_pos -= *ip++ << 2;
#endif
	TEST_LOOKBEHIND(m_pos,out); NEED_OP(3);
	*op++ = *m_pos++; *op++ = *m_pos++; *op++ = *m_pos;
#endif
	goto match_done;

	while (TEST_IP && TEST_OP)
	{
match:
	    if (t >= 64)
	    {
#if defined(COPY_DICT)
#if defined(LZO1X)
		m_off = 1 + ((t >> 2) & 7) + (*ip++ << 3);
		t = (t >> 5) - 1;
#elif defined(LZO1Y)
		m_off = 1 + ((t >> 2) & 3) + (*ip++ << 2);
		t = (t >> 4) - 3;
#elif defined(LZO1Z)
		m_off = t & 0x1f;
		if (m_off >= 0x1c)
		    m_off = last_m_off;
		else
		{
		    m_off = 1 + (m_off << 6) + (*ip++ >> 2);
		    last_m_off = m_off;
		}
		t = (t >> 5) - 1;
#endif
#else
#if defined(LZO1X)
		m_pos = op - 1;
		m_pos -= (t >> 2) & 7;
		m_pos -= *ip++ << 3;
		t = (t >> 5) - 1;
#elif defined(LZO1Y)
		m_pos = op - 1;
		m_pos -= (t >> 2) & 3;
		m_pos -= *ip++ << 2;
		t = (t >> 4) - 3;
#elif defined(LZO1Z)
		{
		    lzo_uint off = t & 0x1f;
		    m_pos = op;
		    if (off >= 0x1c)
		    {
			assert(last_m_off > 0);
			m_pos -= last_m_off;
		    }
		    else
		    {
			off = 1 + (off << 6) + (*ip++ >> 2);
			m_pos -= off;
			last_m_off = off;
		    }
		}
		t = (t >> 5) - 1;
#endif
		TEST_LOOKBEHIND(m_pos,out); assert(t > 0); NEED_OP(t+3-1);
		goto copy_match;
#endif
	    }
	    else if (t >= 32)
	    {
		t &= 31;
		if (t == 0)
		{
		    NEED_IP(1);
		    while (*ip == 0)
		    {
			t += 255;
			ip++;
			NEED_IP(1);
		    }
		    t += 31 + *ip++;
		}
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off = 1 + (ip[0] << 6) + (ip[1] >> 2);
		last_m_off = m_off;
#else
		m_off = 1 + (ip[0] >> 2) + (ip[1] << 6);
#endif
#else
#if defined(LZO1Z)
		{
		    lzo_uint off = 1 + (ip[0] << 6) + (ip[1] >> 2);
		    m_pos = op - off;
		    last_m_off = off;
		}
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		m_pos = op - 1;
		m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
		m_pos = op - 1;
		m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
#endif
		ip += 2;
	    }
	    else if (t >= 16)
	    {
#if defined(COPY_DICT)
		m_off = (t & 8) << 11;
#else
		m_pos = op;
		m_pos -= (t & 8) << 11;
#endif
		t &= 7;
		if (t == 0)
		{
		    NEED_IP(1);
		    while (*ip == 0)
		    {
			t += 255;
			ip++;
			NEED_IP(1);
		    }
		    t += 7 + *ip++;
		}
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off += (ip[0] << 6) + (ip[1] >> 2);
#else
		m_off += (ip[0] >> 2) + (ip[1] << 6);
#endif
		ip += 2;
		if (m_off == 0)
		    goto eof_found;
		m_off += 0x4000;
#if defined(LZO1Z)
		last_m_off = m_off;
#endif
#else
#if defined(LZO1Z)
		m_pos -= (ip[0] << 6) + (ip[1] >> 2);
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		m_pos -= (* (const lzo_ushortp) ip) >> 2;
#else
		m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
		ip += 2;
		if (m_pos == op)
		    goto eof_found;
		m_pos -= 0x4000;
#if defined(LZO1Z)
		last_m_off = op - m_pos;
#endif
#endif
	    }
	    else
	    {
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off = 1 + (t << 6) + (*ip++ >> 2);
		last_m_off = m_off;
#else
		m_off = 1 + (t >> 2) + (*ip++ << 2);
#endif
		NEED_OP(2);
		t = 2; COPY_DICT(t,m_off)
#else
#if defined(LZO1Z)
		t = 1 + (t << 6) + (*ip++ >> 2);
		m_pos = op - t;
		last_m_off = t;
#else
		m_pos = op - 1;
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;
#endif
		TEST_LOOKBEHIND(m_pos,out); NEED_OP(2);
		*op++ = *m_pos++; *op++ = *m_pos;
#endif
		goto match_done;
	    }

#if defined(COPY_DICT)

	    NEED_OP(t+3-1);
	    t += 3-1; COPY_DICT(t,m_off)

#else

	    TEST_LOOKBEHIND(m_pos,out); assert(t > 0); NEED_OP(t+3-1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
	    if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op,m_pos))
	    {
		assert((op - m_pos) >= 4);
#else
	    if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4)
	    {
#endif
		COPY4(op,m_pos);
		op += 4; m_pos += 4; t -= 4 - (3 - 1);
		do {
		    COPY4(op,m_pos);
		    op += 4; m_pos += 4; t -= 4;
		} while (t >= 4);
		if (t > 0) do *op++ = *m_pos++; while (--t > 0);
	    }
	    else
#endif
	    {
copy_match:
		*op++ = *m_pos++; *op++ = *m_pos++;
		do *op++ = *m_pos++; while (--t > 0);
	    }

#endif

match_done:
#if defined(LZO1Z)
	    t = ip[-1] & 3;
#else
	    t = ip[-2] & 3;
#endif
	    if (t == 0)
		break;

match_next:
	    assert(t > 0); NEED_OP(t); NEED_IP(t+1);
	    do *op++ = *ip++; while (--t > 0);
	    t = *ip++;
	}
    }

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
    *out_len = op - out;
    return LZO_E_EOF_NOT_FOUND;
#endif

eof_found:
    assert(t == 1);
    *out_len = op - out;
    return (ip == ip_end ? LZO_E_OK :
	   (ip < ip_end  ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

#if defined(HAVE_NEED_IP)
input_overrun:
    *out_len = op - out;
    return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
output_overrun:
    *out_len = op - out;
    return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
lookbehind_overrun:
    *out_len = op - out;
    return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}

/***** End of minilzo.c *****/

///////////////////////////////////////////end minilzo.c //////////////////////////////////////////////////

///////////////////////////////////////////begin ultra.c //////////////////////////////////////////////////
/*
 * ultra.c
 *
 * Routines to implement ultra based encoding (minilzo).
 * ultrazip supports packed rectangles if the rects are tiny...
 * This improves performance as lzo has more data to work with at once
 * This is 'UltraZip' and is currently not implemented.
 */

#include <rfb/rfb.h>
#include "minilzo.h"

/*
 * lzoBeforeBuf contains pixel data in the client's format.
 * lzoAfterBuf contains the lzo (deflated) encoding version.
 * If the lzo compressed/encoded version is
 * larger than the raw data or if it exceeds lzoAfterBufSize then
 * raw encoding is used instead.
 */

static int lzoBeforeBufSize = 0;
static char *lzoBeforeBuf = NULL;

static int lzoAfterBufSize = 0;
static char *lzoAfterBuf = NULL;
static int lzoAfterBufLen = 0;

/*
 * rfbSendOneRectEncodingZlib - send a given rectangle using one Zlib
 *                              rectangle encoding.
 */

#define MAX_WRKMEM ((LZO1X_1_MEM_COMPRESS) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)

void rfbUltraCleanup(rfbScreenInfoPtr screen)
{
  if (lzoBeforeBufSize) {
    free(lzoBeforeBuf);
    lzoBeforeBufSize=0;
  }
  if (lzoAfterBufSize) {
    free(lzoAfterBuf);
    lzoAfterBufSize=0;
  }
}

void rfbFreeUltraData(rfbClientPtr cl) {
  if (cl->compStreamInitedLZO) {
    free(cl->lzoWrkMem);
    cl->compStreamInitedLZO=FALSE;
  }
}


static rfbBool
rfbSendOneRectEncodingUltra(rfbClientPtr cl,
                           int x,
                           int y,
                           int w,
                           int h)
{
    rfbFramebufferUpdateRectHeader rect;
    rfbZlibHeader hdr;
    int deflateResult;
    int i;
    char *fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
    	   + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    int maxRawSize;
    int maxCompSize;

    maxRawSize = (w * h * (cl->format.bitsPerPixel / 8));

    if (lzoBeforeBufSize < maxRawSize) {
	lzoBeforeBufSize = maxRawSize;
	if (lzoBeforeBuf == NULL)
	    lzoBeforeBuf = (char *)malloc(lzoBeforeBufSize);
	else
	    lzoBeforeBuf = (char *)realloc(lzoBeforeBuf, lzoBeforeBufSize);
    }

    /*
     * lzo requires output buffer to be slightly larger than the input
     * buffer, in the worst case.
     */
    maxCompSize = (maxRawSize + maxRawSize / 16 + 64 + 3);

    if (lzoAfterBufSize < maxCompSize) {
	lzoAfterBufSize = maxCompSize;
	if (lzoAfterBuf == NULL)
	    lzoAfterBuf = (char *)malloc(lzoAfterBufSize);
	else
	    lzoAfterBuf = (char *)realloc(lzoAfterBuf, lzoAfterBufSize);
    }

    /* 
     * Convert pixel data to client format.
     */
    (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
		       &cl->format, fbptr, lzoBeforeBuf,
		       cl->scaledScreen->paddedWidthInBytes, w, h);

    if ( cl->compStreamInitedLZO == FALSE ) {
        cl->compStreamInitedLZO = TRUE;
        /* Work-memory needed for compression. Allocate memory in units
         * of `lzo_align_t' (instead of `char') to make sure it is properly aligned.
         */  
        cl->lzoWrkMem = malloc(sizeof(lzo_align_t) * (((LZO1X_1_MEM_COMPRESS) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t)));
    }

    /* Perform the compression here. */
    deflateResult = lzo1x_1_compress((unsigned char *)lzoBeforeBuf, (lzo_uint)(w * h * (cl->format.bitsPerPixel / 8)), (unsigned char *)lzoAfterBuf, (lzo_uint *)&maxCompSize, cl->lzoWrkMem);
    /* maxCompSize now contains the compressed size */

    /* Find the total size of the resulting compressed data. */
    lzoAfterBufLen = maxCompSize;

    if ( deflateResult != LZO_E_OK ) {
        rfbErr("lzo deflation error: %d\n", deflateResult);
        return FALSE;
    }

    /* Update statics */
    rfbStatRecordEncodingSent(cl, rfbEncodingUltra, sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader + lzoAfterBufLen, maxRawSize);

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader
	> UPDATE_BUF_SIZE)
    {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingUltra);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
	   sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    hdr.nBytes = Swap32IfLE(lzoAfterBufLen);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&hdr, sz_rfbZlibHeader);
    cl->ublen += sz_rfbZlibHeader;

    /* We might want to try sending the data directly... */
    for (i = 0; i < lzoAfterBufLen;) {

	int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

	if (i + bytesToCopy > lzoAfterBufLen) {
	    bytesToCopy = lzoAfterBufLen - i;
	}

	memcpy(&cl->updateBuf[cl->ublen], &lzoAfterBuf[i], bytesToCopy);

	cl->ublen += bytesToCopy;
	i += bytesToCopy;

	if (cl->ublen == UPDATE_BUF_SIZE) {
	    if (!rfbSendUpdateBuf(cl))
		return FALSE;
	}
    }

    return TRUE;

}

/*
 * rfbSendRectEncodingUltra - send a given rectangle using one or more
 *                           LZO encoding rectangles.
 */

rfbBool
rfbSendRectEncodingUltra(rfbClientPtr cl,
                        int x,
                        int y,
                        int w,
                        int h)
{
    int  maxLines;
    int  linesRemaining;
    rfbRectangle partialRect;

    partialRect.x = x;
    partialRect.y = y;
    partialRect.w = w;
    partialRect.h = h;

    /* Determine maximum pixel/scan lines allowed per rectangle. */
    maxLines = ( ULTRA_MAX_SIZE(w) / w );

    /* Initialize number of scan lines left to do. */
    linesRemaining = h;

    /* Loop until all work is done. */
    while ( linesRemaining > 0 ) {

        int linesToComp;

        if ( maxLines < linesRemaining )
            linesToComp = maxLines;
        else
            linesToComp = linesRemaining;

        partialRect.h = linesToComp;

        /* Encode (compress) and send the next rectangle. */
        if ( ! rfbSendOneRectEncodingUltra( cl,
                                           partialRect.x,
                                           partialRect.y,
                                           partialRect.w,
                                           partialRect.h )) {

            return FALSE;
        }

        /* Technically, flushing the buffer here is not extrememly
         * efficient.  However, this improves the overall throughput
         * of the system over very slow networks.  By flushing
         * the buffer with every maximum size lzo rectangle, we
         * improve the pipelining usage of the server CPU, network,
         * and viewer CPU components.  Insuring that these components
         * are working in parallel actually improves the performance
         * seen by the user.
         * Since, lzo is most useful for slow networks, this flush
         * is appropriate for the desired behavior of the lzo encoding.
         */
        if (( cl->ublen > 0 ) &&
            ( linesToComp == maxLines )) {
            if (!rfbSendUpdateBuf(cl)) {

                return FALSE;
            }
        }

        /* Update remaining and incremental rectangle location. */
        linesRemaining -= linesToComp;
        partialRect.y += linesToComp;

    }

    return TRUE;

}
///////////////////////////////////////////end ultra.c //////////////////////////////////////////////////

///////////////////////////////////////////begin scale.c //////////////////////////////////////////////////
/*
 * scale.c - deal with server-side scaling.
 */

/*
 *  Copyright (C) 2005 Rohit Kumar, Johannes E. Schindelin
 *  Copyright (C) 2002 RealVNC Ltd.
 *  OSXvnc Copyright (C) 2001 Dan McGuirk <mcguirk@incompleteness.net>.
 *  Original Xvnc code Copyright (C) 1999 AT&T Laboratories Cambridge.  
 *  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

#ifdef __STRICT_ANSI__
#define _BSD_SOURCE
#endif
#include <string.h>
#include <rfb/rfb.h>
#include <rfb/rfbregion.h>
#include "private.h"

#ifdef LIBVNCSERVER_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef WIN32
#define write(sock,buf,len) send(sock,buf,len,0)
#else
#ifdef LIBVNCSERVER_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <pwd.h>
#ifdef LIBVNCSERVER_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef LIBVNCSERVER_HAVE_NETINET_IN_H
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif
#endif

#ifdef CORBA
#include <vncserverctrl.h>
#endif

#ifdef DEBUGPROTO
#undef DEBUGPROTO
#define DEBUGPROTO(x) x
#else
#define DEBUGPROTO(x)
#endif

/****************************/
#define CEIL(x)  ( (double) ((int) (x)) == (x) ? \
        (double) ((int) (x)) : (double) ((int) (x) + 1) )
#define FLOOR(x) ( (double) ((int) (x)) )


int ScaleX(rfbScreenInfoPtr from, rfbScreenInfoPtr to, int x)
{
    if ((from==to) || (from==NULL) || (to==NULL)) return x;
    return ((int)(((double) x / (double)from->width) * (double)to->width ));
}

int ScaleY(rfbScreenInfoPtr from, rfbScreenInfoPtr to, int y)
{
    if ((from==to) || (from==NULL) || (to==NULL)) return y;
    return ((int)(((double) y / (double)from->height) * (double)to->height ));
}

/* So, all of the encodings point to the ->screen->frameBuffer,
 * We need to change this!
 */
void rfbScaledCorrection(rfbScreenInfoPtr from, rfbScreenInfoPtr to, int *x, int *y, int *w, int *h, char *function)
{
    double x1,y1,w1,h1, x2, y2, w2, h2;
    double scaleW = ((double) to->width) / ((double) from->width);
    double scaleH = ((double) to->height) / ((double) from->height);


    /*
     * rfbLog("rfbScaledCorrection(%p -> %p, %dx%d->%dx%d (%dXx%dY-%dWx%dH)\n",
     * from, to, from->width, from->height, to->width, to->height, *x, *y, *w, *h);
     */

    /* If it's the original framebuffer... */
    if (from==to) return;

    x1 = ((double) *x) * scaleW;
    y1 = ((double) *y) * scaleH;
    w1 = ((double) *w) * scaleW;
    h1 = ((double) *h) * scaleH;


    /*cast from double to int is same as "*x = floor(x1);" */
    x2 = FLOOR(x1);
    y2 = FLOOR(y1);

    /* include into W and H the jitter of scaling X and Y */
    w2 = CEIL(w1 + ( x1 - x2 ));
    h2 = CEIL(h1 + ( y1 - y2 ));

    /*
     * rfbLog("%s (%dXx%dY-%dWx%dH  ->  %fXx%fY-%fWx%fH) {%dWx%dH -> %dWx%dH}\n",
     *    function, *x, *y, *w, *h, x2, y2, w2, h2,
     *    from->width, from->height, to->width, to->height);
     */

    /* simulate ceil() without math library */
    *x = (int)x2;
    *y = (int)y2;
    *w = (int)w2;
    *h = (int)h2;

    /* Small changes for a thumbnail may be scaled to zero */
    if (*w==0) (*w)++;
    if (*h==0) (*h)++;
    /* scaling from small to big may overstep the size a bit */
    if (*x+*w > to->width)  *w=to->width - *x;
    if (*y+*h > to->height) *h=to->height - *y;
}

void rfbScaledScreenUpdateRect(rfbScreenInfoPtr screen, rfbScreenInfoPtr ptr, int x0, int y0, int w0, int h0)
{
    int x,y,w,v,z;
    int x1, y1, w1, h1;
    int bitsPerPixel, bytesPerPixel, bytesPerLine, areaX, areaY, area2;
    unsigned char *srcptr, *dstptr;

    /* Nothing to do!!! */
    if (screen==ptr) return;

    x1 = x0;
    y1 = y0;
    w1 = w0;
    h1 = h0;

    rfbScaledCorrection(screen, ptr, &x1, &y1, &w1, &h1, "rfbScaledScreenUpdateRect");
    x0 = ScaleX(ptr, screen, x1);
    y0 = ScaleY(ptr, screen, y1);
    w0 = ScaleX(ptr, screen, w1);
    h0 = ScaleY(ptr, screen, h1);

    bitsPerPixel = screen->bitsPerPixel;
    bytesPerPixel = bitsPerPixel / 8;
    bytesPerLine = w1 * bytesPerPixel;
    srcptr = (unsigned char *)(screen->frameBuffer +
     (y0 * screen->paddedWidthInBytes + x0 * bytesPerPixel));
    dstptr = (unsigned char *)(ptr->frameBuffer +
     ( y1 * ptr->paddedWidthInBytes + x1 * bytesPerPixel));
    /* The area of the source framebuffer for each destination pixel */
    areaX = ScaleX(ptr,screen,1);
    areaY = ScaleY(ptr,screen,1);
    area2 = areaX*areaY;


    /* Ensure that we do not go out of bounds */
    if ((x1+w1) > (ptr->width))
    {
      if (x1==0) w1=ptr->width; else x1 = ptr->width - w1;
    }
    if ((y1+h1) > (ptr->height))
    {
      if (y1==0) h1=ptr->height; else y1 = ptr->height - h1;
    }
    /*
     * rfbLog("rfbScaledScreenUpdateRect(%dXx%dY-%dWx%dH  ->  %dXx%dY-%dWx%dH <%dx%d>) {%dWx%dH -> %dWx%dH} 0x%p\n",
     *    x0, y0, w0, h0, x1, y1, w1, h1, areaX, areaY,
     *    screen->width, screen->height, ptr->width, ptr->height, ptr->frameBuffer);
     */

    if (screen->serverFormat.trueColour) { /* Blend neighbouring pixels together */
      unsigned char *srcptr2;
      unsigned long pixel_value, red, green, blue;
      unsigned int redShift = screen->serverFormat.redShift;
      unsigned int greenShift = screen->serverFormat.greenShift;
      unsigned int blueShift = screen->serverFormat.blueShift;
      unsigned long redMax = screen->serverFormat.redMax;
      unsigned long greenMax = screen->serverFormat.greenMax;
      unsigned long blueMax = screen->serverFormat.blueMax;

     /* for each *destination* pixel... */
     for (y = 0; y < h1; y++) {
       for (x = 0; x < w1; x++) {
         red = green = blue = 0;
         /* Get the totals for rgb from the source grid... */
         for (w = 0; w < areaX; w++) {
           for (v = 0; v < areaY; v++) {
             srcptr2 = &srcptr[(((x * areaX) + w) * bytesPerPixel) +
                               (v * screen->paddedWidthInBytes)];
             pixel_value = 0;


             switch (bytesPerPixel) {
             case 4: pixel_value = *((unsigned int *)srcptr2);   break;
             case 2: pixel_value = *((unsigned short *)srcptr2); break;
             case 1: pixel_value = *((unsigned char *)srcptr2);  break;
             default:
               /* fixme: endianess problem? */
               for (z = 0; z < bytesPerPixel; z++)
                 pixel_value += (srcptr2[z] << (8 * z));
                break;
              }
              /*
              srcptr2 += bytesPerPixel;
              */

            red += ((pixel_value >> redShift) & redMax);
            green += ((pixel_value >> greenShift) & greenMax);
            blue += ((pixel_value >> blueShift) & blueMax);

           }
         }
         /* We now have a total for all of the colors, find the average! */
         red /= area2;
         green /= area2;
         blue /= area2;
          /* Stuff the new value back into memory */
         pixel_value = ((red & redMax) << redShift) | ((green & greenMax) << greenShift) | ((blue & blueMax) << blueShift);

         switch (bytesPerPixel) {
         case 4: *((unsigned int *)dstptr)   = (unsigned int)   pixel_value; break;
         case 2: *((unsigned short *)dstptr) = (unsigned short) pixel_value; break;
         case 1: *((unsigned char *)dstptr)  = (unsigned char)  pixel_value; break;
         default:
           /* fixme: endianess problem? */
           for (z = 0; z < bytesPerPixel; z++)
             dstptr[z]=(pixel_value >> (8 * z)) & 0xff;
            break;
          }
          dstptr += bytesPerPixel;
       }
       srcptr += (screen->paddedWidthInBytes * areaY);
       dstptr += (ptr->paddedWidthInBytes - bytesPerLine);
     }
   } else
   { /* Not truecolour, so we can't blend. Just use the top-left pixel instead */
     for (y = y1; y < (y1+h1); y++) {
       for (x = x1; x < (x1+w1); x++)
         memcpy (&ptr->frameBuffer[(y *ptr->paddedWidthInBytes) + (x * bytesPerPixel)],
                 &screen->frameBuffer[(y * areaY * screen->paddedWidthInBytes) + (x *areaX * bytesPerPixel)], bytesPerPixel);
     }
  }
}

void rfbScaledScreenUpdate(rfbScreenInfoPtr screen, int x1, int y1, int x2, int y2)
{
    /* ok, now the task is to update each and every scaled version of the framebuffer
     * and we only have to do this for this specific changed rectangle!
     */
    rfbScreenInfoPtr ptr;
    int count=0;

    /* We don't point to cl->screen as it is the original */
    for (ptr=screen->scaledScreenNext;ptr!=NULL;ptr=ptr->scaledScreenNext)
    {
        /* Only update if it has active clients... */
        if (ptr->scaledScreenRefCount>0)
        {
          rfbScaledScreenUpdateRect(screen, ptr, x1, y1, x2-x1, y2-y1);
          count++;
        }
    }
}

/* Create a new scaled version of the framebuffer */
rfbScreenInfoPtr rfbScaledScreenAllocate(rfbClientPtr cl, int width, int height)
{
    rfbScreenInfoPtr ptr;
    ptr = malloc(sizeof(rfbScreenInfo));
    if (ptr!=NULL)
    {
        /* copy *everything* (we don't use most of it, but just in case) */
        memcpy(ptr, cl->screen, sizeof(rfbScreenInfo));
        ptr->width = width;
        ptr->height = height;
        ptr->paddedWidthInBytes = (ptr->bitsPerPixel/8)*ptr->width;

        /* Need to by multiples of 4 for Sparc systems */
        ptr->paddedWidthInBytes += (ptr->paddedWidthInBytes % 4);

        /* Reset the reference count to 0! */
        ptr->scaledScreenRefCount = 0;

        ptr->sizeInBytes = ptr->paddedWidthInBytes * ptr->height;
        ptr->serverFormat = cl->screen->serverFormat;

        ptr->frameBuffer = malloc(ptr->sizeInBytes);
        if (ptr->frameBuffer!=NULL)
        {
            /* Reset to a known condition: scale the entire framebuffer */
            rfbScaledScreenUpdateRect(cl->screen, ptr, 0, 0, cl->screen->width, cl->screen->height);
            /* Now, insert into the chain */
            LOCK(cl->updateMutex);
            ptr->scaledScreenNext = cl->screen->scaledScreenNext;
            cl->screen->scaledScreenNext = ptr;
            UNLOCK(cl->updateMutex);
        }
        else
        {
            /* Failed to malloc the new frameBuffer, cleanup */
            free(ptr);
            ptr=NULL;
        }
    }
    return ptr;
}

/* Find an active scaled version of the framebuffer
 * TODO: implement a refcount per scaled screen to prevent
 * unreferenced scaled screens from hanging around
 */
rfbScreenInfoPtr rfbScalingFind(rfbClientPtr cl, int width, int height)
{
    rfbScreenInfoPtr ptr;
    /* include the original in the search (ie: fine 1:1 scaled version of the frameBuffer) */
    for (ptr=cl->screen; ptr!=NULL; ptr=ptr->scaledScreenNext)
    {
        if ((ptr->width==width) && (ptr->height==height))
            return ptr;
    }
    return NULL;
}

/* Future needs "scale to 320x240, as that's the client's screen size */
void rfbScalingSetup(rfbClientPtr cl, int width, int height)
{
    rfbScreenInfoPtr ptr;

    ptr = rfbScalingFind(cl,width,height);
    if (ptr==NULL)
        ptr = rfbScaledScreenAllocate(cl,width,height);
    /* Now, there is a new screen available (if ptr is not NULL) */
    if (ptr!=NULL)
    {
        /* Update it! */
        if (ptr->scaledScreenRefCount<1)
            rfbScaledScreenUpdateRect(cl->screen, ptr, 0, 0, cl->screen->width, cl->screen->height);
        /*
         * rfbLog("Taking one from %dx%d-%d and adding it to %dx%d-%d\n",
         *    cl->scaledScreen->width, cl->scaledScreen->height,
         *    cl->scaledScreen->scaledScreenRefCount,
         *    ptr->width, ptr->height, ptr->scaledScreenRefCount);
         */

        LOCK(cl->updateMutex);
        cl->scaledScreen->scaledScreenRefCount--;
        ptr->scaledScreenRefCount++;
        cl->scaledScreen=ptr;
        cl->newFBSizePending = TRUE;
        UNLOCK(cl->updateMutex);

        rfbLog("Scaling to %dx%d (refcount=%d)\n",width,height,ptr->scaledScreenRefCount);
    }
    else
        rfbLog("Scaling to %dx%d failed, leaving things alone\n",width,height);
}

int rfbSendNewScaleSize(rfbClientPtr cl)
{
    /* if the client supports newFBsize Encoding, use it */
    if (cl->useNewFBSize && cl->newFBSizePending)
	return FALSE;

    LOCK(cl->updateMutex);
    cl->newFBSizePending = FALSE;
    UNLOCK(cl->updateMutex);

    if (cl->PalmVNC==TRUE)
    {
        rfbPalmVNCReSizeFrameBufferMsg pmsg;
        pmsg.type = rfbPalmVNCReSizeFrameBuffer;
        pmsg.pad1 = 0;
        pmsg.desktop_w = Swap16IfLE(cl->screen->width);
        pmsg.desktop_h = Swap16IfLE(cl->screen->height);
        pmsg.buffer_w  = Swap16IfLE(cl->scaledScreen->width);
        pmsg.buffer_h  = Swap16IfLE(cl->scaledScreen->height);
        pmsg.pad2 = 0;

        rfbLog("Sending a response to a PalmVNC style frameuffer resize event (%dx%d)\n", cl->scaledScreen->width, cl->scaledScreen->height);
        if (rfbWriteExact(cl, (char *)&pmsg, sz_rfbPalmVNCReSizeFrameBufferMsg) < 0) {
            rfbLogPerror("rfbNewClient: write");
            rfbCloseClient(cl);
            rfbClientConnectionGone(cl);
            return FALSE;
        }
    }
    else
    {
        rfbResizeFrameBufferMsg        rmsg;
        rmsg.type = rfbResizeFrameBuffer;
        rmsg.pad1=0;
        rmsg.framebufferWidth  = Swap16IfLE(cl->scaledScreen->width);
        rmsg.framebufferHeigth = Swap16IfLE(cl->scaledScreen->height);
        rfbLog("Sending a response to a UltraVNC style frameuffer resize event (%dx%d)\n", cl->scaledScreen->width, cl->scaledScreen->height);
        if (rfbWriteExact(cl, (char *)&rmsg, sz_rfbResizeFrameBufferMsg) < 0) {
            rfbLogPerror("rfbNewClient: write");
            rfbCloseClient(cl);
            rfbClientConnectionGone(cl);
            return FALSE;
        }
    }
    return TRUE;
}
/****************************/
///////////////////////////////////////////end scale.c //////////////////////////////////////////////////

///////////////////////////////////////////begin zlib.c //////////////////////////////////////////////////
/*
 * zlib.c
 *
 * Routines to implement zlib based encoding (deflate).
 */

/*
 *  Copyright (C) 2000 Tridia Corporation.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 *
 * For the latest source code, please check:
 *
 * http://www.developVNC.org/
 *
 * or send email to feedback@developvnc.org.
 */

#include <rfb/rfb.h>

/*
 * zlibBeforeBuf contains pixel data in the client's format.
 * zlibAfterBuf contains the zlib (deflated) encoding version.
 * If the zlib compressed/encoded version is
 * larger than the raw data or if it exceeds zlibAfterBufSize then
 * raw encoding is used instead.
 */

static int zlibBeforeBufSize = 0;
static char *zlibBeforeBuf = NULL;

static int zlibAfterBufSize = 0;
static char *zlibAfterBuf = NULL;
static int zlibAfterBufLen;

void rfbZlibCleanup(rfbScreenInfoPtr screen)
{
  if (zlibBeforeBufSize) {
    free(zlibBeforeBuf);
    zlibBeforeBufSize=0;
  }
  if (zlibAfterBufSize) {
    zlibAfterBufSize=0;
    free(zlibAfterBuf);
  }
}


/*
 * rfbSendOneRectEncodingZlib - send a given rectangle using one Zlib
 *                              rectangle encoding.
 */

static rfbBool
rfbSendOneRectEncodingZlib(rfbClientPtr cl,
                           int x,
                           int y,
                           int w,
                           int h)
{
    rfbFramebufferUpdateRectHeader rect;
    rfbZlibHeader hdr;
    int deflateResult;
    int previousOut;
    int i;
    char *fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
    	   + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    int maxRawSize;
    int maxCompSize;

    maxRawSize = (cl->scaledScreen->width * cl->scaledScreen->height
                  * (cl->format.bitsPerPixel / 8));

    if (zlibBeforeBufSize < maxRawSize) {
	zlibBeforeBufSize = maxRawSize;
	if (zlibBeforeBuf == NULL)
	    zlibBeforeBuf = (char *)malloc(zlibBeforeBufSize);
	else
	    zlibBeforeBuf = (char *)realloc(zlibBeforeBuf, zlibBeforeBufSize);
    }

    /* zlib compression is not useful for very small data sets.
     * So, we just send these raw without any compression.
     */
    if (( w * h * (cl->scaledScreen->bitsPerPixel / 8)) <
          VNC_ENCODE_ZLIB_MIN_COMP_SIZE ) {

        int result;

        /* The translation function (used also by the in raw encoding)
         * requires 4/2/1 byte alignment in the output buffer (which is
         * updateBuf for the raw encoding) based on the bitsPerPixel of
         * the viewer/client.  This prevents SIGBUS errors on some
         * architectures like SPARC, PARISC...
         */
        if (( cl->format.bitsPerPixel > 8 ) &&
            ( cl->ublen % ( cl->format.bitsPerPixel / 8 )) != 0 ) {
            if (!rfbSendUpdateBuf(cl))
                return FALSE;
        }

        result = rfbSendRectEncodingRaw(cl, x, y, w, h);

        return result;

    }

    /*
     * zlib requires output buffer to be slightly larger than the input
     * buffer, in the worst case.
     */
    maxCompSize = maxRawSize + (( maxRawSize + 99 ) / 100 ) + 12;

    if (zlibAfterBufSize < maxCompSize) {
	zlibAfterBufSize = maxCompSize;
	if (zlibAfterBuf == NULL)
	    zlibAfterBuf = (char *)malloc(zlibAfterBufSize);
	else
	    zlibAfterBuf = (char *)realloc(zlibAfterBuf, zlibAfterBufSize);
    }


    /* 
     * Convert pixel data to client format.
     */
    (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
		       &cl->format, fbptr, zlibBeforeBuf,
		       cl->scaledScreen->paddedWidthInBytes, w, h);

    cl->compStream.next_in = ( Bytef * )zlibBeforeBuf;
    cl->compStream.avail_in = w * h * (cl->format.bitsPerPixel / 8);
    cl->compStream.next_out = ( Bytef * )zlibAfterBuf;
    cl->compStream.avail_out = maxCompSize;
    cl->compStream.data_type = Z_BINARY;

    /* Initialize the deflation state. */
    if ( cl->compStreamInited == FALSE ) {

        cl->compStream.total_in = 0;
        cl->compStream.total_out = 0;
        cl->compStream.zalloc = Z_NULL;
        cl->compStream.zfree = Z_NULL;
        cl->compStream.opaque = Z_NULL;

        deflateInit2( &(cl->compStream),
                        cl->zlibCompressLevel,
                        Z_DEFLATED,
                        MAX_WBITS,
                        MAX_MEM_LEVEL,
                        Z_DEFAULT_STRATEGY );
        /* deflateInit( &(cl->compStream), Z_BEST_COMPRESSION ); */
        /* deflateInit( &(cl->compStream), Z_BEST_SPEED ); */
        cl->compStreamInited = TRUE;

    }

    previousOut = cl->compStream.total_out;

    /* Perform the compression here. */
    deflateResult = deflate( &(cl->compStream), Z_SYNC_FLUSH );

    /* Find the total size of the resulting compressed data. */
    zlibAfterBufLen = cl->compStream.total_out - previousOut;

    if ( deflateResult != Z_OK ) {
        rfbErr("zlib deflation error: %s\n", cl->compStream.msg);
        return FALSE;
    }

    /* Note that it is not possible to switch zlib parameters based on
     * the results of the compression pass.  The reason is
     * that we rely on the compressor and decompressor states being
     * in sync.  Compressing and then discarding the results would
     * cause lose of synchronization.
     */

    /* Update statics */
    rfbStatRecordEncodingSent(cl, rfbEncodingZlib, sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader + zlibAfterBufLen,
        + w * (cl->format.bitsPerPixel / 8) * h);

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbZlibHeader
	> UPDATE_BUF_SIZE)
    {
	if (!rfbSendUpdateBuf(cl))
	    return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingZlib);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
	   sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    hdr.nBytes = Swap32IfLE(zlibAfterBufLen);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&hdr, sz_rfbZlibHeader);
    cl->ublen += sz_rfbZlibHeader;

    for (i = 0; i < zlibAfterBufLen;) {

	int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

	if (i + bytesToCopy > zlibAfterBufLen) {
	    bytesToCopy = zlibAfterBufLen - i;
	}

	memcpy(&cl->updateBuf[cl->ublen], &zlibAfterBuf[i], bytesToCopy);

	cl->ublen += bytesToCopy;
	i += bytesToCopy;

	if (cl->ublen == UPDATE_BUF_SIZE) {
	    if (!rfbSendUpdateBuf(cl))
		return FALSE;
	}
    }

    return TRUE;

}


/*
 * rfbSendRectEncodingZlib - send a given rectangle using one or more
 *                           Zlib encoding rectangles.
 */

rfbBool
rfbSendRectEncodingZlib(rfbClientPtr cl,
                        int x,
                        int y,
                        int w,
                        int h)
{
    int  maxLines;
    int  linesRemaining;
    rfbRectangle partialRect;

    partialRect.x = x;
    partialRect.y = y;
    partialRect.w = w;
    partialRect.h = h;

    /* Determine maximum pixel/scan lines allowed per rectangle. */
    maxLines = ( ZLIB_MAX_SIZE(w) / w );

    /* Initialize number of scan lines left to do. */
    linesRemaining = h;

    /* Loop until all work is done. */
    while ( linesRemaining > 0 ) {

        int linesToComp;

        if ( maxLines < linesRemaining )
            linesToComp = maxLines;
        else
            linesToComp = linesRemaining;

        partialRect.h = linesToComp;

        /* Encode (compress) and send the next rectangle. */
        if ( ! rfbSendOneRectEncodingZlib( cl,
                                           partialRect.x,
                                           partialRect.y,
                                           partialRect.w,
                                           partialRect.h )) {

            return FALSE;
        }

        /* Technically, flushing the buffer here is not extrememly
         * efficient.  However, this improves the overall throughput
         * of the system over very slow networks.  By flushing
         * the buffer with every maximum size zlib rectangle, we
         * improve the pipelining usage of the server CPU, network,
         * and viewer CPU components.  Insuring that these components
         * are working in parallel actually improves the performance
         * seen by the user.
         * Since, zlib is most useful for slow networks, this flush
         * is appropriate for the desired behavior of the zlib encoding.
         */
        if (( cl->ublen > 0 ) &&
            ( linesToComp == maxLines )) {
            if (!rfbSendUpdateBuf(cl)) {

                return FALSE;
            }
        }

        /* Update remaining and incremental rectangle location. */
        linesRemaining -= linesToComp;
        partialRect.y += linesToComp;

    }

    return TRUE;

}

///////////////////////////////////////////end zlib.c //////////////////////////////////////////////////

///////////////////////////////////////////begin zrle.c //////////////////////////////////////////////////
/*
 * Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

/*
 * zrle.c
 *
 * Routines to implement Zlib Run-length Encoding (ZRLE).
 */

#include "rfb/rfb.h"
#include "private.h"
#include "zrleoutstream.h"


#define GET_IMAGE_INTO_BUF(tx,ty,tw,th,buf)                                \
{  char *fbptr = (cl->scaledScreen->frameBuffer                                   \
		 + (cl->scaledScreen->paddedWidthInBytes * ty)                   \
                 + (tx * (cl->scaledScreen->bitsPerPixel / 8)));                 \
                                                                           \
  (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,\
                     &cl->format, fbptr, (char*)buf,                       \
                     cl->scaledScreen->paddedWidthInBytes, tw, th); }

#define EXTRA_ARGS , rfbClientPtr cl

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
#define ENDIAN_NO 2
#define BPP 8
#define ZYWRLE_ENDIAN ENDIAN_NO
#include <zrleencodetemplate.c>
#undef BPP
#define BPP 15
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <zrleencodetemplate.c>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <zrleencodetemplate.c>
#undef BPP
#define BPP 16
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <zrleencodetemplate.c>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <zrleencodetemplate.c>
#undef BPP
#define BPP 32
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <zrleencodetemplate.c>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <zrleencodetemplate.c>
#define CPIXEL 24A
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <zrleencodetemplate.c>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <zrleencodetemplate.c>
#undef CPIXEL
#define CPIXEL 24B
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_LITTLE
#include <zrleencodetemplate.c>
#undef ZYWRLE_ENDIAN
#define ZYWRLE_ENDIAN ENDIAN_BIG
#include <zrleencodetemplate.c>
#undef CPIXEL
#undef BPP


/*
 * zrleBeforeBuf contains pixel data in the client's format.  It must be at
 * least one pixel bigger than the largest tile of pixel data, since the
 * ZRLE encoding algorithm writes to the position one past the end of the pixel
 * data.
 */

/* TODO: put into rfbClient struct */
static char zrleBeforeBuf[rfbZRLETileWidth * rfbZRLETileHeight * 4 + 4];



/*
 * rfbSendRectEncodingZRLE - send a given rectangle using ZRLE encoding.
 */


rfbBool rfbSendRectEncodingZRLE(rfbClientPtr cl, int x, int y, int w, int h)
{
  zrleOutStream* zos;
  rfbFramebufferUpdateRectHeader rect;
  rfbZRLEHeader hdr;
  int i;

  if (cl->preferredEncoding == rfbEncodingZYWRLE) {
	  if (cl->tightQualityLevel < 0) {
		  cl->zywrleLevel = 1;
	  } else if (cl->tightQualityLevel < 3) {
		  cl->zywrleLevel = 3;
	  } else if (cl->tightQualityLevel < 6) {
		  cl->zywrleLevel = 2;
	  } else {
		  cl->zywrleLevel = 1;
	  }
  } else
	  cl->zywrleLevel = 0;

  if (!cl->zrleData)
    cl->zrleData = zrleOutStreamNew();
  zos = cl->zrleData;
  zos->in.ptr = zos->in.start;
  zos->out.ptr = zos->out.start;

  switch (cl->format.bitsPerPixel) {

  case 8:
    zrleEncode8NE(x, y, w, h, zos, zrleBeforeBuf, cl);
    break;

  case 16:
	if (cl->format.greenMax > 0x1F) {
		if (cl->format.bigEndian)
		  zrleEncode16BE(x, y, w, h, zos, zrleBeforeBuf, cl);
		else
		  zrleEncode16LE(x, y, w, h, zos, zrleBeforeBuf, cl);
	} else {
		if (cl->format.bigEndian)
		  zrleEncode15BE(x, y, w, h, zos, zrleBeforeBuf, cl);
		else
		  zrleEncode15LE(x, y, w, h, zos, zrleBeforeBuf, cl);
	}
    break;

  case 32: {
    rfbBool fitsInLS3Bytes
      = ((cl->format.redMax   << cl->format.redShift)   < (1<<24) &&
         (cl->format.greenMax << cl->format.greenShift) < (1<<24) &&
         (cl->format.blueMax  << cl->format.blueShift)  < (1<<24));

    rfbBool fitsInMS3Bytes = (cl->format.redShift   > 7  &&
                           cl->format.greenShift > 7  &&
                           cl->format.blueShift  > 7);

    if ((fitsInLS3Bytes && !cl->format.bigEndian) ||
        (fitsInMS3Bytes && cl->format.bigEndian)) {
	if (cl->format.bigEndian)
		zrleEncode24ABE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode24ALE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
    else if ((fitsInLS3Bytes && cl->format.bigEndian) ||
             (fitsInMS3Bytes && !cl->format.bigEndian)) {
	if (cl->format.bigEndian)
		zrleEncode24BBE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode24BLE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
    else {
	if (cl->format.bigEndian)
		zrleEncode32BE(x, y, w, h, zos, zrleBeforeBuf, cl);
	else
		zrleEncode32LE(x, y, w, h, zos, zrleBeforeBuf, cl);
    }
  }
    break;
  }

  rfbStatRecordEncodingSent(cl, rfbEncodingZRLE, sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader + ZRLE_BUFFER_LENGTH(&zos->out),
      + w * (cl->format.bitsPerPixel / 8) * h);

  if (cl->ublen + sz_rfbFramebufferUpdateRectHeader + sz_rfbZRLEHeader
      > UPDATE_BUF_SIZE)
    {
      if (!rfbSendUpdateBuf(cl))
        return FALSE;
    }

  rect.r.x = Swap16IfLE(x);
  rect.r.y = Swap16IfLE(y);
  rect.r.w = Swap16IfLE(w);
  rect.r.h = Swap16IfLE(h);
  rect.encoding = Swap32IfLE(cl->preferredEncoding);

  memcpy(cl->updateBuf+cl->ublen, (char *)&rect,
         sz_rfbFramebufferUpdateRectHeader);
  cl->ublen += sz_rfbFramebufferUpdateRectHeader;

  hdr.length = Swap32IfLE(ZRLE_BUFFER_LENGTH(&zos->out));

  memcpy(cl->updateBuf+cl->ublen, (char *)&hdr, sz_rfbZRLEHeader);
  cl->ublen += sz_rfbZRLEHeader;

  /* copy into updateBuf and send from there.  Maybe should send directly? */

  for (i = 0; i < ZRLE_BUFFER_LENGTH(&zos->out);) {

    int bytesToCopy = UPDATE_BUF_SIZE - cl->ublen;

    if (i + bytesToCopy > ZRLE_BUFFER_LENGTH(&zos->out)) {
      bytesToCopy = ZRLE_BUFFER_LENGTH(&zos->out) - i;
    }

    memcpy(cl->updateBuf+cl->ublen, (uint8_t*)zos->out.start + i, bytesToCopy);

    cl->ublen += bytesToCopy;
    i += bytesToCopy;

    if (cl->ublen == UPDATE_BUF_SIZE) {
      if (!rfbSendUpdateBuf(cl))
        return FALSE;
    }
  }

  return TRUE;
}


void rfbFreeZrleData(rfbClientPtr cl)
{
  if (cl->zrleData)
    zrleOutStreamFree(cl->zrleData);
  cl->zrleData = NULL;
}

///////////////////////////////////////////end zrle.c //////////////////////////////////////////////////

///////////////////////////////////////////begin zrleoutstream.c //////////////////////////////////////////////////
/*
 * Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include "zrleoutstream.h"
#include <stdlib.h>

#define ZRLE_IN_BUFFER_SIZE  16384
#define ZRLE_OUT_BUFFER_SIZE 1024
#undef  ZRLE_DEBUG

static rfbBool zrleBufferAlloc(zrleBuffer *buffer, int size)
{
  buffer->ptr = buffer->start = malloc(size);
  if (buffer->start == NULL) {
    buffer->end = NULL;
    return FALSE;
  }

  buffer->end = buffer->start + size;

  return TRUE;
}

static void zrleBufferFree(zrleBuffer *buffer)
{
  if (buffer->start)
    free(buffer->start);
  buffer->start = buffer->ptr = buffer->end = NULL;
}

static rfbBool zrleBufferGrow(zrleBuffer *buffer, int size)
{
  int offset;

  size  += buffer->end - buffer->start;
  offset = ZRLE_BUFFER_LENGTH (buffer);

  buffer->start = realloc(buffer->start, size);
  if (!buffer->start) {
    return FALSE;
  }

  buffer->end = buffer->start + size;
  buffer->ptr = buffer->start + offset;

  return TRUE;
}

zrleOutStream *zrleOutStreamNew(void)
{
  zrleOutStream *os;

  os = malloc(sizeof(zrleOutStream));
  if (os == NULL)
    return NULL;

  if (!zrleBufferAlloc(&os->in, ZRLE_IN_BUFFER_SIZE)) {
    free(os);
    return NULL;
  }

  if (!zrleBufferAlloc(&os->out, ZRLE_OUT_BUFFER_SIZE)) {
    zrleBufferFree(&os->in);
    free(os);
    return NULL;
  }

  os->zs.zalloc = Z_NULL;
  os->zs.zfree  = Z_NULL;
  os->zs.opaque = Z_NULL;
  if (deflateInit(&os->zs, Z_DEFAULT_COMPRESSION) != Z_OK) {
    zrleBufferFree(&os->in);
    free(os);
    return NULL;
  }

  return os;
}

void zrleOutStreamFree (zrleOutStream *os)
{
  deflateEnd(&os->zs);
  zrleBufferFree(&os->in);
  zrleBufferFree(&os->out);
  free(os);
}

rfbBool zrleOutStreamFlush(zrleOutStream *os)
{
  os->zs.next_in = os->in.start;
  os->zs.avail_in = ZRLE_BUFFER_LENGTH (&os->in);
  
#ifdef ZRLE_DEBUG
  rfbLog("zrleOutStreamFlush: avail_in %d\n", os->zs.avail_in);
#endif

  while (os->zs.avail_in != 0) {
    do {
      int ret;

      if (os->out.ptr >= os->out.end &&
	  !zrleBufferGrow(&os->out, os->out.end - os->out.start)) {
	rfbLog("zrleOutStreamFlush: failed to grow output buffer\n");
	return FALSE;
      }

      os->zs.next_out = os->out.ptr;
      os->zs.avail_out = os->out.end - os->out.ptr;

#ifdef ZRLE_DEBUG
      rfbLog("zrleOutStreamFlush: calling deflate, avail_in %d, avail_out %d\n",
	     os->zs.avail_in, os->zs.avail_out);
#endif 

      if ((ret = deflate(&os->zs, Z_SYNC_FLUSH)) != Z_OK) {
	rfbLog("zrleOutStreamFlush: deflate failed with error code %d\n", ret);
	return FALSE;
      }

#ifdef ZRLE_DEBUG
      rfbLog("zrleOutStreamFlush: after deflate: %d bytes\n",
	     os->zs.next_out - os->out.ptr);
#endif

      os->out.ptr = os->zs.next_out;
    } while (os->zs.avail_out == 0);
  }

  os->in.ptr = os->in.start;
 
  return TRUE;
}

static int zrleOutStreamOverrun(zrleOutStream *os,
				int            size)
{
#ifdef ZRLE_DEBUG
  rfbLog("zrleOutStreamOverrun\n");
#endif

  while (os->in.end - os->in.ptr < size && os->in.ptr > os->in.start) {
    os->zs.next_in = os->in.start;
    os->zs.avail_in = ZRLE_BUFFER_LENGTH (&os->in);

    do {
      int ret;

      if (os->out.ptr >= os->out.end &&
	  !zrleBufferGrow(&os->out, os->out.end - os->out.start)) {
	rfbLog("zrleOutStreamOverrun: failed to grow output buffer\n");
	return FALSE;
      }

      os->zs.next_out = os->out.ptr;
      os->zs.avail_out = os->out.end - os->out.ptr;

#ifdef ZRLE_DEBUG
      rfbLog("zrleOutStreamOverrun: calling deflate, avail_in %d, avail_out %d\n",
	     os->zs.avail_in, os->zs.avail_out);
#endif

      if ((ret = deflate(&os->zs, 0)) != Z_OK) {
	rfbLog("zrleOutStreamOverrun: deflate failed with error code %d\n", ret);
	return 0;
      }

#ifdef ZRLE_DEBUG
      rfbLog("zrleOutStreamOverrun: after deflate: %d bytes\n",
	     os->zs.next_out - os->out.ptr);
#endif

      os->out.ptr = os->zs.next_out;
    } while (os->zs.avail_out == 0);

    /* output buffer not full */

    if (os->zs.avail_in == 0) {
      os->in.ptr = os->in.start;
    } else {
      /* but didn't consume all the data?  try shifting what's left to the
       * start of the buffer.
       */
      rfbLog("zrleOutStreamOverrun: out buf not full, but in data not consumed\n");
      memmove(os->in.start, os->zs.next_in, os->in.ptr - os->zs.next_in);
      os->in.ptr -= os->zs.next_in - os->in.start;
    }
  }

  if (size > os->in.end - os->in.ptr)
    size = os->in.end - os->in.ptr;

  return size;
}

static int zrleOutStreamCheck(zrleOutStream *os, int size)
{
  if (os->in.ptr + size > os->in.end) {
    return zrleOutStreamOverrun(os, size);
  }
  return size;
}

void zrleOutStreamWriteBytes(zrleOutStream *os,
			     const zrle_U8 *data,
			     int            length)
{
  const zrle_U8* dataEnd = data + length;
  while (data < dataEnd) {
    int n = zrleOutStreamCheck(os, dataEnd - data);
    memcpy(os->in.ptr, data, n);
    os->in.ptr += n;
    data += n;
  }
}

void zrleOutStreamWriteU8(zrleOutStream *os, zrle_U8 u)
{
  zrleOutStreamCheck(os, 1);
  *os->in.ptr++ = u;
}

void zrleOutStreamWriteOpaque8(zrleOutStream *os, zrle_U8 u)
{
  zrleOutStreamCheck(os, 1);
  *os->in.ptr++ = u;
}

void zrleOutStreamWriteOpaque16 (zrleOutStream *os, zrle_U16 u)
{
  zrleOutStreamCheck(os, 2);
  *os->in.ptr++ = ((zrle_U8*)&u)[0];
  *os->in.ptr++ = ((zrle_U8*)&u)[1];
}

void zrleOutStreamWriteOpaque32 (zrleOutStream *os, zrle_U32 u)
{
  zrleOutStreamCheck(os, 4);
  *os->in.ptr++ = ((zrle_U8*)&u)[0];
  *os->in.ptr++ = ((zrle_U8*)&u)[1];
  *os->in.ptr++ = ((zrle_U8*)&u)[2];
  *os->in.ptr++ = ((zrle_U8*)&u)[3];
}

void zrleOutStreamWriteOpaque24A(zrleOutStream *os, zrle_U32 u)
{
  zrleOutStreamCheck(os, 3);
  *os->in.ptr++ = ((zrle_U8*)&u)[0];
  *os->in.ptr++ = ((zrle_U8*)&u)[1];
  *os->in.ptr++ = ((zrle_U8*)&u)[2];
}

void zrleOutStreamWriteOpaque24B(zrleOutStream *os, zrle_U32 u)
{
  zrleOutStreamCheck(os, 3);
  *os->in.ptr++ = ((zrle_U8*)&u)[1];
  *os->in.ptr++ = ((zrle_U8*)&u)[2];
  *os->in.ptr++ = ((zrle_U8*)&u)[3];
}
///////////////////////////////////////////end zrleoutstream.c //////////////////////////////////////////////////

///////////////////////////////////////////begin zrlepalettehelper.c //////////////////////////////////////////////////
/*
 * Copyright (C) 2002 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#include "zrlepalettehelper.h"
#include <assert.h>
#include <string.h>

#define ZRLE_HASH(pix) (((pix) ^ ((pix) >> 17)) & 4095)

void zrlePaletteHelperInit(zrlePaletteHelper *helper)
{
  memset(helper->palette, 0, sizeof(helper->palette));
  memset(helper->index, 255, sizeof(helper->index));
  memset(helper->key, 0, sizeof(helper->key));
  helper->size = 0;
}

void zrlePaletteHelperInsert(zrlePaletteHelper *helper, zrle_U32 pix)
{
  if (helper->size < ZRLE_PALETTE_MAX_SIZE) {
    int i = ZRLE_HASH(pix);

    while (helper->index[i] != 255 && helper->key[i] != pix)
      i++;
    if (helper->index[i] != 255) return;

    helper->index[i] = helper->size;
    helper->key[i] = pix;
    helper->palette[helper->size] = pix;
  }
  helper->size++;
}

int zrlePaletteHelperLookup(zrlePaletteHelper *helper, zrle_U32 pix)
{
  int i = ZRLE_HASH(pix);

  assert(helper->size <= ZRLE_PALETTE_MAX_SIZE);
  
  while (helper->index[i] != 255 && helper->key[i] != pix)
    i++;
  if (helper->index[i] != 255) return helper->index[i];

  return -1;
}
///////////////////////////////////////////end zrlepalettehelper.c //////////////////////////////////////////////////

///////////////////////////////////////////begin zywrletemplate.c //////////////////////////////////////////////////

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE 'ZYWRLE' VNC CODEC SOURCE CODE.         *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A FOLLOWING BSD-STYLE SOURCE LICENSE.                *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE 'ZYWRLE' VNC CODEC SOURCE CODE IS (C) COPYRIGHT 2006         *
 * BY Hitachi Systems & Services, Ltd.                              *
 * (Noriaki Yamazaki, Research & Developement Center)               *                                                                 *
 *                                                                  *
 ********************************************************************
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Hitachi Systems & Services, Ltd. nor
the names of its contributors may be used to endorse or promote
products derived from this software without specific prior written
permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/

/* Change Log:
     V0.02 : 2008/02/04 : Fix mis encode/decode when width != scanline
	                     (Thanks Johannes Schindelin, author of LibVNC
						  Server/Client)
     V0.01 : 2007/02/06 : Initial release
*/

/* #define ZYWRLE_ENCODE */
/* #define ZYWRLE_DECODE */
#define ZYWRLE_QUANTIZE

/*
[References]
 PLHarr:
   Senecal, J. G., P. Lindstrom, M. A. Duchaineau, and K. I. Joy, "An Improved N-Bit to N-Bit Reversible Haar-Like Transform," Pacific Graphics 2004, October 2004, pp. 371-380.
 EZW:
   Shapiro, JM: Embedded Image Coding Using Zerotrees of Wavelet Coefficients, IEEE Trans. Signal. Process., Vol.41, pp.3445-3462 (1993).
*/


/* Template Macro stuffs. */
#undef ZYWRLE_ANALYZE
#undef ZYWRLE_SYNTHESIZE
#define ZYWRLE_ANALYZE __RFB_CONCAT3E(zywrleAnalyze,BPP,END_FIX)
#define ZYWRLE_SYNTHESIZE __RFB_CONCAT3E(zywrleSynthesize,BPP,END_FIX)

#define ZYWRLE_RGBYUV __RFB_CONCAT3E(zywrleRGBYUV,BPP,END_FIX)
#define ZYWRLE_YUVRGB __RFB_CONCAT3E(zywrleYUVRGB,BPP,END_FIX)
#define ZYWRLE_YMASK __RFB_CONCAT2E(ZYWRLE_YMASK,BPP)
#define ZYWRLE_UVMASK __RFB_CONCAT2E(ZYWRLE_UVMASK,BPP)
#define ZYWRLE_LOAD_PIXEL __RFB_CONCAT2E(ZYWRLE_LOAD_PIXEL,BPP)
#define ZYWRLE_SAVE_PIXEL __RFB_CONCAT2E(ZYWRLE_SAVE_PIXEL,BPP)

/* Packing/Unpacking pixel stuffs.
   Endian conversion stuffs. */
#undef S_0
#undef S_1
#undef L_0
#undef L_1
#undef L_2
#if ZYWRLE_ENDIAN == ENDIAN_BIG
#  define S_0	1
#  define S_1	0
#  define L_0	3
#  define L_1	2
#  define L_2	1
#else
#  define S_0	0
#  define S_1	1
#  define L_0	0
#  define L_1	1
#  define L_2	2
#endif

/*   Load/Save pixel stuffs. */
#define ZYWRLE_YMASK15  0xFFFFFFF8
#define ZYWRLE_UVMASK15 0xFFFFFFF8
#define ZYWRLE_LOAD_PIXEL15(pSrc,R,G,B) { \
	R =  (((unsigned char*)pSrc)[S_1]<< 1)& 0xF8;	\
	G = ((((unsigned char*)pSrc)[S_1]<< 6)|(((unsigned char*)pSrc)[S_0]>> 2))& 0xF8;	\
	B =  (((unsigned char*)pSrc)[S_0]<< 3)& 0xF8;	\
}
#define ZYWRLE_SAVE_PIXEL15(pDst,R,G,B) { \
	R &= 0xF8;	\
	G &= 0xF8;	\
	B &= 0xF8;	\
	((unsigned char*)pDst)[S_1] = (unsigned char)( (R>>1)|(G>>6)       );	\
	((unsigned char*)pDst)[S_0] = (unsigned char)(((B>>3)|(G<<2))& 0xFF);	\
}
#define ZYWRLE_YMASK16  0xFFFFFFFC
#define ZYWRLE_UVMASK16 0xFFFFFFF8
#define ZYWRLE_LOAD_PIXEL16(pSrc,R,G,B) { \
	R =   ((unsigned char*)pSrc)[S_1]     & 0xF8;	\
	G = ((((unsigned char*)pSrc)[S_1]<< 5)|(((unsigned char*)pSrc)[S_0]>> 3))& 0xFC;	\
	B =  (((unsigned char*)pSrc)[S_0]<< 3)& 0xF8;	\
}
#define ZYWRLE_SAVE_PIXEL16(pDst,R,G,B) { \
	R &= 0xF8;	\
	G &= 0xFC;	\
	B &= 0xF8;	\
	((unsigned char*)pDst)[S_1] = (unsigned char)(  R    |(G>>5)       );	\
	((unsigned char*)pDst)[S_0] = (unsigned char)(((B>>3)|(G<<3))& 0xFF);	\
}
#define ZYWRLE_YMASK32  0xFFFFFFFF
#define ZYWRLE_UVMASK32 0xFFFFFFFF
#define ZYWRLE_LOAD_PIXEL32(pSrc,R,G,B) { \
	R = ((unsigned char*)pSrc)[L_2];	\
	G = ((unsigned char*)pSrc)[L_1];	\
	B = ((unsigned char*)pSrc)[L_0];	\
}
#define ZYWRLE_SAVE_PIXEL32(pDst,R,G,B) { \
	((unsigned char*)pDst)[L_2] = (unsigned char)R;	\
	((unsigned char*)pDst)[L_1] = (unsigned char)G;	\
	((unsigned char*)pDst)[L_0] = (unsigned char)B;	\
}

#ifndef ZYWRLE_ONCE
#define ZYWRLE_ONCE

#ifdef WIN32
#define InlineX __inline
#else
#define InlineX inline
#endif

#ifdef ZYWRLE_ENCODE
/* Tables for Coefficients filtering. */
#  ifndef ZYWRLE_QUANTIZE
/* Type A:lower bit omitting of EZW style. */
const static unsigned int zywrleParam[3][3]={
	{0x0000F000,0x00000000,0x00000000},
	{0x0000C000,0x00F0F0F0,0x00000000},
	{0x0000C000,0x00C0C0C0,0x00F0F0F0},
/*	{0x0000FF00,0x00000000,0x00000000},
	{0x0000FF00,0x00FFFFFF,0x00000000},
	{0x0000FF00,0x00FFFFFF,0x00FFFFFF}, */
};
#  else
/* Type B:Non liner quantization filter. */
static const signed char zywrleConv[4][256]={
{	/* bi=5, bo=5 r=0.0:PSNR=24.849 */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
},
{	/* bi=5, bo=5 r=2.0:PSNR=74.031 */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 32,
	32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32,
	48, 48, 48, 48, 48, 48, 48, 48,
	48, 48, 48, 56, 56, 56, 56, 56,
	56, 56, 56, 56, 64, 64, 64, 64,
	64, 64, 64, 64, 72, 72, 72, 72,
	72, 72, 72, 72, 80, 80, 80, 80,
	80, 80, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 96, 96,
	96, 96, 96, 104, 104, 104, 104, 104,
	104, 104, 104, 104, 104, 112, 112, 112,
	112, 112, 112, 112, 112, 112, 120, 120,
	120, 120, 120, 120, 120, 120, 120, 120,
	0, -120, -120, -120, -120, -120, -120, -120,
	-120, -120, -120, -112, -112, -112, -112, -112,
	-112, -112, -112, -112, -104, -104, -104, -104,
	-104, -104, -104, -104, -104, -104, -96, -96,
	-96, -96, -96, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -80,
	-80, -80, -80, -80, -80, -72, -72, -72,
	-72, -72, -72, -72, -72, -64, -64, -64,
	-64, -64, -64, -64, -64, -56, -56, -56,
	-56, -56, -56, -56, -56, -56, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -32, -32, -32, -32, -32, -32, -32,
	-32, -32, -32, -32, -32, -32, -32, -32,
	-32, -32, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
},
{	/* bi=5, bo=4 r=2.0:PSNR=64.441 */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	48, 48, 48, 48, 48, 48, 48, 48,
	48, 48, 48, 48, 48, 48, 48, 48,
	48, 48, 48, 48, 48, 48, 48, 48,
	64, 64, 64, 64, 64, 64, 64, 64,
	64, 64, 64, 64, 64, 64, 64, 64,
	80, 80, 80, 80, 80, 80, 80, 80,
	80, 80, 80, 80, 80, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	104, 104, 104, 104, 104, 104, 104, 104,
	104, 104, 104, 112, 112, 112, 112, 112,
	112, 112, 112, 112, 120, 120, 120, 120,
	120, 120, 120, 120, 120, 120, 120, 120,
	0, -120, -120, -120, -120, -120, -120, -120,
	-120, -120, -120, -120, -120, -112, -112, -112,
	-112, -112, -112, -112, -112, -112, -104, -104,
	-104, -104, -104, -104, -104, -104, -104, -104,
	-104, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -80, -80, -80, -80,
	-80, -80, -80, -80, -80, -80, -80, -80,
	-80, -64, -64, -64, -64, -64, -64, -64,
	-64, -64, -64, -64, -64, -64, -64, -64,
	-64, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48,
	-48, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
},
{	/* bi=5, bo=2 r=2.0:PSNR=43.175 */
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	88, 88, 88, 88, 88, 88, 88, 88,
	0, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, -88, -88, -88, -88, -88, -88, -88,
	-88, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
}
};
const static signed char* zywrleParam[3][3][3]={
	{{zywrleConv[0],zywrleConv[2],zywrleConv[0]},{zywrleConv[0],zywrleConv[0],zywrleConv[0]},{zywrleConv[0],zywrleConv[0],zywrleConv[0]}},
	{{zywrleConv[0],zywrleConv[3],zywrleConv[0]},{zywrleConv[1],zywrleConv[1],zywrleConv[1]},{zywrleConv[0],zywrleConv[0],zywrleConv[0]}},
	{{zywrleConv[0],zywrleConv[3],zywrleConv[0]},{zywrleConv[2],zywrleConv[2],zywrleConv[2]},{zywrleConv[1],zywrleConv[1],zywrleConv[1]}},
};
#  endif
#endif

static InlineX void Harr(signed char* pX0, signed char* pX1)
{
	/* Piecewise-Linear Harr(PLHarr) */
	int X0 = (int)*pX0, X1 = (int)*pX1;
	int orgX0 = X0, orgX1 = X1;
	if ((X0 ^ X1) & 0x80) {
		/* differ sign */
		X1 += X0;
		if (((X1^orgX1)&0x80)==0) {
			/* |X1| > |X0| */
			X0 -= X1;	/* H = -B */
		}
	} else {
		/* same sign */
		X0 -= X1;
		if (((X0 ^ orgX0) & 0x80) == 0) {
			/* |X0| > |X1| */
			X1 += X0;	/* L = A */
		}
	}
	*pX0 = (signed char)X1;
	*pX1 = (signed char)X0;
}
/*
 1D-Wavelet transform.

 In coefficients array, the famous 'pyramid' decomposition is well used.

 1D Model:
   |L0L0L0L0|L0L0L0L0|H0H0H0H0|H0H0H0H0| : level 0
   |L1L1L1L1|H1H1H1H1|H0H0H0H0|H0H0H0H0| : level 1

 But this method needs line buffer because H/L is different position from X0/X1.
 So, I used 'interleave' decomposition instead of it.

 1D Model:
   |L0H0L0H0|L0H0L0H0|L0H0L0H0|L0H0L0H0| : level 0
   |L1H0H1H0|L1H0H1H0|L1H0H1H0|L1H0H1H0| : level 1

 In this method, H/L and X0/X1 is always same position.
 This lead us to more speed and less memory.
 Of cause, the result of both method is quite same
 because it's only difference that coefficient position.
*/
static InlineX void WaveletLevel(int* data, int size, int l, int SkipPixel)
{
	int s, ofs;
	signed char* pX0;
	signed char* end;

	pX0 = (signed char*)data;
	s = (8<<l)*SkipPixel;
	end = pX0+(size>>(l+1))*s;
	s -= 2;
	ofs = (4<<l)*SkipPixel;
	while (pX0 < end) {
		Harr(pX0, pX0+ofs);
		pX0++;
		Harr(pX0, pX0+ofs);
		pX0++;
		Harr(pX0, pX0+ofs);
		pX0 += s;
	}
}
#define InvWaveletLevel(d,s,l,pix) WaveletLevel(d,s,l,pix)

#ifdef ZYWRLE_ENCODE
#  ifndef ZYWRLE_QUANTIZE
/* Type A:lower bit omitting of EZW style. */
static InlineX void FilterWaveletSquare(int* pBuf, int width, int height, int level, int l)
{
	int r, s;
	int x, y;
	int* pH;
	const unsigned int* pM;

	pM = &(zywrleParam[level-1][l]);
	s = 2<<l;
	for (r = 1; r < 4; r++) {
		pH   = pBuf;
		if (r & 0x01)
			pH +=  s>>1;
		if (r & 0x02)
			pH += (s>>1)*width;
		for (y = 0; y < height / s; y++) {
			for (x = 0; x < width / s; x++) {
				/*
				 these are same following code.
				     pH[x] = pH[x] / (~pM[x]+1) * (~pM[x]+1);
				     ( round pH[x] with pM[x] bit )
				 '&' operator isn't 'round' but is 'floor'.
				 So, we must offset when pH[x] is negative.
				*/
				if (((signed char*)pH)[0] & 0x80)
					((signed char*)pH)[0] += ~((signed char*)pM)[0];
				if (((signed char*)pH)[1] & 0x80)
					((signed char*)pH)[1] += ~((signed char*)pM)[1];
				if (((signed char*)pH)[2] & 0x80)
					((signed char*)pH)[2] += ~((signed char*)pM)[2];
				*pH &= *pM;
				pH += s;
			}
			pH += (s-1)*width;
		}
	}
}
#  else
/*
 Type B:Non liner quantization filter.

 Coefficients have Gaussian curve and smaller value which is
 large part of coefficients isn't more important than larger value.
 So, I use filter of Non liner quantize/dequantize table.
 In general, Non liner quantize formula is explained as following.

    y=f(x)   = sign(x)*round( ((abs(x)/(2^7))^ r   )* 2^(bo-1) )*2^(8-bo)
    x=f-1(y) = sign(y)*round( ((abs(y)/(2^7))^(1/r))* 2^(bi-1) )*2^(8-bi)
 ( r:power coefficient  bi:effective MSB in input  bo:effective MSB in output )

   r < 1.0 : Smaller value is more important than larger value.
   r > 1.0 : Larger value is more important than smaller value.
   r = 1.0 : Liner quantization which is same with EZW style.

 r = 0.75 is famous non liner quantization used in MP3 audio codec.
 In contrast to audio data, larger value is important in wavelet coefficients.
 So, I select r = 2.0 table( quantize is x^2, dequantize sqrt(x) ).

 As compared with EZW style liner quantization, this filter tended to be
 more sharp edge and be more compression rate but be more blocking noise and be less quality.
 Especially, the surface of graphic objects has distinguishable noise in middle quality mode.

 We need only quantized-dequantized(filtered) value rather than quantized value itself
 because all values are packed or palette-lized in later ZRLE section.
 This lead us not to need to modify client decoder when we change
 the filtering procedure in future.
 Client only decodes coefficients given by encoder.
*/
static InlineX void FilterWaveletSquare(int* pBuf, int width, int height, int level, int l)
{
	int r, s;
	int x, y;
	int* pH;
	const signed char** pM;

	pM = zywrleParam[level-1][l];
	s = 2<<l;
	for (r = 1; r < 4; r++) {
		pH   = pBuf;
		if (r & 0x01)
			pH +=  s>>1;
		if (r & 0x02)
			pH += (s>>1)*width;
		for (y = 0; y < height / s; y++) {
			for (x = 0; x < width / s; x++) {
				((signed char*)pH)[0] = pM[0][((unsigned char*)pH)[0]];
				((signed char*)pH)[1] = pM[1][((unsigned char*)pH)[1]];
				((signed char*)pH)[2] = pM[2][((unsigned char*)pH)[2]];
				pH += s;
			}
			pH += (s-1)*width;
		}
	}
}
#  endif

static InlineX void Wavelet(int* pBuf, int width, int height, int level)
{
	int l, s;
	int* pTop;
	int* pEnd;

	for (l = 0; l < level; l++) {
		pTop = pBuf;
		pEnd = pBuf+height*width;
		s = width<<l;
		while (pTop < pEnd) {
			WaveletLevel(pTop, width, l, 1);
			pTop += s;
		}
		pTop = pBuf;
		pEnd = pBuf+width;
		s = 1<<l;
		while (pTop < pEnd) {
			WaveletLevel(pTop, height,l, width);
			pTop += s;
		}
		FilterWaveletSquare(pBuf, width, height, level, l);
	}
}
#endif
#ifdef ZYWRLE_DECODE
static InlineX void InvWavelet(int* pBuf, int width, int height, int level)
{
	int l, s;
	int* pTop;
	int* pEnd;

	for (l = level - 1; l >= 0; l--) {
		pTop = pBuf;
		pEnd = pBuf+width;
		s = 1<<l;
		while (pTop < pEnd) {
			InvWaveletLevel(pTop, height,l, width);
			pTop += s;
		}
		pTop = pBuf;
		pEnd = pBuf+height*width;
		s = width<<l;
		while (pTop < pEnd) {
			InvWaveletLevel(pTop, width, l, 1);
			pTop += s;
		}
	}
}
#endif

/* Load/Save coefficients stuffs.
 Coefficients manages as 24 bits little-endian pixel. */
#define ZYWRLE_LOAD_COEFF(pSrc,R,G,B) { \
	R = ((signed char*)pSrc)[2];	\
	G = ((signed char*)pSrc)[1];	\
	B = ((signed char*)pSrc)[0];	\
}
#define ZYWRLE_SAVE_COEFF(pDst,R,G,B) { \
	((signed char*)pDst)[2] = (signed char)R;	\
	((signed char*)pDst)[1] = (signed char)G;	\
	((signed char*)pDst)[0] = (signed char)B;	\
}

/*
 RGB <=> YUV conversion stuffs.
 YUV coversion is explained as following formula in strict meaning:
   Y =  0.299R + 0.587G + 0.114B (   0<=Y<=255)
   U = -0.169R - 0.331G + 0.500B (-128<=U<=127)
   V =  0.500R - 0.419G - 0.081B (-128<=V<=127)

 I use simple conversion RCT(reversible color transform) which is described
 in JPEG-2000 specification.
   Y = (R + 2G + B)/4 (   0<=Y<=255)
   U = B-G (-256<=U<=255)
   V = R-G (-256<=V<=255)
*/
#define ROUND(x) (((x)<0)?0:(((x)>255)?255:(x)))
	/* RCT is N-bit RGB to N-bit Y and N+1-bit UV.
	 For make Same N-bit, UV is lossy.
	 More exact PLHarr, we reduce to odd range(-127<=x<=127). */
#define ZYWRLE_RGBYUV1(R,G,B,Y,U,V,ymask,uvmask) { \
	Y = (R+(G<<1)+B)>>2;	\
	U =  B-G;	\
	V =  R-G;	\
	Y -= 128;	\
	U >>= 1;	\
	V >>= 1;	\
	Y &= ymask;	\
	U &= uvmask;	\
	V &= uvmask;	\
	if (Y == -128)	\
		Y += (0xFFFFFFFF-ymask+1);	\
	if (U == -128)	\
		U += (0xFFFFFFFF-uvmask+1);	\
	if (V == -128)	\
		V += (0xFFFFFFFF-uvmask+1);	\
}
#define ZYWRLE_YUVRGB1(R,G,B,Y,U,V) { \
	Y += 128;	\
	U <<= 1;	\
	V <<= 1;	\
	G = Y-((U+V)>>2);	\
	B = U+G;	\
	R = V+G;	\
	G = ROUND(G);	\
	B = ROUND(B);	\
	R = ROUND(R);	\
}

/*
 coefficient packing/unpacking stuffs.
 Wavelet transform makes 4 sub coefficient image from 1 original image.

 model with pyramid decomposition:
   +------+------+
   |      |      |
   |  L   |  Hx  |
   |      |      |
   +------+------+
   |      |      |
   |  H   |  Hxy |
   |      |      |
   +------+------+

 So, we must transfer each sub images individually in strict meaning.
 But at least ZRLE meaning, following one decompositon image is same as
 avobe individual sub image. I use this format.
 (Strictly saying, transfer order is reverse(Hxy->Hy->Hx->L)
  for simplified procedure for any wavelet level.)

   +------+------+
   |      L      |
   +------+------+
   |      Hx     |
   +------+------+
   |      Hy     |
   +------+------+
   |      Hxy    |
   +------+------+
*/
#define INC_PTR(data) \
	data++;	\
	if( data-pData >= (w+uw) ){	\
		data += scanline-(w+uw);	\
		pData = data;	\
	}

#define ZYWRLE_TRANSFER_COEFF(pBuf,data,r,w,h,scanline,level,TRANS)	\
	pH = pBuf;	\
	s = 2<<level;	\
	if (r & 0x01)	\
		pH +=  s>>1;	\
	if (r & 0x02)	\
		pH += (s>>1)*w;	\
	pEnd = pH+h*w;	\
	while (pH < pEnd) {	\
		pLine = pH+w;	\
		while (pH < pLine) {	\
			TRANS	\
			INC_PTR(data)	\
			pH += s;	\
		}	\
		pH += (s-1)*w;	\
	}

#define ZYWRLE_PACK_COEFF(pBuf,data,r,width,height,scanline,level)	\
	ZYWRLE_TRANSFER_COEFF(pBuf,data,r,width,height,scanline,level,ZYWRLE_LOAD_COEFF(pH,R,G,B);ZYWRLE_SAVE_PIXEL(data,R,G,B);)

#define ZYWRLE_UNPACK_COEFF(pBuf,data,r,width,height,scanline,level)	\
	ZYWRLE_TRANSFER_COEFF(pBuf,data,r,width,height,scanline,level,ZYWRLE_LOAD_PIXEL(data,R,G,B);ZYWRLE_SAVE_COEFF(pH,R,G,B);)

#define ZYWRLE_SAVE_UNALIGN(data,TRANS)	\
	pTop = pBuf+w*h;	\
	pEnd = pBuf + (w+uw)*(h+uh);	\
	while (pTop < pEnd) {	\
		TRANS	\
		INC_PTR(data)	\
		pTop++;	\
	}

#define ZYWRLE_LOAD_UNALIGN(data,TRANS)	\
	pTop = pBuf+w*h;	\
	if (uw) {	\
		pData=         data + w;	\
		pEnd = (int*)(pData+ h*scanline);	\
		while (pData < (PIXEL_T*)pEnd) {	\
			pLine = (int*)(pData + uw);	\
			while (pData < (PIXEL_T*)pLine) {	\
				TRANS	\
				pData++;	\
				pTop++;	\
			}	\
			pData += scanline-uw;	\
		}	\
	}	\
	if (uh) {	\
		pData=         data +  h*scanline;	\
		pEnd = (int*)(pData+ uh*scanline);	\
		while (pData < (PIXEL_T*)pEnd) {	\
			pLine = (int*)(pData + w);	\
			while (pData < (PIXEL_T*)pLine) {	\
				TRANS	\
				pData++;	\
				pTop++;	\
			}	\
			pData += scanline-w;	\
		}	\
	}	\
	if (uw && uh) {	\
		pData=         data + w+ h*scanline;	\
		pEnd = (int*)(pData+   uh*scanline);	\
		while (pData < (PIXEL_T*)pEnd) {	\
			pLine = (int*)(pData + uw);	\
			while (pData < (PIXEL_T*)pLine) {	\
				TRANS	\
				pData++;	\
				pTop++;	\
			}	\
			pData += scanline-uw;	\
		}	\
	}

static InlineX void zywrleCalcSize(int* pW, int* pH, int level)
{
	*pW &= ~((1<<level)-1);
	*pH &= ~((1<<level)-1);
}

#endif /* ZYWRLE_ONCE */

#ifndef CPIXEL
#ifdef ZYWRLE_ENCODE
static InlineX void ZYWRLE_RGBYUV(int* pBuf, PIXEL_T* data, int width, int height, int scanline)
{
	int R, G, B;
	int Y, U, V;
	int* pLine;
	int* pEnd;
	pEnd = pBuf+height*width;
	while (pBuf < pEnd) {
		pLine = pBuf+width;
		while (pBuf < pLine) {
			ZYWRLE_LOAD_PIXEL(data,R,G,B);
			ZYWRLE_RGBYUV1(R,G,B,Y,U,V,ZYWRLE_YMASK,ZYWRLE_UVMASK);
			ZYWRLE_SAVE_COEFF(pBuf,V,Y,U);
			pBuf++;
			data++;
		}
		data += scanline-width;
	}
}
#endif
#ifdef ZYWRLE_DECODE
static InlineX void ZYWRLE_YUVRGB(int* pBuf, PIXEL_T* data, int width, int height, int scanline) {
	int R, G, B;
	int Y, U, V;
	int* pLine;
	int* pEnd;
	pEnd = pBuf+height*width;
	while (pBuf < pEnd) {
		pLine = pBuf+width;
		while (pBuf < pLine) {
			ZYWRLE_LOAD_COEFF(pBuf,V,Y,U);
			ZYWRLE_YUVRGB1(R,G,B,Y,U,V);
			ZYWRLE_SAVE_PIXEL(data,R,G,B);
			pBuf++;
			data++;
		}
		data += scanline-width;
	}
}
#endif

#ifdef ZYWRLE_ENCODE
PIXEL_T* ZYWRLE_ANALYZE(PIXEL_T* dst, PIXEL_T* src, int w, int h, int scanline, int level, int* pBuf) {
	int l;
	int uw = w;
	int uh = h;
	int* pTop;
	int* pEnd;
	int* pLine;
	PIXEL_T* pData;
	int R, G, B;
	int s;
	int* pH;

	zywrleCalcSize(&w, &h, level);
	if (w == 0 || h == 0)
		return NULL;
	uw -= w;
	uh -= h;

	pData = dst;
	ZYWRLE_LOAD_UNALIGN(src,*(PIXEL_T*)pTop=*pData;)
	ZYWRLE_RGBYUV(pBuf, src, w, h, scanline);
	Wavelet(pBuf, w, h, level);
	for (l = 0; l < level; l++) {
		ZYWRLE_PACK_COEFF(pBuf, dst, 3, w, h, scanline, l);
		ZYWRLE_PACK_COEFF(pBuf, dst, 2, w, h, scanline, l);
		ZYWRLE_PACK_COEFF(pBuf, dst, 1, w, h, scanline, l);
		if (l == level - 1) {
			ZYWRLE_PACK_COEFF(pBuf, dst, 0, w, h, scanline, l);
		}
	}
	ZYWRLE_SAVE_UNALIGN(dst,*dst=*(PIXEL_T*)pTop;)
	return dst;
}
#endif
#ifdef ZYWRLE_DECODE
PIXEL_T* ZYWRLE_SYNTHESIZE(PIXEL_T* dst, PIXEL_T* src, int w, int h, int scanline, int level, int* pBuf)
{
	int l;
	int uw = w;
	int uh = h;
	int* pTop;
	int* pEnd;
	int* pLine;
	PIXEL_T* pData;
	int R, G, B;
	int s;
	int* pH;

	zywrleCalcSize(&w, &h, level);
	if (w == 0 || h == 0)
		return NULL;
	uw -= w;
	uh -= h;

	pData = src;
	for (l = 0; l < level; l++) {
		ZYWRLE_UNPACK_COEFF(pBuf, src, 3, w, h, scanline, l);
		ZYWRLE_UNPACK_COEFF(pBuf, src, 2, w, h, scanline, l);
		ZYWRLE_UNPACK_COEFF(pBuf, src, 1, w, h, scanline, l);
		if (l == level - 1) {
			ZYWRLE_UNPACK_COEFF(pBuf, src, 0, w, h, scanline, l);
		}
	}
	ZYWRLE_SAVE_UNALIGN(src,*(PIXEL_T*)pTop=*src;)
	InvWavelet(pBuf, w, h, level);
	ZYWRLE_YUVRGB(pBuf, dst, w, h, scanline);
	ZYWRLE_LOAD_UNALIGN(dst,*pData=*(PIXEL_T*)pTop;)
	return src;
}
#endif
#endif  /* CPIXEL */

#undef ZYWRLE_RGBYUV
#undef ZYWRLE_YUVRGB
#undef ZYWRLE_LOAD_PIXEL
#undef ZYWRLE_SAVE_PIXEL
///////////////////////////////////////////end zywrletemplate.c //////////////////////////////////////////////////

///////////////////////////////////////////begin tight.c //////////////////////////////////////////////////
/*
 * tight.c
 *
 * Routines to implement Tight Encoding
 */

/*
 *  Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*#include <stdio.h>*/
#include <rfb/rfb.h>
#include "private.h"

#ifdef WIN32
#define XMD_H
#undef FAR
#define NEEDFAR_POINTERS
#endif

#include <jpeglib.h>

/* Note: The following constant should not be changed. */
#define TIGHT_MIN_TO_COMPRESS 12

/* The parameters below may be adjusted. */
#define MIN_SPLIT_RECT_SIZE     4096
#define MIN_SOLID_SUBRECT_SIZE  2048
#define MAX_SPLIT_TILE_SIZE       16

/* May be set to TRUE with "-lazytight" Xvnc option. */
rfbBool rfbTightDisableGradient = FALSE;

/* This variable is set on every rfbSendRectEncodingTight() call. */
static rfbBool usePixelFormat24;


/* Compression level stuff. The following array contains various
   encoder parameters for each of 10 compression levels (0..9).
   Last three parameters correspond to JPEG quality levels (0..9). */

typedef struct TIGHT_CONF_s {
    int maxRectSize, maxRectWidth;
    int monoMinRectSize, gradientMinRectSize;
    int idxZlibLevel, monoZlibLevel, rawZlibLevel, gradientZlibLevel;
    int gradientThreshold, gradientThreshold24;
    int idxMaxColorsDivisor;
    int jpegQuality, jpegThreshold, jpegThreshold24;
} TIGHT_CONF;

static TIGHT_CONF tightConf[10] = {
    {   512,   32,   6, 65536, 0, 0, 0, 0,   0,   0,   4,  5, 10000, 23000 },
    {  2048,  128,   6, 65536, 1, 1, 1, 0,   0,   0,   8, 10,  8000, 18000 },
    {  6144,  256,   8, 65536, 3, 3, 2, 0,   0,   0,  24, 15,  6500, 15000 },
    { 10240, 1024,  12, 65536, 5, 5, 3, 0,   0,   0,  32, 25,  5000, 12000 },
    { 16384, 2048,  12, 65536, 6, 6, 4, 0,   0,   0,  32, 37,  4000, 10000 },
    { 32768, 2048,  12,  4096, 7, 7, 5, 4, 150, 380,  32, 50,  3000,  8000 },
    { 65536, 2048,  16,  4096, 7, 7, 6, 4, 170, 420,  48, 60,  2000,  5000 },
    { 65536, 2048,  16,  4096, 8, 8, 7, 5, 180, 450,  64, 70,  1000,  2500 },
    { 65536, 2048,  32,  8192, 9, 9, 8, 6, 190, 475,  64, 75,   500,  1200 },
    { 65536, 2048,  32,  8192, 9, 9, 9, 6, 200, 500,  96, 80,   200,   500 }
};

static int compressLevel;
static int qualityLevel;

/* Stuff dealing with palettes. */

typedef struct COLOR_LIST_s {
    struct COLOR_LIST_s *next;
    int idx;
    uint32_t rgb;
} COLOR_LIST;

typedef struct PALETTE_ENTRY_s {
    COLOR_LIST *listNode;
    int numPixels;
} PALETTE_ENTRY;

typedef struct PALETTE_s {
    PALETTE_ENTRY entry[256];
    COLOR_LIST *hash[256];
    COLOR_LIST list[256];
} PALETTE;

/* TODO: move into rfbScreen struct */
static int paletteNumColors, paletteMaxColors;
static uint32_t monoBackground, monoForeground;
static PALETTE palette;

/* Pointers to dynamically-allocated buffers. */

static int tightBeforeBufSize = 0;
static char *tightBeforeBuf = NULL;

static int tightAfterBufSize = 0;
static char *tightAfterBuf = NULL;

static int *prevRowBuf = NULL;

void rfbTightCleanup(rfbScreenInfoPtr screen)
{
  if(tightBeforeBufSize) {
    free(tightBeforeBuf);
    tightBeforeBufSize=0;
  }
  if(tightAfterBufSize) {
    free(tightAfterBuf);
    tightAfterBufSize=0;
  }
}

/* Prototypes for static functions. */

static void FindBestSolidArea (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t colorValue, int *w_ptr, int *h_ptr);
static void ExtendSolidArea   (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t colorValue,
                               int *x_ptr, int *y_ptr, int *w_ptr, int *h_ptr);
static rfbBool CheckSolidTile    (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile8   (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile16  (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t *colorPtr, rfbBool needSameColor);
static rfbBool CheckSolidTile32  (rfbClientPtr cl, int x, int y, int w, int h,
                               uint32_t *colorPtr, rfbBool needSameColor);

static rfbBool SendRectSimple    (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendSubrect       (rfbClientPtr cl, int x, int y, int w, int h);
static rfbBool SendTightHeader   (rfbClientPtr cl, int x, int y, int w, int h);

static rfbBool SendSolidRect     (rfbClientPtr cl);
static rfbBool SendMonoRect      (rfbClientPtr cl, int w, int h);
static rfbBool SendIndexedRect   (rfbClientPtr cl, int w, int h);
static rfbBool SendFullColorRect (rfbClientPtr cl, int w, int h);
static rfbBool SendGradientRect  (rfbClientPtr cl, int w, int h);

static rfbBool CompressData(rfbClientPtr cl, int streamId, int dataLen,
                         int zlibLevel, int zlibStrategy);
static rfbBool SendCompressedData(rfbClientPtr cl, int compressedLen);

static void FillPalette8(int count);
static void FillPalette16(int count);
static void FillPalette32(int count);

static void PaletteReset(void);
static int PaletteInsert(uint32_t rgb, int numPixels, int bpp);

static void Pack24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int count);

static void EncodeIndexedRect16(uint8_t *buf, int count);
static void EncodeIndexedRect32(uint8_t *buf, int count);

static void EncodeMonoRect8(uint8_t *buf, int w, int h);
static void EncodeMonoRect16(uint8_t *buf, int w, int h);
static void EncodeMonoRect32(uint8_t *buf, int w, int h);

static void FilterGradient24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int w, int h);
static void FilterGradient16(rfbClientPtr cl, uint16_t *buf, rfbPixelFormat *fmt, int w, int h);
static void FilterGradient32(rfbClientPtr cl, uint32_t *buf, rfbPixelFormat *fmt, int w, int h);

static int DetectSmoothImage(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage24(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage16(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);
static unsigned long DetectSmoothImage32(rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h);

static rfbBool SendJpegRect(rfbClientPtr cl, int x, int y, int w, int h,
                         int quality);
static void PrepareRowForJpeg(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg24(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg16(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);
static void PrepareRowForJpeg32(rfbClientPtr cl, uint8_t *dst, int x, int y, int count);

static void JpegInitDestination(j_compress_ptr cinfo);
static boolean JpegEmptyOutputBuffer(j_compress_ptr cinfo);
static void JpegTermDestination(j_compress_ptr cinfo);
static void JpegSetDstManager(j_compress_ptr cinfo);


/*
 * Tight encoding implementation.
 */

int
rfbNumCodedRectsTight(rfbClientPtr cl,
                      int x,
                      int y,
                      int w,
                      int h)
{
    int maxRectSize, maxRectWidth;
    int subrectMaxWidth, subrectMaxHeight;

    /* No matter how many rectangles we will send if LastRect markers
       are used to terminate rectangle stream. */
    if (cl->enableLastRectEncoding && w * h >= MIN_SPLIT_RECT_SIZE)
      return 0;

    maxRectSize = tightConf[cl->tightCompressLevel].maxRectSize;
    maxRectWidth = tightConf[cl->tightCompressLevel].maxRectWidth;

    if (w > maxRectWidth || w * h > maxRectSize) {
        subrectMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        subrectMaxHeight = maxRectSize / subrectMaxWidth;
        return (((w - 1) / maxRectWidth + 1) *
                ((h - 1) / subrectMaxHeight + 1));
    } else {
        return 1;
    }
}

rfbBool
rfbSendRectEncodingTight(rfbClientPtr cl,
                         int x,
                         int y,
                         int w,
                         int h)
{
    int nMaxRows;
    uint32_t colorValue;
    int dx, dy, dw, dh;
    int x_best, y_best, w_best, h_best;
    char *fbptr;

    rfbSendUpdateBuf(cl);

    compressLevel = cl->tightCompressLevel;
    qualityLevel = cl->tightQualityLevel;

    if ( cl->format.depth == 24 && cl->format.redMax == 0xFF &&
         cl->format.greenMax == 0xFF && cl->format.blueMax == 0xFF ) {
        usePixelFormat24 = TRUE;
    } else {
        usePixelFormat24 = FALSE;
    }

    if (!cl->enableLastRectEncoding || w * h < MIN_SPLIT_RECT_SIZE)
        return SendRectSimple(cl, x, y, w, h);

    /* Make sure we can write at least one pixel into tightBeforeBuf. */

    if (tightBeforeBufSize < 4) {
        tightBeforeBufSize = 4;
        if (tightBeforeBuf == NULL)
            tightBeforeBuf = (char *)malloc(tightBeforeBufSize);
        else
            tightBeforeBuf = (char *)realloc(tightBeforeBuf,
                                              tightBeforeBufSize);
    }

    /* Calculate maximum number of rows in one non-solid rectangle. */

    {
        int maxRectSize, maxRectWidth, nMaxWidth;

        maxRectSize = tightConf[compressLevel].maxRectSize;
        maxRectWidth = tightConf[compressLevel].maxRectWidth;
        nMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        nMaxRows = maxRectSize / nMaxWidth;
    }

    /* Try to find large solid-color areas and send them separately. */

    for (dy = y; dy < y + h; dy += MAX_SPLIT_TILE_SIZE) {

        /* If a rectangle becomes too large, send its upper part now. */

        if (dy - y >= nMaxRows) {
            if (!SendRectSimple(cl, x, y, w, nMaxRows))
                return 0;
            y += nMaxRows;
            h -= nMaxRows;
        }

        dh = (dy + MAX_SPLIT_TILE_SIZE <= y + h) ?
            MAX_SPLIT_TILE_SIZE : (y + h - dy);

        for (dx = x; dx < x + w; dx += MAX_SPLIT_TILE_SIZE) {

            dw = (dx + MAX_SPLIT_TILE_SIZE <= x + w) ?
                MAX_SPLIT_TILE_SIZE : (x + w - dx);

            if (CheckSolidTile(cl, dx, dy, dw, dh, &colorValue, FALSE)) {

                /* Get dimensions of solid-color area. */

                FindBestSolidArea(cl, dx, dy, w - (dx - x), h - (dy - y),
				  colorValue, &w_best, &h_best);

                /* Make sure a solid rectangle is large enough
                   (or the whole rectangle is of the same color). */

                if ( w_best * h_best != w * h &&
                     w_best * h_best < MIN_SOLID_SUBRECT_SIZE )
                    continue;

                /* Try to extend solid rectangle to maximum size. */

                x_best = dx; y_best = dy;
                ExtendSolidArea(cl, x, y, w, h, colorValue,
                                &x_best, &y_best, &w_best, &h_best);

                /* Send rectangles at top and left to solid-color area. */

                if ( y_best != y &&
                     !SendRectSimple(cl, x, y, w, y_best-y) )
                    return FALSE;
                if ( x_best != x &&
                     !rfbSendRectEncodingTight(cl, x, y_best,
                                               x_best-x, h_best) )
                    return FALSE;

                /* Send solid-color rectangle. */

                if (!SendTightHeader(cl, x_best, y_best, w_best, h_best))
                    return FALSE;

                fbptr = (cl->scaledScreen->frameBuffer +
                         (cl->scaledScreen->paddedWidthInBytes * y_best) +
                         (x_best * (cl->scaledScreen->bitsPerPixel / 8)));

                (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
                                   &cl->format, fbptr, tightBeforeBuf,
                                   cl->scaledScreen->paddedWidthInBytes, 1, 1);

                if (!SendSolidRect(cl))
                    return FALSE;

                /* Send remaining rectangles (at right and bottom). */

                if ( x_best + w_best != x + w &&
                     !rfbSendRectEncodingTight(cl, x_best+w_best, y_best,
                                               w-(x_best-x)-w_best, h_best) )
                    return FALSE;
                if ( y_best + h_best != y + h &&
                     !rfbSendRectEncodingTight(cl, x, y_best+h_best,
                                               w, h-(y_best-y)-h_best) )
                    return FALSE;

                /* Return after all recursive calls are done. */

                return TRUE;
            }

        }

    }

    /* No suitable solid-color rectangles found. */

    return SendRectSimple(cl, x, y, w, h);
}

static void
FindBestSolidArea(rfbClientPtr cl,
                  int x,
                  int y,
                  int w,
                  int h,
                  uint32_t colorValue,
                  int *w_ptr,
                  int *h_ptr)
{
    int dx, dy, dw, dh;
    int w_prev;
    int w_best = 0, h_best = 0;

    w_prev = w;

    for (dy = y; dy < y + h; dy += MAX_SPLIT_TILE_SIZE) {

        dh = (dy + MAX_SPLIT_TILE_SIZE <= y + h) ?
            MAX_SPLIT_TILE_SIZE : (y + h - dy);
        dw = (w_prev > MAX_SPLIT_TILE_SIZE) ?
            MAX_SPLIT_TILE_SIZE : w_prev;

        if (!CheckSolidTile(cl, x, dy, dw, dh, &colorValue, TRUE))
            break;

        for (dx = x + dw; dx < x + w_prev;) {
            dw = (dx + MAX_SPLIT_TILE_SIZE <= x + w_prev) ?
                MAX_SPLIT_TILE_SIZE : (x + w_prev - dx);
            if (!CheckSolidTile(cl, dx, dy, dw, dh, &colorValue, TRUE))
                break;
	    dx += dw;
        }

        w_prev = dx - x;
        if (w_prev * (dy + dh - y) > w_best * h_best) {
            w_best = w_prev;
            h_best = dy + dh - y;
        }
    }

    *w_ptr = w_best;
    *h_ptr = h_best;
}

static void
ExtendSolidArea(rfbClientPtr cl,
                int x,
                int y,
                int w,
                int h,
                uint32_t colorValue,
                int *x_ptr,
                int *y_ptr,
                int *w_ptr,
                int *h_ptr)
{
    int cx, cy;

    /* Try to extend the area upwards. */
    for ( cy = *y_ptr - 1;
          cy >= y && CheckSolidTile(cl, *x_ptr, cy, *w_ptr, 1, &colorValue, TRUE);
          cy-- );
    *h_ptr += *y_ptr - (cy + 1);
    *y_ptr = cy + 1;

    /* ... downwards. */
    for ( cy = *y_ptr + *h_ptr;
          cy < y + h &&
              CheckSolidTile(cl, *x_ptr, cy, *w_ptr, 1, &colorValue, TRUE);
          cy++ );
    *h_ptr += cy - (*y_ptr + *h_ptr);

    /* ... to the left. */
    for ( cx = *x_ptr - 1;
          cx >= x && CheckSolidTile(cl, cx, *y_ptr, 1, *h_ptr, &colorValue, TRUE);
          cx-- );
    *w_ptr += *x_ptr - (cx + 1);
    *x_ptr = cx + 1;

    /* ... to the right. */
    for ( cx = *x_ptr + *w_ptr;
          cx < x + w &&
              CheckSolidTile(cl, cx, *y_ptr, 1, *h_ptr, &colorValue, TRUE);
          cx++ );
    *w_ptr += cx - (*x_ptr + *w_ptr);
}

/*
 * Check if a rectangle is all of the same color. If needSameColor is
 * set to non-zero, then also check that its color equals to the
 * *colorPtr value. The result is 1 if the test is successfull, and in
 * that case new color will be stored in *colorPtr.
 */

static rfbBool CheckSolidTile(rfbClientPtr cl, int x, int y, int w, int h, uint32_t* colorPtr, rfbBool needSameColor)
{
    switch(cl->screen->serverFormat.bitsPerPixel) {
    case 32:
        return CheckSolidTile32(cl, x, y, w, h, colorPtr, needSameColor);
    case 16:
        return CheckSolidTile16(cl, x, y, w, h, colorPtr, needSameColor);
    default:
        return CheckSolidTile8(cl, x, y, w, h, colorPtr, needSameColor);
    }
}

#define DEFINE_CHECK_SOLID_FUNCTION(bpp)                                      \
                                                                              \
static rfbBool                                                                \
CheckSolidTile##bpp(rfbClientPtr cl, int x, int y, int w, int h,              \
		uint32_t* colorPtr, rfbBool needSameColor)                    \
{                                                                             \
    uint##bpp##_t *fbptr;                                                     \
    uint##bpp##_t colorValue;                                                 \
    int dx, dy;                                                               \
                                                                              \
    fbptr = (uint##bpp##_t *)                                                 \
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes + x * (bpp/8)]; \
                                                                              \
    colorValue = *fbptr;                                                      \
    if (needSameColor && (uint32_t)colorValue != *colorPtr)                   \
        return FALSE;                                                         \
                                                                              \
    for (dy = 0; dy < h; dy++) {                                              \
        for (dx = 0; dx < w; dx++) {                                          \
            if (colorValue != fbptr[dx])                                      \
                return FALSE;                                                 \
        }                                                                     \
        fbptr = (uint##bpp##_t *)((uint8_t *)fbptr + cl->scaledScreen->paddedWidthInBytes); \
    }                                                                         \
                                                                              \
    *colorPtr = (uint32_t)colorValue;                                         \
    return TRUE;                                                              \
}

DEFINE_CHECK_SOLID_FUNCTION(8)
DEFINE_CHECK_SOLID_FUNCTION(16)
DEFINE_CHECK_SOLID_FUNCTION(32)

static rfbBool
SendRectSimple(rfbClientPtr cl, int x, int y, int w, int h)
{
    int maxBeforeSize, maxAfterSize;
    int maxRectSize, maxRectWidth;
    int subrectMaxWidth, subrectMaxHeight;
    int dx, dy;
    int rw, rh;

    maxRectSize = tightConf[compressLevel].maxRectSize;
    maxRectWidth = tightConf[compressLevel].maxRectWidth;

    maxBeforeSize = maxRectSize * (cl->format.bitsPerPixel / 8);
    maxAfterSize = maxBeforeSize + (maxBeforeSize + 99) / 100 + 12;

    if (tightBeforeBufSize < maxBeforeSize) {
        tightBeforeBufSize = maxBeforeSize;
        if (tightBeforeBuf == NULL)
            tightBeforeBuf = (char *)malloc(tightBeforeBufSize);
        else
            tightBeforeBuf = (char *)realloc(tightBeforeBuf,
                                              tightBeforeBufSize);
    }

    if (tightAfterBufSize < maxAfterSize) {
        tightAfterBufSize = maxAfterSize;
        if (tightAfterBuf == NULL)
            tightAfterBuf = (char *)malloc(tightAfterBufSize);
        else
            tightAfterBuf = (char *)realloc(tightAfterBuf,
                                             tightAfterBufSize);
    }

    if (w > maxRectWidth || w * h > maxRectSize) {
        subrectMaxWidth = (w > maxRectWidth) ? maxRectWidth : w;
        subrectMaxHeight = maxRectSize / subrectMaxWidth;

        for (dy = 0; dy < h; dy += subrectMaxHeight) {
            for (dx = 0; dx < w; dx += maxRectWidth) {
                rw = (dx + maxRectWidth < w) ? maxRectWidth : w - dx;
                rh = (dy + subrectMaxHeight < h) ? subrectMaxHeight : h - dy;
                if (!SendSubrect(cl, x+dx, y+dy, rw, rh))
                    return FALSE;
            }
        }
    } else {
        if (!SendSubrect(cl, x, y, w, h))
            return FALSE;
    }

    return TRUE;
}

static rfbBool
SendSubrect(rfbClientPtr cl,
            int x,
            int y,
            int w,
            int h)
{
    char *fbptr;
    rfbBool success = FALSE;

    /* Send pending data if there is more than 128 bytes. */
    if (cl->ublen > 128) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    if (!SendTightHeader(cl, x, y, w, h))
        return FALSE;

    fbptr = (cl->scaledScreen->frameBuffer + (cl->scaledScreen->paddedWidthInBytes * y)
             + (x * (cl->scaledScreen->bitsPerPixel / 8)));

    (*cl->translateFn)(cl->translateLookupTable, &cl->screen->serverFormat,
                       &cl->format, fbptr, tightBeforeBuf,
                       cl->scaledScreen->paddedWidthInBytes, w, h);

    paletteMaxColors = w * h / tightConf[compressLevel].idxMaxColorsDivisor;
    if ( paletteMaxColors < 2 &&
         w * h >= tightConf[compressLevel].monoMinRectSize ) {
        paletteMaxColors = 2;
    }
    switch (cl->format.bitsPerPixel) {
    case 8:
        FillPalette8(w * h);
        break;
    case 16:
        FillPalette16(w * h);
        break;
    default:
        FillPalette32(w * h);
    }

    switch (paletteNumColors) {
    case 0:
        /* Truecolor image */
        if (DetectSmoothImage(cl, &cl->format, w, h)) {
            if (qualityLevel != -1) {
                success = SendJpegRect(cl, x, y, w, h,
                                       tightConf[qualityLevel].jpegQuality);
            } else {
                success = SendGradientRect(cl, w, h);
            }
        } else {
            success = SendFullColorRect(cl, w, h);
        }
        break;
    case 1:
        /* Solid rectangle */
        success = SendSolidRect(cl);
        break;
    case 2:
        /* Two-color rectangle */
        success = SendMonoRect(cl, w, h);
        break;
    default:
        /* Up to 256 different colors */
        if ( paletteNumColors > 96 &&
             qualityLevel != -1 && qualityLevel <= 3 &&
             DetectSmoothImage(cl, &cl->format, w, h) ) {
            success = SendJpegRect(cl, x, y, w, h,
                                   tightConf[qualityLevel].jpegQuality);
        } else {
            success = SendIndexedRect(cl, w, h);
        }
    }
    return success;
}

static rfbBool
SendTightHeader(rfbClientPtr cl,
                int x,
                int y,
                int w,
                int h)
{
    rfbFramebufferUpdateRectHeader rect;

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingTight);

    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    rfbStatRecordEncodingSent(cl, rfbEncodingTight, sz_rfbFramebufferUpdateRectHeader,
                              sz_rfbFramebufferUpdateRectHeader + w * (cl->format.bitsPerPixel / 8) * h);

    return TRUE;
}

/*
 * Subencoding implementations.
 */

static rfbBool
SendSolidRect(rfbClientPtr cl)
{
    int len;

    if (usePixelFormat24) {
        Pack24(cl, tightBeforeBuf, &cl->format, 1);
        len = 3;
    } else
        len = cl->format.bitsPerPixel / 8;

    if (cl->ublen + 1 + len > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightFill << 4);
    memcpy (&cl->updateBuf[cl->ublen], tightBeforeBuf, len);
    cl->ublen += len;

    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, len+1);

    return TRUE;
}

static rfbBool
SendMonoRect(rfbClientPtr cl,
             int w,
             int h)
{
    int streamId = 1;
    int paletteLen, dataLen;

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 2 * cl->format.bitsPerPixel / 8 > UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
    dataLen = (w + 7) / 8;
    dataLen *= h;

    cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterPalette;
    cl->updateBuf[cl->ublen++] = 1;

    /* Prepare palette, convert image. */
    switch (cl->format.bitsPerPixel) {

    case 32:
        EncodeMonoRect32((uint8_t *)tightBeforeBuf, w, h);

        ((uint32_t *)tightAfterBuf)[0] = monoBackground;
        ((uint32_t *)tightAfterBuf)[1] = monoForeground;
        if (usePixelFormat24) {
            Pack24(cl, tightAfterBuf, &cl->format, 2);
            paletteLen = 6;
        } else
            paletteLen = 8;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteLen);
        cl->ublen += paletteLen;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteLen);
        break;

    case 16:
        EncodeMonoRect16((uint8_t *)tightBeforeBuf, w, h);

        ((uint16_t *)tightAfterBuf)[0] = (uint16_t)monoBackground;
        ((uint16_t *)tightAfterBuf)[1] = (uint16_t)monoForeground;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, 4);
        cl->ublen += 4;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 7);
        break;

    default:
        EncodeMonoRect8((uint8_t *)tightBeforeBuf, w, h);

        cl->updateBuf[cl->ublen++] = (char)monoBackground;
        cl->updateBuf[cl->ublen++] = (char)monoForeground;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 5);
    }

    return CompressData(cl, streamId, dataLen,
                        tightConf[compressLevel].monoZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendIndexedRect(rfbClientPtr cl,
                int w,
                int h)
{
    int streamId = 2;
    int i, entryLen;

    if ( cl->ublen + TIGHT_MIN_TO_COMPRESS + 6 +
	 paletteNumColors * cl->format.bitsPerPixel / 8 >
         UPDATE_BUF_SIZE ) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    /* Prepare tight encoding header. */
    cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterPalette;
    cl->updateBuf[cl->ublen++] = (char)(paletteNumColors - 1);

    /* Prepare palette, convert image. */
    switch (cl->format.bitsPerPixel) {

    case 32:
        EncodeIndexedRect32((uint8_t *)tightBeforeBuf, w * h);

        for (i = 0; i < paletteNumColors; i++) {
            ((uint32_t *)tightAfterBuf)[i] =
                palette.entry[i].listNode->rgb;
        }
        if (usePixelFormat24) {
            Pack24(cl, tightAfterBuf, &cl->format, paletteNumColors);
            entryLen = 3;
        } else
            entryLen = 4;

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteNumColors * entryLen);
        cl->ublen += paletteNumColors * entryLen;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteNumColors * entryLen);
        break;

    case 16:
        EncodeIndexedRect16((uint8_t *)tightBeforeBuf, w * h);

        for (i = 0; i < paletteNumColors; i++) {
            ((uint16_t *)tightAfterBuf)[i] =
                (uint16_t)palette.entry[i].listNode->rgb;
        }

        memcpy(&cl->updateBuf[cl->ublen], tightAfterBuf, paletteNumColors * 2);
        cl->ublen += paletteNumColors * 2;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 3 + paletteNumColors * 2);
        break;

    default:
        return FALSE;           /* Should never happen. */
    }

    return CompressData(cl, streamId, w * h,
                        tightConf[compressLevel].idxZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendFullColorRect(rfbClientPtr cl,
                  int w,
                  int h)
{
    int streamId = 0;
    int len;

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = 0x00;  /* stream id = 0, no flushing, no filter */
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);

    if (usePixelFormat24) {
        Pack24(cl, tightBeforeBuf, &cl->format, w * h);
        len = 3;
    } else
        len = cl->format.bitsPerPixel / 8;

    return CompressData(cl, streamId, w * h * len,
                        tightConf[compressLevel].rawZlibLevel,
                        Z_DEFAULT_STRATEGY);
}

static rfbBool
SendGradientRect(rfbClientPtr cl,
                 int w,
                 int h)
{
    int streamId = 3;
    int len;

    if (cl->format.bitsPerPixel == 8)
        return SendFullColorRect(cl, w, h);

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 2 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    if (prevRowBuf == NULL)
        prevRowBuf = (int *)malloc(2048 * 3 * sizeof(int));

    cl->updateBuf[cl->ublen++] = (streamId | rfbTightExplicitFilter) << 4;
    cl->updateBuf[cl->ublen++] = rfbTightFilterGradient;
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 2);

    if (usePixelFormat24) {
        FilterGradient24(cl, tightBeforeBuf, &cl->format, w, h);
        len = 3;
    } else if (cl->format.bitsPerPixel == 32) {
        FilterGradient32(cl, (uint32_t *)tightBeforeBuf, &cl->format, w, h);
        len = 4;
    } else {
        FilterGradient16(cl, (uint16_t *)tightBeforeBuf, &cl->format, w, h);
        len = 2;
    }

    return CompressData(cl, streamId, w * h * len,
                        tightConf[compressLevel].gradientZlibLevel,
                        Z_FILTERED);
}

static rfbBool
CompressData(rfbClientPtr cl,
             int streamId,
             int dataLen,
             int zlibLevel,
             int zlibStrategy)
{
    z_streamp pz;
    int err;

    if (dataLen < TIGHT_MIN_TO_COMPRESS) {
        memcpy(&cl->updateBuf[cl->ublen], tightBeforeBuf, dataLen);
        cl->ublen += dataLen;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, dataLen);
        return TRUE;
    }

    pz = &cl->zsStruct[streamId];

    /* Initialize compression stream if needed. */
    if (!cl->zsActive[streamId]) {
        pz->zalloc = Z_NULL;
        pz->zfree = Z_NULL;
        pz->opaque = Z_NULL;

        err = deflateInit2 (pz, zlibLevel, Z_DEFLATED, MAX_WBITS,
                            MAX_MEM_LEVEL, zlibStrategy);
        if (err != Z_OK)
            return FALSE;

        cl->zsActive[streamId] = TRUE;
        cl->zsLevel[streamId] = zlibLevel;
    }

    /* Prepare buffer pointers. */
    pz->next_in = (Bytef *)tightBeforeBuf;
    pz->avail_in = dataLen;
    pz->next_out = (Bytef *)tightAfterBuf;
    pz->avail_out = tightAfterBufSize;

    /* Change compression parameters if needed. */
    if (zlibLevel != cl->zsLevel[streamId]) {
        if (deflateParams (pz, zlibLevel, zlibStrategy) != Z_OK) {
            return FALSE;
        }
        cl->zsLevel[streamId] = zlibLevel;
    }

    /* Actual compression. */
    if ( deflate (pz, Z_SYNC_FLUSH) != Z_OK ||
         pz->avail_in != 0 || pz->avail_out == 0 ) {
        return FALSE;
    }

    return SendCompressedData(cl, tightAfterBufSize - pz->avail_out);
}

static rfbBool SendCompressedData(rfbClientPtr cl,
                                  int compressedLen)
{
    int i, portionLen;

    cl->updateBuf[cl->ublen++] = compressedLen & 0x7F;
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
    if (compressedLen > 0x7F) {
        cl->updateBuf[cl->ublen-1] |= 0x80;
        cl->updateBuf[cl->ublen++] = compressedLen >> 7 & 0x7F;
        rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
        if (compressedLen > 0x3FFF) {
            cl->updateBuf[cl->ublen-1] |= 0x80;
            cl->updateBuf[cl->ublen++] = compressedLen >> 14 & 0xFF;
            rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);
        }
    }

    portionLen = UPDATE_BUF_SIZE;
    for (i = 0; i < compressedLen; i += portionLen) {
        if (i + portionLen > compressedLen) {
            portionLen = compressedLen - i;
        }
        if (cl->ublen + portionLen > UPDATE_BUF_SIZE) {
            if (!rfbSendUpdateBuf(cl))
                return FALSE;
        }
        memcpy(&cl->updateBuf[cl->ublen], &tightAfterBuf[i], portionLen);
        cl->ublen += portionLen;
    }
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, compressedLen);

    return TRUE;
}

/*
 * Code to determine how many different colors used in rectangle.
 */

static void
FillPalette8(int count)
{
    uint8_t *data = (uint8_t *)tightBeforeBuf;
    uint8_t c0, c1;
    int i, n0, n1;

    paletteNumColors = 0;

    c0 = data[0];
    for (i = 1; i < count && data[i] == c0; i++);
    if (i == count) {
        paletteNumColors = 1;
        return;                 /* Solid rectangle */
    }

    if (paletteMaxColors < 2)
        return;

    n0 = i;
    c1 = data[i];
    n1 = 0;
    for (i++; i < count; i++) {
        if (data[i] == c0) {
            n0++;
        } else if (data[i] == c1) {
            n1++;
        } else
            break;
    }
    if (i == count) {
        if (n0 > n1) {
            monoBackground = (uint32_t)c0;
            monoForeground = (uint32_t)c1;
        } else {
            monoBackground = (uint32_t)c1;
            monoForeground = (uint32_t)c0;
        }
        paletteNumColors = 2;   /* Two colors */
    }
}

#define DEFINE_FILL_PALETTE_FUNCTION(bpp)                               \
                                                                        \
static void                                                             \
FillPalette##bpp(int count) {                                           \
    uint##bpp##_t *data = (uint##bpp##_t *)tightBeforeBuf;              \
    uint##bpp##_t c0, c1, ci;                                           \
    int i, n0, n1, ni;                                                  \
                                                                        \
    c0 = data[0];                                                       \
    for (i = 1; i < count && data[i] == c0; i++);                       \
    if (i >= count) {                                                   \
        paletteNumColors = 1;   /* Solid rectangle */                   \
        return;                                                         \
    }                                                                   \
                                                                        \
    if (paletteMaxColors < 2) {                                         \
        paletteNumColors = 0;   /* Full-color encoding preferred */     \
        return;                                                         \
    }                                                                   \
                                                                        \
    n0 = i;                                                             \
    c1 = data[i];                                                       \
    n1 = 0;                                                             \
    for (i++; i < count; i++) {                                         \
        ci = data[i];                                                   \
        if (ci == c0) {                                                 \
            n0++;                                                       \
        } else if (ci == c1) {                                          \
            n1++;                                                       \
        } else                                                          \
            break;                                                      \
    }                                                                   \
    if (i >= count) {                                                   \
        if (n0 > n1) {                                                  \
            monoBackground = (uint32_t)c0;                              \
            monoForeground = (uint32_t)c1;                              \
        } else {                                                        \
            monoBackground = (uint32_t)c1;                              \
            monoForeground = (uint32_t)c0;                              \
        }                                                               \
        paletteNumColors = 2;   /* Two colors */                        \
        return;                                                         \
    }                                                                   \
                                                                        \
    PaletteReset();                                                     \
    PaletteInsert (c0, (uint32_t)n0, bpp);                              \
    PaletteInsert (c1, (uint32_t)n1, bpp);                              \
                                                                        \
    ni = 1;                                                             \
    for (i++; i < count; i++) {                                         \
        if (data[i] == ci) {                                            \
            ni++;                                                       \
        } else {                                                        \
            if (!PaletteInsert (ci, (uint32_t)ni, bpp))                 \
                return;                                                 \
            ci = data[i];                                               \
            ni = 1;                                                     \
        }                                                               \
    }                                                                   \
    PaletteInsert (ci, (uint32_t)ni, bpp);                              \
}

DEFINE_FILL_PALETTE_FUNCTION(16)
DEFINE_FILL_PALETTE_FUNCTION(32)


/*
 * Functions to operate with palette structures.
 */

#define HASH_FUNC16(rgb) ((int)(((rgb >> 8) + rgb) & 0xFF))
#define HASH_FUNC32(rgb) ((int)(((rgb >> 16) + (rgb >> 8)) & 0xFF))

static void
PaletteReset(void)
{
    paletteNumColors = 0;
    memset(palette.hash, 0, 256 * sizeof(COLOR_LIST *));
}

static int
PaletteInsert(uint32_t rgb,
              int numPixels,
              int bpp)
{
    COLOR_LIST *pnode;
    COLOR_LIST *prev_pnode = NULL;
    int hash_key, idx, new_idx, count;

    hash_key = (bpp == 16) ? HASH_FUNC16(rgb) : HASH_FUNC32(rgb);

    pnode = palette.hash[hash_key];

    while (pnode != NULL) {
        if (pnode->rgb == rgb) {
            /* Such palette entry already exists. */
            new_idx = idx = pnode->idx;
            count = palette.entry[idx].numPixels + numPixels;
            if (new_idx && palette.entry[new_idx-1].numPixels < count) {
                do {
                    palette.entry[new_idx] = palette.entry[new_idx-1];
                    palette.entry[new_idx].listNode->idx = new_idx;
                    new_idx--;
                }
                while (new_idx && palette.entry[new_idx-1].numPixels < count);
                palette.entry[new_idx].listNode = pnode;
                pnode->idx = new_idx;
            }
            palette.entry[new_idx].numPixels = count;
            return paletteNumColors;
        }
        prev_pnode = pnode;
        pnode = pnode->next;
    }

    /* Check if palette is full. */
    if (paletteNumColors == 256 || paletteNumColors == paletteMaxColors) {
        paletteNumColors = 0;
        return 0;
    }

    /* Move palette entries with lesser pixel counts. */
    for ( idx = paletteNumColors;
          idx > 0 && palette.entry[idx-1].numPixels < numPixels;
          idx-- ) {
        palette.entry[idx] = palette.entry[idx-1];
        palette.entry[idx].listNode->idx = idx;
    }

    /* Add new palette entry into the freed slot. */
    pnode = &palette.list[paletteNumColors];
    if (prev_pnode != NULL) {
        prev_pnode->next = pnode;
    } else {
        palette.hash[hash_key] = pnode;
    }
    pnode->next = NULL;
    pnode->idx = idx;
    pnode->rgb = rgb;
    palette.entry[idx].listNode = pnode;
    palette.entry[idx].numPixels = numPixels;

    return (++paletteNumColors);
}


/*
 * Converting 32-bit color samples into 24-bit colors.
 * Should be called only when redMax, greenMax and blueMax are 255.
 * Color components assumed to be byte-aligned.
 */

static void Pack24(rfbClientPtr cl,
                   char *buf,
                   rfbPixelFormat *fmt,
                   int count)
{
    uint32_t *buf32;
    uint32_t pix;
    int r_shift, g_shift, b_shift;

    buf32 = (uint32_t *)buf;

    if (!cl->screen->serverFormat.bigEndian == !fmt->bigEndian) {
        r_shift = fmt->redShift;
        g_shift = fmt->greenShift;
        b_shift = fmt->blueShift;
    } else {
        r_shift = 24 - fmt->redShift;
        g_shift = 24 - fmt->greenShift;
        b_shift = 24 - fmt->blueShift;
    }

    while (count--) {
        pix = *buf32++;
        *buf++ = (char)(pix >> r_shift);
        *buf++ = (char)(pix >> g_shift);
        *buf++ = (char)(pix >> b_shift);
    }
}


/*
 * Converting truecolor samples into palette indices.
 */

#define DEFINE_IDX_ENCODE_FUNCTION(bpp)                                 \
                                                                        \
static void                                                             \
EncodeIndexedRect##bpp(uint8_t *buf, int count) {                       \
    COLOR_LIST *pnode;                                                  \
    uint##bpp##_t *src;                                                 \
    uint##bpp##_t rgb;                                                  \
    int rep = 0;                                                        \
                                                                        \
    src = (uint##bpp##_t *) buf;                                        \
                                                                        \
    while (count--) {                                                   \
        rgb = *src++;                                                   \
        while (count && *src == rgb) {                                  \
            rep++, src++, count--;                                      \
        }                                                               \
        pnode = palette.hash[HASH_FUNC##bpp(rgb)];                      \
        while (pnode != NULL) {                                         \
            if ((uint##bpp##_t)pnode->rgb == rgb) {                     \
                *buf++ = (uint8_t)pnode->idx;                           \
                while (rep) {                                           \
                    *buf++ = (uint8_t)pnode->idx;                       \
                    rep--;                                              \
                }                                                       \
                break;                                                  \
            }                                                           \
            pnode = pnode->next;                                        \
        }                                                               \
    }                                                                   \
}

DEFINE_IDX_ENCODE_FUNCTION(16)
DEFINE_IDX_ENCODE_FUNCTION(32)

#define DEFINE_MONO_ENCODE_FUNCTION(bpp)                                \
                                                                        \
static void                                                             \
EncodeMonoRect##bpp(uint8_t *buf, int w, int h) {                       \
    uint##bpp##_t *ptr;                                                 \
    uint##bpp##_t bg;                                                   \
    unsigned int value, mask;                                           \
    int aligned_width;                                                  \
    int x, y, bg_bits;                                                  \
                                                                        \
    ptr = (uint##bpp##_t *) buf;                                        \
    bg = (uint##bpp##_t) monoBackground;                                \
    aligned_width = w - w % 8;                                          \
                                                                        \
    for (y = 0; y < h; y++) {                                           \
        for (x = 0; x < aligned_width; x += 8) {                        \
            for (bg_bits = 0; bg_bits < 8; bg_bits++) {                 \
                if (*ptr++ != bg)                                       \
                    break;                                              \
            }                                                           \
            if (bg_bits == 8) {                                         \
                *buf++ = 0;                                             \
                continue;                                               \
            }                                                           \
            mask = 0x80 >> bg_bits;                                     \
            value = mask;                                               \
            for (bg_bits++; bg_bits < 8; bg_bits++) {                   \
                mask >>= 1;                                             \
                if (*ptr++ != bg) {                                     \
                    value |= mask;                                      \
                }                                                       \
            }                                                           \
            *buf++ = (uint8_t)value;                                    \
        }                                                               \
                                                                        \
        mask = 0x80;                                                    \
        value = 0;                                                      \
        if (x >= w)                                                     \
            continue;                                                   \
                                                                        \
        for (; x < w; x++) {                                            \
            if (*ptr++ != bg) {                                         \
                value |= mask;                                          \
            }                                                           \
            mask >>= 1;                                                 \
        }                                                               \
        *buf++ = (uint8_t)value;                                        \
    }                                                                   \
}

DEFINE_MONO_ENCODE_FUNCTION(8)
DEFINE_MONO_ENCODE_FUNCTION(16)
DEFINE_MONO_ENCODE_FUNCTION(32)


/*
 * ``Gradient'' filter for 24-bit color samples.
 * Should be called only when redMax, greenMax and blueMax are 255.
 * Color components assumed to be byte-aligned.
 */

static void
FilterGradient24(rfbClientPtr cl, char *buf, rfbPixelFormat *fmt, int w, int h)
{
    uint32_t *buf32;
    uint32_t pix32;
    int *prevRowPtr;
    int shiftBits[3];
    int pixHere[3], pixUpper[3], pixLeft[3], pixUpperLeft[3];
    int prediction;
    int x, y, c;

    buf32 = (uint32_t *)buf;
    memset (prevRowBuf, 0, w * 3 * sizeof(int));

    if (!cl->screen->serverFormat.bigEndian == !fmt->bigEndian) {
        shiftBits[0] = fmt->redShift;
        shiftBits[1] = fmt->greenShift;
        shiftBits[2] = fmt->blueShift;
    } else {
        shiftBits[0] = 24 - fmt->redShift;
        shiftBits[1] = 24 - fmt->greenShift;
        shiftBits[2] = 24 - fmt->blueShift;
    }

    for (y = 0; y < h; y++) {
        for (c = 0; c < 3; c++) {
            pixUpper[c] = 0;
            pixHere[c] = 0;
        }
        prevRowPtr = prevRowBuf;
        for (x = 0; x < w; x++) {
            pix32 = *buf32++;
            for (c = 0; c < 3; c++) {
                pixUpperLeft[c] = pixUpper[c];
                pixLeft[c] = pixHere[c];
                pixUpper[c] = *prevRowPtr;
                pixHere[c] = (int)(pix32 >> shiftBits[c] & 0xFF);
                *prevRowPtr++ = pixHere[c];

                prediction = pixLeft[c] + pixUpper[c] - pixUpperLeft[c];
                if (prediction < 0) {
                    prediction = 0;
                } else if (prediction > 0xFF) {
                    prediction = 0xFF;
                }
                *buf++ = (char)(pixHere[c] - prediction);
            }
        }
    }
}


/*
 * ``Gradient'' filter for other color depths.
 */

#define DEFINE_GRADIENT_FILTER_FUNCTION(bpp)                             \
                                                                         \
static void                                                              \
FilterGradient##bpp(rfbClientPtr cl, uint##bpp##_t *buf,                 \
		rfbPixelFormat *fmt, int w, int h) {                     \
    uint##bpp##_t pix, diff;                                             \
    rfbBool endianMismatch;                                              \
    int *prevRowPtr;                                                     \
    int maxColor[3], shiftBits[3];                                       \
    int pixHere[3], pixUpper[3], pixLeft[3], pixUpperLeft[3];            \
    int prediction;                                                      \
    int x, y, c;                                                         \
                                                                         \
    memset (prevRowBuf, 0, w * 3 * sizeof(int));                         \
                                                                         \
    endianMismatch = (!cl->screen->serverFormat.bigEndian != !fmt->bigEndian);    \
                                                                         \
    maxColor[0] = fmt->redMax;                                           \
    maxColor[1] = fmt->greenMax;                                         \
    maxColor[2] = fmt->blueMax;                                          \
    shiftBits[0] = fmt->redShift;                                        \
    shiftBits[1] = fmt->greenShift;                                      \
    shiftBits[2] = fmt->blueShift;                                       \
                                                                         \
    for (y = 0; y < h; y++) {                                            \
        for (c = 0; c < 3; c++) {                                        \
            pixUpper[c] = 0;                                             \
            pixHere[c] = 0;                                              \
        }                                                                \
        prevRowPtr = prevRowBuf;                                         \
        for (x = 0; x < w; x++) {                                        \
            pix = *buf;                                                  \
            if (endianMismatch) {                                        \
                pix = Swap##bpp(pix);                                    \
            }                                                            \
            diff = 0;                                                    \
            for (c = 0; c < 3; c++) {                                    \
                pixUpperLeft[c] = pixUpper[c];                           \
                pixLeft[c] = pixHere[c];                                 \
                pixUpper[c] = *prevRowPtr;                               \
                pixHere[c] = (int)(pix >> shiftBits[c] & maxColor[c]);   \
                *prevRowPtr++ = pixHere[c];                              \
                                                                         \
                prediction = pixLeft[c] + pixUpper[c] - pixUpperLeft[c]; \
                if (prediction < 0) {                                    \
                    prediction = 0;                                      \
                } else if (prediction > maxColor[c]) {                   \
                    prediction = maxColor[c];                            \
                }                                                        \
                diff |= ((pixHere[c] - prediction) & maxColor[c])        \
                    << shiftBits[c];                                     \
            }                                                            \
            if (endianMismatch) {                                        \
                diff = Swap##bpp(diff);                                  \
            }                                                            \
            *buf++ = diff;                                               \
        }                                                                \
    }                                                                    \
}

DEFINE_GRADIENT_FILTER_FUNCTION(16)
DEFINE_GRADIENT_FILTER_FUNCTION(32)


/*
 * Code to guess if given rectangle is suitable for smooth image
 * compression (by applying "gradient" filter or JPEG coder).
 */

#define JPEG_MIN_RECT_SIZE  4096

#define DETECT_SUBROW_WIDTH    7
#define DETECT_MIN_WIDTH       8
#define DETECT_MIN_HEIGHT      8

static int
DetectSmoothImage (rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h)
{
    long avgError;

    if ( cl->screen->serverFormat.bitsPerPixel == 8 || fmt->bitsPerPixel == 8 ||
         w < DETECT_MIN_WIDTH || h < DETECT_MIN_HEIGHT ) {
        return 0;
    }

    if (qualityLevel != -1) {
        if (w * h < JPEG_MIN_RECT_SIZE) {
            return 0;
        }
    } else {
        if ( rfbTightDisableGradient ||
             w * h < tightConf[compressLevel].gradientMinRectSize ) {
            return 0;
        }
    }

    if (fmt->bitsPerPixel == 32) {
        if (usePixelFormat24) {
            avgError = DetectSmoothImage24(cl, fmt, w, h);
            if (qualityLevel != -1) {
                return (avgError < tightConf[qualityLevel].jpegThreshold24);
            }
            return (avgError < tightConf[compressLevel].gradientThreshold24);
        } else {
            avgError = DetectSmoothImage32(cl, fmt, w, h);
        }
    } else {
        avgError = DetectSmoothImage16(cl, fmt, w, h);
    }
    if (qualityLevel != -1) {
        return (avgError < tightConf[qualityLevel].jpegThreshold);
    }
    return (avgError < tightConf[compressLevel].gradientThreshold);
}

static unsigned long
DetectSmoothImage24 (rfbClientPtr cl,
                     rfbPixelFormat *fmt,
                     int w,
                     int h)
{
    int off;
    int x, y, d, dx, c;
    int diffStat[256];
    int pixelCount = 0;
    int pix, left[3];
    unsigned long avgError;

    /* If client is big-endian, color samples begin from the second
       byte (offset 1) of a 32-bit pixel value. */
    off = (fmt->bigEndian != 0);

    memset(diffStat, 0, 256*sizeof(int));

    y = 0, x = 0;
    while (y < h && x < w) {
        for (d = 0; d < h - y && d < w - x - DETECT_SUBROW_WIDTH; d++) {
            for (c = 0; c < 3; c++) {
                left[c] = (int)tightBeforeBuf[((y+d)*w+x+d)*4+off+c] & 0xFF;
            }
            for (dx = 1; dx <= DETECT_SUBROW_WIDTH; dx++) {
                for (c = 0; c < 3; c++) {
                    pix = (int)tightBeforeBuf[((y+d)*w+x+d+dx)*4+off+c] & 0xFF;
                    diffStat[abs(pix - left[c])]++;
                    left[c] = pix;
                }
                pixelCount++;
            }
        }
        if (w > h) {
            x += h;
            y = 0;
        } else {
            x = 0;
            y += w;
        }
    }

    if (diffStat[0] * 33 / pixelCount >= 95)
        return 0;

    avgError = 0;
    for (c = 1; c < 8; c++) {
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);
        if (diffStat[c] == 0 || diffStat[c] > diffStat[c-1] * 2)
            return 0;
    }
    for (; c < 256; c++) {
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);
    }
    avgError /= (pixelCount * 3 - diffStat[0]);

    return avgError;
}

#define DEFINE_DETECT_FUNCTION(bpp)                                          \
                                                                             \
static unsigned long                                                         \
DetectSmoothImage##bpp (rfbClientPtr cl, rfbPixelFormat *fmt, int w, int h) {\
    rfbBool endianMismatch;                                                  \
    uint##bpp##_t pix;                                                       \
    int maxColor[3], shiftBits[3];                                           \
    int x, y, d, dx, c;                                                      \
    int diffStat[256];                                                       \
    int pixelCount = 0;                                                      \
    int sample, sum, left[3];                                                \
    unsigned long avgError;                                                  \
                                                                             \
    endianMismatch = (!cl->screen->serverFormat.bigEndian != !fmt->bigEndian); \
                                                                             \
    maxColor[0] = fmt->redMax;                                               \
    maxColor[1] = fmt->greenMax;                                             \
    maxColor[2] = fmt->blueMax;                                              \
    shiftBits[0] = fmt->redShift;                                            \
    shiftBits[1] = fmt->greenShift;                                          \
    shiftBits[2] = fmt->blueShift;                                           \
                                                                             \
    memset(diffStat, 0, 256*sizeof(int));                                    \
                                                                             \
    y = 0, x = 0;                                                            \
    while (y < h && x < w) {                                                 \
        for (d = 0; d < h - y && d < w - x - DETECT_SUBROW_WIDTH; d++) {     \
            pix = ((uint##bpp##_t *)tightBeforeBuf)[(y+d)*w+x+d];            \
            if (endianMismatch) {                                            \
                pix = Swap##bpp(pix);                                        \
            }                                                                \
            for (c = 0; c < 3; c++) {                                        \
                left[c] = (int)(pix >> shiftBits[c] & maxColor[c]);          \
            }                                                                \
            for (dx = 1; dx <= DETECT_SUBROW_WIDTH; dx++) {                  \
                pix = ((uint##bpp##_t *)tightBeforeBuf)[(y+d)*w+x+d+dx];     \
                if (endianMismatch) {                                        \
                    pix = Swap##bpp(pix);                                    \
                }                                                            \
                sum = 0;                                                     \
                for (c = 0; c < 3; c++) {                                    \
                    sample = (int)(pix >> shiftBits[c] & maxColor[c]);       \
                    sum += abs(sample - left[c]);                            \
                    left[c] = sample;                                        \
                }                                                            \
                if (sum > 255)                                               \
                    sum = 255;                                               \
                diffStat[sum]++;                                             \
                pixelCount++;                                                \
            }                                                                \
        }                                                                    \
        if (w > h) {                                                         \
            x += h;                                                          \
            y = 0;                                                           \
        } else {                                                             \
            x = 0;                                                           \
            y += w;                                                          \
        }                                                                    \
    }                                                                        \
                                                                             \
    if ((diffStat[0] + diffStat[1]) * 100 / pixelCount >= 90)                \
        return 0;                                                            \
                                                                             \
    avgError = 0;                                                            \
    for (c = 1; c < 8; c++) {                                                \
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);     \
        if (diffStat[c] == 0 || diffStat[c] > diffStat[c-1] * 2)             \
            return 0;                                                        \
    }                                                                        \
    for (; c < 256; c++) {                                                   \
        avgError += (unsigned long)diffStat[c] * (unsigned long)(c * c);     \
    }                                                                        \
    avgError /= (pixelCount - diffStat[0]);                                  \
                                                                             \
    return avgError;                                                         \
}

DEFINE_DETECT_FUNCTION(16)
DEFINE_DETECT_FUNCTION(32)


/*
 * JPEG compression stuff.
 */

static struct jpeg_destination_mgr jpegDstManager;
static rfbBool jpegError;
static int jpegDstDataLen;

static rfbBool
SendJpegRect(rfbClientPtr cl, int x, int y, int w, int h, int quality)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    uint8_t *srcBuf;
    JSAMPROW rowPointer[1];
    int dy;

    if (cl->screen->serverFormat.bitsPerPixel == 8)
        return SendFullColorRect(cl, w, h);

    srcBuf = (uint8_t *)malloc(w * 3);
    if (srcBuf == NULL) {
        return SendFullColorRect(cl, w, h);
    }
    rowPointer[0] = srcBuf;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    JpegSetDstManager (&cinfo);

    jpeg_start_compress(&cinfo, TRUE);

    for (dy = 0; dy < h; dy++) {
        PrepareRowForJpeg(cl, srcBuf, x, y + dy, w);
        jpeg_write_scanlines(&cinfo, rowPointer, 1);
        if (jpegError)
            break;
    }

    if (!jpegError)
        jpeg_finish_compress(&cinfo);

    jpeg_destroy_compress(&cinfo);
    free(srcBuf);

    if (jpegError)
        return SendFullColorRect(cl, w, h);

    if (cl->ublen + TIGHT_MIN_TO_COMPRESS + 1 > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    cl->updateBuf[cl->ublen++] = (char)(rfbTightJpeg << 4);
    rfbStatRecordEncodingSentAdd(cl, rfbEncodingTight, 1);

    return SendCompressedData(cl, jpegDstDataLen);
}

static void
PrepareRowForJpeg(rfbClientPtr cl,
                  uint8_t *dst,
                  int x,
                  int y,
                  int count)
{
    if (cl->screen->serverFormat.bitsPerPixel == 32) {
        if ( cl->screen->serverFormat.redMax == 0xFF &&
             cl->screen->serverFormat.greenMax == 0xFF &&
             cl->screen->serverFormat.blueMax == 0xFF ) {
            PrepareRowForJpeg24(cl, dst, x, y, count);
        } else {
            PrepareRowForJpeg32(cl, dst, x, y, count);
        }
    } else {
        /* 16 bpp assumed. */
        PrepareRowForJpeg16(cl, dst, x, y, count);
    }
}

static void
PrepareRowForJpeg24(rfbClientPtr cl,
                    uint8_t *dst,
                    int x,
                    int y,
                    int count)
{
    uint32_t *fbptr;
    uint32_t pix;

    fbptr = (uint32_t *)
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes + x * 4];

    while (count--) {
        pix = *fbptr++;
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.redShift);
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.greenShift);
        *dst++ = (uint8_t)(pix >> cl->screen->serverFormat.blueShift);
    }
}

#define DEFINE_JPEG_GET_ROW_FUNCTION(bpp)                                   \
                                                                            \
static void                                                                 \
PrepareRowForJpeg##bpp(rfbClientPtr cl, uint8_t *dst, int x, int y, int count) { \
    uint##bpp##_t *fbptr;                                                   \
    uint##bpp##_t pix;                                                      \
    int inRed, inGreen, inBlue;                                             \
                                                                            \
    fbptr = (uint##bpp##_t *)                                               \
        &cl->scaledScreen->frameBuffer[y * cl->scaledScreen->paddedWidthInBytes +       \
                             x * (bpp / 8)];                                \
                                                                            \
    while (count--) {                                                       \
        pix = *fbptr++;                                                     \
                                                                            \
        inRed = (int)                                                       \
            (pix >> cl->screen->serverFormat.redShift   & cl->screen->serverFormat.redMax); \
        inGreen = (int)                                                     \
            (pix >> cl->screen->serverFormat.greenShift & cl->screen->serverFormat.greenMax); \
        inBlue  = (int)                                                     \
            (pix >> cl->screen->serverFormat.blueShift  & cl->screen->serverFormat.blueMax); \
                                                                            \
	*dst++ = (uint8_t)((inRed   * 255 + cl->screen->serverFormat.redMax / 2) / \
                         cl->screen->serverFormat.redMax);                  \
	*dst++ = (uint8_t)((inGreen * 255 + cl->screen->serverFormat.greenMax / 2) / \
                         cl->screen->serverFormat.greenMax);                \
	*dst++ = (uint8_t)((inBlue  * 255 + cl->screen->serverFormat.blueMax / 2) / \
                         cl->screen->serverFormat.blueMax);                 \
    }                                                                       \
}

DEFINE_JPEG_GET_ROW_FUNCTION(16)
DEFINE_JPEG_GET_ROW_FUNCTION(32)

/*
 * Destination manager implementation for JPEG library.
 */

static void
JpegInitDestination(j_compress_ptr cinfo)
{
    jpegError = FALSE;
    jpegDstManager.next_output_byte = (JOCTET *)tightAfterBuf;
    jpegDstManager.free_in_buffer = (size_t)tightAfterBufSize;
}

static boolean
JpegEmptyOutputBuffer(j_compress_ptr cinfo)
{
    jpegError = TRUE;
    jpegDstManager.next_output_byte = (JOCTET *)tightAfterBuf;
    jpegDstManager.free_in_buffer = (size_t)tightAfterBufSize;

    return TRUE;
}

static void
JpegTermDestination(j_compress_ptr cinfo)
{
    jpegDstDataLen = tightAfterBufSize - jpegDstManager.free_in_buffer;
}

static void
JpegSetDstManager(j_compress_ptr cinfo)
{
    jpegDstManager.init_destination = JpegInitDestination;
    jpegDstManager.empty_output_buffer = JpegEmptyOutputBuffer;
    jpegDstManager.term_destination = JpegTermDestination;
    cinfo->dest = &jpegDstManager;
}

///////////////////////////////////////////end tight.c //////////////////////////////////////////////////
