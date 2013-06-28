/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_fbvideo_h
#define _SDL_fbvideo_h

#include <sys/types.h>
#include <termios.h>

//MAC includes
#include <string.h>
#include <errno.h>

#include <sys/mman.h>

#include <fcntl.h>
#include <unistd.h>

#include <libkms/libkms.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
//

#include "SDL_mouse.h"
#include "SDL_mutex.h"
#include "../SDL_sysvideo.h"
#if SDL_INPUT_TSLIB
#include "tslib.h"
#endif

/* Hidden "this" pointer for the video functions */
#define _THIS	SDL_VideoDevice *this

typedef void KMS_bitBlit(
		Uint8 *src_pos,
		int src_right_delta,	/* pixels, not bytes */
		int src_down_delta,		/* pixels, not bytes */
		Uint8 *dst_pos,
		int dst_linebytes,
		int width,
		int height);

/* This is the structure we use to keep track of video memory */
typedef struct vidmem_bucket {
	struct vidmem_bucket *prev;
	int used;
	int dirty;
	char *base;
	unsigned int size;
	struct vidmem_bucket *next;
} vidmem_bucket;

/* Private display data */
struct SDL_PrivateVideoData {
	int console_fd;
	drmModeModeInfo cache_modinfo;
	drmModeModeInfo saved_modinfo;
	drmModeFB cache_fbinfo;
	int saved_cmaplen;
	__u16 *saved_cmap;

	int current_vt;
	int saved_vt;
	int keyboard_fd;
	int saved_kbd_mode;
	struct termios saved_kbd_termios;

	int mouse_fd;
#if SDL_INPUT_TSLIB
	struct tsdev *ts_dev;
#endif

	char *mapped_mem;
	char *shadow_mem;
	int mapped_memlen;
	int mapped_offset;
	char *mapped_io;
	long mapped_iolen;
	int rotate;
	int shadow_fb;
	KMS_bitBlit *blitFunc;
	int physlinebytes; /* Length of a line in bytes in physical fb */

#define NUM_MODELISTS	4		/* 8, 16, 24, and 32 bits-per-pixel */
	int SDL_nummodes[NUM_MODELISTS];
	SDL_Rect **SDL_modelist[NUM_MODELISTS];

	vidmem_bucket surfaces;
	int surfaces_memtotal;
	int surfaces_memleft;

	SDL_mutex *hw_lock;
	int switched_away;
	drmModeModeInfo screen_modinfo;
	Uint32 screen_arealen;
	Uint8 *screen_contents;
	__u16  screen_palette[3*256];

	void (*wait_vbl)(_THIS);
	void (*wait_idle)(_THIS);
};
/* Old variable names */
#define console_fd		(this->hidden->console_fd)
#define current_vt		(this->hidden->current_vt)
#define saved_vt		(this->hidden->saved_vt)
#define keyboard_fd		(this->hidden->keyboard_fd)
#define saved_kbd_mode		(this->hidden->saved_kbd_mode)
#define saved_kbd_termios	(this->hidden->saved_kbd_termios)
#define mouse_fd		(this->hidden->mouse_fd)
#if SDL_INPUT_TSLIB
#define ts_dev			(this->hidden->ts_dev)
#endif
#define cache_modinfo		(this->hidden->cache_modinfo)
#define cache_fbinfo            (this->hidden->cache_fbinfo)
#define saved_modinfo		(this->hidden->saved_modinfo)
#define saved_cmaplen		(this->hidden->saved_cmaplen)
#define saved_cmap		(this->hidden->saved_cmap)
#define mapped_mem		(this->hidden->mapped_mem)
#define mapped_mem2             (this->hidden->mapped_mem2)
#define shadow_mem		(this->hidden->shadow_mem)
#define mapped_memlen		(this->hidden->mapped_memlen)
#define mapped_offset		(this->hidden->mapped_offset)
#define mapped_io		(this->hidden->mapped_io)
#define mapped_iolen		(this->hidden->mapped_iolen)
#define rotate			(this->hidden->rotate)
#define shadow_fb		(this->hidden->shadow_fb)
#define blitFunc		(this->hidden->blitFunc)
#define physlinebytes		(this->hidden->physlinebytes)
#define SDL_nummodes		(this->hidden->SDL_nummodes)
#define SDL_modelist		(this->hidden->SDL_modelist)
#define surfaces		(this->hidden->surfaces)
#define surfaces_memtotal	(this->hidden->surfaces_memtotal)
#define surfaces_memleft	(this->hidden->surfaces_memleft)
#define hw_lock			(this->hidden->hw_lock)
#define switched_away		(this->hidden->switched_away)
#define screen_vinfo		(this->hidden->screen_vinfo)
#define screen_arealen		(this->hidden->screen_arealen)
#define screen_contents		(this->hidden->screen_contents)
#define screen_palette		(this->hidden->screen_palette)
#define wait_vbl		(this->hidden->wait_vbl)
#define wait_idle		(this->hidden->wait_idle)

/* These are utility functions for working with video surfaces */

static __inline__ void KMS_AddBusySurface(SDL_Surface *surface)
{
	((vidmem_bucket *)surface->hwdata)->dirty = 1;
}

static __inline__ int KMS_IsSurfaceBusy(SDL_Surface *surface)
{
	return ((vidmem_bucket *)surface->hwdata)->dirty;
}

static __inline__ void KMS_WaitBusySurfaces(_THIS)
{
	vidmem_bucket *bucket;

	/* Wait for graphic operations to complete */
	wait_idle(this);

	/* Clear all surface dirty bits */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		bucket->dirty = 0;
	}
}

static __inline__ void KMS_dst_to_xy(_THIS, SDL_Surface *dst, int *x, int *y)
{
	*x = (long)((char *)dst->pixels - mapped_mem)%this->screen->pitch;
	*y = (long)((char *)dst->pixels - mapped_mem)/this->screen->pitch;
	if ( dst == this->screen ) {
		*x += this->offset_x;
		*y += this->offset_y;
	}
}

#endif /* _SDL_fbvideo_h */
