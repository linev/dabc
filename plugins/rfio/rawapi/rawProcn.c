/**********************************************************************
 * Copyright:
 *   GSI, Gesellschaft fuer Schwerionenforschung mbH
 *   Planckstr. 1
 *   D-64291 Darmstadt
 *   Germany
 * created 14. 2.1996, Horst Goeringer
 **********************************************************************
 * rawProcn.c
 *    utility programs for gStore package: client and server
 **********************************************************************
 * rawAddStrings:    add 2 messages avoiding overflows
 * rawCheckClientFile: check gStore naming conventions for client file
 * rawDelFile:       delete single file in GSI mass storage
 * rawDelList:       delete list of files in GSI mass storage
 * rawGetFSpName:    get file space name from user specification
 * rawGetHLName:     get high level object name from path
 * rawGetLLName:     get low level object name from file name
 * rawGetFileSize:   get file size (byte) via Posix call stat
 * rawGetPathName:   get path name from high level object name
 * rawQueryFile:     get query information for single file
 * rawRecvError:     receive error message
 * rawRecvHead:      receive common buffer header
 * rawRecvHeadC:     receive common buffer header and check
 * rawRecvRequest:   receive request for next buffer
 * rawRecvStatus:    receive status buffer
 * rawSendRequest:   send request for next buffer
 * rawSendStatus:    send status buffer
 * rawTestFileName:  verify that specified name is a valid file name
 **********************************************************************
 * 23. 4.1996., H.G: rawGetHLName: remove device dependency in hl-name,
 *                   as in ADSM 213 class may be set in dsmSendObj
 * 16. 5.1997, H.G.: keep only modules for all clients here
 * 16. 6.1997, H.G.: ported to Linux
 * 15.10.1997, H.G.: rawCheckAuth moved to rawCheckAuth.c
 * 22.10.1997, H.G.: rawDelFilelist added
 *  8. 7.1998, H.G.: new filetype STREAM
 * 19.10.1998, H.G.: rawapitd.h introduced, dsmapitd.h removed
 *  1.12.2000, H.G.: rawSendStatus: send also buffer request headers
 *  7. 2.2001, H.G.: new function rawQueryString
 * 21. 2.2001, H.G.: renamed file rawQueryFile.c -> rawProcQuery.c
 *                   function rawQueryList added
 * 18. 5.2001, H.G.: function rawDelFilelist -> rawcliproc.c
 *                   merge into new file rawcliproc.c: rawProcQuery.c
 * 18. 6.2001, H.G.: function rawTestFile: renamed to  rawTestFileName,
 *                   handle mixed case
 *  2.11.2001, H.G.: ported to W2000
 * 14.11.2001, H.G.: take alternate delimiter for Windows into account
 *  9. 1.2002, H.G.: rawGetLLName: pass object and delimiter as argument
 * 19. 2.2002, H.G.: rawGetFileSize added
 * 20. 2.2002, H.G.: rawGetCmdParms removed (unused)
 *  7. 3.2002, H.G.: rawQueryPrint: new type GSI_MEDIA_INCOMPLETE
 *                   rawRecvHeadC added
 *  7. 6.2002, H.G.: rawRecvHeadC: check always for error status
 * 23. 7.2002, H.G.: replace old printf() -> fprintf(fLogFile)
 *  5. 8.2002, H.G.: rawQueryFile: handle QUERY_RETRIEVE, QUERY_STAGE
 * 14.10.2002, H.G.: ported to Lynx
 * 31. 1.2003, H.G.: use rawdefn.h
 * 17. 2.2003, H.G.: rawCheckFileList, rawCheckObjList -> rawcliprocn.c
 *  3. 6.2003, H.G.: renamed from rawprocn to rawProcn
 *  9. 7.2003, H.G.: rawTestFileName: call rawGetFileSize to identify
 *                   directories
 * 13. 8.2003, H.G.: rawQueryPrint: add user who staged file in output
 *  5.12.2003, H.G.: rawRecvStatus: convert status header to host format
 * 28. 1.2004, H.G.: GSI_MEDIA_CACHE: rawQueryPrint, rawQueryString
 *                   GSI_MEDIA_INCOMPLETE: in rawQueryString
 * 26. 5.2004, H.G.: rawQueryPrint, rawQueryString: new print modes:
 *                   stage status not checked
 * 29. 6.2004, H.G.: rawRecvHeadC: recv error msg, if write cache full
 * 24.11.2004, H.G.: rawQueryFile: handle status values
 *                   STA_NO_ACCESS, STA_ARCHIVE_NOT_AVAIL
 * 25. 1.2005, H.G.: use rawapitd-gsin.h, new media names:
 *                   GSI_..._LOCKED, GSI_..._INCOMPLETE
 *  1. 2.2005, H.G.: ported to Linux and gcc322
 * 25.11.2005, H.G.: rename rawGetFSName to rawGetFSpName (file space),
 *                   rawGetFSName in rawProcPooln (get file system name)
 *  3. 4.2006, H.G.: ported to sarge
 * 29. 9.2006, H.G.: rawQueryPrint: handle ATL server
 *  4.10.2006, H.G.: rawQueryPrint: handle cacheDB entry V4
 *  2.11.2006, H.G.: rawQueryString: handle ATL server, cacheDB entry V4
 *  8.12.2006, H.G.: rawQueryFile: handle query on several ATL Servers
 *  8. 5.2008, H.G.: rawGetFullFile: move -> rawCliProcn.c
 *                   rawQueryFile: moved from rawCliProcn.c, repl printf
 *                   rawQueryList: moved from rawCliProcn.c, repl printf
 * 16. 5.2008, H.G.: rawGetHLName: remove './' and trailing '/.' in path
 * 28. 8.2008, H.G.: rawQueryPrint, rawQueryString:
 *                   remove ATL server no. in query output
 *  6.10.2008, H.G.: replace perror by strerr(errno), improve msgs
 *                   rawRecvRequest: better handling rc=0 from recv()
 * 30.10.2008, H.G.: rawQueryFile,rawQueryList: better: rc=0 from recv()
 *  5.11.2008, H.G.: rawRecvError: better handling rc=0 from recv()
 *                      arg 3: char ** -> char *
 *                   rawRecvHead: better handling rc=0 from recv()
 *                      arg 3: char ** -> char *
 *                   rawRecvStatus: better handling rc=0 from recv()
 *                      arg 3: char ** -> char *
 * 11.11.2008, H.G.: add suggestions of Hakan
 *  3.12.2008, H.G.: add suggestions of Hakan, part II
 * 12.12.2008, H.G.: handle CUR_QUERY_LIMIT
 *  4. 5.2009, H.G.: rawRecvHeadC: improve debug output
 *  7. 5.2009, H.G.: new entry rawAddStrings
 * 22. 6.2008, H.G.: replace long->int if 64bit client (ifdef SYSTEM64)
 *  8. 7.2008, H.G.: rawQueryPrint/String: print filesize with %10d
 * 23.11.2009, H.G.: rawTestFileName: inhibit '*', '?' as file name part
 * 10.12.2009, H.G.: rawQueryFile, rawQueryPrint, rawQueryString:
 *                      handle enhanced structure srawObjAttr
 *                      old version info also available in new structure
 *                   rawQueryList not upgraded, obviously unused???
 * 29. 1.2010, H.G.: replace MAX_FILE -> MAX_FULL_FILE in:
 *                      static string cPath,
 *                      rawTestFilePath: cTemp
 *                   enhance cMsg: 256 -> STATUS_LEN in:
 *                      rawRecvHeadC, rawRecvRequest
 *  5. 2.2010, H.G.: move rawQueryPrint,rawQueryString -> rawCliProcn.c
 *                   move rawQueryList -> Unused.c
 * 11. 2.2010, H.G.: rename rawTestFilePath -> rawCheckClientFile
 *                   rawCheckClientFile: all checks of local file name
 *                      concerning gStore conventions are done here
 *  3. 5.2010, H.G.: rawGetFileSize: specific error msg if filesize>2GB
 * 23. 8.2010, H.G.: rawGetFileSize, rawTestFileName:
 *                   remove SYSTEM64, allow "long"
 *  5.11.2010, H.G.: reset errno after error,
 *                   better error handling after recv/send
 * 18.11.2010, H.G.: rawRecvError: provide also incomplete msg
 * 25.08.2011, H.G.: rawRecv..., some broken connections: -E- -> -W-
 *  5.09.2011, H.G.: rawCheckClientFile: allow wildcard chars in dirname
 *  1.12.2011, H.G.: rawGetFileSize: better msg handling
 * 24.12.2012, H.G.: rawQueryFile: enable query with variable hl
 *  4.12.2013, H.G.: rawRecvStatus: modify last arg to 'srawStatus *'
 * 23. 1.2014, H.G.: rawQueryFile: handle broken connection to master
 * 29. 1.2014, H.G.: rawCheckClientFile: adapted arg types, better names
 * 12. 9.2014, H.G.: rawDelFile, rawDelList: added from rawCliProcn.c,
 *                   as also needed on DMs for ARCHIVE_OVER from lustre
 * 15. 9.2014, H.G.: rawDelFile, rawDelList: replace old printf()
 *                      -> fprintf(fLogFile)
 *                   rawQueryFile: provide storage info (for rawDelFile)
 * 20.11.2014, H.G.: rawQueryFile: improve checks for file meta data
 *                      consistency
 *  9. 1.2015, H.G.: rawQueryFile: correct handling of WC files
 *                      staged in GRC
 **********************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef WIN32          /* Windows */
#include <sys\types.h>
#include <winsock.h>
#include <windows.h>
#include <process.h>
#include <sys\stat.h>
#else                 /* all Unix */
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

#ifdef Lynx           /* Lynx */
#include <sys/types.h>
#include <socket.h>
#elif Linux           /* Linux */
#include <sys/socket.h>
#else                 /* AIX */
#include <sys/socket.h>
#endif

#endif              /* all Unix */

#include "rawapitd.h"
#include "rawapitd-gsin.h"
#include "rawcommn.h"
#include "rawdefn.h"
#include "rawclin.h"
#include "rawentn.h"
#include "rawapplcli.h"

extern FILE *fLogFile;

static char cPath[MAX_FULL_FILE] = "";
static char cNamefs[MAX_OBJ_FS] = "";
static char cNamehl[MAX_OBJ_HL] = "";
static char *pcNull = (char *) "";/* to be returned in case of error */
                        /* return(NULL) works in AIX, but not in VMS */

static int iObjAttr = sizeof(srawObjAttr);
static int ichar = sizeof(char);

/*********************************************************************
 * rawAddStrings
 *    add 2 messages avoiding string overflows
 * created 7.5.2009, Horst Goeringer
 *********************************************************************
 */

int rawAddStrings(char *pcMsg1,       /* 1st string, always complete */
                  int iMaxLength1,          /* max length of string1 */
                  char *pcMsg2,        /* added string, possibly cut */
                  int iMaxLength2,          /* max length of string2 */
                  int iErrno,   /* = 0: add contents of string2
                                   > 0: add system error msg (errno) */
                  char *pcMsgE,      /* error msg in case of problem */
                  int iMaxLengthE)     /* max length of error string */
{
   char cModule[32]="rawAddStrings";
   int iDebug = 0;

   int iRC = 0;   /* = 0: okay
                     < 0: problem:
                        =-1: specified string too large
                        =-2: string without contents provided
                        =-3: max length <=0 provided
                        =-4: iErrno, but string2 < STATUS_LEN byte
                        =-5: error string < STATUS_LEN byte provided
                     > 0: no. of chars cut from the 2nd string */
   int iString1 = 0;
   int iString2 = 0;
   int iStringE = 0;
   int iString = 0;                     /* sum of string1 + string2 */
   int iMinLen = STATUS_LEN;               /* for pcMsgE, if needed */
   char cMsg[STATUS_LEN] = "";                     /* internal copy */
   char *pcc;

   iString1 = strlen(pcMsg1);
   iString2 = strlen(pcMsg2);
   iString = iString1 + iString2;

   if (iDebug)
   {
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);
      fprintf(fLogFile, "    string1 (%d byte, max %d byte):\n    %s",
         iString1, iMaxLength1, pcMsg1);
      if (iErrno)
         fprintf(fLogFile, "    add msg for errno=%d\n", errno);
      else
         fprintf(fLogFile, "    add string2 (%d byte, max %d byte):\n    %s",
            iString2, iMaxLength2, pcMsg2);
   }

   if ( (iString1 <= iMaxLength1) &&
        (iString2 <= iMaxLength2) )
   {
      if (iErrno)
      {
         if (iMaxLength2 >= iMinLen)
         {
            sprintf(pcMsg2, "    %s\n", strerror(errno));
            iString2 = strlen(pcMsg2);
            iString = iString1 + iString2;
         }
         else
         {
            iRC = -4;
            sprintf(cMsg,
               "-E- %s: 2nd string (%d byte) possibly too small for system error: requ. %d byte\n",
               cModule, iMaxLength2, iMinLen);

            goto gErrorMsg;
         }
      } /* (iErrno) */

      if (iString < iMaxLength1)
      {
         strcat(pcMsg1, pcMsg2);
         if (iDebug) fprintf(fLogFile,
            "    returned msg (%d byte):\n    %s", iString, pcMsg1);
      }
      else
      {
         strncat(pcMsg1, pcMsg2, iMaxLength1-iString1-1);
         strncat(pcMsg1, "\n", 1);
         iRC = iString -iMaxLength1;
         if (iDebug)
         {
            fprintf(fLogFile, "    returned msg:\n    %s", pcMsg1);
            pcc = pcMsg2;
            pcc+= iMaxLength1-iString1-1;
            fprintf(fLogFile, "    skipped msg part:\n    %s", pcc);
         }
      }
   }
   else
   {
      if (iMaxLength1 <= 0)
      {
         iRC = -3;
         sprintf(cMsg, "-E- %s: 1st string length (%d byte) invalid\n",
            cModule, iMaxLength1);
      }
      if (iMaxLength2 <= 0)
      {
         iRC = -3;
         sprintf(cMsg, "-E- %s: 2nd string length (%d byte) invalid\n",
            cModule, iMaxLength2);
      }
      else if (iString == 0)
      {
         iRC = -2;
         sprintf(cMsg, "-E- %s: empty strings provided\n", cModule);
      }
      else if (iString1 > iMaxLength1)
      {
         iRC = -1;
         sprintf(cMsg, "-E- %s: 1st string longer (%d byte) than length provided (%d)\n",
            cModule, iString1, iMaxLength1);
      }
      else if ( (iString2 > iMaxLength2) && (iErrno == 0) )
      {
         iRC = -1;
         sprintf(cMsg, "-E- %s: 2nd string longer (%d byte) than length provided (%d)\n",
            cModule, iString2, iMaxLength2);
      }
   }

   if (iRC >= 0)
   {
      strcpy(pcMsgE, "");
      goto gEndAddStrings;
   }

gErrorMsg:
   iStringE = strlen(cMsg);
   if (iDebug) fprintf(fLogFile,
      "    error string (%d byte, max %d byte):\n    %s\n",
      iStringE, iMaxLengthE, cMsg);

   if (iMaxLengthE < iStringE)
   {
      iRC = -5;
      fprintf(fLogFile, "-E- %s: size error string too small: max %d byte (needed %d)",
         cModule, iMaxLengthE, iStringE);
      strncat(pcMsgE, cMsg, iMaxLengthE);
      fprintf(fLogFile, ", provided: %s\n", pcMsgE);
   }
   else
   {
      if (iDebug)
         fprintf(fLogFile, "%s", cMsg);
      strcpy(pcMsgE, cMsg);
   }

gEndAddStrings:
   if (iDebug)
      fprintf(fLogFile, "-D- end %s: rc=%d\n\n", cModule, iRC);

   return iRC;

} /* rawAddStrings */

/********************************************************************
 * rawCheckClientFile:
 *    check client file name for gStore naming conventions
 *    verify that file path not generic
 *    if tape specified get tape drive name
 *    returns 0 if valid disk file name
 *    returns >0 if valid tape file name
 *
 * created 15. 3.96, Horst Goeringer
 ********************************************************************
 */

int rawCheckClientFile(
      char *pcFullFile, char *pcFile, char *pcTape)
{

   char cModule[32]="rawCheckClientFile";
   int iDebug = 0;

   int iloc, iRC, ii;
   int iDevType = 1;                                /* default: disk */

   bool_t bup = bFalse;

   char cText[12] = "";
   char cTemp[MAX_FULL_FILE] = "";
   char *plocd = NULL, *plocs = NULL;
   char *pdelim = NULL, *pgen = NULL;
   const char *pcSemi = ";";
   char *pcc = NULL;

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: test file %s\n", cModule, pcFullFile);

   strcpy(cTemp, pcFullFile);

   /* check if device name specified */
   plocd = strchr(cTemp, *pcDevDelim);
   if (plocd != NULL)
   {
      iRC = strncmp(cTemp, "/dev", 4);
      {
         fprintf(fLogFile,
            "-E- invalid file name: %s\n    path names must not contain '%s'\n",
            pcFullFile, pcDevDelim);
         return(-2);
      }

      if (iRC)
      pdelim = strrchr(cTemp, *pcFileDelim);
      if ( (pdelim == NULL) || (pdelim > plocd) )
      {
         fprintf(fLogFile,
            "-E- invalid disk file name: %s\n    path names must not contain '%s'\n",
            pcFullFile, pcDevDelim);
         return(-2);
      }

      pcc = plocd;
      pcc++;
      if (strlen(pcc) > MAX_TAPE_FILE)
      {
         ii = MAX_TAPE_FILE;
         fprintf(fLogFile,
            "-E- invalid tape file name: %s\n    name too long: max %d byte after dev name\n",
            pcFullFile, ii);
         return(-2);
      }

      iDevType = 2;        /* tape, if Unix; if VMS: get it later */
      iRC = strncmp(++pcc, "\0", 1);
      if (iRC == 0)
      {
#ifdef VMS
         fprintf(fLogFile,
            "-E- file name must be specified explicitly\n");
#else
         fprintf(fLogFile, "-E- file name missing in %s\n", pcFullFile);
#endif
         return(-2);
      }
   } /* (plocd != NULL) */

#ifdef VMS
   /* eliminate version number */
   plocs = strchr(cTemp, *pcSemi);
   if (plocs)
   {
      ++plocs;
      iRC = isalnum(*plocs);
      if (iRC)                      /* char after semicolon alphanum. */
      {
         iRC = isalpha(*plocs);
         if (iRC == 0)              /* char after semicolon no letter */
            fprintf(fLogFile,
               "-W- version number in file specification removed\n");
         else
            fprintf(fLogFile,
               "-W- invalid version in file specification removed\n");
      }
      else
         fprintf(fLogFile,
            "-W- semicolon in file specification removed\n");
      strncpy(--plocs, "\0", 1);
      strcpy(pcFullFile, cTemp);
   }

   iDevType = rawCheckDevice(cTemp, &pcFile, &pcTape);
   switch(iDevType)
   {
      case 0:
         if (iDebug) fprintf(fLogFile,
            "    no device name specified\n");
         break;
      case 1:
         if (iDebug)
         {
            if (strlen(pcFile) == 0)
               fprintf(fLogFile, "   disk device %s\n", pcTape);
            else
               fprintf(fLogFile,
                  "    file %s on disk %s\n", pcFile, pcTape);
         }
         break;
      case 2:
         if (iDebug)
         {
            if (strlen(pcFile) == 0)
               fprintf(fLogFile, "   tape device %s\n", pcTape);
            else
               fprintf(fLogFile, "    file %s on tape %s\n", pcFile, pcTape);
         }
         break;
      default:
         fprintf(fLogFile, "-E- invalid file name %s\n", pcFullFile);
         return(-1);

   } /* switch(iDevType) */
#endif /* VMS */

   if (iDevType == 2)                                       /* tape */
   {
#ifndef VMS
      /* check string after 1st device delimiter */
      iRC = strncmp(pcc, pcDevDelim, 1);
      if (iRC == 0)
      {
         fprintf(fLogFile,
            "-E- node specification not allowed in file name: %s\n",
            pcFullFile);
         return(-2);
      }
#endif /* Unix */

      strncpy(plocd++, "\0", 1);              /* cut string at colon */
      strcpy(pcTape, cTemp);
#ifdef VMS
      strncat(pcTape, pcDevDelim, 1);         /* append colon in VMS */

#else
      pgen = strchr(pcTape, *pcStar);
      if (pgen != NULL)
      {
         fprintf(fLogFile,
            "-E- specified device %s has generic path\n", pcTape);
         fprintf(fLogFile,
            "    only the relative file name may be generic\n");
         return(-1);
      }
#endif

      strcpy(pcFile, plocd);
      if (iDebug) fprintf(fLogFile,
         "    file %s on tape %s\n", pcFile, pcTape);
      strcpy(cText, "tape");
      iRC = 1;

   } /* (iDevType == 2) */
   else
   {
      strcpy(pcFile, cTemp);
      strcpy(cText, "disk");
      iRC = 0;
   }

   if (iDebug) fprintf(fLogFile,
      "    %s file %s\n", cText, pcFile);

   pdelim = strrchr(pcFile, *pcFileDelim);
   if (pdelim != NULL)                             /* path specified */
   {
#ifndef VMS
      if (iDevType == 2)                             /* file on tape */
      {
         fprintf(fLogFile,
            "-E- path in tape file name not allowed: %s\n", pcFile);
         return(-1);
      }
#endif

      /* disk file prefixed with path */
      pgen = strchr(pcFile, *pcStar);
      if (pgen == NULL)
         pgen = strchr(pcFile, *pcQM);
      if ( (pgen != NULL) && (pgen < pdelim) && (iDebug) )
         fprintf(fLogFile,
            "    specified %s file %s has wildcard chars in path name\n",
            cText, pcFile);

      pdelim++;                          /* points to rel. file name */

   } /* (pdelim != NULL) */
   else pdelim = pcFile;

   ii = MAX_OBJ_LL - 2;
   if (strlen(pdelim) > ii)
   {
      fprintf(fLogFile,
         "-E- rel file name %s too long:\n    has %d chars, max allowed %d\n",
         pdelim, (int) strlen(pdelim), ii);
      return(-1);
   }

#ifndef VMS
   /* tape: convert file name from uppercase if necessary */
   if (iDevType == 2)
   {
      pcc = pdelim;
      while (*pcc != '\0')
      {
         iloc = isupper(*pcc);
         if (iloc != 0)
         {
            if (!bup) fprintf(fLogFile,
               "-W- upper case in (relative) file name not supported: %s\n",
               pdelim);
            bup = bTrue;
            *pcc = tolower(*pcc);            /* change to lower case */
         }
         pcc++;
      } /* while (*pcc != '\0') */
   }

   if (bup)
      fprintf(fLogFile, "    instead assumed: %s\n", pdelim);
#endif

   if (iDebug) fprintf(fLogFile,
      "-D- end %s: file name %s okay\n\n", cModule, pcFullFile);

   return(iRC);

} /* rawCheckClientFile */

/*********************************************************************
 * rawDelFile: delete single file in GSI mass storage
 *
 * created  8.10.1996, Horst Goeringer
 *********************************************************************
 */

int rawDelFile( int iSocket, srawComm *psComm)
{
   char cModule[32] = "rawDelFile";
   int iDebug = 0;

   int iFSidRC = 0;
   int iFSidWC = 0;

   int iRC;
   int iBufComm;
   char *pcc;
   void *pBuf;
   char cMsg[STATUS_LEN] = "";

   srawStatus sStatus;

   iBufComm = ntohl(psComm->iCommLen) + HEAD_LEN;
   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: delete file %s%s%s\n",
      cModule, psComm->cNamefs, psComm->cNamehl, psComm->cNamell);

   if (psComm->iStageFSid)
      iFSidRC = ntohl(psComm->iStageFSid);
   if (psComm->iFSidWC)
      iFSidWC = ntohl(psComm->iFSidWC);

   if (iDebug)
   {
      fprintf(fLogFile,
         "    object %s%s%s found (objId %u-%u)",
         psComm->cNamefs, psComm->cNamehl, psComm->cNamell,
         ntohl(psComm->iObjHigh), ntohl(psComm->iObjLow));
      if (iFSidRC) fprintf(fLogFile,
         ", on %s in read cache FS %d\n", psComm->cNodeRC, iFSidRC);
      else
         fprintf(fLogFile, "\n");
      if (iFSidWC) fprintf(fLogFile,
         "     on %s in write cache FS %d\n", psComm->cNodeWC, iFSidWC);
   }

   psComm->iAction = htonl(REMOVE);

   pcc = (char *) psComm;
   if ( (iRC = send( iSocket, pcc, (unsigned) iBufComm, 0 )) < iBufComm)
   {
      if (iRC < 0) fprintf(fLogFile,
         "-E- %s: sending delete request for file %s (%d byte)\n",
         cModule, psComm->cNamell, iBufComm);
      else fprintf(fLogFile,
         "-E- %s: delete request for file %s (%d byte) incompletely sent (%d byte)\n",
         cModule, psComm->cNamell, iBufComm, iRC);
      if (errno)
         fprintf(fLogFile, "    %s\n", strerror(errno));
      perror("-E- sending delete request");

      return -1;
   }

   if (iDebug) fprintf(fLogFile,
      "    delete command sent to server (%d bytes), look for reply\n",
      iBufComm);

   /******************* look for reply from server *******************/

   iRC = rawRecvStatus(iSocket, &sStatus);
   if (iRC != HEAD_LEN)
   {
      if (iRC < HEAD_LEN) fprintf(fLogFile,
         "-E- %s: receiving status buffer\n", cModule);
      else
      {
         fprintf(fLogFile,
            "-E- %s: message received from server:\n", cModule);
         fprintf(fLogFile, "%s", sStatus.cStatus);
      }

      if (iDebug)
         fprintf(fLogFile, "\n-D- end %s\n\n", cModule);

      return(-1);
   }

   if (iDebug) fprintf(fLogFile,
      "    status (%d) received from server (%d bytes)\n",
      sStatus.iStatus, iRC);

   /* delete in this proc only if WC and RC, 2nd step WC */
   if (iDebug)
   {
      fprintf(fLogFile,
         "-I- gStore object %s%s%s successfully deleted",
         psComm->cNamefs, psComm->cNamehl, psComm->cNamell);
      if (iFSidWC)
         fprintf(fLogFile, " from write cache\n");
      else
         fprintf(fLogFile, "\n");

      fprintf(fLogFile, "-D- end %s\n\n", cModule);
   }

   return 0;

} /* end rawDelFile */

/*********************************************************************
 * rawDelList:  delete list of files in GSI mass storage
 *
 * created 22.10.1997, Horst Goeringer
 *********************************************************************
 */

int rawDelList( int iSocketMaster,
                            /* socket for connection to entry server */
                int iDataMover,                /* no. of data movers */
                srawDataMoverAttr *pDataMover0,
                srawComm *psComm,
                char **pcFileList,
                char **pcObjList)
{
   char cModule[32] = "rawDelList";
   int iDebug = 0;

   int iSocket;
   char *pcfl, *pcol;
   int *pifl, *piol;
   int ifile;
   int iobj0, iobj;
   int iobjBuf;

   srawFileList *psFile, *psFile0;           /* files to be archived */
   srawRetrList *psObj;                  /* objects already archived */
   srawDataMoverAttr *pDataMover;              /* current data mover */

   char *pcc, *pdelim;

   bool_t bDelete, bDelDone;
   int **piptr;                  /* points to pointer to next buffer */

   int ii, jj, kk;
   int iRC, idel;

   pcfl = *pcFileList;
   pifl = (int *) pcfl;
   ifile = pifl[0];                            /* number of files */
   psFile = (srawFileList *) ++pifl;  /* points now to first file */
   psFile0 = psFile;
   pifl--;                     /* points again to number of files */

   if (iDebug)
   {
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);
      fprintf(fLogFile, "    initial %d files, first file %s\n",
         ifile, psFile0->cFile);
   }

   pDataMover = pDataMover0;
   pcol = *pcObjList;
   piol = (int *) pcol;
   iobj0 = 0;                     /* total no. of archived objects */

   iobjBuf = 0;                             /* count query buffers */
   idel = 0;                                /* count deleted files */
   bDelDone = bFalse;
   while (!bDelDone)
   {
      iobjBuf++;
      iobj = piol[0];            /* no. of objects in query buffer */
      psObj = (srawRetrList *) ++piol;
                                     /* points now to first object */
      if (iDebug) fprintf(fLogFile,
         "    buffer %d: %d objects, first obj %s%s (server %d)\n",
         iobjBuf, iobj, psObj->cNamehl, psObj->cNamell,
         psObj->iATLServer);

      psComm->iAction = htonl(REMOVE);
      for (ii=1; ii<=iobj; ii++)    /* loop over objects in buffer */
      {
         iobj0++;
         pcc = (char *) psObj->cNamell;
         pcc++;                          /* skip object delimiter  */

         if (iDebug) fprintf(fLogFile,
            "    obj %d: %s%s, objId %d-%d\n",
            ii, psObj->cNamehl, psObj->cNamell,
            psObj->iObjHigh, psObj->iObjLow);

         bDelete = bFalse;
         psFile = psFile0;
         for (jj=1; jj<=ifile; jj++)                  /* file loop */
         {
            if (iDebug) fprintf(fLogFile,
               "    file %d: %s\n", jj, psFile->cFile);

            pdelim = strrchr(psFile->cFile, *pcFileDelim);
            if (pdelim == NULL)
            {
#ifdef VMS
               pdelim = strrchr(psFile->cFile, *pcFileDelim2);
               if (pdelim != NULL) pdelim++;
               else
#endif
               pdelim = psFile->cFile;
            }
            else pdelim++;                 /* skip file delimiter  */

            iRC = strcmp(pdelim, pcc);
            if ( iRC == 0 )
            {
               bDelete = bTrue;
               break;
            }
            psFile++;
         } /* file loop (jj) */

         if (bDelete)
         {
            if (iDebug)
            {
               fprintf(fLogFile, "    matching file %d: %s, obj %d: %s%s",
                      jj, psFile->cFile, ii, psObj->cNamehl, psObj->cNamell);

               if (psObj->iStageFS) fprintf(fLogFile,
                   ", on DM %s in StageFS %d\n",
                   psObj->cMoverStage, psObj->iStageFS);
               else if (psObj->iCacheFS)
               {
                  fprintf(fLogFile, ", on DM %s in ArchiveFS %d\n",
                         psObj->cMoverStage, psObj->iCacheFS);
                  fprintf(fLogFile, "    archived at %s by %s\n",
                         psObj->cArchiveDate, psObj->cOwner);
               }
               else
                  fprintf(fLogFile," (not in disk pool)\n");
            }

            psComm->iObjHigh = htonl(psObj->iObjHigh);
            psComm->iObjLow = htonl(psObj->iObjLow);
            psComm->iATLServer = htonl(psObj->iATLServer);

            if (psObj->iStageFS)
            {
               psComm->iPoolIdRC = htonl(psObj->iPoolIdRC);
               psComm->iStageFSid = htonl(psObj->iStageFS);
               strcpy(psComm->cNodeRC, psObj->cMoverStage);
            }
            else
            {
               psComm->iPoolIdRC = htonl(0);
               psComm->iStageFSid = htonl(0);
               strcpy(psComm->cNodeRC, "");
            }

            if (psObj->iCacheFS)
            {
               if (psObj->iStageFS)
                  psComm->iPoolIdWC = htonl(0); /* WC poolId unavail */
               else
                  psComm->iPoolIdWC = htonl(psObj->iPoolIdRC);
               psComm->iFSidWC = htonl(psObj->iCacheFS);
               strcpy(psComm->cNodeWC, psObj->cMoverCache);
            }
            else
            {
               psComm->iPoolIdWC = htonl(0);
               psComm->iFSidWC = htonl(0);
               strcpy(psComm->cNodeWC, "");
            }

            iRC = rawGetLLName(psFile->cFile,
                               pcObjDelim, psComm->cNamell);

            if (iDataMover > 1)
            {
               if ((strcmp(pDataMover->cNode, psObj->cMoverStage) == 0) ||
                   (strlen(psObj->cMoverStage) == 0))  /* not staged */
               {
                  iSocket = pDataMover->iSocket;
                  if (iDebug) fprintf(fLogFile,
                     "    current data mover %s, socket %d\n",
                     pDataMover->cNode, iSocket);
               }
               else
               {
                  pDataMover = pDataMover0;
                  for (kk=1; kk<=iDataMover; kk++)
                  {
                     if (strcmp(pDataMover->cNode,
                                psObj->cMoverStage) == 0)
                        break;
                     pDataMover++;
                  }

                  if (kk > iDataMover)
                  {
                     fprintf(fLogFile,
                        "-E- %s: data mover %s not found in list\n",
                        cModule, psObj->cMoverStage);

                     return -1;
                  }

                  iSocket = pDataMover->iSocket;
                  if (iDebug) fprintf(fLogFile,
                     "    new data mover %s, socket %d\n",
                     pDataMover->cNode, iSocket);
               }
            } /* (iDataMover > 1) */

            iRC = rawDelFile(iSocketMaster, psComm);
            if (iRC)
            {
               if (iDebug)
                  fprintf(fLogFile, "    rawDelFile: rc = %d\n", iRC);
               if (iRC < 0)
               {
                  fprintf(fLogFile,
                     "-E- %s: file %s could not be deleted\n",
                     cModule, psFile->cFile);
                  if (iDebug)
                     fprintf(fLogFile, "-D- end %s\n\n", cModule);

                  return -1;
               }
               /* else: object not found, ignore */

            } /* (iRC) */

            idel++;

         } /* if (bDelete) */
         else if (iDebug) fprintf(fLogFile,
            "    file %s: obj %s%s not found in gStore\n",
            psFile0->cFile, psObj->cNamehl, psObj->cNamell);

         psObj++;

      } /* loop over objects in query buffer (ii) */

      piptr = (int **) psObj;
      if (*piptr == NULL) bDelDone = bTrue;
      else piol = *piptr;

   } /* while (!bDelDone) */

   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return(idel);

} /* rawDelList */

/**********************************************************************
 * rawGetFSpName
 *    get file space name from user specification
 * created 22.3.1996, Horst Goeringer
 **********************************************************************
 */

char *rawGetFSpName( char *pcUser )
{
   char cModule[32]="rawGetFSpName";
   int iDebug = 0;

   char *pc;

   if (iDebug)
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);

   pc = pcUser;
   if ( (strchr(pcUser, *pcStar) != NULL) ||
        (strchr(pcUser, *pcQM) != NULL) )
   {
      fprintf(fLogFile,
         "-E- %s: generic archive name '%s' not allowed\n",
         cModule, pcUser);
      return(pcNull);
   }

   strcpy(cNamefs, "");                    /* initialize */
   if (iDebug)
      fprintf(fLogFile, "-D- %s: FS %s, in %s-%s, delim %s\n",
              cModule, cNamefs, pcUser, pc, pcObjDelim);
   if (strncmp(pc, pcObjDelim, 1) == 0)
      pc++;
   else
      /* strncpy(cNamefs, pcObjDelim, 1); */
      /* in adsmcli session: beginning with 2nd invocation,
         gives incorrect results due to missing \0 ! H.G. 27.10.97 */
      strcpy(cNamefs, pcObjDelim);

   if (isalpha(*pc) == 0)
   {
      fprintf(fLogFile,
              "-E- archive name '%s' must start with a letter\n", pc);
      return(pcNull);
   }

   strcat(cNamefs, pcUser);
   if (iDebug) fprintf(fLogFile,
      "-D- end %s: FS %s\n", cNamefs, cModule);

   return( (char *) cNamefs);

} /* rawGetFSpName */

/*********************************************************************
 * rawGetHLName: get high level object name from path
 *
 * insert name part at beginning depending on device
 * PLATFORM DEPENDENT
 *
 * created 16. 2.96, Horst Goeringer
 *********************************************************************
 */

char *rawGetHLName( char *pcPath)
{
   char cModule[32]="rawGetHLName";
   int iDebug = 0;

   char *pcc;
   char *pdelim;
   char cNameTemp[MAX_OBJ_HL] = "";
   int ii = 0, ii1 = 0;

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s\n    path specified: %s\n", cModule, pcPath);

   pdelim = strchr(pcPath, *pcObjDelim);
   if (pdelim != pcPath)
   {
      strcpy(cNamehl, pcObjDelim);
      if (iDebug) fprintf(fLogFile,
         "    delimiter '%s' inserted at begin of archive path\n",
         pcObjDelim);
      strcat(cNamehl, pcPath);
   }
   else strcpy(cNamehl, pcPath);

   pdelim = strstr(cNamehl, "/./");
   if (pdelim != NULL)
   {
      strcpy(cNameTemp, cNamehl);
      pdelim = strstr(cNameTemp, "/./");

      while (pdelim != NULL)
      {
         ii++;
         strncpy(pdelim, "\0", 1);
         strcpy(cNamehl, cNameTemp);
         pdelim++;
         pdelim++;
         strcat(cNamehl, pdelim);

         strcpy(cNameTemp, cNamehl);
         pdelim = strstr(cNameTemp, "/./");
      }
      if (iDebug) fprintf(fLogFile,
         "    %d unnecessary './' removed\n", ii);
   }

   pdelim = strstr(cNamehl, "/.");
   if (pdelim != NULL)
   {
      pcc= cNamehl;
      ii = strlen(cNamehl);
      ii1 = pdelim - pcc;
      if (ii1 < 0)
         ii1 = -ii1;
      if (ii1+2 == ii)
      {
         strncpy(pdelim, "\0", 1);
         if (iDebug) fprintf(fLogFile,
            "    trailing '/.' removed\n");
      }
   }

   if (iDebug) fprintf(fLogFile,
      "    hl name: %s\n-D- end %s\n\n", cNamehl, cModule);

   return(cNamehl);

} /* rawGetHLName */

/*********************************************************************
 * rawGetLLName: get low level object name from file name
 *    PLATFORM DEPENDENT
 *
 * created 16. 2.1996, Horst Goeringer
 *********************************************************************
 *  9. 1.2002, H.G.: pass object and delimiter as argument
 *********************************************************************
 */

int rawGetLLName( char *pcFile, const char *pcDelimiter,
                  char *pcObject)
{
   char cModule[32] = "rawGetLLName";
   int iDebug = 0;

   char cNamell[MAX_OBJ_LL] = "";
   char *ploc = NULL;

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s\n    file %s, object delimiter %s\n",
      cModule, pcFile, pcDelimiter);

   strcpy(cNamell, pcDelimiter);

   ploc = strrchr(pcFile, *pcFileDelim);
   if (ploc != NULL)
      strcat(cNamell, ++ploc);   /* begin copy after file delimiter */
   else
   {
      ploc = strrchr(pcFile, *pcObjDelimAlt);
      if (ploc != NULL)
         strcat(cNamell, ++ploc);/* begin copy after file delimiter */
      else
      {
#ifdef VMS
         ploc = strrchr(pcFile, *pcDevDelim); /* look for disk device */
         if (ploc != NULL)
            strcat(cNamell, ++ploc);
                                /* begin copy after device delimiter */
         else
#endif
         strcat(cNamell, pcFile);
      }
   }
   strcpy(pcObject, cNamell);

   if (iDebug) fprintf(fLogFile,
      "    ll name %s\n-D- end %s\n\n", pcObject, cModule);

   return 0;

} /* rawGetLLName */

/**********************************************************************
 * rawGetFileSize
 *    get file size (bytes)
 * created 19.2.2002, Horst Goeringer
 **********************************************************************
 */

int rawGetFileSize(char *pcFile,
                   unsigned long *piSize,
                   unsigned int *piRecl)      /* needed only for VMS */
{
   char cModule[32] = "rawGetFileSize";
   int iDebug = 0;

   int iRC;
   int iRecl = 0;
   unsigned long iSize = 0;

#ifdef WIN32          /* Windows */
   struct _stat sFileStatus, *pFileStatus;
#else                 /* Unix or VMS */
   struct stat sFileStatus, *pFileStatus;
#endif

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: file %s\n", cModule, pcFile);

   pFileStatus = &sFileStatus;

#ifdef WIN32          /* Windows */
   iRC = _stat(pcFile, pFileStatus);
#else                 /* Unix or VMS */
   iRC = stat(pcFile, pFileStatus);
#endif
   if (iRC)
   {
      if (iDebug) fprintf(fLogFile,
         "-E- %s: file %s unavailable (stat)\n", cModule, pcFile);
      if (errno)
      {
         if (strcmp(strerror(errno),
               "No such file or directory") != 0)
            fprintf(fLogFile, "    %s\n", strerror(errno));

         /* valid only for 32 bit OS */
         if (strcmp(strerror(errno),
               "Value too large for defined data type") == 0)
         {
            fprintf(fLogFile,
               "-E- file size %s > 2 GByte: use 64 bit gStore client\n",
               pcFile);

            errno = 0;
            return -99;
         }
         errno = 0;
      }

      return -1;
   }

#ifdef VMS
      iRecl = pFileStatus->st_fab_mrs;
#else
#ifdef WIN32
      iRecl = 0;    /* not available in Windows */
#else
      iRecl = pFileStatus->st_blksize;
#endif
#endif
   iSize = pFileStatus->st_size;
   if (pFileStatus->st_mode & S_IFREG)
   {
      iRC = 0;
      if (iDebug) fprintf(fLogFile,
         "    file %s: size %lu, recl %d (byte)\n",
         pcFile, iSize, iRecl);
   }
   else
   {
      if (pFileStatus->st_mode & S_IFDIR)
      {
         iRC = 1;
         if (iDebug) fprintf(fLogFile,
            "-W- %s is a directory, size %lu\n", pcFile, iSize);
      }
#ifndef WIN32
#ifndef VMS
      else if (pFileStatus->st_mode & S_IFLNK)
      {
         iRC = 2;
         if (iDebug) fprintf(fLogFile,
            "-W- %s is a symbolic link, size %lu\n", pcFile, iSize);
      }
#endif
#endif
      else
      {
         iRC = 3;
         if (iDebug) fprintf(fLogFile,
            "-W- unexpected item %s, size %lu\n", pcFile, iSize);
      }
   }

   *piRecl = iRecl;
   *piSize = iSize;
   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return iRC;

} /* rawGetFileSize */

/*********************************************************************
 * rawGetPathName:   get path name from high level object name
 *
 * created 21. 3.1996, Horst Goeringer
 *********************************************************************
 * 23. 4.1996, H.G: no more device dependency in hl-name, as with
 *                  ADSM V213 management class can be set in dsmSendObj
 *********************************************************************
 */

char *rawGetPathName( char *pcNamehl)
{
   char cModule[32] = "rawGetPathName";
   int iDebug = 0;

   char *ploc = NULL, *ploc1 = NULL;
   char *pcc;

   if (iDebug)
      fprintf(fLogFile, "-D- begin %s: hl: %s\n", cModule, pcNamehl);

   if (strlen(pcNamehl) == 0)
   {
      fprintf(fLogFile, "-E- %s: high level name empty\n", cModule);
      return(pcNull);
   }

   pcc = pcNamehl;
   ploc = strchr(pcNamehl, *pcObjDelim);
   ploc1 = strchr(pcNamehl, *pcObjDelimAlt);
   if ( (ploc != pcc) && (ploc1 != pcc) )
   {
      fprintf(fLogFile,
		      "-E- %s: invalid prefix in high level name %s\n",
              cModule, pcNamehl);
      return(pcNull);
   }

   strcpy(cPath, pcc);
   if (iDebug) fprintf(fLogFile,
		"-D- end %s: path %s\n\n", cModule, cPath);

   return cPath;

} /* rawGetPathName */

/********************************************************************
 * rawQueryFile: get query information for single file
 *
 * used for tape input handled sequentially
 * buffers for communication and query info allocated in calling
 *    procedure
 ********************************************************************
 */

int rawQueryFile(
       int iSocket,
       int iAltMaster,                          /* currently unused */
       srawComm *pCommBuf,
       void **pQueryBuf)
{
   char cModule[32] = "rawQueryFile";
   int iDebug = 0;

   int iATLServer = 0;   /* =1: prod TSM server, =2: test TSM server */
   int iNameVar = -1;      /* = 1: path/file contains wildcard chars */
   int iRC;
   int iAction = 0;
   int iIdent = 0;
   int iQuery = -1;
   int  iAttrLen = 0;
   int iQueryAll = 0;
   int iStatus = 0, iStatusLen = 0;
   int iBuf = 0, iBufComm = 0;
   int ii, jj;
   char *pcc;

   int iVersionObjAttr = 0;
      /* version no. of srawObjAttr:
         =3:  288 byte cache buffer, 2 restore fields
         =4:  304 byte, 5 restore fields, ATL server no.
         =5:  384 byte, maior enhancement */
   int iPoolId = 0;
   int iObjLow = 0;
   int iFS = 0;
   int iMediaClass = 0;

   srawComm *pComm;
   srawQueryResult *pQuery;
   srawObjAttr *pObjAttr;

   /* query data space to be allocated, if several objects found */
   char *pcQueryBuffer;
   int iQueryBuffer = 0;

   pComm = pCommBuf;
   iAction = ntohl(pComm->iAction);
   iATLServer = ntohl(pComm->iATLServer);
   pQuery = (srawQueryResult *) (*pQueryBuf);

   iBufComm = ntohl(pComm->iCommLen) + HEAD_LEN;
   if (iDebug)
   {
      fprintf(fLogFile,
         "\n-D- begin %s: query file %s%s in ATL Server %d (socket %d, action %d)\n",
         cModule, pComm->cNamehl, pComm->cNamell, iATLServer, iSocket, iAction);
      fflush(fLogFile);
   }

   switch(iAction)
   {
      case QUERY:
      case QUERY_ARCHIVE:
      case QUERY_ARCHIVE_RECORD:
      case QUERY_ARCHIVE_OVER:
      case QUERY_ARCHIVE_TO_CACHE:
      case QUERY_ARCHIVE_MGR:
      case QUERY_REMOVE:
      case QUERY_REMOVE_MGR:
      case QUERY_RETRIEVE:
      case QUERY_RETRIEVE_RECORD:
      case QUERY_STAGE:
         ;                          /* okay */
         break;
      default:
         fprintf(fLogFile,
            "-E- %s: invalid action %d\n", cModule, iAction);
         iQueryAll = -1;
         goto gEndQueryFile;
   }

   pcc = (char *) pComm;
   iRC = send(iSocket, pcc, (unsigned) iBufComm, 0);
   if (iRC < iBufComm)
   {
      if (iRC < 0) fprintf(fLogFile,
         "-E- %s: sending command buffer for query file %s%s\n",
         cModule, pComm->cNamehl, pComm->cNamell);
      else fprintf(fLogFile,
         "-E- %s: query command buffer incompletely sent (%d of %d byte)\n",
         cModule, iRC, iBufComm);

      if (errno)
      {
         fprintf(fLogFile, "    %s\n", strerror(errno));
         errno = 0;
      }

      iQueryAll = -1;
      goto gEndQueryFile;
   }

   if (iDebug) fprintf(fLogFile,
      "    query command sent to entry server (%d bytes), look for reply\n",
      iBufComm);
   fflush(fLogFile);

   /******************* look for reply from server *******************/

gNextReply:
   pcc = (char *) pQuery;
   iRC = rawRecvHead(iSocket, pcc);
   if (iRC <= 0)
   {
      fprintf(fLogFile,
         "-E- %s: receiving buffer header with query information\n",
         cModule);
      iQueryAll = -1;
      goto gEndQueryFile;
   }

   pcc += HEAD_LEN;
   iIdent = ntohl(pQuery->iIdent);
   iQuery = ntohl(pQuery->iObjCount);
   iAttrLen = ntohl(pQuery->iAttrLen);

   if (iDebug) fprintf(fLogFile,
      "    %d bytes received: iIdent %d, iQuery %d, iAttrLen %d\n",
      iRC, iIdent, iQuery, iAttrLen);
   fflush(fLogFile);

   iQueryAll += iQuery;
   if ( (iIdent != IDENT_QUERY) && (iIdent != IDENT_QUERY_DONE) &&
        (iIdent != IDENT_QUERY_ARCHDB) )
   {
      iStatus = iQuery;
      iQueryAll = -1;                                /* query failed */

      if (iIdent == IDENT_STATUS)
      {
         iStatusLen = iAttrLen;
         if ( (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) ||
              (iStatus == STA_ARCHIVE_NOT_AVAIL) ||
              (iStatus == STA_NO_ACCESS) )
         {
            if ( (iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) )
               fprintf(fLogFile,
                  "-E- %s: error status from entry server instead of query data:\n",
                  cModule);

            if (iStatusLen > 0)
            {
               iRC = rawRecvError(iSocket, iStatusLen, pcc);
               if (iRC < 0) fprintf(fLogFile,
                  "-E- receiving error msg from server, rc = %d\n",
                  iRC);
               else if ( (iDebug) || (iStatus == STA_ERROR) ||
                         (iStatus == STA_ERROR_EOF) )
                  fprintf(fLogFile, "%s", pcc);
            }
            else
               fprintf(fLogFile,"    no error message available\n");

            if (iStatus == STA_ARCHIVE_NOT_AVAIL)
               iQueryAll = -1000;
            else if (iStatus == STA_NO_ACCESS)
               iQueryAll = -1001;

         } /* (iStatus == STA_ERROR || STA_ERROR_EOF ||
               STA_ARCHIVE_NOT_AVAIL || STA_NO_ACCESS) */
         else fprintf(fLogFile,
           "-E- %s: unexpected status (type %d) received from server\n",
           cModule, iStatus);

      } /* iIdent == IDENT_STATUS */
      else fprintf(fLogFile,
         "-E- %s: unexpected header (type %d) received from server\n",
         cModule, iIdent);

      goto gEndQueryFile;

   } /* iIdent != IDENT_QUERY  && != IDENT_QUERY_DONE &&
               != IDENT_QUERY_ARCHDB */

   if (iQuery > 0)
   {
      if (iQuery > 1)
      {
         iQueryBuffer = iQuery * iAttrLen;
         if ( (pcQueryBuffer =
              (char *) calloc(iQueryBuffer, ichar) ) == NULL)
         {
            fprintf(fLogFile,
               "-E- %s: allocating query info buffer for %d objs (%d byte)\n",
               cModule, iQuery, iQueryBuffer);
            if (errno)
               fprintf(fLogFile, "    %s\n", strerror(errno));

            goto gEndQueryFile;
         }
         pcc = pcQueryBuffer;

         if (iDebug) fprintf(fLogFile,
            "    query info buffer for %d objs allocated (%d byte)\n",
            iQuery, iQueryBuffer);

      } /* (iQuery > 1) */
      else
      {
         pcc = (char *) pQuery + HEAD_LEN;
         iQueryBuffer = iAttrLen;
      }

      pObjAttr = (srawObjAttr *) pcc;
      iBuf = iQueryBuffer;
      while(iBuf > 0)
      {
         if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
         {
            if (iRC < 0)
            {
               fprintf(fLogFile,
                  "-E- %s: receiving buffer with query results (%d byte)\n",
                  cModule, iQueryBuffer);

               iQueryAll = -1;
            }
            else
            {
               jj = iQueryBuffer - iBuf;
               fprintf(fLogFile,
                  "-E- %s: connection to server broken, only %d bytes of query result (%d byte) received\n",
                  cModule, jj,  iQueryBuffer);
            }

            if (errno)
            {
               fprintf(fLogFile, "    %s\n", strerror(errno));
               errno = 0;
            }

            goto gEndQueryFile;
         }

         iBuf -= iRC;
         pcc += iRC;

      } /* while(iBuf > 0) */

      if (iDebug) fprintf(fLogFile,
         "    query data of %d objs received (%d byte)\n",
         iQuery, iQueryBuffer);

      if (iQuery > 1)
         pObjAttr = (srawObjAttr *) pcQueryBuffer;
      else
      {
         pcc = (char *) pQuery + HEAD_LEN;
         pObjAttr = (srawObjAttr *) pcc;
      }

      for (ii=1; ii<=iQuery; ii++)
      {
         iPoolId = ntohl(pObjAttr->iPoolId);

         if (iDebug)
         {
            fprintf(fLogFile, "%d: %s%s%s (poolId %d)\n",
               ii, pObjAttr->cNamefs, pObjAttr->cNamehl,
               pObjAttr->cNamell, iPoolId);
         }

         /* fill comm. buffer values */
         if (iQuery == 1)
         {
            pComm->iObjHigh = pObjAttr->iObjHigh;      /* net format */
            pComm->iObjLow = pObjAttr->iObjLow;
            iMediaClass = ntohl(pObjAttr->iMediaClass);

            if ( (ntohl(pObjAttr->iObjHigh) == 0) &&
                 (ntohl(pObjAttr->iObjLow) == 0) )
            {
               if ( (iMediaClass == GSI_MEDIA_CACHE) ||
                    (iMediaClass == GSI_CACHE_LOCKED) ||
                    (iMediaClass == GSI_CACHE_COPY) ||
                    (iMediaClass == GSI_CACHE_INCOMPLETE) )
               {
                  pComm->iPoolIdWC = pObjAttr->iPoolId;
                  pComm->iFSidWC = pObjAttr->iFS;
                  strcpy(pComm->cNodeWC, pObjAttr->cNode);

               } /* file in write cache and not staged */
               else
               {
                  if ( (iPoolId == 1) || (iPoolId == 2) ||
                       (iPoolId == 5) )
                  {
                     /* historically: ObjHigh/Low not available */
                     if ( (iMediaClass == GSI_MEDIA_STAGE) ||
                          (iMediaClass == GSI_MEDIA_LOCKED) ||
                          (iMediaClass == GSI_MEDIA_INCOMPLETE) )
                     {
                        /* also in read cache */
                        pComm->iPoolIdRC = pObjAttr->iPoolId;
                        pComm->iStageFSid = pObjAttr->iFS;
                        strcpy(pComm->cNodeRC, pObjAttr->cNode);
                     }
                     else fprintf(fLogFile,
                        "-E- %s: unexpected meta data for file %s%s%s: objId High/Low %d-%d, poolId %d, iMediaClass %d\n",
                        cModule, pObjAttr->cNamefs, pObjAttr->cNamehl,
                        pObjAttr->cNamell, pObjAttr->iObjHigh,
                        pObjAttr->iObjLow, iPoolId,
                        iMediaClass);
                  }
                  else fprintf(fLogFile,
                     "-E- %s: inconsistent meta data for file %s%s%s: objId High/Low %d-%d, poolId %d, iMediaClass %d\n",
                     cModule, pObjAttr->cNamefs, pObjAttr->cNamehl,
                     pObjAttr->cNamell, pObjAttr->iObjHigh,
                     pObjAttr->iObjLow, iPoolId, iMediaClass);

                     /* in GRC objIds should be available) */

               } /* file not in write cache */
            } /* (iObjHigh == 0) && (iObjLow == 0) */
            else
            {
               /* file in TSM storage */
               if ( (iMediaClass == GSI_MEDIA_STAGE) ||
                    (iMediaClass == GSI_MEDIA_LOCKED) ||
                    (iMediaClass == GSI_MEDIA_INCOMPLETE) )
               {
                  /* also in read cache */
                  pComm->iPoolIdRC = pObjAttr->iPoolId;
                  pComm->iStageFSid = pObjAttr->iFS;
                  strcpy(pComm->cNodeRC, pObjAttr->cNode);
               }
               else
               {
                  pComm->iPoolIdRC = htonl(0);
                  pComm->iStageFSid = htonl(0);
                  strcpy(pComm->cNodeRC, "");
               }
            }
         } /* (iQuery == 1) */

         iVersionObjAttr = ntohl(pObjAttr->iVersion);
         if ( (iVersionObjAttr != VERSION_SRAWOBJATTR) &&
              (iVersionObjAttr != VERSION_SRAWOBJATTR-1) )
         {
            fprintf(fLogFile,
               "-E- %d: %s%s%s has invalid cacheDB entry version %d\n",
               ii, pObjAttr->cNamefs, pObjAttr->cNamehl,
               pObjAttr->cNamell, iVersionObjAttr);

            continue;
         }

         pObjAttr++;

      } /* object loop */
   } /* (iQuery > 0) */
   else if ( (iQuery == 0) && (iDebug) )
      fprintf(fLogFile, "    no objs found\n");

   /* if object explicitly specified, stop query if found */
   if ( (strchr(pComm->cNamell, *pcStar) == NULL) &&
        (strchr(pComm->cNamell, *pcQM) == NULL) &&
        (strchr(pComm->cNamell, *pcPerc) == NULL) &&
        (strchr(pComm->cNamehl, *pcStar) == NULL) &&
        (strchr(pComm->cNamehl, *pcQM) == NULL) &&
        (strchr(pComm->cNamehl, *pcPerc) == NULL) )
      iNameVar = 0;
   else
      iNameVar = 1;

   if ( (iNameVar == 0) && (iQueryAll > 0) )    /* fixed name, found */
      goto gEndQueryFile;

   if ( (iIdent == IDENT_QUERY) || (iIdent == IDENT_QUERY_ARCHDB) )
      goto gNextReply;

gEndQueryFile:
   switch (iQueryAll)
   {
      case 0:
         if (iDebug) fprintf(fLogFile,
            "    no matching object %s%s%s in gStore found\n",
            pComm->cNamefs, pComm->cNamehl, pComm->cNamell);
         break;
      case 1:
         if (iDebug) fprintf(fLogFile,
            "    file %s%s%s available in gStore\n",
            pObjAttr->cNamefs, pComm->cNamehl, pComm->cNamell);
         break;
      case -1:
         fprintf(fLogFile,
            "-E- %s: query in gStore could not be executed\n",
            cModule);
         break;
      case -1000:
         if (iDebug) fprintf(fLogFile,
            "    requested archive %s not existing in gStore\n",
            pComm->cNamefs);
         break;
      case -1001:
         if (iDebug) fprintf(fLogFile,
            "    access to requested file %s%s%s not allowed\n",
            pObjAttr->cNamefs, pComm->cNamehl, pComm->cNamell);
         break;
      default:
         if ( (iQueryAll > 1) && (iNameVar == 0) )
         {
            fprintf(fLogFile, "    %d versions of %s%s exist!\n",
               iQueryAll, pComm->cNamehl, pComm->cNamell);
         }
   } /* switch (iQueryAll) */

   if (iDebug)
   {
      fprintf(fLogFile, "-D- end %s\n\n", cModule);
      fflush(fLogFile);
   }

   return iQueryAll;

} /* end rawQueryFile */

/*********************************************************************/
/* rawRecvError:   receive error message */
/*    returns length of error message */
/* */
/* created 14. 3.96, Horst Goeringer */
/*********************************************************************/

int rawRecvError(int iSocket, int iLen, char *pcMsg)
{
   char cModule[32]="rawRecvError";
   int iDebug = 0;

   int iRC = 0, ii;
   int iBuf, iBufs;
   char *pcc;

   if (iDebug)
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);

   strcpy(pcMsg, "");
   pcc = pcMsg;         /* points now to buffer in calling procedure */

   iBuf = iLen;                           /* length of error message */
   iBufs = iBuf;
   while(iBuf > 0)
   {
      if ( (iRC = recv(iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
      {
         if (iRC < 0)
         {
            fprintf(fLogFile,
               "-E- %s: receiving error message\n", cModule);

            if (errno)
            {
               fprintf(fLogFile, "    %s\n", strerror(errno));
               errno = 0;
            }

            iRC = -9;
            break;
         }
         else
         {
            ii = iLen - iBuf;
            if (ii) /* append delimiters after part of message received */
            {
               *pcc = '\0';                   /* delimit message string */
               pcc++;
               *pcc = '\n';
               pcc++;
               if (iDebug) fprintf(fLogFile,
                  "-E- incomplete error message received:\n    %s", pcMsg);
            }
            fprintf(fLogFile,
               "-E- %s: connection to sender broken, %d byte of error message (%d byte) received\n",
               cModule, ii, iLen);

            if (errno)
            {
               fprintf(fLogFile, "    %s\n", strerror(errno));
               errno = 0;
            }

            iRC = -8;
            break;
         }
      }

      iBuf -= iRC;
      pcc += iRC;

   } /* while(iBuf > 0) */

   if (iBuf < 0)
   {
      fprintf(fLogFile,
         "-E- %s: more error data received than expected:\n     %s",
         pcMsg, cModule);
      iRC = -2;
   }

   if (iRC == -9)
   {
      if (iDebug)
         fprintf(fLogFile, "-D- end %s\n\n", cModule);

      return iRC;
   }
   else
   {
      /* also okay if no message (iLen = 0) */
      *pcc = '\0';                         /* delimit message string */

      /* only msg part received */
      if (iRC == -8)
         iBufs = ii;

      if (iDebug)
      {
         fprintf(fLogFile,
            "    error message received (%d bytes):\n    %s",
            iBufs, pcMsg);
         fprintf(fLogFile, "-D- end %s\n\n", cModule);
      }

      return iBufs;
   }

} /* rawRecvError */

/*********************************************************************
 * rawRecvHead: receive common buffer header
 *    returns no. of bytes received or error (< 0)
 *
 * created  5. 3.96, Horst Goeringer
 *********************************************************************
 */

int rawRecvHead( int iSocket, char *pcBuf)
{
   char cModule[32]="rawRecvHead";
   int iDebug = 0;

   int iBuf, iBufs;
   int iRC, ii;
   int iIdent;
   int iStatus;
   int iDataLen;
   char *pcc;
   int *pint;

   if (iDebug)
   {
      fprintf(fLogFile, "\n-D- begin %s: socket %d\n", cModule, iSocket);
      fflush(fLogFile);
   }

   strcpy(pcBuf, "");
   pcc = pcBuf;        /* points now to buffer in calling procedure */
   pint = (int *) pcc;

   iBuf = HEAD_LEN;
   iBufs = iBuf;
   while(iBuf > 0)
   {
      iRC = recv(iSocket, pcc, (unsigned) iBuf, 0);
      if (iRC <= 0)
      {
         if (iRC < 0) fprintf(fLogFile,
            "-E- %s: receiving buffer header\n", cModule);
         else
         {
            ii = iBufs - iBuf;
            fprintf(fLogFile,
               "-W- %s: connection to sender broken, %d byte of buffer header (%d byte) received\n",
               cModule, ii, iBufs);
         }

         if (errno)
         {
            fprintf(fLogFile, "    %s\n", strerror(errno));
            errno = 0;
         }

         return -1;
      }

      iBuf -= iRC;
      pcc += iRC;

   } /* while(iBuf > 0) */

   iIdent = ntohl(*pint);
   pint++;
   iStatus = ntohl(*pint);
   pint++;
   iDataLen = ntohl(*pint);
   if (iDebug) fprintf(fLogFile,
      "    ident %d, status %d, datalen %d\n",
      iIdent, iStatus, iDataLen);

   if ( (iIdent == IDENT_STATUS) &&
        ((iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF)) &&
        (iDataLen > 0) )
   {
      if (iDebug) fprintf(fLogFile,
         "-W- %s: error message available for receive (%d byte)\n",
         cModule, iDataLen);
      return iDataLen;
   }

   if (iDebug)
   {
      fprintf(fLogFile,
         "-D- end %s: buffer header received (%d bytes)\n\n",
         cModule, iBufs);
      fflush(fLogFile);
   }

   return iBufs;

} /* rawRecvHead */

/*********************************************************************
 * rawRecvHeadC: receive common buffer header and check
 *    return values:
 *           >= 0: no error occurred:
 *     = HEAD_LEN: header received (and okay)
 *     > HEAD_LEN: status message of this size in pcMsg
 *           =  0: error status received, but no message
 *
 *           <  0: error occurred:
 *           = -1: recv failed
 *           = -2: connection to sender lost
 *           = -3: receiving error msg failed
 *           = -4: unexpected identifier received
 *           = -5: unexpected status received
 *
 * created  7. 3.2002, Horst Goeringer
 *********************************************************************
 */

int rawRecvHeadC(int iSocket,
                 char *pcBuf,
                 int iIdentReq,    /* < 0 => check, >= 0 => no check */
                 int iStatusReq,   /* >= 0 => check, < 0 => no check */
                 char *pcMsg)
{
   char cModule[32] = "rawRecvHeadC";
   int iDebug = 0;

   /* header to be received */
   int iIdent;
   int iStatus;
   int iDataLen;

   char cMsg1[STATUS_LEN] = "";

   int iRC, ii;
   int iBuf, iBufs;
   char *pcc;
   int *pint;

   if (iDebug)
   {
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);

      if (iIdentReq < 0) fprintf(fLogFile,
         "    check Ident, expect %d\n", iIdentReq);
      else
         fprintf(fLogFile, "    no check of Ident\n");
      if (iStatusReq >= 0) fprintf(fLogFile,
         "    check Status, expect %d\n", iStatusReq);
      else
         fprintf(fLogFile, "    no check of Status\n");
   }

   strcpy(pcMsg, "");
   strcpy(pcBuf, "");
   pcc = pcBuf;         /* points now to buffer in calling procedure */

   iBuf = HEAD_LEN;
   iBufs = iBuf;
   while(iBuf > 0)
   {
      if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
      {
         if (iRC < 0)
         {
            sprintf(pcMsg, "-E- %s: receiving buffer header\n",
               cModule);
            iRC = -1;
         }
         else
         {
            ii = iBufs - iBuf;
            sprintf(pcMsg,
               "-W- %s: connection to sender broken, %d byte of buffer header (%d byte) received\n",
               cModule, ii, iBufs);
            iRC = -2;
         }

         if (errno)
         {
            sprintf(cMsg1, "    %s\n", strerror(errno));
            strcat(pcMsg, cMsg1);
            errno = 0;
         }

         if (iDebug)
            fprintf(fLogFile, "%s", pcMsg);

         goto gEndRecvHeadC;
      }

      iBuf -= iRC;
      pcc += iRC;

   } /* while(iBuf > 0) */

   if (iDebug) fprintf(fLogFile,
      "    buffer header received (%d bytes)\n", iBufs);

   pint = (int *) pcBuf;
   iIdent = ntohl(*pint);
   pint++;
   iStatus = ntohl(*pint);
   pint++;
   iDataLen = ntohl(*pint);

   if (iDebug) fprintf(fLogFile,
      "    ident %d, status %d, datalen %d\n",
      iIdent, iStatus, iDataLen);

   if (iIdent == IDENT_STATUS)
   {
      if (iDebug)
      {
         fprintf(fLogFile, "    status received");
         if ( (iDataLen) &&
              ((iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF)) )
            fprintf(fLogFile, " with error message\n");
         else
            fprintf(fLogFile, "\n");
      }

      if ( (iDataLen) &&
           ((iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF) ||
            (iStatus == STA_CACHE_FULL)) )
      {
         pcc = cMsg1;
         iRC = rawRecvError(iSocket, iDataLen, pcc);
         if (iRC < 0)
         {
            sprintf(pcMsg, "-E- %s: receiving error msg, rc=%d\n",
               cModule, iRC);
            if (iDebug)
               fprintf(fLogFile, "%s", pcMsg);

            iRC = -3;
            goto gEndRecvHeadC;
         }

         if (iDebug) fprintf(fLogFile,
            "    msg (%d byte): %s\n", iDataLen, pcc);
         strcat(pcMsg, pcc);
         iRC = strlen(pcMsg);
         goto gEndRecvHeadC;
      }

      if ( (iDataLen == 0) &&
           ((iStatus == STA_ERROR) || (iStatus == STA_ERROR_EOF)) )
      {
         strcat(pcMsg,
            "-W- error status received, but no error message\n");
         iRC = 0;
         goto gEndRecvHeadC;
      }
   } /* (iIdent == IDENT_STATUS) */

   if (iIdentReq < 0)    /* test for expected identifier, values < 0 */
   {
      if (iDebug)
         fprintf(fLogFile, "    check identifier\n");

      if (iIdent != iIdentReq)
      {
         sprintf(pcMsg,
            "-E- %s: unexpected header (ident %d) received\n",
            cModule, iIdent);
         if (iDebug)
            fprintf(fLogFile, "%s", pcMsg);

         iRC = -4;
         goto gEndRecvHeadC;

      } /* unexpected ident) */
      else
      {
         if (iStatusReq >= 0)            /* test for expected status */
         {
            if (iDebug)
               fprintf(fLogFile, "    check status\n");

            if (iStatusReq != iStatus)
            {
               sprintf(pcMsg,
                  "-E- %s: unexpected header (status %d) received\n",
                  cModule, iStatus);
               if (iDebug)
                  fprintf(fLogFile, "%s", pcMsg);

               iRC = -5;
               goto gEndRecvHeadC;
            }
         }
      } /* expected ident */
   } /* check ident requested */

   iRC = HEAD_LEN;

gEndRecvHeadC:
   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return iRC;

} /* rawRecvHeadC */

/*********************************************************************
 * rawRecvRequest:
 *    receive request for next buffer and convert to host format
 *    returns   0: request received
 *              2: EOF status received
 *              3: EOS status received
 *           4, 5: error status received
 *            < 0: error occured
 *
 * created 21.12.2000, Horst Goeringer
 *********************************************************************
 */

int rawRecvRequest(int iSocket,
                   int *piSeekMode,
                   int *piOffset,
                   int *piBufferSize)
{
   char cModule[32]="rawRecvRequest";
   int iDebug = 0;

   int iRC;
   int iError = 0;
   int iRequSize = sizeof(srawRequest);
   int iBuf, iBuf0;
   int ii, iimax;
   char *pcc;
   char cMsg[STATUS_LEN] = "";                 /* error msg received */

   srawRequest sRequest, *pRequest;

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: receive request buffer\n", cModule);

   pRequest = &sRequest;
   pcc = (char *) pRequest;
   iBuf = HEAD_LEN;
   iimax = HEAD_LEN;
   while(iBuf > 0)
   {
      if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
      {
         if (iRC < 0)
         {
            fprintf(fLogFile,
               "-E- %s: receiving buffer header\n", cModule);
            iError = -1;
         }
         else
         {
            ii = iimax - iBuf;
            fprintf(fLogFile,
               "-W- %s: connection to sender broken, %d byte of buffer header (%d byte) received\n",
               cModule, ii, iimax);
            iError = -5;
         }

         goto gErrorRecvRequest;
      }

      iBuf -= iRC;
      pcc += iRC;

   } /* while(iBuf > 0) */

   if (iBuf < 0)
   {
      fprintf(fLogFile,
         "-E- %s: more buffer header data received than expected\n",
         cModule);

      iError = -2;
      goto gErrorRecvRequest;
   }

   pRequest->iIdent = ntohl(pRequest->iIdent);
   if (iDebug) fprintf(fLogFile,
      "    buffer header received (%d bytes, id %d)\n",
      iimax, pRequest->iIdent);

   if ( (pRequest->iIdent != IDENT_NEXT_BUFFER) &&
        (pRequest->iIdent != IDENT_STATUS) )
   {
      fprintf(fLogFile, "-E- %s: invalid buffer received (id %d)\n",
              cModule, pRequest->iIdent);
      iError = -3;
      goto gErrorRecvRequest;
   }

   pRequest->iStatus = ntohl(pRequest->iStatus);
   pRequest->iStatusLen = ntohl(pRequest->iStatusLen);
   iBuf0 = pRequest->iStatusLen;
   iBuf = iBuf0;
   if (pRequest->iIdent != IDENT_NEXT_BUFFER)
      pcc = cMsg;                               /* more space needed */

   while(iBuf > 0)
   {
      if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
      {
         if (iRC < 0)
         {
            fprintf(fLogFile,
               "-E- %s: receiving buffer data\n", cModule);
            iError = -1;
         }
         else
         {
            ii = iBuf0 - iBuf;
            fprintf(fLogFile,
               "-W- %s: connection to sender broken, %d byte of data (%d byte) received\n",
               cModule, ii, iBuf0);
            iError = -5;
         }

         if (errno)
         {
            fprintf(fLogFile, "    %s\n", strerror(errno));
            errno = 0;
         }

         goto gErrorRecvRequest;
      }

      iBuf -= iRC;
      pcc += iRC;

   } /* while(iBuf > 0) */

   if (iBuf < 0)
   {
      fprintf(fLogFile, "-E- %s: more data received than expected\n",
         cModule);
      iError = -2;
      goto gErrorRecvRequest;
   }

   if (pRequest->iIdent == IDENT_NEXT_BUFFER)
   {
      if (iDebug) fprintf(fLogFile,
         "    %s: request data received (%d bytes)\n", cModule, iBuf0);

      if (iBuf0 + HEAD_LEN != iRequSize)
      {
         fprintf(fLogFile,
            "-E- %s: invalid data size (%d) in request buffer (expected %d byte)\n",
            cModule, iBuf0, iRequSize-HEAD_LEN);
         iError = -4;
         goto gErrorRecvRequest;
      }

      *piSeekMode = ntohl(pRequest->iSeekMode);
      *piOffset = ntohl(pRequest->iOffset);
      *piBufferSize = ntohl(pRequest->iBufferSize);

      if (iDebug)
         fprintf(fLogFile, "-D- end %s\n\n", cModule);

      return 0;

   } /* (pRequest->iIdent == IDENT_NEXT_BUFFER) */
   else if (pRequest->iIdent == IDENT_STATUS)
   {
      if ( (pRequest->iStatus == STA_END_OF_FILE) ||
           (pRequest->iStatus == STA_END_OF_SESSION) )
      {
         if (iDebug) fprintf(fLogFile,
            "    %s: status info received: end session\n", cModule);
      }
      else if ( (pRequest->iStatus == STA_ERROR) ||
                (pRequest->iStatus == STA_ERROR_EOF) )

      {
         if (iDebug) fprintf(fLogFile,
            "-W- %s: error status received: end session\n", cModule);
         if (iBuf0 > 0) fprintf(fLogFile,
            "-W- %s: error message from client:\n    %s\n",
            cModule, cMsg);
         iError = -4;
         goto gErrorRecvRequest;
      }
      else
      {
         fprintf(fLogFile,
            "-E- %s: invalid status buffer received (id %d)\n",
            cModule, pRequest->iStatus);
         iError = -3;
         goto gErrorRecvRequest;
      }

      iError = pRequest->iStatus;
      goto gErrorRecvRequest;

   } /* (pRequest->iIdent == IDENT_STATUS) */
   else
   {
      fprintf(fLogFile, "-E- %s: invalid buffer received (ident %d)\n",
              cModule, pRequest->iIdent);
      iError = -2;
      goto gErrorRecvRequest;
   }

gErrorRecvRequest:
   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   if (iError)
      return iError;

   return 0;

} /* rawRecvRequest */

/*********************************************************************
 * rawRecvStatus: receive status header
 *    returns no. of bytes received or error (< 0)
 *    status header will be converted to host format
 *
 * created 18. 3.96, Horst Goeringer
 *********************************************************************
 */

int rawRecvStatus(int iSocket, srawStatus *psStatus)
{
   char cModule[32]="rawRecvStatus";
   int iDebug = 0;

   int iRC, ii;
   int iBuf, iBufs;
   int iLen;
   char *pcc;

   if (iDebug)
      fprintf(fLogFile, "\n-D- begin %s\n", cModule);

   /* clear status structure */
   memset(psStatus, 0X00, sizeof(srawStatus));

   pcc = (char *) psStatus;
   iBuf = HEAD_LEN;
   iBufs = iBuf;

   while(iBuf > 0)
   {
      iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 );
      if (iRC <= 0 )
      {
         if (iRC < 0) fprintf(fLogFile,
            "-E- %s: receiving status header\n", cModule);
         else
         {
            ii = iBufs - iBuf;
            fprintf(fLogFile,
               "-W- %s: connection to sender broken, %d byte of status header (%d byte) received\n",
               cModule, ii, iBufs);
         }

         if (errno)
         {
            fprintf(fLogFile, "    %s\n", strerror(errno));
            errno = 0;
         }

         return -1;
      }

      iBuf -= iRC;
      pcc += iRC;
      fflush(fLogFile);

   } /* while(iBuf > 0) */

   if (iBuf < 0)
   {
      fprintf(fLogFile,
         "-E- %s: more status header data received than expected\n",
         cModule);
      return -2;
   }

   psStatus->iIdent = ntohl(psStatus->iIdent);
   psStatus->iStatus = ntohl(psStatus->iStatus);
   psStatus->iStatusLen = ntohl(psStatus->iStatusLen);

   if (iDebug)
   {
      fprintf(fLogFile, "    status header received (%d bytes)\n",
         iBufs);
      fprintf(fLogFile, "    ident %d, status %d, status len %d\n",
         psStatus->iIdent, psStatus->iStatus, psStatus->iStatusLen);
      fflush(fLogFile);
   }

   if (psStatus->iIdent != IDENT_STATUS)
   {
      fprintf(fLogFile, "-E- %s: invalid status header received (%d)\n",
              cModule, psStatus->iIdent);
      return -3;
   }

   iLen = psStatus->iStatusLen;
   if (iLen > 0)
   {
      iBuf = iLen;                      /* length of status message */
      iBufs += iBuf;
      while(iBuf > 0)
      {
         if ( (iRC = recv( iSocket, pcc, (unsigned) iBuf, 0 )) <= 0 )
         {
            if (iRC < 0) fprintf(fLogFile,
               "-E- %s: receiving status message\n", cModule);
            else
            {
               ii = iLen - iBuf;
               fprintf(fLogFile,
                  "-W- %s: connection to sender broken, %d byte of status message (%d byte) received\n",
                  cModule, ii, iLen);
            }

            if (errno)
            {
               fprintf(fLogFile, "    %s\n", strerror(errno));
               errno = 0;
            }

            return -4;
         }

         iBuf -= iRC;
         pcc += iRC;

      } /* while(iBuf > 0) */

      if (iBuf < 0)
      {
         fprintf(fLogFile,
            "-E- %s: more status data received than expected\n",
            cModule);
         return -5;
      }

      if (iDebug) fprintf(fLogFile,
         "    status message received (%d bytes):\n%s\n",
         iLen, psStatus->cStatus);
      fflush(fLogFile);

   } /*  iLen > 0 */

   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return iBufs;

} /* rawRecvStatus */

/*********************************************************************
 * rawSendRequest: send request buffer
 *    returns 0 or error (< 0)
 *    status header will be converted to net format
 *
 * created 21.12.2000, Horst Goeringer
 *********************************************************************
 */

int rawSendRequest(int iSocket,
                   int iSeekMode,
                   int iOffset,
                   int iBufferSize)
{
   char cModule[32]="rawSendRequest";
   int iDebug = 0;

   int iBuf, iRC;
   char *pcc;

   srawRequest sRequest;
   int iRequSize = sizeof(srawRequest);

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: send request buffer\n", cModule);

   sRequest.iIdent = htonl((unsigned int) IDENT_NEXT_BUFFER);
   if (iSeekMode < 0)
      sRequest.iStatus = htonl(STA_NEXT_BUFFER);
   else
      sRequest.iStatus = htonl(STA_SEEK_BUFFER);
   sRequest.iStatusLen = htonl(iRequSize - HEAD_LEN);

   sRequest.iSeekMode = htonl(iSeekMode);
   sRequest.iOffset = htonl(iOffset);
   if (iBufferSize < 0)
   {
      fprintf(fLogFile,
         "-E- %s: invalid buffer size %d\n", cModule, iBufferSize);
      return -1;
   }
   sRequest.iBufferSize = htonl(iBufferSize);

   iBuf = iRequSize;
   pcc = (char *) &sRequest;
   iRC = send(iSocket, pcc, (unsigned) iBuf, 0);
   if (iRC < iBuf)
   {
      if (iRC < 0) fprintf(fLogFile,
         "-E- %s: sending request buffer\n", cModule);
      else fprintf(fLogFile,
         "-E- %s: request buffer incompletely sent (%d of %d byte)\n",
         cModule, iRC, iBuf);

      if (errno)
      {
         fprintf(fLogFile, "    %s\n", strerror(errno));
         errno = 0;
      }

      return -1;
   }

   if (iDebug) fprintf(fLogFile,
      "-D- end %s: request buffer sent\n\n", cModule);

   return 0;

} /* rawSendRequest */

/*********************************************************************
 * rawSendStatus: send status buffer
 *    returns no. of bytes sent or error (< 0)
 *    status header will be converted to net format
 *
 * created  5. 3.96, Horst Goeringer
 *********************************************************************
 */

int rawSendStatus( int iSocket, int iStatus, char *pcMsg)
{
   char cModule[32]="rawSendStatus";
   int iDebug = 0;

   int iBuf = HEAD_LEN;
   int iRC, iMsgLen;
   char *pcc;
   srawStatus sStatus;

   if (iDebug) fprintf(fLogFile,
      "\n-D- begin %s: send status buffer (%d) to socket %d\n",
      cModule, iStatus, iSocket);

   sStatus.iIdent = htonl((unsigned int) IDENT_STATUS);
   sStatus.iStatus = htonl(iStatus);
   sStatus.iStatusLen = htonl(0);

   if (pcMsg == NULL)
      iMsgLen = 0;
   else
      iMsgLen = strlen(pcMsg);
   if (iMsgLen)
   {
      iBuf += iMsgLen;
      sStatus.iStatusLen = htonl(iMsgLen);
      strcpy(sStatus.cStatus, pcMsg);
      if (iDebug) fprintf(fLogFile,
         "    status message (%d bytes):\n    %s\n", iMsgLen, pcMsg);
   }

   pcc = (char *) &sStatus;
   if (iDebug)
   {
      fprintf(fLogFile,
         "    now send (iIdent %d, iStatus %d, iStatusLen %d), %d byte:\n",
         ntohl(sStatus.iIdent), ntohl(sStatus.iStatus),
         ntohl(sStatus.iStatusLen), iBuf);
      fflush(fLogFile);
   }

   iRC = send(iSocket, pcc, (unsigned) iBuf, 0);
   if (iRC < iBuf)
   {
      if (iRC < 0) fprintf(fLogFile,
         "-E- %s: sending status buffer\n", cModule);
      else fprintf(fLogFile,
         "-E- %s: status buffer incompletely sent (%d of %d byte)\n",
         cModule, iRC, iBuf);

      if (errno)
      {
         fprintf(fLogFile, "    %s\n", strerror(errno));
         errno = 0;
      }

      return -1;
   }

   if (iDebug) fprintf(fLogFile,
      "-D- end %s: status buffer sent (%d byte)\n\n", cModule, iBuf);

   return(iBuf);

} /* rawSendStatus */

/*********************************************************************
 * rawTestFileName:
 *    verify that specified name is a valid file name
 *    and has no wildcards
 * created 15.3.1996, Horst Goeringer
 *********************************************************************
 */

int rawTestFileName( char *pcFile)
{
   char cModule[32] = "rawTestFileName";
   int iDebug = 0;

   int iRC;
   int ilen;
   int iError = 0;
   unsigned long lFileSize = 0;          /* dummy for rawGetFileSize */
   unsigned int iSize = 0;                /* dummy for rawGetFileSize */
   char *pdir;

   if (iDebug) fprintf(fLogFile,
      "-D- begin %s: input file name %s\n", cModule, pcFile);

   if ( (pdir = strrchr(pcFile, '*')) != NULL)
   {
      fprintf(fLogFile,
         "-E- invalid file name '%s': '*' not allowed as part of file name\n",
         pcFile);
      iError = 3;
      goto gErrorTest;
   }

   if ( (pdir = strrchr(pcFile, '?')) != NULL)
   {
      fprintf(fLogFile,
         "-E- invalid file name '%s': '?' not allowed as part of file name\n",
         pcFile);
      iError = 3;
      goto gErrorTest;
   }

   if ( (pdir = strrchr(pcFile, '/')) == NULL)
   {
      if (iDebug)
         fprintf(fLogFile, "    name %s okay\n", pcFile);
   }
   else       /* name contains '/' */
   {
      ilen = strlen(pdir);
      if (iDebug) fprintf(fLogFile,
         "    trailor %s (len %d)\n", pdir, ilen);
      if (ilen == 1)
      {
         strncpy(pdir, "\0", 1);
         if (iDebug)
            fprintf(fLogFile, "    %s is a directory\n", pcFile);
         iError = 2;
         goto gErrorTest;
      }
      else if (iDebug)
         fprintf(fLogFile, "    rel name %s okay\n", pcFile);
   }

   iRC = rawGetFileSize(pcFile, &lFileSize, &iSize);
   if (iDebug) fprintf(fLogFile,
      "    after rawGetFileSize, rc = %d\n", iRC);

   if (iRC)
   {
      if (iDebug)
      {
         fprintf(fLogFile, "-W- %s NOT archived", pcFile);
         if (iRC == 1)
            fprintf(fLogFile, " - is a directory\n");
         else if (iRC == 2)
            fprintf(fLogFile, " - is a symbolic link\n");
         else if (iRC == 3)
            fprintf(fLogFile, " - is not a regular file\n");
         else
            fprintf(fLogFile, "\n");
      }

      iError = 2;
      goto gErrorTest;
   }

   if (iDebug) fprintf(fLogFile,
      "    %s is a regular file\n", pcFile);

gErrorTest:
   if (iDebug)
      fprintf(fLogFile, "-D- end %s\n\n", cModule);

   return iError;

} /* rawTestFileName */

