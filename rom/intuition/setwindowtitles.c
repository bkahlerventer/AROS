/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/08/29 13:57:38  digulla
    Commented
    Moved common code from driver to Intuition

    Revision 1.1  1996/08/23 17:28:18  digulla
    Several new functions; some still empty.


    Desc:
    Lang: english
*/
#include "intuition_intern.h"

extern void intui_SetWindowTitles (struct Window *, UBYTE *, UBYTE *);

/*****************************************************************************

    NAME */
	#include <clib/intuition_protos.h>

	__AROS_LH3(void, SetWindowTitles,

/*  SYNOPSIS */
	__AROS_LHA(struct Window *, window, A0),
	__AROS_LHA(UBYTE         *, windowTitle, A1),
	__AROS_LHA(UBYTE         *, screenTitle, A2),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 46, Intuition)

/*  FUNCTION
	Changes the current window and/or the screen title.

    INPUTS
	window - Change the title for this window or the screen which the
		window contains.
	windowTitle - New title for the window or ((UBYTE *)~0L) to keep the
		old title or NULL for no title. If you specify a string,
		this string is *NOT* copied.
	screenTitle - New title for the screen of the window or ((UBYTE *)~0L)
		to keep the old title or NULL for no title. If you specify
		a title for the screen, this title will be shown when the
		window becomes active. If you specify a string, this string
		is *NOT* copied.

    RESULT
	None.

    NOTES
	You should be careful with specifying a screen title because that
	may irritate the user.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)

    /* Call driver before changing the window to allow it to examine
	the old values. */
    intui_SetWindowTitles (window, windowTitle, screenTitle);

    /* Change window's title */
    if (!windowTitle)
	window->Title = NULL;
    else if (windowTitle != (UBYTE *)~0L)
	window->Title = windowTitle;

    /* Change screen's title */
    if (!screenTitle)
	window->ScreenTitle = NULL;
    else if (screenTitle != (UBYTE *)~0L)
	window->ScreenTitle = screenTitle;

    __AROS_FUNC_EXIT
} /* SetWindowTitles */
