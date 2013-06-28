#define DRM_DISPLAY_MODE_LEN    32
#define DRM_MODE_TYPE_DRIVER    (1<<6)
#define DRM_MODE_FLAG_NHSYNC    (1<<1)
#define DRM_MODE_FLAG_NVSYNC    (1<<3)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define DRM_MODE(nm, t, c, hd, hss, hse, ht, hsk, vd, vss, vse, vt, vs, f) \
        .name = nm, .status = 0, .type = (t), .clock = (c), \
        .hdisplay = (hd), .hsync_start = (hss), .hsync_end = (hse), \
        .htotal = (ht), .hskew = (hsk), .vdisplay = (vd), \
        .vsync_start = (vss), .vsync_end = (vse), .vtotal = (vt), \
        .vscan = (vs), .flags = (f), .vrefresh = 0

enum drm_mode_status {
    MODE_OK     = 0,    /* Mode OK */
    MODE_HSYNC,         /* hsync out of range */
    MODE_VSYNC,         /* vsync out of range */
    MODE_H_ILLEGAL,     /* mode has illegal horizontal timings */
    MODE_V_ILLEGAL,     /* mode has illegal horizontal timings */
    MODE_BAD_WIDTH,     /* requires an unsupported linepitch */
    MODE_NOMODE,        /* no mode with a matching name */
    MODE_NO_INTERLACE,  /* interlaced mode not supported */
    MODE_NO_DBLESCAN,   /* doublescan mode not supported */
    MODE_NO_VSCAN,      /* multiscan mode not supported */
    MODE_MEM,           /* insufficient video memory */
    MODE_VIRTUAL_X,     /* mode width too large for specified virtual size */
    MODE_VIRTUAL_Y,     /* mode height too large for specified virtual size */
    MODE_MEM_VIRT,      /* insufficient video memory given virtual size */
    MODE_NOCLOCK,       /* no fixed clock available */
    MODE_CLOCK_HIGH,    /* clock required is too high */
    MODE_CLOCK_LOW,     /* clock required is too low */
    MODE_CLOCK_RANGE,   /* clock/mode isn't in a ClockRange */
    MODE_BAD_HVALUE,    /* horizontal timing was out of range */
    MODE_BAD_VVALUE,    /* vertical timing was out of range */
    MODE_BAD_VSCAN,     /* VScan value out of range */
    MODE_HSYNC_NARROW,  /* horizontal sync too narrow */
    MODE_HSYNC_WIDE,    /* horizontal sync too wide */
    MODE_HBLANK_NARROW, /* horizontal blanking too narrow */
    MODE_HBLANK_WIDE,   /* horizontal blanking too wide */
    MODE_VSYNC_NARROW,  /* vertical sync too narrow */
    MODE_VSYNC_WIDE,    /* vertical sync too wide */
    MODE_VBLANK_NARROW, /* vertical blanking too narrow */
    MODE_VBLANK_WIDE,   /* vertical blanking too wide */
    MODE_PANEL,         /* exceeds panel dimensions */
    MODE_INTERLACE_WIDTH, /* width too large for interlaced mode */
    MODE_ONE_WIDTH,     /* only one width is supported */
    MODE_ONE_HEIGHT,    /* only one height is supported */
    MODE_ONE_SIZE,      /* only one resolution is supported */
    MODE_NO_REDUCED,    /* monitor doesn't accept reduced blanking */
    MODE_UNVERIFIED = -3, /* mode needs to reverified */
    MODE_BAD = -2,      /* unspecified reason */
    MODE_ERROR  = -1    /* error condition */
};

struct list_head {
        struct list_head *next, *prev;
};

struct drm_mode_object {
        uint32_t id;
        uint32_t type;
        struct drm_object_properties *properties;
};

struct drm_display_mode {
   	/* Header */
        struct list_head head;
        struct drm_mode_object base;

        char name[DRM_DISPLAY_MODE_LEN];

        enum drm_mode_status status;
        unsigned int type;

        /* Proposed mode values */
        int clock;              /* in kHz */
        int hdisplay;
        int hsync_start;
        int hsync_end;
        int htotal;
        int hskew;
        int vdisplay;
        int vsync_start;
        int vsync_end;
        int vtotal;
        int vscan;
        unsigned int flags;

        /* Addressable image size (may be 0 for projectors, etc.) */
        int width_mm;

        /* Actual mode we give to hw */
        int clock_index;
        int synth_clock;
        int crtc_hdisplay;
        int crtc_hblank_start;
        int crtc_hblank_end;
        int crtc_hsync_start;
        int crtc_hsync_end;
        int crtc_htotal;
        int crtc_hskew;
        int crtc_vdisplay;
        int crtc_vblank_start;
        int crtc_vblank_end;
        int crtc_vsync_start;
        int crtc_vsync_end;
        int crtc_vtotal;
        int crtc_hadjusted;
        int crtc_vadjusted;

        /* Driver private mode info */
        int private_size;
        int *private;
        int private_flags;

        int vrefresh;           /* in Hz */
        int hsync;              /* in kHz */
};

static void drm_crtc_convert_to_umode(struct drm_mode_modeinfo *out,
                                      const struct drm_display_mode *in)
{
        out->clock = in->clock;
        out->hdisplay = in->hdisplay;
        out->hsync_start = in->hsync_start;
        out->hsync_end = in->hsync_end;
        out->htotal = in->htotal;
        out->hskew = in->hskew;
        out->vdisplay = in->vdisplay;
        out->vsync_start = in->vsync_start;
        out->vsync_end = in->vsync_end;
        out->vtotal = in->vtotal;
        out->vscan = in->vscan;
        out->vrefresh = in->vrefresh;
        out->flags = in->flags;
        out->type = in->type;
        strncpy(out->name, in->name, DRM_DISPLAY_MODE_LEN);
        out->name[DRM_DISPLAY_MODE_LEN-1] = 0;
}

static const struct drm_display_mode edid_cea_modes[] = {
        /* 1 - 640x480@60Hz */
        { DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
                   752, 800, 0, 480, 490, 492, 525, 0,
                   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
        /* 2 - 720x480@60Hz */
        { DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
                   798, 858, 0, 480, 489, 495, 525, 0,
                   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC) },
 	 /* 16 - 1920x1080@60Hz */
        { DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
                   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
                   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC) },
};
static const int drm_num_cea_modes = ARRAY_SIZE(edid_cea_modes);
