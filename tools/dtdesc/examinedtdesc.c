/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: DataTypesDescriptorExaminator
    Lang: English.
*/

/*
 *  includes
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <c_iff.h>

#include "dtdesc.h"

/*
 *  prototypes
 */

void PrintID(CARD32 ID, CARD8 *Buffer);

/*
 *  functions
 */

int main(int argc, char **argv)
{
 struct IFFHandle *IH;
 char Buffer[128];
 long Size;
 CARD8 ID[5];
 int i;

 if(!(argc==2))
 {
  fprintf(stderr, "usage: %s datatypedescriptor\n", argv[0]);

  return(0);
 }

 IH=OpenIFF(argv[1]);
 if(!IH)
 {
  fprintf(stderr, "Could not open %s\n", argv[1]);

  return(0);
 }

 if(!(IH->IFFType==MAKE_ID('D','T','Y','P')))
 {
  fprintf(stderr, "This IFF is not a valid DataType-Descriptor\n");

  CloseIFF(IH);
  return(0);
 }

 fprintf(stdout, "# DataTypeDescription for %s\n", argv[1]);
 fprintf(stdout, "# Automatically generated by %s\n\n", argv[0]);

 while(TRUE)
 {
  if(!ReadChunkHeader(IH))
  {
   break;
  }

  switch(IH->ChunkID)
  {
   case MAKE_ID('F','V','E','R'):
   {
    fprintf(stdout, "Version=");

    while(TRUE)
    {
     Size=ReadChunkData(IH, Buffer, 127);
     if(Size<=0)
     {
      break;
     }

     Buffer[Size]='\0';
     fprintf(stdout, "%s", Buffer);
    }

    fprintf(stdout, "\n");

    SkipChunkData(IH);
    break;
   }

   case MAKE_ID('N','A','M','E'):
   {
    fprintf(stdout, "Name=");

    while(TRUE)
    {
     Size=ReadChunkData(IH, Buffer, 127);
     if(Size<=0)
     {
      break;
     }

     Buffer[Size]='\0';
     fprintf(stdout, "%s", Buffer);
    }

    fprintf(stdout, "\n");

    SkipChunkData(IH);
    break;
   }

   case MAKE_ID('D','T','H','D'):
   {
    struct DataTypeHeader *DTH;
    unsigned char *DTHDBuffer;

    DTHDBuffer=malloc(IH->BytesLeftInChunk);
    if(!DTHDBuffer)
    {
     goto TheEnd;
    }

    Size=IH->BytesLeftInChunk;
    if(!(ReadChunkData(IH, DTHDBuffer, IH->BytesLeftInChunk)==Size))
    {
     free((void *) DTHDBuffer);
     goto TheEnd;
    }

    DTH=(struct DataTypeHeader *) DTHDBuffer;

    DTH->dth_Name     = (CARD8 *)  Swap32IfLE(((CARD32) DTH->dth_Name));
    DTH->dth_BaseName = (CARD8 *)  Swap32IfLE(((CARD32) DTH->dth_BaseName));
    DTH->dth_Pattern  = (CARD8 *)  Swap32IfLE(((CARD32) DTH->dth_Pattern));
    DTH->dth_Mask     = (CARD16 *) Swap32IfLE(((CARD32) DTH->dth_Mask));
    DTH->dth_GroupID  = Swap32IfLE(DTH->dth_GroupID);
    DTH->dth_ID       = Swap32IfLE(DTH->dth_ID);
    DTH->dth_MaskLen  = Swap16IfLE(DTH->dth_MaskLen);
    DTH->dth_Pad      = Swap16IfLE(DTH->dth_Pad);
    DTH->dth_Flags    = Swap16IfLE(DTH->dth_Flags);
    DTH->dth_Priority = Swap16IfLE(DTH->dth_Priority);

    fprintf(stdout, "BaseName=%s\n", (char *) (((long) DTHDBuffer) + ((long) DTH->dth_BaseName)));

    fprintf(stdout, "Pattern=%s\n", (char *) (((long) DTHDBuffer) + ((long) DTH->dth_Pattern)));

    fprintf(stdout, "Mask=");
    for(i=1; i<=DTH->dth_MaskLen; i++)
    {
     fputc((int) *((char *) (((long) DTHDBuffer) + ((long) DTH->dth_Mask) + (2*i-1))), stdout);
    }
    fprintf(stdout, "\n");

    PrintID(DTH->dth_GroupID, ID);
    fprintf(stdout, "GroupID=%s\n", ID);

    PrintID(DTH->dth_ID, ID);
    fprintf(stdout, "ID=%s\n", ID);

    fprintf(stdout, "Flags=");

    switch(DTH->dth_Flags&0x000F)
    {
     case 0:
     {
      fprintf(stdout, "DTF_BINARY");

      break;
     }

     case 1:
     {
      fprintf(stdout, "DTF_ASCII");

      break;
     }

     case 2:
     {
      fprintf(stdout, "DTF_IFF");

      break;
     }

     case 3:
     {
      fprintf(stdout, "DTF_MISC");

      break;
     }
    }

    if(DTH->dth_Flags & 0x0010)
    {
     fprintf(stdout, ",DTF_CASE");
    }

    if(DTH->dth_Flags & 0x1000)
    {
     fprintf(stdout, ",DTF_SYSTEM1");
    }

    fprintf(stdout, "\n");

    fprintf(stdout, "Priority=%hu\n", DTH->dth_Priority);

    free((void *) DTHDBuffer);
    SkipChunkData(IH);
    break;
   }

   default:
   {
#if 0
    PrintID(IH->ChunkID, ID);
    fprintf(stderr, "Unsupported chunk : %s\n", ID);
#endif /* 0 */
    SkipChunkData(IH);
    break;
   }
  }
 }

TheEnd:

 CloseIFF(IH);

 return(0);
}

void PrintID(CARD32 ID, CARD8 *Buffer)
{
 if(!Buffer)
 {
  return;
 }

 Buffer[0]=(ID&0xFF000000)>>24;
 Buffer[1]=(ID&0xFF0000)  >>16;
 Buffer[2]=(ID&0xFF00)    >> 8;
 Buffer[3]=(ID&0xFF);
 Buffer[4]='\0';
}


