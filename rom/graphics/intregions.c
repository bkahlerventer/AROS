/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: Code for various operations on Regions and Rectangles
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE 1

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/regions.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#include <clib/macros.h>
#include "intregions.h"
#include "graphics_intern.h"

#include <aros/debug.h>

/* to test these functions under an unix-like os, like linux, compile this file this way:

       gcc -o test intregions.c <optimization flags> -I<path to the AROS includes> -DLINTEST=1

   and then do

       time ./test

   and tell me whether it's fast or not :))
*/

#if LINTEST
#    include <stdlib.h>
#    include <stdio.h>

#    define DOTEST 1

#    undef  _NewRegionRectangleExtChunk
#    define _NewRegionRectangleExtChunk() malloc(sizeof(struct RegionRectangleExtChunk))

#    undef  _DisposeRegionRectangleExtChunk
#    define _DisposeRegionRectangleExtChunk(mem) free(mem)

#    undef  NewRegion
#    define NewRegion()                                      \
     ({                                                      \
         struct Region *reg = malloc(sizeof(struct Region)); \
	 reg->RegionRectangle = NULL;                        \
         reg;                                                \
     })

#    undef  DisposeRegion
#    define DisposeRegion(reg) free(reg)

#    undef  InitRegion
#    define InitRegion(reg)             \
     {                                  \
         (reg)->RegionRectangle = NULL; \
     }

#    undef  ClearRegion
#    define ClearRegion(region)                                                                           \
     {                                                                                                    \
         if (region->RegionRectangle)                                                                     \
         {                                                                                                \
             struct RegionRectangleExtChunk *NextChunk;                                                   \
                                                                                                          \
             NextChunk = Chunk(region->RegionRectangle)->FirstChunk;                                      \
                                                                                                          \
             while(NextChunk)                                                                             \
             {                                                                                            \
                 struct RegionRectangleExtChunk *OldChunk = NextChunk;                                    \
                                                                                                          \
                 NextChunk = (struct RegionRectangleExtChunk *)NextChunk->Rects[SIZERECTBUF - 1].RR.Next; \
                                                                                                          \
                 if (NextChunk)                                                                           \
                     NextChunk = Chunk(NextChunk->Rects);                                                 \
                                                                                                          \
                 _DisposeRegionRectangleExtChunk(OldChunk);                                               \
	     }                                                                                            \
                                                                                                          \
             InitRegion(region);                                                                          \
         }                                                                                                \
     }

     static struct GfxBase *GfxBase;
#    undef  kprintf
#    define kprintf(format, x...) printf(format, ##x)

#    endif
#endif

inline struct RegionRectangle *_NewRegionRectangle
(
    struct RegionRectangle **LastRectPtr,
    struct GfxBase *GfxBase
)
{
    struct RegionRectangleExt *RRE = RRE(*LastRectPtr);

    if (!RRE)
    {
        struct RegionRectangleExtChunk *Chunk;

        Chunk = _NewRegionRectangleExtChunk();

        if (Chunk)
            RRE = Chunk->Rects;

        if (RRE)
        {
            RRE->Counter = 0;
            RRE->RR.Prev = NULL;
            RRE->RR.Next = NULL;

            RRE[SIZERECTBUF-1].RR.Next = NULL; /* IMPORTANT !!! */

            Chunk->FirstChunk = Chunk;
        }
    }
    else
    if (RRE->Counter == SIZERECTBUF - 1)
    {
        struct RegionRectangleExtChunk *Chunk;

        Chunk = _NewRegionRectangleExtChunk();

        if (Chunk)
        {
            Chunk->FirstChunk = Chunk(RRE)->FirstChunk;
            RRE = Chunk->Rects;
        }
        else
            RRE = NULL;

        if (RRE)
        {
            RRE->Counter = 0;
            RRE->RR.Prev = *LastRectPtr;
            RRE->RR.Next = NULL;
            (*LastRectPtr)->Next = &RRE->RR;

            RRE[SIZERECTBUF-1].RR.Next = NULL; /* IMPORTANT !!! */
        }
    }
    else
    {
	struct RegionRectangleExt *Prev = RRE++;

        RRE->RR.Next    = NULL;
        RRE->RR.Prev    = &Prev->RR;
        Prev->RR.Next   = &RRE->RR;

        RRE->Counter    = Prev->Counter + 1;
    }

    return *LastRectPtr = (struct RegionRectangle *)RRE;
}



void _DisposeRegionRectangleList
(
    struct RegionRectangle *RR,
    struct GfxBase         *GfxBase
)
{
    struct RegionRectangleExtChunk *NextChunk;

    if (!RR)
	return;

    if (RR->Prev)
    {
        RR->Prev->Next = NULL;
    }

    if (!Counter(RR))
    {
        NextChunk = Chunk(RR);
    }
    else
    {
	RR = &Chunk(RR)->Rects[SIZERECTBUF - 1].RR;
        NextChunk = (struct RegionRectangleExtChunk *)RR->Next;
        RR->Next = NULL;
    }

    while(NextChunk)
    {
        struct RegionRectangleExtChunk *OldChunk = NextChunk;

        NextChunk = (struct RegionRectangleExtChunk *)NextChunk->Rects[SIZERECTBUF - 1].RR.Next;

        _DisposeRegionRectangleExtChunk(OldChunk);
    }
}

/*
   Takes all the rectangles from src and linkes them with the rectangle
   to which *dstptr points. *dstptr at the end will hold the pointer
   to the LAST rectangle in the resulting list.
   If the system runs out of memory the function will deallocate any allocated
   memory and will return FALSE. TRUE, otherwise.
 */

BOOL _LinkRegionRectangleList
(
    struct RegionRectangle  *src,
    struct RegionRectangle **dstptr,
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle *prev = *dstptr;

    for (; src; src = src->Next)
    {
        struct RegionRectangle *new = _NewRegionRectangle(dstptr, GfxBase);

        if (!new)
        {
            if (prev)
                _DisposeRegionRectangleList(prev->Next, GfxBase);

            return FALSE;
        }

        new->bounds = src->bounds;
    }

    return TRUE;
}



#define Bounds(x) (&(x)->bounds)
#define MinX(rr)   (Bounds(rr)->MinX)
#define MaxX(rr)   (Bounds(rr)->MaxX)
#define MinY(rr)   (Bounds(rr)->MinY)
#define MaxY(rr)   (Bounds(rr)->MaxY)

#define ADVANCE(nextbandptr, rr)                    \
{                                                   \
    if ((rr)->Next && MinY((rr)->Next) == MinY(rr)) \
        rr = (rr)->Next;                            \
    else                                            \
    {                                               \
    	if (nextbandptr)                            \
            *nextbandptr = (rr)->Next;              \
        rr = NULL;                                  \
    }                                               \
}

#define NEWREG(prevptr, rr)                     \
{                                               \
    rr = _NewRegionRectangle(prevptr, GfxBase); \
    if (!rr)                                    \
        return FALSE;                           \
}

#define ADDRECT(minx, maxx)    \
{                              \
   struct RegionRectangle *rr; \
   NEWREG(DstPtr, rr);         \
                               \
   MinX(rr) = minx;            \
   MinY(rr) = MinY;            \
   MaxX(rr) = maxx;            \
   MaxY(rr) = MaxY;            \
}

#define ADDRECTMERGE(minx, maxx)     \
{                                    \
    if                               \
    (                                \
        !*(DstPtr) ||                \
        (MinY(*(DstPtr)) != MinY) || \
        ((minx-1) > MaxX(*DstPtr))   \
    )                                \
    {                                \
	ADDRECT((minx), (maxx));     \
    }                                \
    else                             \
    if (MaxX(*DstPtr) < maxx)        \
    {                                \
        MaxX(*DstPtr) = maxx;        \
    }                                \
}

#define FIXLASTBAND(first, last)                      \
if (!first)                                           \
{                                                     \
    first = last;                                     \
}                                                     \
else                                                  \
{                                                     \
    struct RegionRectangle *newlastrect = last->Prev; \
                                                      \
    first->Prev->Next = last;                         \
    last->Prev = first->Prev;                         \
    first->Prev = newlastrect;                        \
}

#define DOBANDCHECKS(firstlastdst, lastdst, curdst)                        \
if (curdst != lastdst)                                                     \
{                                                                          \
    if (!lastdst)                                                          \
    {                                                                      \
        firstlastdst = &Chunk(curdst)->FirstChunk->Rects[0].RR;            \
        DstBounds->MinY = MinY(firstlastdst);                              \
        DstBounds->MinX = MinX(firstlastdst);                              \
        DstBounds->MaxX = MaxX(curdst);                                    \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        if (DstBounds->MinX > MinX(lastdst->Next))                         \
            DstBounds->MinX = MinX(lastdst->Next);                         \
        if (DstBounds->MaxX < MaxX(curdst))                                \
            DstBounds->MaxX = MaxX(curdst);                                \
                                                                           \
        if (MaxY(firstlastdst) == MinY(curdst) - 1)                        \
        {                                                                  \
            struct RegionRectangle *_one = firstlastdst;                   \
            struct RegionRectangle *_two = lastdst->Next;                  \
                                                                           \
            while (_one != lastdst->Next && _two)                          \
            {                                                              \
		if                                                         \
                (                                                          \
                    MinX(_one) == MinX(_two) &&                            \
                    MaxX(_one) == MaxX(_two)                               \
                )                                                          \
                {                                                          \
                    _one = _one->Next;                                     \
                    _two = _two->Next;                                     \
                }                                                          \
                else                                                       \
                {                                                          \
                    break;                                                 \
                }                                                          \
	    }                                                              \
                                                                           \
            if (_one == lastdst->Next && !_two)                            \
            {                                                              \
                LONG MaxY = MaxY(curdst);                                  \
                _one = firstlastdst;                                       \
                _DisposeRegionRectangleList(lastdst->Next, GfxBase);       \
                while (_one)                                               \
                {                                                          \
                    MaxY(_one) = MaxY;                                     \
                    _one = _one->Next;                                     \
                }                                                          \
                                                                           \
                curdst = lastdst;                                          \
            }                                                              \
            else                                                           \
                firstlastdst = lastdst->Next;                              \
	}                                                                  \
        else                                                               \
            firstlastdst = lastdst->Next;                                  \
    }                                                                      \
    lastdst = curdst;                                                      \
}

#if DOTEST
#    define DEBUG 1
#endif

#if DEBUG
void dumprect(struct Rectangle *rec)
{
    if (!rec)
    {
        kprintf("NULL\n");
        return;
    }

    kprintf("(%d,%d)-(%d,%d)\n", (int)rec->MinX, (int)rec->MinY,
                                 (int)rec->MaxX, (int)rec->MaxY);
}
void dumpregion(struct Region *reg)
{
    struct RegionRectangle *rr;

    if (!reg)
    {
        kprintf("NULL\n");
        return;
    }

    kprintf("Bounds: "); dumprect(&reg->bounds);

    for (rr = reg->RegionRectangle; rr; rr = rr->Next)
    {
        kprintf("    Rectangle %p: ", rr); dumprect(&rr->bounds);
    }
}

void dumpregionrectangles(struct RegionRectangle *rr)
{

    while (rr)
    {
	kprintf("%p (prev: %p - next: %p): ", rr, rr->Prev, rr->Next);
        dumprect(&rr->bounds);
        rr = rr->Next;
    }
}

void dumpband(struct RegionRectangle *rr, LONG OffX, LONG MinY, LONG MaxY)
{
    if (rr)
    {
	while (1)
        {
	    struct Rectangle r;
            r.MinX = MinX(rr) + OffX;
            r.MaxX = MaxX(rr) + OffX;
            r.MinY = MinY;
            r.MaxY = MaxY;
            kprintf("%p (prev: %p - next: %p): ", rr, rr->Prev, rr->Next);
            dumprect(&r);
            if (rr->Next && MinY(rr->Next) == MinY(rr)) rr = rr->Next; else break;
        }
    }
    else
        kprintf("\n");
}
#endif

static BOOL _OrBandBand
(
    LONG                     OffX1,
    LONG                     OffX2,
    LONG                     MinY,
    LONG                     MaxY,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct RegionRectangle **NextSrc1Ptr,
    struct RegionRectangle **NextSrc2Ptr,
    struct GfxBase          *GfxBase
)
{
    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    ADDRECTMERGE(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        }
        else
        {
	    ADDRECTMERGE(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
            ADVANCE(NextSrc2Ptr, Src2);
        }
    }

    if (Src1)
    {
        do
        {
            ADDRECTMERGE(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
            ADVANCE(NextSrc1Ptr, Src1);
        } while (Src1);
    }
    else
    while (Src2)
    {
	ADDRECTMERGE(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
    }

    return TRUE;
}

BOOL _AndBandBand
(
    LONG                     OffX1,
    LONG                     OffX2,
    LONG                     MinY,
    LONG                     MaxY,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct RegionRectangle **NextSrc1Ptr,
    struct RegionRectangle **NextSrc2Ptr,
    struct GfxBase          *GfxBase
)
{
    while (Src1 && Src2)
    {
        if (MinX(Src1) + OffX1 < MinX(Src2) + OffX2)
        {
	    if (MaxX(Src1) + OffX1 >= MaxX(Src2) + OffX2)
            {
            	/* Src1 totally covers Src2 */
                ADDRECT(MinX(Src2) + OffX2, MaxX(Src2) + OffX2);
 	        ADVANCE(NextSrc2Ptr, Src2);
            }
            else
            {
                if (MaxX(Src1) + OffX1 >= MinX(Src2) + OffX2)
            	    /* Src1 partially covers Src2 */
                    ADDRECT(MinX(Src2) + OffX2, MaxX(Src1) + OffX1);

                ADVANCE(NextSrc1Ptr, Src1);
            }
        }
	else
        {
	    if (MaxX(Src2) + OffX2 >= MaxX(Src1) + OffX1)
            {
            	/* Src2 totally covers Src1 */
                ADDRECT(MinX(Src1) + OffX1, MaxX(Src1) + OffX1);
 	        ADVANCE(NextSrc1Ptr, Src1);
            }
            else
            {
                if (MaxX(Src2) + OffX2 >= MinX(Src1) + OffX1)
            	    /* Src2 partially covers Src1 */
                    ADDRECT(MinX(Src1) + OffX1, MaxX(Src2) + OffX2);

                ADVANCE(NextSrc2Ptr, Src2);
            }
        }
    }

    if (Src1)
    {
        do ADVANCE(NextSrc1Ptr, Src1) while (Src1);
    }
    else
    while (Src2) ADVANCE(NextSrc2Ptr, Src2);

    return TRUE;
}

BOOL _ClearBandBand
(
    LONG                     OffX1,
    LONG                     OffX2,
    LONG                     MinY,
    LONG                     MaxY,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct RegionRectangle **NextSrc1Ptr,
    struct RegionRectangle **NextSrc2Ptr,
    struct GfxBase          *GfxBase
)
{
    LONG MinX = 0;

    if (Src2)
        MinX = MinX(Src2) + OffX2;

    while (Src1 && Src2)
    {
	if (MaxX(Src1) + OffX1 < MinX)
        {
	    /* Subtrahend doesn't overlap minuend. Just skip it */
            ADVANCE(NextSrc1Ptr, Src1);
        }
        else
        if (MinX(Src1) + OffX1 <= MinX)
        {
            /* Subtrahend precedes minuend: nuke left edge of minuend */
	    MinX = MaxX(Src1) + OffX1 + 1;

            if (MinX > MaxX(Src2) + OffX2)
            {
		/*
                   Subtrahend completely overlaps minuend, so advance
                   to the next minuend and reset MinX to its left
   	        */
                ADVANCE(NextSrc2Ptr, Src2);
                if (Src2)
                    MinX = MinX(Src2) + OffX2;
            }
            else
            {
                /* Subtrahend doesn't extend beyond minuend, so advence to the next one */
                ADVANCE(NextSrc1Ptr, Src1);
            }
  	}
        else
        if (MinX(Src1) + OffX1 <= MaxX(Src2) + OffX2)
        {
	    /*
               Subtrahend covers part of minuend.
               Add uncovered part of minuend to the band and jump to the next
               subtrahend
     	    */

            ADDRECT(MinX, MinX(Src1) + OffX1 - 1);

            MinX = MaxX(Src1) + OffX1 + 1;

            if (MinX > MaxX(Src2) + OffX2)
            {
                /*Minuend used up: advance to the next one */
                ADVANCE(NextSrc2Ptr, Src2);
                if (Src2)
                    MinX = MinX(Src2) + OffX2;
            }
            else
            {
                /* Subtrahend used up */
                ADVANCE(NextSrc1Ptr, Src1);
            }
	}
        else
        {
	    /*
	       Minuend used up: add any remaining piece before advancing.
	     */

             if (MaxX(Src2) + OffX2 >= MinX)
             {
                 ADDRECT(MinX, MaxX(Src2) + OffX2);
             }

             ADVANCE(NextSrc2Ptr, Src2);
             if (Src2)
                 MinX = MinX(Src2) + OffX2;
        }
    }

    if (Src1)
    {
        do ADVANCE(NextSrc1Ptr, Src1) while (Src1);
    }
    else
    while (Src2)
    {
	ADDRECT(MinX, MaxX(Src2) + OffX2);
        ADVANCE(NextSrc2Ptr, Src2);
        if (Src2)
        {
            MinX = MinX(Src2) + OffX2;
        }
    }

    return TRUE;
}

typedef BOOL (*BandOperation)
(
    LONG                     OffX1,
    LONG                     OffX2,
    LONG                     MinY,
    LONG                     MaxY,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct RegionRectangle **NextSrc1Ptr,
    struct RegionRectangle **NextSrc2Ptr,
    struct GfxBase          *GfxBase
);

BOOL _DoOperationBandBand
(
    BandOperation            Operation,
    LONG                     OffX1,
    LONG                     OffX2,
    LONG 		     OffY1,
    LONG 		     OffY2,
    struct RegionRectangle  *Src1,
    struct RegionRectangle  *Src2,
    struct RegionRectangle **DstPtr,
    struct Rectangle        *DstBounds,
    struct GfxBase          *GfxBase
)
{
    struct RegionRectangle  *Dst, *LastDst, *FirstLastDst;
    struct RegionRectangle **NextSrc1Ptr = (void *)~0;
    struct RegionRectangle **NextSrc2Ptr = (void *)~0;
    struct RegionRectangle  *Band1 = Src1, *Band2 = Src2;

    BOOL res = TRUE;

    LONG TopY1 = 0, TopY2 = 0;

    FirstLastDst = LastDst = Dst = *DstPtr = NULL;

    while (Src1 && Src2)
    {
	LONG MinY, MaxY;

        if (NextSrc1Ptr)
        {
            TopY1 = MinY(Src1) + OffY1;
            NextSrc1Ptr = NULL;
        }

	if (NextSrc2Ptr)
        {
            TopY2 = MinY(Src2) + OffY2;
            NextSrc2Ptr = NULL;
        }

	if (TopY1 < TopY2)
        {
	    MinY = TopY1;
	    MaxY = MIN(MaxY(Src1) + OffY1, TopY2 - 1);
	    TopY1 = MaxY + 1;

            Band1 = Src1;
            Band2 = NULL;
        }
        else
        if (TopY2 < TopY1)
        {
	    MinY = TopY2;
	    MaxY = MIN(MaxY(Src2) + OffY2, TopY1 - 1);
	    TopY2 = MaxY + 1;

            Band1 = NULL;
            Band2 = Src2;
        }
        else
        {
	    MinY = TopY1;
	    MaxY = MIN(MaxY(Src1) + OffY1, MaxY(Src2) + OffY2);
	    TopY1 = TopY2 = MaxY + 1;

            Band1 = Src1;
            Band2 = Src2;
	}

	NextSrc1Ptr = (MaxY == MaxY(Src1) + OffY1) ? &Src1 : NULL;
	NextSrc2Ptr = (MaxY == MaxY(Src2) + OffY2) ? &Src2 : NULL;

        if
        (
            !Operation
            (
                OffX1, OffX2,
                MinY, MaxY,
                Band1, Band2,
                &Dst,
                NextSrc1Ptr, NextSrc2Ptr,
                GfxBase
	    )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);

    }


    while (Src1)
    {
	if (NextSrc1Ptr)
            TopY1 = MinY(Src1) + OffY1;

        NextSrc1Ptr = (void *)~0;


        if
        (
            !Operation
            (
                OffX1, OffX2,
                TopY1, MaxY(Src1) + OffY1,
                Src1, NULL,
                &Dst,
                &Src1, NULL,
                GfxBase
            )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);
    }

    while (Src2)
    {
	if (NextSrc2Ptr)
            TopY2 = MinY(Src2) + OffY2;

        NextSrc2Ptr = (void *)~0;

        if
        (
            !Operation
            (
                OffX1, OffX2,
                TopY2, MaxY(Src2) + OffY2,
                NULL, Src2,
                &Dst,
                NULL, &Src2,
                GfxBase
            )
        )
        {
            res = FALSE;
            goto end;
        }

	DOBANDCHECKS(FirstLastDst, LastDst, Dst);
    }

end:

    if (Dst)
    {
        if (res)
        {
            DstBounds->MaxY = MaxY(Dst);
            *DstPtr = &Chunk(Dst)->FirstChunk->Rects[0].RR;
        }
        else
            _DisposeRegionRectangleList(&Chunk(Dst)->FirstChunk->Rects[0].RR, GfxBase);
    }

    return res;
}


BOOL _OrRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _OrBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
        ClearRegion(R2);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _OrRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region Res;
    struct RegionRectangle rr;

    InitRegion(&Res);

    rr.bounds = *Rect;
    rr.Next   = NULL;
    rr.Prev   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _OrBandBand,
            MinX(Reg),
            0,
	    MinY(Reg),
            0,
            Reg->RegionRectangle,
            &rr,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
	ClearRegion(Reg);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;
}

BOOL _XorRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
);

BOOL _XorRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region R;
    struct RegionRectangle rr;

    InitRegion(&R);

    R.bounds = *Rect;
    R.RegionRectangle = &rr;

    rr.Next = NULL;
    MinX(&rr) = MinY(&rr) = 0;
    MaxX(&rr) = Rect->MaxX - Rect->MinX;
    MaxY(&rr) = Rect->MaxY - Rect->MinY;

    return _XorRegionRegion(&R, Reg, GfxBase);
}

BOOL _ClearRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct Region Res;
    struct RegionRectangle rr;

    InitRegion(&Res);

    rr.bounds = *Rect;
    rr.Next   = NULL;
    rr.Prev   = NULL;

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            0,
            MinX(Reg),
            0,
	    MinY(Reg),
            &rr,
            Reg->RegionRectangle,
            &Res.RegionRectangle,
            &Res.bounds,
            GfxBase
        )
    )
    {
	ClearRegion(Reg);

        *Reg = Res;

        _TranslateRegionRectangles(Res.RegionRectangle, -MinX(&Res), -MinY(&Res));

        return TRUE;
    }

    return FALSE;
}

static inline struct RegionRectangle * ShrinkBand
(
    LONG OffX,
    LONG OffY,
    struct RegionRectangle  *Band,
    LONG MinX,
    LONG MinY,
    LONG MaxX,
    LONG MaxY,
    struct RegionRectangle **NextBandPtr
)
{
    struct RegionRectangle *NewFirst;
    struct RegionRectangle *LastBand = Band;

    while (Band && MinX > MaxX(Band))
    {
        ADVANCE(NextBandPtr, Band);
    }


    if (!Band)
        NewFirst = *NextBandPtr;
    else
        NewFirst = Band;

    if (Chunk(NewFirst) != Chunk(LastBand))
        _DisposeRegionRectangleExtChunk(Chunk(LastBand));


    if (Band)
    {
        if (MinX(Band) < MinX)
            MinX(Band) = MinX + OffX;
    }

    while (Band && MaxX > MaxX(Band))
    {
        MinY(Band) = MinY + OffY;
        MaxY(Band) = MaxY + OffY;

        MaxX(Band) += OffX;

        ADVANCE(NextBandPtr, Band);
    }

    if (Band)
    {
	struct RegionRectangle *LastRR;

        MinY(Band) = MinY + OffY;
        MaxY(Band) = MaxY + OffY;

        MaxX(Band) = MaxX + OffX;

        LastRR = Band;
	while (Band)
        {
            ADVANCE(NextBandPtr, Band);
        }

        if (*NextBandPtr)
            (*NextBandPtr)->Prev = LastRR;
        LastRR->Next = *NextBandPtr;
    }

    return NewFirst;
}

BOOL _AndRectRegion
(
    struct Region    *Reg,
    struct Rectangle *Rect,
    struct GfxBase   *GfxBase
)
{
    struct RegionRectangle *rr = Reg->RegionRectangle;
    struct Rectangle OldBounds = Reg->bounds;

    if (!rr || !AndRectRect(Rect, &OldBounds, &Reg->bounds))
    {
        ClearRegion(Reg);
        return TRUE;
    }

    if
    (
        MinX(Reg) == OldBounds.MinX &&
        MinY(Reg) == OldBounds.MinY &&
        MaxX(Reg) == OldBounds.MaxX &&
        MaxY(Reg) == OldBounds.MaxY
    )
    {
        return TRUE;
    }

    while
    (
	Rect->MinY > MaxY(rr) + OldBounds.MinY
    )
    {
        rr = rr->Next;
    }

    Reg->RegionRectangle = rr;
    rr->Prev = NULL;

    while
    (
        rr                                      &&
        Rect->MaxY >= MinY(rr) + OldBounds.MinY
    )
    {
        /* The band overlaps with the rectangle */
	struct RegionRectangle *LastRR   = rr->Prev;
        struct RegionRectangle *NewFirst;

        NewFirst = ShrinkBand
        (
            OldBounds.MinX - MinX(Reg),
            OldBounds.MinY - MinY(Reg),
            rr,
            Rect->MinX - OldBounds.MinX,
            MAX(Rect->MinY - OldBounds.MinY, MinY(rr)),
            Rect->MaxX - OldBounds.MinX,
            MIN(Rect->MaxY - OldBounds.MinY, MaxY(rr)),
            &rr
	);

        if (LastRR)
            LastRR->Next = NewFirst;
        else
            Reg->RegionRectangle = NewFirst;

        NewFirst->Prev = LastRR;
    }

    _DisposeRegionRectangleList(rr, GfxBase);

    return TRUE;
}

BOOL _AndRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _AndBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
	ClearRegion(R2);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _ClearRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;

    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
	ClearRegion(R2);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));

        return TRUE;
    }

    return FALSE;
}

BOOL _XorRegionRegion
(
    struct Region  *R1,
    struct Region  *R2,
    struct GfxBase *GfxBase
)
{
    struct Region R3;
    struct RegionRectangle *Diff1 = NULL;
    struct RegionRectangle *Diff2 = NULL;
    LONG res = FALSE;

    InitRegion(&R3);

    if
    (
        _DoOperationBandBand
        (
            _ClearBandBand,
            MinX(R1),
            MinX(R2),
	    MinY(R1),
            MinY(R2),
            R1->RegionRectangle,
            R2->RegionRectangle,
            &Diff1,
            &R3.bounds,
            GfxBase
        ) &&

        _DoOperationBandBand
        (
            _ClearBandBand,
            MinX(R2),
            MinX(R1),
            MinY(R2),
	    MinY(R1),
            R2->RegionRectangle,
            R1->RegionRectangle,
            &Diff2,
            &R3.bounds,
            GfxBase
        ) &&

        _DoOperationBandBand
        (
            _ClearBandBand,
	    0,
            0,
            0,
            0,
            Diff1,
            Diff2,
            &R3.RegionRectangle,
            &R3.bounds,
            GfxBase
        )
    )
    {
	res = TRUE;

        ClearRegion(R2);

        *R2 = R3;

        _TranslateRegionRectangles(R3.RegionRectangle, -MinX(&R3), -MinY(&R3));
    }

    _DisposeRegionRectangleList(Diff1, GfxBase);
    _DisposeRegionRectangleList(Diff2, GfxBase);

    return res;

}


#if DOTEST

#undef GfxBase
#include <proto/graphics.h>

int main(void)
{
    int i;

    struct Region *R1 = NewRegion();
    struct Region *R2 = NewRegion();

    for (i = 0; i < 10; i++)
    {
        int l = i*20;

	struct Rectangle r = {l, 0, l+11, 201};
        _OrRectRegion(R1, &r, GfxBase);
    }

    for (i = 0; i < 10; i++)
    {
        int u = i*20;

	struct Rectangle r = {0, u, 201, u+11};
        _OrRectRegion(R2, &r, GfxBase);
    }

    for (i = 0; i<100000; i++)
    {
        _XorRegionRegion(R2, R1, GfxBase);
    }

    DisposeRegion(R2);
    DisposeRegion(R1);
    return 0;
}
#endif

