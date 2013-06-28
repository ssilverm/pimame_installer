#include "SDL_config.h"

/* Dispmanx based SDL video driver implementation.
*  SDL - Simple DirectMedia Layer
*  Copyright (C) 1997-2012 Sam Lantinga
*  
*  SDL dispmanx backend
*  Copyright (C) 2012 Manuel Alfayate
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

//MAC includes
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "bcm_host.h"
//

#ifndef HAVE_GETPAGESIZE
#include <asm/page.h>		/* For definition of PAGE_SIZE */
#endif
#define ALIGN_UP(x,y)  ((x + (y)-1) & ~((y)-1))
#include <linux/vt.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "SDL_fbvideo.h"
#include "SDL_fbmouse_c.h"
#include "SDL_fbevents_c.h"

#define min(a,b) ((a)<(b)?(a):(b))

/* Initialization/Query functions */
static int DISPMANX_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DISPMANX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DISPMANX_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DISPMANX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void DISPMANX_VideoQuit(_THIS);

/* Hardware surface functions */
static int DISPMANX_InitHWSurfaces(_THIS, SDL_Surface *screen, char *base, int size);
static void DISPMANX_FreeHWSurfaces(_THIS);
static int DISPMANX_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DISPMANX_LockHWSurface(_THIS, SDL_Surface *surface);
static void DISPMANX_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DISPMANX_FreeHWSurface(_THIS, SDL_Surface *surface);
static void DISPMANX_WaitVBL(_THIS);
static void DISPMANX_WaitIdle(_THIS);
static int DISPMANX_FlipHWSurface(_THIS, SDL_Surface *surface);
static void DISPMANX_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);

//MAC Variables para la inicialización del buffer
uint32_t src_width, src_height, src_offsetx, src_offsety;
int flip_page = 0;

typedef struct {
    DISPMANX_DISPLAY_HANDLE_T   display;
    DISPMANX_MODEINFO_T         amode;
    void                       *pixmem;
    DISPMANX_UPDATE_HANDLE_T    update;
    DISPMANX_RESOURCE_HANDLE_T  resources[2];
    DISPMANX_ELEMENT_HANDLE_T   element;
    VC_IMAGE_TYPE_T 		pix_format;
    uint32_t                    vc_image_ptr;
    VC_DISPMANX_ALPHA_T	       *alpha;    
    VC_RECT_T       src_rect;
    VC_RECT_T 	    dst_rect; 
    VC_RECT_T	    bmp_rect;
    int bits_per_pixel;
    int pitch;	
} __DISPMAN_VARIABLES_T;

static __DISPMAN_VARIABLES_T _DISPMAN_VARS;
static __DISPMAN_VARIABLES_T *dispvars = &_DISPMAN_VARS;

/* FB driver bootstrap functions */

static int DISPMANX_Available(void)
{
	//MAC Hacemos que retorme siempre true. Buena gana de comprobar esto.
	return (1);
}

static void DISPMANX_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *DISPMANX_CreateDevice(int devindex)
{
	//MAC Esta función no toca nada del framebuffer sino que 
	//sólo inicializa la memoria interna de lo que en SDL es una
	//abstracción del dispositivo.
	SDL_VideoDevice *this;

	/* Initialize all variables that we clean on shutdown */
	this = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
	if ( this ) {
		SDL_memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateVideoData *)
				SDL_malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		SDL_OutOfMemory();
		if ( this ) {
			SDL_free(this);
		}
		return(0);
	}
	SDL_memset(this->hidden, 0, (sizeof *this->hidden));
	wait_vbl = DISPMANX_WaitVBL;
	wait_idle = DISPMANX_WaitIdle;
	mouse_fd = -1;
	keyboard_fd = -1;

	/* Set the function pointers */
	this->VideoInit = DISPMANX_VideoInit;
	this->ListModes = DISPMANX_ListModes;
	this->SetVideoMode = DISPMANX_SetVideoMode;
	this->SetColors = DISPMANX_SetColors;
	this->UpdateRects = DISPMANX_DirectUpdate;
	this->VideoQuit = DISPMANX_VideoQuit;
	this->AllocHWSurface = DISPMANX_AllocHWSurface;
	this->CheckHWBlit = NULL;
	this->FillHWRect = NULL;
	this->SetHWColorKey = NULL;
	this->SetHWAlpha = NULL;
	this->LockHWSurface = DISPMANX_LockHWSurface;
	this->UnlockHWSurface = DISPMANX_UnlockHWSurface;
	this->FlipHWSurface = DISPMANX_FlipHWSurface;
	this->FreeHWSurface = DISPMANX_FreeHWSurface;
	this->SetCaption = NULL;
	this->SetIcon = NULL;
	this->IconifyWindow = NULL;
	this->GrabInput = NULL;
	this->GetWMInfo = NULL;
	this->InitOSKeymap = DISPMANX_InitOSKeymap;
	this->PumpEvents = DISPMANX_PumpEvents;
	this->CreateYUVOverlay = NULL;	

	this->free = DISPMANX_DeleteDevice;

	return this;
}

VideoBootStrap DISPMANX_bootstrap = {
	"dispmanx", "Dispmanx Raspberry Pi VC",
	DISPMANX_Available, DISPMANX_CreateDevice
};

static int DISPMANX_AddMode(_THIS, int index, unsigned int w, unsigned int h, int check_timings)
{
	SDL_Rect *mode;
	int next_mode;

	// Check to see if we already have this mode
	if ( SDL_nummodes[index] > 0 ) {
		mode = SDL_modelist[index][SDL_nummodes[index]-1];
		if ( (mode->w == w) && (mode->h == h) ) {
			printf("\nWe already have mode %dx%d at %d bytes per pixel\n", w, h, index+1);
			return(0);
		}
	}

	// MAC Aquí comprobábamos si teníamos un modo en la lista de VESA timmings. Ya no lo comprobamos.
	/*if ( check_timings ) {
		int found_timing = 0;
		for ( i=0; i<(sizeof(vesa_timings)/sizeof(vesa_timings[0])); ++i ) {
			if ( (w == vesa_timings[i].xres) &&
			     (h == vesa_timings[i].yres) && vesa_timings[i].pixclock ) {
				found_timing = 1;
				break;
			}
		}
		if ( !found_timing ) {
			printf("\nNo valid timing line for mode %dx%d\n", w, h);
			return(0);
		}
	}*/

	//Set up the new video mode rectangle 
	mode = (SDL_Rect *)SDL_malloc(sizeof *mode);
	if ( mode == NULL ) {
		SDL_OutOfMemory();
		return(-1);
	}
	mode->x = 0;
	mode->y = 0;
	mode->w = w;
	mode->h = h;
	
	// Allocate the new list of modes, and fill in the new mode
	next_mode = SDL_nummodes[index];
	SDL_modelist[index] = (SDL_Rect **)
	       SDL_realloc(SDL_modelist[index], (1+next_mode+1)*sizeof(SDL_Rect *));
	if ( SDL_modelist[index] == NULL ) {
		SDL_OutOfMemory();
		SDL_nummodes[index] = 0;
		SDL_free(mode);
		return(-1);
	}
	SDL_modelist[index][next_mode] = mode;
	SDL_modelist[index][next_mode+1] = NULL;
	SDL_nummodes[index]++;

	return(0);
}

/*static int cmpmodes(const void *va, const void *vb)
{
    const SDL_Rect *a = *(const SDL_Rect**)va;
    const SDL_Rect *b = *(const SDL_Rect**)vb;
    if ( a->h == b->h )
        return b->w - a->w;
    else
        return b->h - a->h;
}*/

 
/*static void DISPMANX_SortModes(_THIS)
{
	int i;
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_nummodes[i] > 0 ) {
			SDL_qsort(SDL_modelist[i], SDL_nummodes[i], sizeof *SDL_modelist[i], cmpmodes);
		}
	}
}*/

static int DISPMANX_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i;
	int ret = 0;
#if !SDL_THREADS_DISABLED
	/* Create the hardware surface lock mutex */
	hw_lock = SDL_CreateMutex();
	if ( hw_lock == NULL ) {
		SDL_SetError("Unable to create lock mutex");
		DISPMANX_VideoQuit(this);
		return(-1);
	}
#endif
	//MAC Inicializamos el SOC
	bcm_host_init();
		
	//MAC Abrimos el display dispmanx
	uint32_t screen = 0;
	printf("dispmanx: Opening display[%i]...\n", screen );
        dispvars->display = vc_dispmanx_display_open( screen );
	
	//MAC Recuperamos algunos datos de la configuración del buffer actual
	vc_dispmanx_display_get_info( dispvars->display, &(dispvars->amode));
	assert(ret == 0);
	vformat->BitsPerPixel = 16; //Pon lo que quieras.Era para restaurar fb
	
	//MAC Para que las funciones GetVideoInfo() devuelvan un SDL_VideoInfo con contenidos.
	this->info.current_w = dispvars->amode.width;
        this->info.current_h = dispvars->amode.height;
        this->info.wm_available = 0;
        this->info.hw_available = 1;
	this->info.video_mem = 32768 /1024;
		
	printf( "Physical video mode is %d x %d\n", 
	   dispvars->amode.width, dispvars->amode.height );
	
	/* Limpiamos LAS listas de modos disponibles */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_nummodes[i] = 0;
		SDL_modelist[i] = NULL;
	}	
	
	// Añadimos nuestros modos de vídeo	
	
	//En DISPMANX sólo tenemos el modo de vídeo que se está usando 
	//actualmente, sea cual sea, y los demás modos se escalan a ese 
	//por hardware. SDL NO entiende de timings (incluyendo tasas de 
	//refresco), así que eso no se tiene que resolver aquí, supongo.

	for (i = 0; i < NUM_MODELISTS; i++){
              //Añado cada modo a la lista 0 (8bpp), lista 1 (16), lista 2(24)..
              //Por eso itero hasta NUM_MODELIST: cada MODELIST es para un bpp.
              DISPMANX_AddMode(this, i, dispvars->amode.width, 
	          dispvars->amode.height, 0);
              printf("Adding video mode: %d x %d - %d bpp\n", dispvars->amode.width,
                 dispvars->amode.height, (i+1)*8);
        }

	/* Enable mouse and keyboard support */
	if ( DISPMANX_OpenKeyboard(this) < 0 ) {
		DISPMANX_VideoQuit(this);
		return(-1);
	}
	if ( DISPMANX_OpenMouse(this) < 0 ) {
		const char *sdl_nomouse;
		//MAC Si esto da problemas, es por los premisos de gpm sobre
		//el ratón en /dev/mice. Edita /etc/init.d/gpm y añade
		//en la sección start() la línea chmod 0666{MOUSEDEV}
		sdl_nomouse = SDL_getenv("SDL_NOMOUSE");
		if ( ! sdl_nomouse ) {
			printf("\nERR - Couldn't open mouse. Look for permissions in /etc/init.d/gpm.\n");
			DISPMANX_VideoQuit(this);
			return(-1);
		}
	}

	/* We're done! */
	return(0);
}

static SDL_Rect **DISPMANX_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

static SDL_Surface *DISPMANX_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
//MAC Recuerda que aquí ya nos llegan las dimensiones de un modo de vídeo
// aproximado en SDL_Video.c de entre los modos de vídeo disponibles. 
//La jugada es dejar que un modo de 320x200 se vea en el modo que se nos ha 
//aproximado, seguramente mucho mayor, y luego escalar por hardware el área 
//visible al modo aproximado (de ahí que creemos el plano de overlay a parte, 
//ya que cuando SDL_Video.c llama a SetVideoMode aún no se tienen listos los 
//offsets horizontal y vertical donde empieza el modo de vídeo pequeño 
//(el original, ideal) sobre el grande (que sería el aproximado).
//Otra cosa es la tasa de refresco. Tendrás que usar modos físicos 
//concretos (config.txt) para ajustarte a 50, 60 o 70 Hz.
	
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	char *surfaces_mem; 
	int surfaces_len;
		
	dispvars->bits_per_pixel = bpp;	
	
	switch (bpp){
	   case 8:
		dispvars->pix_format = VC_IMAGE_8BPP;	       
		break;

	   case 16:
		dispvars->pix_format = VC_IMAGE_RGB565;	       
		break;

	   case 32:
		dispvars->pix_format = VC_IMAGE_XRGB8888;	       
	        break;
           
           default:
	      printf ("\nERR - wrong bpp: %d\n",bpp);
	      return (NULL);
	}	
    	
	//MAC blah 
	this->UpdateRects = DISPMANX_DirectUpdate;

	//MAC Establecemos los rects para el escalado
	//this->offset_x = (dispvars->amode.width - width)/2;
	//this->offset_y = (dispvars->amode.height - height)/2;
	
	printf ("\nUsing internal program mode: width=%d height=%d\n", 
		width, height);	

	//MAC Por ahora en DISPMANX usamos el mismo modo q ya está establecido
	printf ("\nUsing physical mode: %d x %d %d bpp\n",
		dispvars->amode.width, dispvars->amode.height,
		dispvars->bits_per_pixel);
	
	//MAC Establecemos el pitch en fn del bpp deseado	
    	//Lo alineamos a 32 porque es el aligment interno de dispmanx(en ejemp)
	dispvars->pitch = ( ALIGN_UP( width, 16 ) * (bpp/8) );
	//Alineamos la atura a 16 por el mismo motivo (ver ejemplo hello_disp)
	height = ALIGN_UP( height, 16);
	
	//Dejamos configurados los rects
	vc_dispmanx_rect_set (&(dispvars->bmp_rect), 0, 0, 
	   width, height);	
	
	vc_dispmanx_rect_set (&(dispvars->src_rect), 0, 0, 
	   width << 16, height << 16);	

	vc_dispmanx_rect_set( &(dispvars->dst_rect), 0, 0, 
	   dispvars->amode.width , dispvars->amode.height );
	
	//MAC Establecemos alpha. Para transparencia descomentar flags con or.
	VC_DISPMANX_ALPHA_T layerAlpha;
	/*layerAlpha.flags = (DISPMANX_FLAGS_ALPHA_FROM_SOURCE | 
           DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS);*/
	layerAlpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	layerAlpha.opacity = 255;
	layerAlpha.mask	   = 0;
	dispvars->alpha = &layerAlpha;
	
	//MAC Creo los resources. Me hacen falta dos para el double buffering
	dispvars->resources[0] = vc_dispmanx_resource_create( 
	   dispvars->pix_format, width, height, 
	   &(dispvars->vc_image_ptr) );
    	assert (dispvars->resources[0]);
    	
	dispvars->resources[1] = vc_dispmanx_resource_create( 
	   dispvars->pix_format, width, height,
	   &(dispvars->vc_image_ptr) );
    	assert (dispvars->resources[1]);
	
	//Reservo memoria para el array de pixles en RAM 
    	dispvars->pixmem=calloc( 1, dispvars->pitch * height);
    	//dispvars->pixmem=malloc ( dispvars->pitch * dispvars->amode.height );
	
	//MAC Esto se usa, como mínimo y que yo sepa, para DirectUpdate
	//cache_modinfo = *modinfo;	
	//cache_fbinfo  = *(drmModeGetFB (fd, fb_id));	
	
	//MAC Esta llamada a ReallocFormat es lo que impedía ver algo...
	Rmask = 0;
	Gmask = 0;
	Bmask = 0;
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0) ) {
		return(NULL);
	}
	
	//Preparamos SDL para trabajar sobre el nuevo framebuffer
	shadow_fb = 0;
	current->flags |= SDL_HWSURFACE;
	current->flags |= SDL_FULLSCREEN;
	current->flags |= SDL_HWPALETTE;
	if (flags & SDL_DOUBLEBUF)
	   current->flags |= SDL_DOUBLEBUF;	

	current->w = width;
	current->h = height;

	mapped_mem    = dispvars->pixmem;
	mapped_memlen =  (dispvars->pitch * height); 
	current->pitch  = dispvars->pitch;
	current->pixels = mapped_mem;
	
	/* Set up the information for hardware surfaces */
	surfaces_mem = (char *)current->pixels +
		(dispvars->pitch * height);
	surfaces_len = (mapped_memlen-(surfaces_mem-mapped_mem));
		
	DISPMANX_FreeHWSurfaces(this);
	DISPMANX_InitHWSurfaces(this, current, surfaces_mem, surfaces_len);
	
	/* Update for double-buffering, if we can */
	//Recuerda que necesitamos dos buffers porque hay que mantener uno
	//sin tocar durante todo el ciclo de fotograma (16ms) ya que es el que
	//el monitor lee como scanout durante ese tiempo.
	
	this->screen = current;
	this->screen = NULL;

	//Colocamos el element. Alpha 0 estabiliza %uso de cpu
	dispvars->update = vc_dispmanx_update_start( 0 );
	
	dispvars->element = vc_dispmanx_element_add( dispvars->update, 
	   dispvars->display, 0 /*layer*/, &(dispvars->dst_rect), 	   
	   dispvars->resources[flip_page], &(dispvars->src_rect), 
	   DISPMANX_PROTECTION_NONE, dispvars->alpha, 0 /*clamp*/, 
	   /*VC_IMAGE_ROT0*/ 0 );
	
	vc_dispmanx_update_submit_sync( dispvars->update );		
	
	/* We're done */
	//MAC Disable graphics 1
	// Set the terminal into graphics mode 
        printf ("\nDISPMANX_SetVideoMode activating keyboard and exiting");
	if ( DISPMANX_EnterGraphicsMode(this) < 0 )
        	return(NULL);
	
	return(current);
}

static int DISPMANX_InitHWSurfaces(_THIS, SDL_Surface *screen, char *base, int size)
{
	vidmem_bucket *bucket;

	surfaces_memtotal = size;
	surfaces_memleft = size;

	if ( surfaces_memleft > 0 ) {
		bucket = (vidmem_bucket *)SDL_malloc(sizeof(*bucket));
		if ( bucket == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		bucket->prev = &surfaces;
		bucket->used = 0;
		bucket->dirty = 0;
		bucket->base = base;
		bucket->size = size;
		bucket->next = NULL;
	} else {
		bucket = NULL;
	}

	surfaces.prev = NULL;
	surfaces.used = 1;
	surfaces.dirty = 0;
	surfaces.base = screen->pixels;
	surfaces.size = (unsigned int)((long)base - (long)surfaces.base);
	surfaces.next = bucket;
	screen->hwdata = (struct private_hwdata *)&surfaces;
	return(0);
}
static void DISPMANX_FreeHWSurfaces(_THIS)
{
	vidmem_bucket *bucket, *freeable;

	bucket = surfaces.next;
	while ( bucket ) {
		freeable = bucket;
		bucket = bucket->next;
		SDL_free(freeable);
	}
	surfaces.next = NULL;
}

static int DISPMANX_AllocHWSurface(_THIS, SDL_Surface *surface)
{
	vidmem_bucket *bucket;
	int size;
	int extra;

/* Temporarily, we only allow surfaces the same width as display.
   Some blitters require the pitch between two hardware surfaces
   to be the same.  Others have interesting alignment restrictions.
   Until someone who knows these details looks at the code...
*/
if ( surface->pitch > SDL_VideoSurface->pitch ) {
	SDL_SetError("Surface requested wider than screen");
	return(-1);
}
surface->pitch = SDL_VideoSurface->pitch;
	size = surface->h * surface->pitch;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Allocating bucket of %d bytes\n", size);
#endif

	/* Quick check for available mem */
	if ( size > surfaces_memleft ) {
		SDL_SetError("Not enough video memory");
		return(-1);
	}

	/* Search for an empty bucket big enough */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		if ( ! bucket->used && (size <= bucket->size) ) {
			break;
		}
	}
	if ( bucket == NULL ) {
		SDL_SetError("Video memory too fragmented");
		return(-1);
	}

	/* Create a new bucket for left-over memory */
	extra = (bucket->size - size);
	if ( extra ) {
		vidmem_bucket *newbucket;

#ifdef FBCON_DEBUG
	fprintf(stderr, "Adding new free bucket of %d bytes\n", extra);
#endif
		newbucket = (vidmem_bucket *)SDL_malloc(sizeof(*newbucket));
		if ( newbucket == NULL ) {
			SDL_OutOfMemory();
			return(-1);
		}
		newbucket->prev = bucket;
		newbucket->used = 0;
		newbucket->base = bucket->base+size;
		newbucket->size = extra;
		newbucket->next = bucket->next;
		if ( bucket->next ) {
			bucket->next->prev = newbucket;
		}
		bucket->next = newbucket;
	}

	/* Set the current bucket values and return it! */
	bucket->used = 1;
	bucket->size = size;
	bucket->dirty = 0;
#ifdef FBCON_DEBUG
	fprintf(stderr, "Allocated %d bytes at %p\n", bucket->size, bucket->base);
#endif
	surfaces_memleft -= size;
	surface->flags |= SDL_HWSURFACE;
	surface->pixels = bucket->base;
	surface->hwdata = (struct private_hwdata *)bucket;
	return(0);
}
static void DISPMANX_FreeHWSurface(_THIS, SDL_Surface *surface)
{
	vidmem_bucket *bucket, *freeable;

	/* Look for the bucket in the current list */
	for ( bucket=&surfaces; bucket; bucket=bucket->next ) {
		if ( bucket == (vidmem_bucket *)surface->hwdata ) {
			break;
		}
	}
	if ( bucket && bucket->used ) {
		/* Add the memory back to the total */
#ifdef DGA_DEBUG
	printf("Freeing bucket of %d bytes\n", bucket->size);
#endif
		surfaces_memleft += bucket->size;

		/* Can we merge the space with surrounding buckets? */
		bucket->used = 0;
		if ( bucket->next && ! bucket->next->used ) {
#ifdef DGA_DEBUG
	printf("Merging with next bucket, for %d total bytes\n", bucket->size+bucket->next->size);
#endif
			freeable = bucket->next;
			bucket->size += bucket->next->size;
			bucket->next = bucket->next->next;
			if ( bucket->next ) {
				bucket->next->prev = bucket;
			}
			SDL_free(freeable);
		}
		if ( bucket->prev && ! bucket->prev->used ) {
#ifdef DGA_DEBUG
	printf("Merging with previous bucket, for %d total bytes\n", bucket->prev->size+bucket->size);
#endif
			freeable = bucket;
			bucket->prev->size += bucket->size;
			bucket->prev->next = bucket->next;
			if ( bucket->next ) {
				bucket->next->prev = bucket->prev;
			}
			SDL_free(freeable);
		}
	}
	surface->pixels = NULL;
	surface->hwdata = NULL;
}

static int DISPMANX_LockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( switched_away ) {
		return -2; /* no hardware access */
	}
	if ( surface == this->screen ) {
		SDL_mutexP(hw_lock);
		if ( DISPMANX_IsSurfaceBusy(surface) ) {
			DISPMANX_WaitBusySurfaces(this);
		}
	} else {
		if ( DISPMANX_IsSurfaceBusy(surface) ) {
			DISPMANX_WaitBusySurfaces(this);
		}
	}
	return(0);
}
static void DISPMANX_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( surface == this->screen ) {
		SDL_mutexV(hw_lock);
	}
}

static void DISPMANX_WaitVBL(_THIS)
{

//MAC Sacado de /usr/include/libdrm/drm.h 
//ioctl(fd, DRM_IOCTL_WAIT_VBLANK, 0);
	return;
}

static void DISPMANX_WaitIdle(_THIS)
{
	return;
}

static int DISPMANX_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	/*if ( switched_away ) {
		return -2; // no hardware access
	}*/
	
	//Volcamos desde el ram bitmap buffer al dispmanx resource buffer
	//que toque. cada vez a uno.	
	vc_dispmanx_resource_write_data( dispvars->resources[flip_page], 
	   dispvars->pix_format, dispvars->pitch, dispvars->pixmem, 
	   &(dispvars->bmp_rect) );

	//**Empieza actualización***
	dispvars->update = vc_dispmanx_update_start( 0 );

	vc_dispmanx_element_change_source(dispvars->update, 
	   dispvars->element, dispvars->resources[flip_page]);

	vc_dispmanx_update_submit_sync( dispvars->update );		
	//vc_dispmanx_update_submit(dispvars->update, NULL, NULL); 
	//**Acaba actualización***
	flip_page = !flip_page;
	
	//MAC Esto no hace falta porque SDL siempre escribe en dispvars->pixmem, 
	//que es el buffer en RAM y que copiamos cada vez a un resource.
	//surface->pixels = flip_address[flip_page];
	
	return (0);
}

#define BLOCKSIZE_W 32
#define BLOCKSIZE_H 32

static void DISPMANX_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{	
	//Esto es una solución temporal! no puedo volcar el bitmap buffer 
	//entero cada vez que quiero actualizar un área de pantalla!
	//En resoluciones internas bajas no se nota, pero en altas... 	
	vc_dispmanx_resource_write_data( dispvars->resources[flip_page], 
	   dispvars->pix_format, dispvars->pitch, dispvars->pixmem, 
	   &(dispvars->bmp_rect) );
	
	return;
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/

static int DISPMANX_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	return (1);
}

static void DISPMANX_VideoQuit(_THIS)
{
	int i,j;
		
	if ( this->screen ) {
	   /* Clear screen and tell SDL not to free the pixels */
	   const char *dontClearPixels = SDL_getenv("SDL_FBCON_DONT_CLEAR");
	      //En este caso sí tenemos que limpiar el framebuffer	
	      if ( !dontClearPixels && this->screen->pixels 
	      && DISPMANX_InGraphicsMode(this) ) {
#if defined(__powerpc__) || defined(__ia64__)	
	         /* SIGBUS when using SDL_memset() ?? */
		 Uint8 *rowp = (Uint8 *)this->screen->pixels;
		 int left = this->screen->pitch*this->screen->h;
		 while ( left-- ) { *rowp++ = 0; }
#else
		 SDL_memset(this->screen->pixels,0,
		 this->screen->h*this->screen->pitch);
#endif
	      }
		
	      if ( ((char *)this->screen->pixels >= mapped_mem) &&
	         ( (char *)this->screen->pixels < (mapped_mem+mapped_memlen)) ) 
		    this->screen->pixels = NULL;
		 
	}
	
	/* Clear the lock mutex */
	if ( hw_lock ) {
		SDL_DestroyMutex(hw_lock);
		hw_lock = NULL;
	}

	/* Clean up defined video modes */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		if ( SDL_modelist[i] != NULL ) {
			for ( j=0; SDL_modelist[i][j]; ++j ) {
				SDL_free(SDL_modelist[i][j]);
			}
			SDL_free(SDL_modelist[i]);
			SDL_modelist[i] = NULL;
		}
	}

	/* Clean up the memory bucket list */
	DISPMANX_FreeHWSurfaces(this);

	/* Unmap the video framebuffer and I/O registers */
	if ( mapped_mem ) {
		munmap(mapped_mem, mapped_memlen);
		mapped_mem = NULL;
	}
	if ( mapped_io ) {
		munmap(mapped_io, mapped_iolen);
		mapped_io = NULL;
	}
		
	//MAC liberamos lo relacionado con dispmanx
	printf ("\nDeleting dispmanx elements;\n");
	dispvars->update = vc_dispmanx_update_start( 0 );
    	assert( dispvars->update );
    	vc_dispmanx_element_remove(dispvars->update, dispvars->element);
    	vc_dispmanx_resource_delete( dispvars->resources[0] );
    	vc_dispmanx_resource_delete( dispvars->resources[1] );
	vc_dispmanx_display_close( dispvars->display );
	vc_dispmanx_update_submit_sync( dispvars->update );		
	bcm_host_deinit();

	DISPMANX_CloseMouse(this);
	DISPMANX_CloseKeyboard(this);
	exit (0);
}
