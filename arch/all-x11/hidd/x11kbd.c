/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: X11 hidd handling keypresses.
    Lang: English.
*/


#include <proto/utility.h>
#include <proto/oop.h>
#include <oop/oop.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <hidd/hidd.h>
#include <hidd/keyboard.h>

#include <devices/inputevent.h>

#include "x11.h"

//#define DEBUG 1
#include <aros/debug.h>


long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd);

struct x11kbd_data
{
    VOID (*kbd_callback)(APTR, UWORD);
    APTR callbackdata;
};

static AttrBase HiddKbdAB;

static struct abdescr attrbases[] =
{
    { IID_Hidd_Kbd, &HiddKbdAB },
    { NULL, NULL }
};

static struct _keytable
{
    KeySym keysym;
    WORD   hiddcode;
}
keytable[] =
{
    {XK_Return, 	0x44 },
    {XK_Right,		0x4e },
    {XK_Up,		0x4c },
    {XK_Left,		0x4f },
    {XK_Down,		0x4d },
    {XK_Help,		0x5f },

    {XK_BackSpace,	0x41 },
    {XK_Delete,		0x46 },
    {XK_space,		0x40 },
    {XK_Shift_L,	0x60 },
    {XK_Shift_R,	0x61 },
    {XK_Alt_L,		0x64 },
    {XK_Alt_R,		0x65 },
    {XK_Escape,		0x45 },
    {XK_Tab,		0x42 },

    {XK_F1,		0x50 },
    {XK_F2,		0x51 },
    {XK_F3,		0x52 },
    {XK_F4,		0x53 },
    {XK_F5,		0x54 },
    {XK_F6,		0x55 },
    {XK_F7,		0x56 },
    {XK_F8,		0x57 },
    {XK_F9,		0x58 },
    {XK_F10,		0x59 },
    {XK_F11,		0x5f },		/* HELP */
    {XK_F12,		0x5f },		/* HELP */
    {XK_Home,		0x5f },		/* HELP */

    {XK_KP_Enter,	0x43 },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Decimal,	0x3c },
    {XK_KP_Separator,	0x3c },
    {XK_KP_Delete,	0x3c },
    {XK_KP_Add,		0x5e },
    {XK_KP_Subtract,	0x4a },
    {XK_KP_Multiply,	0x5d },
    {XK_KP_Divide,	0x5c },

    {XK_KP_0,		0x0f },
    {XK_KP_Insert,	0x0f },    
    {XK_KP_1,		0x1d },
    {XK_KP_End,		0x1d },   
    {XK_KP_2,		0x1e },
    {XK_KP_Down,	0x1e },
    {XK_KP_3,		0x1f },
    {XK_KP_Page_Down,	0x1f },
    {XK_KP_4,		0x2d },
    {XK_KP_Left,	0x2d },
    {XK_KP_5,		0x2e },
    {XK_KP_Begin,	0x2e },
    {XK_KP_6,		0x2f },
    {XK_KP_Right,	0x2f },
    {XK_KP_7,		0x3d },
    {XK_KP_Home,	0x3d },
    {XK_KP_8,		0x3e },
    {XK_KP_Up,		0x3e },
    {XK_KP_9,		0x3f },
    {XK_KP_Page_Up,	0x3f },
       
    {XK_E,		0x12 },
    {XK_e,		0x12 },
    {XK_R,		0x13 },
    {XK_r,		0x13 },
    {XK_T,		0x14 },
    {XK_t,		0x14 },
    {XK_U,		0x16 },
    {XK_u,		0x16 },
    {XK_I,		0x17 },
    {XK_i,		0x17 },
    {XK_O,		0x18 },
    {XK_o,		0x18 },
    {XK_P,		0x19 },
    {XK_p,		0x19 },

    {XK_S,		0x21 },
    {XK_s,		0x21 },
    {XK_D,		0x22 },
    {XK_d,		0x22 },
    {XK_F,		0x23 },
    {XK_f,		0x23 },
    {XK_G,		0x24 },
    {XK_g,		0x24 },
    {XK_H,		0x25 },
    {XK_h,		0x25 },
    {XK_J,		0x26 },
    {XK_j,		0x26 },
    {XK_K,		0x27 },
    {XK_k,		0x27 },
    {XK_L,		0x28 },
    {XK_l,		0x28 },

    {XK_X,		0x32 },
    {XK_x,		0x32 },
    {XK_c,		0x33 },
    {XK_C,		0x33 },
    {XK_V,		0x34 },
    {XK_v,		0x34 },
    {XK_B,		0x35 },
    {XK_b,		0x35 },
    {XK_N,		0x36 },    
    {XK_n,		0x36 },
    
    {XK_1,		0x01 },
    {XK_2,		0x02 },
    {XK_3,		0x03 },
    {XK_4,		0x04 },    
    {XK_5,		0x05 },
    {XK_6,		0x06 },
    {XK_7,		0x07 },
    {XK_8,		0x08 },
    {XK_9,		0x09 },
    {XK_0,		0x0A },
    {0, - 1 }
};

/* English keyboard */
static struct _keytable english_keytable[] =
{    
    {XK_Control_L,	0x63 }, /* left control = control */	
    {XK_Multi_key,	0x63 }, /* right control = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right Alt = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_y,		0x15 },
    {XK_Y,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_z,		0x31 },
    {XK_Z,		0x31 },

    
    /* Key left of 1 */
    {XK_grave,		0x00 },
    
    /* Keys right of 0 */
    {XK_minus,		0x0B },
    {XK_equal,		0x0C },
    
    /* Keys right of P */
    {XK_bracketleft,	0x1A },
    {XK_bracketright,	0x1B },
    
    /* Keys right of L */
    {XK_semicolon,	0x29 },
    {XK_apostrophe,	0x2A }, 
    {XK_backslash,	0x2B }, /* Third key right of L might not be present */
    
    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_less,		0x30 }, 
    
    /* Keys 2nd right of N (= usually right of M) */    
    {XK_comma,		0x38 }, 
    {XK_period,		0x39 }, 
    {XK_slash,		0x3A },
        
    {0, -1 }
};
                        
/* German keyboard */
static struct _keytable german_keytable[] =
{
    {XK_Control_L,	0x63 }, /* linke STRG = control */	
    {XK_Multi_key,	0x63 }, /* rechte STRG = control */
    {XK_Super_L,	0x66 },	/* Linke Win = LAMIGA */
    {XK_Super_R,	0x67 },	/* Rechte Win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* Linke Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* Alt Gr = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_Z,		0x15 },
    {XK_z,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_y,		0x31 },
    {XK_Y,		0x31 },
     
     /* Key left of 1 */
    {XK_asciicircum,	0x00 }, /* Akzent links neben 1 Taste */

    /* Keys right of 0 */
    {XK_equal,		0x0A }, /* = */
    {XK_ssharp,		0x0B }, /* scharfes s */
    {XK_acute,		0x0C }, /* Akzent rechts von scharfem s */
    
    /* Keys right of P */
    {XK_udiaeresis,	0x1A }, /* Umlaut u */
    {XK_Udiaeresis,	0x1A },
    {XK_plus,		0x1B }, /* + */

    /* Keys right of L */    
    {XK_odiaeresis,	0x29 }, /* Umlaut o */
    {XK_Odiaeresis,	0x29 },
    {XK_adiaeresis,	0x2A }, /* Umlaut a */
    {XK_Adiaeresis,	0x2A },
    {XK_numbersign,	0x2B }, /* # */

    /* Key right of shift and 2nd left of X (might not be present) */        
    {XK_less,		0x30 }, /* < */

    /* Keys 2nd right of N (= usually right of M) */       
    {XK_comma,		0x38 }, /* Komma */
    {XK_period,		0x39 }, /* Punkt */
    {XK_minus,		0x3A }, /* Minus */
        
    {0, -1 }
};

/* Itialian keyboard */
static struct _keytable italian_keytable[] =
{
    {XK_Control_L,	0x63 }, /* left CTRL = control */	
    {XK_Multi_key,	0x63 }, /* right CTRL = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right alt = RALT */


    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a, 		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_y,		0x15 },
    {XK_Y,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_z,		0x31 },
    {XK_Z,		0x31 },
    
    
    /* Key left of 1 */
    {XK_backslash,	0x00 }, 

    /* Keys right of 0 */
    {XK_apostrophe,	0x0B },
    {XK_Igrave,		0x0C }, 
    {XK_igrave,		0x0C },

    /* Keys right of P */        
    {XK_Egrave,		0x1A },
    {XK_egrave,		0x1A },
    {XK_plus,		0x1B }, /* + */

    /* Keys right of L */    
    {XK_Ograve,		0x29 },
    {XK_ograve,		0x29 },
    {XK_Agrave,		0x2A },
    {XK_agrave,		0x2A },
    {XK_Ugrave,		0x2B }, /* Third key right of L might not be present */
    {XK_ugrave,		0x2B },

    /* Key right of shift and 2nd left of X (might not be present) */        
    {XK_less,		0x30 }, /* < */

    /* Keys 2nd right of N (= usually right of M) */    
    {XK_comma,		0x38 },
    {XK_period,		0x39 }, 
    {XK_minus,		0x3A }, 
        
    {0, -1 }
};

#if 0

/* Use this template to create a keytable for your language:

   Do not touch the right values (rawkey numbers). Only change
   the XK's at the left side. To find out the XK_ names (keysym)
   start "xev" and press the key the comment describes (for
   example "Key left of S" in the xev window. In the Shell
   window you will see output like this:
   
   KeyPress event, serial 30, synthetic NO, window 0x5000001,
    root 0x47, subw 0x5000002, time 3410089115, (24,45), root:(28,69),
    state 0x0, keycode 50 (keysym 0xffe1, Shift_L), same_screen YES,
    XLookupString gives 0 characters:  ""  |
                                           |
   This is the keysym name _______________/
  
   So in this case you would have to write  "XK_Shift_L"

   Check all keys, not just the ones with "XK_????"!!!
*/

static struct _keytable template_keytable[] =
{    
    {XK_Control_L,	0x63 }, /* left control = control */	
    {XK_Multi_key,	0x63 }, /* right control = control */
    {XK_Super_L,	0x66 },	/* left win = LAMIGA */
    {XK_Super_R,	0x67 },	/* right win = RAMIGA */
    {XK_Meta_L,		0x64 }, /* left Alt = LALT */
    {XK_Mode_switch,	0x65 }, /* right Alt = RALT */
    
    /* Key left of S */
    {XK_A,		0x20 },
    {XK_a,		0x20 },
    
    /* Key right of N */
    {XK_M,		0x37 },
    {XK_m,		0x37 },
    
    /* Key right of TAB */
    {XK_Q,		0x10 },
    {XK_q,		0x10 },
    
    /* Key between T and U */
    {XK_????,		0x15 },
    {XK_????,		0x15 },
    
    /* Key left of E */
    {XK_W,		0x11 },
    {XK_w,		0x11 },
    
    /* Key left of X */
    {XK_????,		0x31 },
    {XK_????,		0x31 },

    
    /* Key left of 1 */
    {XK_????,		0x00 },
    
    /* Keys right of 0 */
    {XK_????,		0x0B },
    {XK_????,		0x0C },
    
    /* Keys right of P */
    {XK_????,		0x1A },
    {XK_????,		0x1B },
    
    /* Keys right of L */
    {XK_????,		0x29 },
    {XK_????,		0x2A }, 
    {XK_????,		0x2B }, /* Third key right of L might not be present */
    
    /* Key right of shift and 2nd left of X (might not be present) */       
    {XK_less,		0x30 }, 
    
    /* Keys 2nd right of N (= usually right of M) */
    {XK_comma,		0x38 }, 
    {XK_period,		0x39 }, 
    {XK_slash,		0x3A },
        
    {0, -1 }
};

#endif
                        
/***** X11Kbd::New()  ***************************************/
static Object * x11kbd_new(Class *cl, Object *o, struct pRoot_New *msg)
{
    BOOL has_kbd_hidd = FALSE;
    struct TagItem *tag, *tstate;
    APTR callback = NULL;
    APTR callbackdata = NULL;
    
    EnterFunc(bug("X11Kbd::New()\n"));
 
    ObtainSemaphoreShared( &XSD(cl)->sema);
 
    if (XSD(cl)->kbdhidd)
    	has_kbd_hidd = TRUE;

    ReleaseSemaphore( &XSD(cl)->sema);
 
    if (has_kbd_hidd) /* Cannot open twice */
    	ReturnPtr("X11Kbd::New", Object *, NULL); /* Should have some error code here */

    tstate = msg->attrList;
    D(bug("tstate: %p, tag=%x\n", tstate, tstate->ti_Tag));	
    while ((tag = NextTagItem(&tstate)))
    {
	ULONG idx;
	
	D(bug("Got tag %d, data %x\n", tag->ti_Tag, tag->ti_Data));
	    
	if (IS_HIDDKBD_ATTR(tag->ti_Tag, idx))
	{
	    D(bug("Kbd hidd tag\n"));
	    switch (idx)
	    {
		case aoHidd_Kbd_IrqHandler:
		    callback = (APTR)tag->ti_Data;
		    D(bug("Got callback %p\n", (APTR)tag->ti_Data));
		    break;
			
		case aoHidd_Kbd_IrqHandlerData:
		    callbackdata = (APTR)tag->ti_Data;
		    D(bug("Got data %p\n", (APTR)tag->ti_Data));
		    break;
	    }
	}
	    
    } /* while (tags to process) */
    if (NULL == callback)
    	ReturnPtr("X11Kbd::New", Object *, NULL); /* Should have some error code here */

    o = (Object *)DoSuperMethod(cl, o, (Msg)msg);
    if (o)
    {
	struct x11kbd_data *data = INST_DATA(cl, o);
	data->kbd_callback = (VOID (*)(APTR, UWORD))callback;
	data->callbackdata = callbackdata;
	
	ObtainSemaphore( &XSD(cl)->sema);
	XSD(cl)->kbdhidd = o;
	ReleaseSemaphore( &XSD(cl)->sema);
    }
    ReturnPtr("X11Kbd::New", Object *, o);
}

/***** X11Kbd::HandleEvent()  ***************************************/

static VOID x11kbd_handleevent(Class *cl, Object *o, struct pHidd_X11Kbd_HandleEvent *msg)
{
    struct x11kbd_data * data;
    
    XKeyEvent *xk;

    EnterFunc(bug("x11kbd_handleevent()\n"));
    xk = &(msg->event->xkey);
    data = INST_DATA(cl, o);
    if (msg->event->type == KeyPress)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, XSD(cl)));
		
    }
    else if (msg->event->type == KeyRelease)
    {
	data->kbd_callback(data->callbackdata
		, (UWORD)xkey2hidd(xk, XSD(cl)) | IECODE_UP_PREFIX);
    }

    
    ReturnVoid("X11Kbd::HandleEvent");
}



#undef XSD
#define XSD(cl) xsd

#if 0
long StateToQualifier (unsigned long state)
{
    long result;

    result = 0;

    if (state & ShiftMask)
	result |= SHIFT;

    if (state & ControlMask)
	result |= CTRL;

    if (state & LockMask)
	result |= CAPS;

    if (state & Mod2Mask) /* Right Alt */
	result |= LALT;

    if (state & 0x2000) /* Mode switch */
	result |= RALT;

    if (state & Mod1Mask) /* Left Alt */
	result |= AMIGAKEYS;

    if (state & Button1Mask)
	result |= IEQUALIFIER_LEFTBUTTON;

    if (state & Button2Mask)
	result |= IEQUALIFIER_RBUTTON;

    if (state & Button3Mask)
	result |= IEQUALIFIER_MIDBUTTON;

    return (result);
} /* StateToQualifier */

#endif

WORD lookup_keytable(KeySym *ks, struct _keytable *keytable)
{
    short t;
    WORD result = -1;
    
    for (t=0; keytable[t].hiddcode != -1; t++)
    {
	if (*ks == keytable[t].keysym)
	{
	    D(bug("xktac: found in key table\n"));
	    result = keytable[t].hiddcode;
	    break;
	}
    }
    
    return result;
}

long xkey2hidd (XKeyEvent *xk, struct x11_staticdata *xsd)
{
    char buffer[10];
    KeySym ks;
    int count;
    long result;


 
    D(bug("xkey2hidd\n"));
   
LX11
    xk->state = 0;
    count = XLookupString (xk, buffer, 10, &ks, NULL);
UX11
    D(bug("xk2h: Code %d (0x%x). Event was decoded into %d chars: %d (0x%x)\n",xk->keycode, xk->keycode, count,ks,ks));
    
    result = lookup_keytable(&ks, keytable);
    if (result == -1) result = lookup_keytable(&ks, english_keytable);
    
    if (result != -1)
    {
	ReturnInt ("xk2h", long, result);
    }
    
    D(bug("xk2h: Passing X keycode\n", xk->keycode & 0xffff));

    result = xk->keycode & 0xffff;

    ReturnInt ("xk2h", long, result);
} /* XKeyToAmigaCode */


/********************  init_kbdclass()  *********************************/


#define NUM_ROOT_METHODS 1
#define NUM_X11KBD_METHODS 1

Class *init_kbdclass (struct x11_staticdata *xsd)
{
    Class *cl = NULL;

    struct MethodDescr root_descr[NUM_ROOT_METHODS + 1] = 
    {
    	{METHODDEF(x11kbd_new),		moRoot_New},
	{NULL, 0UL}
    };
    
    struct MethodDescr kbdhidd_descr[NUM_X11KBD_METHODS + 1] = 
    {
    	{METHODDEF(x11kbd_handleevent),	moHidd_X11Kbd_HandleEvent},
	{NULL, 0UL}
    };
    
    struct InterfaceDescr ifdescr[] =
    {
    	{root_descr, 	IID_Root, 		NUM_ROOT_METHODS},
    	{kbdhidd_descr, IID_Hidd_X11Kbd, 	NUM_X11KBD_METHODS},
	{NULL, NULL, 0}
    };
    
    AttrBase MetaAttrBase = ObtainAttrBase(IID_Meta);
	
    struct TagItem tags[] =
    {
	{ aMeta_SuperID,		(IPTR)CLID_Hidd },
	{ aMeta_InterfaceDescr,		(IPTR)ifdescr},
	{ aMeta_InstSize,		(IPTR)sizeof (struct x11kbd_data) },
	{ aMeta_ID,			(IPTR)CLID_Hidd_X11Kbd },
	{TAG_DONE, 0UL}
    };

    EnterFunc(bug("KbdHiddClass init\n"));
    
    if (MetaAttrBase)
    {
    	cl = NewObject(NULL, CLID_HiddMeta, tags);
    	if(cl)
    	{
	    cl->UserData = (APTR)xsd;
	    xsd->kbdclass = cl;
	    
	    if (obtainattrbases(attrbases, OOPBase))
	    {
		D(bug("KbdHiddClass ok\n"));
		
	    	AddClass(cl);
	    }
	    else
	    {
	    	free_kbdclass(xsd);
		cl = NULL;
	    }
	}
	/* Don't need this anymore */
	ReleaseAttrBase(IID_Meta);
    }
    ReturnPtr("init_kbdclass", Class *, cl);
}


/*************** free_kbdclass()  **********************************/
VOID free_kbdclass(struct x11_staticdata *xsd)
{
    EnterFunc(bug("free_kbdclass(xsd=%p)\n", xsd));

    if(xsd)
    {

        RemoveClass(xsd->kbdclass);
	
        if(xsd->kbdclass) DisposeObject((Object *) xsd->kbdclass);
        xsd->kbdclass = NULL;
	
	releaseattrbases(attrbases, OOPBase);

    }

    ReturnVoid("free_kbdclass");
}
