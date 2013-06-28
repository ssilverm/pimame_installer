/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga
    
    Sam Lantinga
    slouken@libsdl.org
    
    SDL KMS driver/backend
    Copyright (C) 2012 Manuel Alfayate 
*/
#include "SDL_config.h"

/* KMS framebuffer SDL video driver implementation.
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#ifndef HAVE_GETPAGESIZE
#include <asm/page.h>		/* For definition of PAGE_SIZE */
#endif

#include <linux/vt.h>

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "SDL_fbvideo.h"
#include "SDL_fbmouse_c.h"
#include "SDL_fbevents_c.h"
#include "hdmi_modes.h"

//MAC Un ejemplo cojonudo de cómo definir una función que vale para distintos
//tipos de parámetros definidos por el usuario, en este caso tipos 
//estructurados. Lo que le pasamos en (res) se copia en res##, asi que si le 
//pasamos connector_type, una de las funciones que se crearán será 
//"char * connector_type_str()"
 
#define type_name_fn(res) \
char * res##_str(int type) {			\
	int i;						\
	for (i = 0; i < sizeof(res##_names); i++) { \
		if (res##_names[i].type == type)	\
			return res##_names[i].name;	\
	}						\
	return "(invalid)";				\
}

struct type_name {
	int type;
	char *name;
};

struct type_name connector_type_names[] = {
	{ DRM_MODE_CONNECTOR_Unknown, "unknown" },
	{ DRM_MODE_CONNECTOR_VGA, "VGA" },
	{ DRM_MODE_CONNECTOR_DVII, "DVI-I" },
	{ DRM_MODE_CONNECTOR_DVID, "DVI-D" },
	{ DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
	{ DRM_MODE_CONNECTOR_Composite, "composite" },
	{ DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
	{ DRM_MODE_CONNECTOR_LVDS, "LVDS" },
	{ DRM_MODE_CONNECTOR_Component, "component" },
	{ DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
	{ DRM_MODE_CONNECTOR_DisplayPort, "displayport" },
	{ DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
	{ DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
	{ DRM_MODE_CONNECTOR_TV, "TV" },
	{ DRM_MODE_CONNECTOR_eDP, "embedded displayport" },
};

type_name_fn(connector_type)

struct type_name encoder_type_names[] = {
	{ DRM_MODE_ENCODER_NONE, "none" },
	{ DRM_MODE_ENCODER_DAC, "DAC" },
	{ DRM_MODE_ENCODER_TMDS, "TMDS" },
	{ DRM_MODE_ENCODER_LVDS, "LVDS" },
	{ DRM_MODE_ENCODER_TVDAC, "TVDAC" },
};

type_name_fn(encoder_type)

#define min(a,b) ((a)<(b)?(a):(b))

/* Initialization/Query functions */
static int KMS_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **KMS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *KMS_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int KMS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void KMS_VideoQuit(_THIS);

/* Hardware surface functions */
static int KMS_InitHWSurfaces(_THIS, SDL_Surface *screen, char *base, int size);
static void KMS_FreeHWSurfaces(_THIS);
static int KMS_AllocHWSurface(_THIS, SDL_Surface *surface);
static int KMS_LockHWSurface(_THIS, SDL_Surface *surface);
static void KMS_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void KMS_FreeHWSurface(_THIS, SDL_Surface *surface);
static void KMS_WaitVBL(_THIS);
static void KMS_WaitIdle(_THIS);
static int KMS_FlipHWSurface(_THIS, SDL_Surface *surface);

//MAC Variables para la inicialización del buffer
int fd, modes;
fd_set fds;
drmModeConnector *connector;
drmModeEncoder* encoder;
drmModeRes *resources;
drmModeModeInfo modinfo;
drmEventContext evctx;
drmModePlaneRes *plane_resources;
drmModePlane *ovr;

uint32_t fb_id[2];
uint32_t plane_id = 0;
uint32_t pixel_format = 0;
int waiting_for_vblank;
uint32_t src_width, src_height, src_offsetx, src_offsety;
int flip_page = 0;
char *mapped_vmem[2];

struct kms_bo *bo[2];
struct kms_driver *kms;

/* FB driver bootstrap functions */

static int KMS_Available(void)
{
	//MAC Hacems que retorme siempre true. Buena gana de comprobar esto.
	return (1);
}

static void KMS_DeleteDevice(SDL_VideoDevice *device)
{
	SDL_free(device->hidden);
	SDL_free(device);
}

static SDL_VideoDevice *KMS_CreateDevice(int devindex)
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
	wait_vbl = KMS_WaitVBL;
	wait_idle = KMS_WaitIdle;
	mouse_fd = -1;
	keyboard_fd = -1;

	/* Set the function pointers */
	this->VideoInit = KMS_VideoInit;
	this->ListModes = KMS_ListModes;
	this->SetVideoMode = KMS_SetVideoMode;
	this->SetColors = KMS_SetColors;
	this->UpdateRects = NULL;
	this->VideoQuit = KMS_VideoQuit;
	this->AllocHWSurface = KMS_AllocHWSurface;
	this->CheckHWBlit = NULL;
	this->FillHWRect = NULL;
	this->SetHWColorKey = NULL;
	this->SetHWAlpha = NULL;
	this->LockHWSurface = KMS_LockHWSurface;
	this->UnlockHWSurface = KMS_UnlockHWSurface;
	this->FlipHWSurface = KMS_FlipHWSurface;
	this->FreeHWSurface = KMS_FreeHWSurface;
	this->SetCaption = NULL;
	this->SetIcon = NULL;
	this->IconifyWindow = NULL;
	this->GrabInput = NULL;
	this->GetWMInfo = NULL;
	this->InitOSKeymap = KMS_InitOSKeymap;
	this->PumpEvents = KMS_PumpEvents;
	this->CreateYUVOverlay = NULL;	

	this->free = KMS_DeleteDevice;

	return this;
}

VideoBootStrap KMS_bootstrap = {
	"kms", "Linux Kernel Modesetting",
	KMS_Available, KMS_CreateDevice
};

static int KMS_AddMode(_THIS, int index, unsigned int w, unsigned int h, int check_timings)
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

static int KMS_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	int i,j;

	char *modules[] = { "omapdrm", "i915", "nouveau", "vmwgfx", "radeon", "exynos" };

	//MAC Abrimos el dispositivo y obtenemos el file descriptor. Hay dónde elegir!
	for (i = 0; i < (sizeof(modules)/sizeof(modules[0])); i++) {
		fd = drmOpen(modules[i], NULL);
		if (fd >= 0) {
			printf("\nCargado con éxito módulo DRM para %s",modules[i]);
			break;
		}
	}	
	
	i = kms_create(fd, &kms);
	if (i) {
                printf("\nfailed to create kms driver\n");
                exit(0);
        }	
	
	//MAC Adquirimos resources		
	resources = drmModeGetResources(fd);
   	if (!resources) printf ("\nHa fallado drmModeGetResources()");	
	
	//Adquirimos conector activo. LVDS vale 7.
	for (i = 0; i < resources->count_connectors; i++){
           connector = drmModeGetConnector(fd, resources->connectors[i]);
           if (connector->connection == DRM_MODE_CONNECTED) {
	      printf ("\nOK - Usando conector %s",
              connector_type_str(connector->connector_type) );
              break;
           }
           drmModeFreeConnector (connector);
	}
       
	if (i == resources->count_connectors){
           printf ("\nERR - No conectores conectados\n");
	   return (-1);	
	}
	
	//Adquirimos el encoder conectado a ese conector
   	for( i = 0; i < resources->count_encoders; i++)
   	{
      		encoder = drmModeGetEncoder(fd,resources->encoders[i]);
      		if(encoder==NULL) { continue; }
      		if( encoder->encoder_id == connector->encoder_id ) {
           	   printf ("\nOK - Usando encoder de tipo %s\n", 
		   encoder_type_str(encoder->encoder_type));
		   break;
		}
        	drmModeFreeEncoder(encoder);
	}
        
	/* Limpiamos listas de modos disponibles y añadimos los modos desde la lista de modos*/
	/* Esto se puede hacer mejor añadiendo los modos desde los modos del conector        */
	for ( i=0; i<NUM_MODELISTS; ++i ) {
		SDL_nummodes[i] = 0;
		SDL_modelist[i] = NULL;
	}	
	
	for (i = 0; i < drm_num_cea_modes; i++){
	   for (j = 0; j < NUM_MODELISTS; j++){
	      //Añado cada modo a la lista 0 (8bpp), lista 1 (16), lista 2(24)..
	      KMS_AddMode(this, j, edid_cea_modes[i].hdisplay, 
	         edid_cea_modes[i].vdisplay, 0);
	      
	      printf("Añadiendo modo %d x %d - %d bpp\n", 
	         edid_cea_modes[i].hdisplay, edid_cea_modes[i].vdisplay,
	         (j+1)*8);
	   }
	}
	
	//MAC Para que las funciones GetVideoInfo() devuelvan un SDL_VideoInfo con contenidos.
        //NOTA: Los valores de current_h y current_w están puestos a fuego pero esto ESTÁ MAL.
	//Arréglalo cuando retomes este backend!!!
	this->info.current_w = 1920;
        this->info.current_h = 1080;
        this->info.wm_available = 0;
        this->info.hw_available = 1;
        this->info.video_mem = 32768 /1024;	
	this->info.video_mem = 32768 /1024;

	/* Enable mouse and keyboard support */
	if ( KMS_OpenKeyboard(this) < 0 ) {
		KMS_VideoQuit(this);
		return(-1);
	}
	if ( KMS_OpenMouse(this) < 0 ) {
		const char *sdl_nomouse;
		//MAC Si esto da problemas, es por los premisos de gpm sobre
		//el ratón en /dev/mice. Edita /etc/init.d/gpm y añade
		//en la sección start() la línea chmod 0666{MOUSEDEV}
		sdl_nomouse = SDL_getenv("SDL_NOMOUSE");
		if ( ! sdl_nomouse ) {
			printf("\nERR - No se pudo abrir mouse.\
			   Mira los permisos en /etc/init.d/gpm.\n");
			KMS_VideoQuit(this);
			return(-1);
		}
	}

#if !SDL_THREADS_DISABLED
	/* Create the hardware surface lock mutex */
	hw_lock = SDL_CreateMutex();
	if ( hw_lock == NULL ) {
		SDL_SetError("Unable to create lock mutex");
		KMS_VideoQuit(this);
		return(-1);
	}
#endif
	
	vformat->BitsPerPixel = 32;
	
	/* We're done! */
	printf ("\nKMS_VideoInit retorna correctamente");
	return(0);
}

static SDL_Rect **KMS_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
	return(SDL_modelist[((format->BitsPerPixel+7)/8)-1]);
}

/* Various screen update functions available */
static void KMS_DirectUpdate(_THIS, int numrects, SDL_Rect *rects);



static int complete_mode (drmModeModeInfo *modinfo)
{
	//MAC En esta función completamos los datos de los modos desde
	//los modos que trae el conector que estamos usando.
	int matched;
	int i;

	matched = 0;
	
	//MAC Primero imprimimos todos los modos
	/*printf ("\nLISTANDO MODOS DEL CONECTOR:");
	for (i = 0; i < connector->count_modes; i++){
		printf ("\nhdisplay = %d vdisplay = %d vrefresh = %d\
		flags = %d",
		connector->modes[i].hdisplay,
		connector->modes[i].vdisplay,
		connector->modes[i].vrefresh,
		connector->modes[i].flags);
	}*/
	for (i = 0; i < drm_num_cea_modes; i++){
		if (edid_cea_modes[i].hdisplay == modinfo->hdisplay &&
		    edid_cea_modes[i].vdisplay == modinfo->vdisplay){
			matched = 1;
			drm_crtc_convert_to_umode ((struct drm_mode_modeinfo *)modinfo, 
			&(edid_cea_modes[i]) );
		}				
	}	
	
	return(matched);

}


static struct kms_bo *
KMS_allocate_buffer(int width, int height, int *stride)
{
        struct kms_bo *bo;
        unsigned bo_attribs[] = {
                KMS_WIDTH,   0,
                KMS_HEIGHT,  0,
                KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
                KMS_TERMINATE_PROP_LIST
        };
        int ret;
        bo_attribs[1] = width;
        bo_attribs[3] = height;

        ret = kms_bo_create(kms, bo_attribs, &bo);
        if (ret) {
           printf("\nERR - failed to alloc buffer with %dx%d\n", width,height);
           return NULL;
        }

        ret = kms_bo_get_prop(bo, KMS_PITCH, (unsigned*)stride);
        if (ret) {
           printf("\nfailed to retreive buffer stride\n");
           kms_bo_destroy(&bo);
           return NULL;
        }

        return bo;
}

static SDL_Surface *KMS_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
	int i,ret;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	char *surfaces_mem;
	int surfaces_len;
	int stride;		
	uint32_t handles[4], pitches[4], offsets[4] = {0};

	unsigned handle;
	
	//Se fuerzan los flags para pantalla completa. 
	shadow_fb = 0;
	current->flags |= SDL_FULLSCREEN;
	current->flags |= SDL_HWSURFACE;
	current->flags |= SDL_HWPALETTE;
	if (flags & SDL_DOUBLEBUF)
           current->flags |= SDL_DOUBLEBUF;

	//De modinfo necesitamos estos y otros 10 vaores que completamos
	//con la llamada a complete_mode()
	//modinfo se usa para poner la res física (crtc), NO para crear el fb
	modinfo.hdisplay = 1920;
	modinfo.vdisplay = 1080;	

	//Al igual que en la Rpi, usamos el modo de vídeo puesto.
	//Sólo permito ajuste de bpp.
	printf ("\nKMS_SetVideoMode Usando modo de aplicación %d x %d %d bpp",
		width, height, bpp);
	
	printf ("\nKMS_SetVideoMode Usando el modo físico %d x %d", 
	   modinfo.hdisplay, modinfo.vdisplay);
	
	//Aquí completaríamos el resto de timmings del modo,
	
	if (complete_mode (&modinfo) == 0){
	      printf ("\nERR - No se pudo completar modo\n");
	      KMS_VideoQuit(this);
              return (NULL);
	}
	
	//Creamos los buffers
	switch (bpp){
           case 8:  
	   case 16:
	      pixel_format =  DRM_FORMAT_RGB565;   
	      break;
	   case 32:
	      pixel_format =  DRM_FORMAT_XRGB8888;     	
      	      break;
           default:
	      printf ("\nERR - bpp erróneo\n");
	      KMS_VideoQuit(this);
              return (NULL);
	}	
	//MAC Creamos los dos buffers
	for (i = 0;  i < 2; i++){
	   bo[i] = KMS_allocate_buffer(modinfo.htotal, modinfo.vtotal, &stride);
	   if (bo[i] == NULL){
	      printf ("\nERR - No se pudo crear buffer %d\n", i);
	      return (NULL);
	   }
	   kms_bo_get_prop(bo[i], KMS_HANDLE, &handle);
	
	   handles[0] = handle;
    	   pitches[0] = stride;
   	   offsets[0] = 0;
   	   ret = drmModeAddFB2(fd, modinfo.hdisplay, modinfo.vdisplay, 
	      pixel_format, handles, pitches, offsets, &(fb_id[i]), 0);   
	   if (ret) { 
	      printf ("\nERR - drmModeAddFB2 buffer %d: %s \n",i,
	         strerror(errno));
             KMS_VideoQuit(this);
	     return(NULL);
	   }
        }
           
	//MAC Disable Graphics 2
	//MAC Aquí sí que ponemos el nuevo videomode: 
	//emparejamos un conector con un framebuffer.
	i = drmModeSetCrtc(fd, encoder->crtc_id, fb_id[0], 0, 0, 
	&(connector->connector_id), 1, &modinfo);
	
	if ( i != 0 ){ 
	   printf ("\nError - No se pudo establecer modo de vídeo\n");
	   KMS_VideoQuit(this);
	   return (NULL);
	};
	
	//MAC Esto se usa, como mínimo y que yo sepa, para DirectUpdate
	cache_modinfo = modinfo;	
	cache_fbinfo  = *(drmModeGetFB (fd, fb_id[0]));	
	
	//MAC Esta llamada a ReallocFormat es lo que impedía ver algo...
	Rmask = 0;
	Gmask = 0;
	Bmask = 0;
	if ( ! SDL_ReallocFormat(current, bpp, Rmask, Gmask, Bmask, 0) ) {
		return(NULL);
	}
	
	current->w = modinfo.hdisplay;
	current->h = modinfo.vdisplay;

	//MAC según robclark, la memoria de un bo está alineada a pesar
	//de la granularidad de mmpap (tamaño de página)
	for (i = 0; i < 2; i++)
		kms_bo_map (bo[i], (void**) &mapped_vmem[i]);
	
	mapped_memlen =  (stride * modinfo.vtotal); 
	current->pitch  = stride;
	current->pixels =  mapped_vmem[0];
	
	//phys_pixels = mapped_vmem;
	//pixels1 = (void *)malloc(mapped_memlen);	
	//pixels2 = (void *)malloc(mapped_memlen);	

	/* Set up the information for hardware surfaces */
	surfaces_mem = (char *)current->pixels +
		(stride * modinfo.vtotal);
	surfaces_len = ((mapped_memlen)-(surfaces_mem-mapped_vmem[0]));
		
	KMS_FreeHWSurfaces(this);
	KMS_InitHWSurfaces(this, current, surfaces_mem, surfaces_len);
	
	//Rellenamos los arrays de buffer handlers y de memoria mapeada
	this->screen = current;
	this->screen = NULL;

	//MAC Bloque de elección de overlay empieza aquí
	//Calculamos los offsets. En SDL_video.c se hace pero ya tras la 
	//llamada a SetVideoMode(),así que lo tenemos que hacer nosotros antes.
	src_offsetx = (modinfo.hdisplay - width)/2;
	src_offsety = (modinfo.vdisplay - height)/2;
	
	printf ("\nsrc_offsetx = %d, src_offsety = %d", 
	   src_offsetx, src_offsety);	
	
	//Pasamos los 4 últimos parámetros de drmModeSetPlane() 
	//a notación 16.16 fixed point.
	
	src_width = (width) << 16;
	src_height = (height) << 16;
	src_offsetx = src_offsetx << 16;
	src_offsety = src_offsety << 16;	
        
	plane_resources = drmModeGetPlaneResources(fd);
        
	if (!plane_resources){
                printf ("\nERROR -  No se pudo recuperar info de overlays\n");
                KMS_VideoQuit(this);
                return(NULL);
        }
                //Buscamos un overlay que podamos conectar a nuestro crtc
        for (i = 0; i < plane_resources->count_planes; i++) {
		ovr = drmModeGetPlane(fd, plane_resources->planes[i]);
                if (ovr->possible_crtcs & encoder->crtc_id){
                        plane_id = ovr->plane_id;
			break;
		}
		drmModeFreePlane(ovr);
        }
        if (!plane_id) {
                printf("\nERROR - No se pudo encontrar ningún overlay\n");
                KMS_VideoQuit(this);
                return (NULL);
        }		
	
	/* Set the update rectangle function */
	this->UpdateRects = KMS_DirectUpdate;

	//MAC Disable graphics 1
	// Set the terminal into graphics mode 
	printf ("\nEstableciendo teclado en modo gráfico");
        if ( KMS_EnterGraphicsMode(this) < 0 )
        	return(NULL);

	/* We're done */
	printf ("\nKMS_SetVideoMode retorna correctamente");
	return(current);
}

static int KMS_InitHWSurfaces(_THIS, SDL_Surface *screen, char *base, int size)
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
static void KMS_FreeHWSurfaces(_THIS)
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

static int KMS_AllocHWSurface(_THIS, SDL_Surface *surface)
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
static void KMS_FreeHWSurface(_THIS, SDL_Surface *surface)
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

static int KMS_LockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( switched_away ) {
		return -2; /* no hardware access */
	}
	if ( surface == this->screen ) {
		SDL_mutexP(hw_lock);
		if ( KMS_IsSurfaceBusy(surface) ) {
			KMS_WaitBusySurfaces(this);
		}
	} else {
		if ( KMS_IsSurfaceBusy(surface) ) {
			KMS_WaitBusySurfaces(this);
		}
	}
	return(0);
}
static void KMS_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
	if ( surface == this->screen ) {
		SDL_mutexV(hw_lock);
	}
}

static void KMS_WaitVBL(_THIS)
{

//MAC Sacado de /usr/include/libdrm/drm.h 
//ioctl(fd, DRM_IOCTL_WAIT_VBLANK, 0);
	return;


}

void page_flip_handler(int fd, unsigned int frame,
                  unsigned int sec, unsigned int usec, void *data)
{

	int *waiting_for_vblank = data;
	*waiting_for_vblank = 0;	



}


static void KMS_WaitIdle(_THIS)
{
	return;
}

static int KMS_FlipHWSurface(_THIS, SDL_Surface *surface)
{
	if ( switched_away ) {
		return -2; // no hardware access
	}

	waiting_for_vblank = 1;
	
	//MAC vbl wait stuff

        memset(&evctx, 0, sizeof evctx);
        evctx.version = DRM_EVENT_CONTEXT_VERSION;
        evctx.vblank_handler = NULL;
        evctx.page_flip_handler = page_flip_handler;

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
	
	//MAC Volcaríamos el frame completo al buffer en GPU que tocase
	//memcpy (flip_address[flip_page], surface->pixels, mapped_memlen);
	
	//Lo de pasarle !flip_page es un fix temporal hasta atomic pageflip. 
	//Ver detalles en cuaderno.
	//printf ("\ncrtc->mode.hdisplay = %d, crtc->mode.vdisplay = %d",
	//crtc->mode.hdisplay, crtc->mode.vdisplay);
	drmModeSetPlane(fd, plane_id, encoder->crtc_id,fb_id[!flip_page],
	   0,0,0,modinfo.hdisplay, modinfo.vdisplay, src_offsetx, 
	   src_offsety, src_width, src_height);

	drmModePageFlip (fd, encoder->crtc_id, fb_id[flip_page],
	   DRM_MODE_PAGE_FLIP_EVENT,&waiting_for_vblank);
	//Así que esperamos a que se de el cambiazo 
	//(durante el período de vsync) para seguir
	while (waiting_for_vblank){        
		select(fd + 1, &fds, NULL, NULL, 0);
		drmHandleEvent(fd, &evctx);
		//MAC Esto es imprescindible para identificar de qué evento se 
		//trata y si es el evento que nosostros esperamos ("page_flip 
		//completado") ejecutamos la función de callback sólo si se 
		//trata de ese evento.Si es otro evento,nos quedamos en el loop.
	}
			

	
	flip_page = !flip_page;
	surface->pixels = mapped_vmem[flip_page];
	
	return (0);
}

#define BLOCKSIZE_W 32
#define BLOCKSIZE_H 32

static void KMS_DirectUpdate(_THIS, int numrects, SDL_Rect *rects)
{	
	
	int i;	
	int width = cache_modinfo.hdisplay;
	int height = cache_modinfo.vdisplay;
	int bytes_per_pixel = (cache_fbinfo.bpp + 7) / 8;
		
	
	/*MAC Empieza simulación de doble buffer*/
	drmModeSetPlane(fd, plane_id, encoder->crtc_id,fb_id[flip_page],
	   0,0,0, modinfo.hdisplay, modinfo.vdisplay, src_offsetx, 
	   src_offsety, src_width, src_height);
	drmModePageFlip (fd, encoder->crtc_id, fb_id[flip_page],
	   DRM_MODE_PAGE_FLIP_EVENT,&waiting_for_vblank);
	return;	
	/*Acaba simulación de doble buffer*/
	
	if (!shadow_fb) {
	   /* The application is already updating the visible video memory */
		return;
	}
	
	if (cache_fbinfo.bpp != 16) {
		SDL_SetError("Shadow copy only implemented for 16 bpp");
		return;
	}
	for (i = 0; i < numrects; i++) {
		int x1, y1, x2, y2;
		int scr_x1, scr_y1, scr_x2, scr_y2;
		int sha_x1, sha_y1;
		int shadow_right_delta;  /* Address change when moving right in dest */
		int shadow_down_delta;   /* Address change when moving down in dest */
		char *src_start;
		char *dst_start;

		x1 = rects[i].x; 
		y1 = rects[i].y;
		x2 = x1 + rects[i].w; 
		y2 = y1 + rects[i].h;

		if (x1 < 0) {
			x1 = 0;
		} else if (x1 > width) {
			x1 = width;
		}
		if (x2 < 0) {
			x2 = 0;
		} else if (x2 > width) {
			x2 = width;
		}
		if (y1 < 0) {
			y1 = 0;
		} else if (y1 > height) {
			y1 = height;
		}
		if (y2 < 0) {
			y2 = 0;
		} else if (y2 > height) {
			y2 = height;
		}
		if (x2 <= x1 || y2 <= y1) {
			continue;
		}

		sha_x1 = scr_x1 = x1;
		sha_y1 = scr_y1 = y1;
		scr_x2 = x2;
		scr_y2 = y2;
		shadow_right_delta = 1;
		shadow_down_delta = width;

		src_start = shadow_mem +
			(sha_y1 * width + sha_x1) * bytes_per_pixel;
		dst_start = mapped_vmem[0] + mapped_offset + scr_y1 * physlinebytes + 
			scr_x1 * bytes_per_pixel;

		blitFunc((Uint8 *) src_start,
				shadow_right_delta, 
				shadow_down_delta, 
				(Uint8 *) dst_start,
				physlinebytes,
				scr_x2 - scr_x1,
				scr_y2 - scr_y1);
	}
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/

static int KMS_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	return (1);
}


static void KMS_VideoQuit(_THIS)
{
	int i, j;
		
	if ( this->screen ) {
	   /* Clear screen and tell SDL not to free the pixels */
	   const char *dontClearPixels = SDL_getenv("SDL_FBCON_DONT_CLEAR");
	      //No tenemos que limpiar el framebuffer: evitamos mostrar el
	      //buffer en que estamos dibujando.
	      if ( dontClearPixels && flip_page == 0 ) {
	         SDL_memcpy(mapped_vmem[0], mapped_vmem[1], 
		 this->screen->pitch * this->screen->h);
	      }
	      //En este caso sí tenemos que limpiar el framebuffer	
	      if ( !dontClearPixels && this->screen->pixels 
	      && KMS_InGraphicsMode(this) ) {
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
	KMS_FreeHWSurfaces(this);

	/* Close console and input file descriptors */
	if ( fd > 0 ) {
		/* Unmap the video framebuffer and I/O registers */
		for (i = 0; i < 2; i++)	{
		   munmap(mapped_vmem[i], mapped_memlen);
		   mapped_vmem[i] = NULL;
		}
		if ( mapped_io ) {
			munmap(mapped_io, mapped_iolen);
			mapped_io = NULL;
		}
		
		
		//MAC liberamos lo relacionado con las kms	
					
		for (i = 0; i < resources->count_fbs; ++i)
		   drmModeRmFB(fd, resources->fbs[i]);	
		
		for ( i = 0; i < 2; i++ ){
		   if (bo[i] != NULL){ 
	   	      kms_bo_unmap(bo[i]);
	   	      kms_bo_destroy(&bo[i]);
		   }
		}
		kms_destroy(&kms);	
		
		drmModeFreeEncoder(encoder);
   		drmModeFreeConnector(connector);
   		drmModeFreeResources(resources);
		
		/* We're all done with the framebuffer */
		close(fd);
		fd = -1;
	}
	KMS_CloseMouse(this);
	KMS_CloseKeyboard(this);
	exit (0);
}
