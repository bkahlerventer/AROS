/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Demo showing x11gfx.hidd
    Lang: English.
*/

#include <stdio.h>
#include <string.h>
#include <exec/types.h>
#include <graphics/rastport.h>
#include <intuition/intuition.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#define SDEBUG 1
#define DEBUG 1

#include <aros/debug.h>

struct IntuitionBase *IntuitionBase;
struct Library *GfxBase;
struct Library *LayersBase;
struct DosLibrary *DOSBase;
struct Screen *screen;
struct Window * window;

struct Screen * openscreen(void);
void closescreen(struct Screen * screen); 


int main(int argc, char **argv)
{
    /* Intialize debugging */
    SDInit();
    
    if ((IntuitionBase = (struct IntuitionBase *) OpenLibrary("intuition.library", 0))) 
    {
	if ((GfxBase = OpenLibrary("graphics.library", 0))) 
        {
	    if ((DOSBase = (struct DosLibrary *) OpenLibrary("dos.library",0)))
	    {
              if ((screen = openscreen())) 
              {
	      
	      	/* Wait forever */
		Wait(0L);
		
/*                init();
		doall();
                restore(); 
*/
		closescreen(screen);
	      }
              CloseLibrary((struct Library *)DOSBase);
	  }
	  CloseLibrary(GfxBase);
	}
	CloseLibrary((struct Library *) IntuitionBase);
    }
    return 0;
} /* main */



struct Screen * openscreen(void)
{
  struct Screen * screen;
   
  printf("Opening screen\n");
  screen = OpenScreenTags(NULL,
                          SA_Width, 	640,
                          SA_Height, 	480,
                          TAG_END);

  printf("Opening window, screen=%p\n", screen);

  window = OpenWindowTags(NULL,
			  WA_IDCMP, IDCMP_RAWKEY,
			  WA_Left,	100,
			  WA_Top,	100,
                          WA_Height, 	200,
                          WA_Width, 	200,
			  WA_CustomScreen, screen,
			  WA_Activate,		TRUE,
			  WA_DepthGadget, 	TRUE,
			  WA_Zoom,		TRUE,
			  WA_CloseGadget,	TRUE,
			  WA_Title,		"X11 gfxhidd demo",
                          TAG_END);

  printf("Window opened\n");

/*  screen->RastPort.longreserved[0] = window->RPort->longreserved[0];

  Draw(&screen->RastPort, 100, 100);
*/
  return screen;
}

void closescreen(struct Screen * screen)
{
  CloseWindow(window);
  CloseScreen(screen);
}
