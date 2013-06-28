SDL 1.2.15 WITH DISPMANX AND KMS BACKENDS
=========================================

This is essentialy a libSDL 1.2.15 release with KMS framebuffer and dispmanx backends.
These backends are made taking the fbcon backend as a base.

COMPILING AND INSTALLING
========================

The KMS backend requires recent versions of libkms and libdrm (NOT the dispmanx backend on the Pi! KMS is different stuff).
The DISPMANX backend is meant to work on Raspbian (Raspberry Pi Debian distro), wich includes all the needed
header files and libs in the /opt/vc directories. 

-To configure for KMS backend just run the MAC_ConfigureKMS.sh script.

-To configure for DISPMANX backend just run the MAC_ConfigureDISPMANX.sh script.

-Run make && sudo make install. 

IMPORTANT NOTES
===============

-The KMS backend is experimental: it may show tearing / flickering. It will be fixed after atomic page-flip is merged
 into the mainline kernel. Raspberry Pi DISPMANX backend is stable.

-These backends are meant for double-buffered fullscreen modes without tearing from the console. They are not meant
 for apps that run in windowed enviroments. Think of them as "extended framebuffer console backends". 

-They won't work on (slow and bloated) X enviroments.

-No physical video mode adjustments are done: the aplication resolution is hardware-scaled to fullscreen native
 resolution via KMS or DISPMANX overlays, so *SCANLING SETTINGS IN THE APLICATIONS SHOULD BE DISABLED* to save CPU.
 This means that a 320x240 aplication will automatucally be scaled to 1920x1080 using the overlays. If the aplication
 has the ability to run in a higher resolution (most 320x240 apps will have this kind of options) you must avoid using
 that. The application HAS to be run at 320x240 and it will be scaled by the backend.

-BE AWARE: SDL_Flip() call, when used with the SDL_DOUBLEBUF flag set, will block until new vsync arrives (and issued 
 framebuffer swap is done, since that swapping occurs during the vsync). It means that apps that arrive to SDL_Flip()
 too late will miss an vsync and be blocked until the next one. This will result on halved framerates: you'll get 30 from
 a near-60 fps app, for example. This is totally expected unless someone proves otherwise. The idea is that apps should be
 getting to the SDL_Flip() call in less than 16ms in a 60Hz video mode, since that's the period between vsyncs: that will
 ensure a smooth refresh at the expected framerate.
 However, if an application is too slow for that, using SDL_Flip() without the SDL_DOUBLEBUF flag set will result in the
 API using SDL_UpdateRect(0,0,0,0) under the hood to refresh the whole screen, wich is non-blocking, but you'll be
 drawing over the visible video buffer and ugly tearing effects will affect the application.
 A lot of apps are in fact fast enough but they have internal sleeps to keep a constant framerate on other backends (X11,
 fbcon) wich have non-blocking and hence *broken* SDL_Flip() implementations. 

-X11 and fbcon backends both have non-blocking SDL_Flip() implementations, for different reasons. That's "wrong",
 so to say. SDL_Flip() is meant to be blocking since we have to 1) keep the visible buffer contents intact until it's 
 replaced with the just drawn one 2) that swap will occur during vsync as noted. 

contact information

Manuel Alfayate Corchete
redwindwanderer@gmail.com
