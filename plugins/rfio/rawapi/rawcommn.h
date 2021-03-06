/*********************************************************************
 * Copyright:
 *   GSI, Gesellschaft fuer Schwerionenforschung mbH
 *   Planckstr. 1
 *   D-64291 Darmstadt
 *   Germany
 * created 25. 1.1996 by Horst Goeringer
 *********************************************************************
 * rawcommn.h
 *    communication structure tsmcli client - servers
 *********************************************************************
 * 11. 9.1996, H.G.: alternate archive tape device "ARCH_TAPE_2" added
 * 26. 9.1996, H.G.: new archive tape device "ARCH_TAPE_MC" added
 *                 "ARCH_TAPE_2" changed to "ARCH_TAPE_MC_2"
 *  8. 1.1997, H.G.: new parameter MAX_FILE_SIZE            
 * 21. 7.1997, H.G.: new parameter srawComm.cliNode          
 * 10. 9.1997, H.G.: new ACTION value STAGE
 * 15. 9.1997, H.G.: new enum RETR_DEVICE                      
 * 26. 9.1997, H.G.: new ACTION value RETRIEVE_STAGE                    
 * 21.10.1997, H.G.: replace ARCHIVE_STAGE by ARCHIVE_OVER       
 * 19. 1.1998, H.G.: new status flag STA_END_OF_FILE_NOT_STAGED   
 *  8. 7.1998, H.G.: use MBUF_SOCK and MBUF_ADSM; set both to 16384
 *                   new filetype STREAM                             
 * 28. 7.1999, H.G.: new ACTION values UNSTAGE, QUERY_UNSTAGE
 * 16. 8.2000, H.G.: new ACTION values QUERY_WORKSPACE, QUERY_POOL
 *                   new structures srawPoolStatus, srawWorkSpace
 * 16. 8.2000, H.G.: mod srawPoolStatus, new srawPoolAttr
 *  5. 9.2000, H.G.: new ACTION value QUERY_STAGE
 * 22.11.2000, H.G.: enhance range of values for srawWorkSpace.iStatus
 *  1.12.2000, H.G.: new status id IDENT_NEXT_BUFFER
 * 21.12.2000, H.G.: new structure srawRequest for seek functionality
 *  1. 2.2001, H.G.: new action values ARCHIVE_STAGE, METADATA_MOD
 * 30. 5.2001, H.G.: increase MAX_TAPE_FILE from 16 to 17
 * 13. 6.2001, H.G.: new STATUS value ARCHIVE_AVAILABLE
 * 22. 6.2001, H.G.: new ACTION QUERY_RETRIEVE
 *                   replace MPORT by new def PORT_MASTER
 * 17. 7.2001, H.G.: enhance srawObjAttr by data mover info
 *                   new structure srawDataMover
 * 23. 8.2001, H.G.: enhance srawComm by stage info
 * 29.10.2001, H.G.: ported to W2000
 * 14.11.2001, H.G.: rename ACTION DELETE -> REMOVE (used in winnt.h)
 * 28. 2.2002, H.G.: new: IDENT_STAGE_xxx, IDENT_SPACE_xx
 * 15. 3.2002, H.G.: rename srawPoolAttr -> srawPoolStatusData, enhance
 * 18. 3.2002, H.G.: new define MAX_NUM_DM (<- rawservn.h)
 * 20. 3.2002, H.G.: new structure srawPoolStatusDMData
 * 27. 5.2002, H.G.: new ACTION QUERY_ARCHIVE_OVER
 *  6. 6.2002, H.G.: new identifier: IDENT_PURGE_INFO
 * 20. 8.2002, H.G.: new identifier: IDENT_SPACE_LOOK
 * 19. 9.2002, H.G.: new identifier: IDENT_CLEAN_REQUEST
 * 26. 9.2002, H.G.: new definition PORT_RFIO_SERV
 * 29.10.2002, H.G.: new identifier: IDENT_FILELIST_REQUEST,
 *                   new define MAX_STAGE_FILE_NO
 * 22.11.2002, H.G.: new status STA_INFO, STA_PURGED, STA_NOT_PURGED
 * 13. 1.2003, H.G.: add definition PORT_MOVER
 * 30. 1.2003, H.G.: add declarations cMasterNode(i), new: MAX_MASTER
 * 13. 2.2003, H.G.: add definition MAX_MOVER
 * 24. 2.2003, H.G.: new actions SPM_REQUEST_MOVER, QUERY_ARCHIVE_STAGE
 *  8. 5.2003, H.G.: add synchronization parameters to srawDataMoverAttr
 *  9. 5.2003, H.G.: add synchronization parameters to srawComm
 * 21. 8.2003, H.G.: srawComm: new string cTapeLib,
 *                   replace ACTION values:
 *                      ARCHIVE_STAGE -> ARCHIVE_TO_STAGE
 *                      METADATA_MOD  -> ARCHIVE_FROM_STAGE
 *                   new ACTION values: QUERY_ARCHIVE_TO_STAGE,
 *                                      QUERY_ARCHIVE_FROM_STAGE
 * 28. 8.2003, H.G.: srawObjAttr: add parameter cStageUser
 * 15. 9.2003, H.G.: increase MAX_NUM_DM: 5 -> 10
 *  6.10.2003, H.G.: cNodeMaster2 now gsitsma, depc134 is backup node
 * 10.10.2003, H.G.: new identifier: IDENT_QUERY_ARCHDB
 * 15.10.2003, H.G.: new action END_SESSION
 * 31.10.2003, H.G.: remove unused definition MAX_MOVER
 *  6.11.2003, H.G.: new action QUERY_ARCHIVE_RECORD
 * 18.11.2003, H.G.: new identifier: IDENT_FULL_ARCHDISK
 * 18.12.2003, H.G.: new identifier: IDENT_ARCHIVE_FILE
 * 22. 1.2004, H.G.: add cNodeMaster2A: gsitsmb
 * 20. 2.2004, H.G.: srawObjAttr: rename iPort (unused) -> iFS
 *                   new structure srawFileSystem
 * 24. 2.2004, H.G.: new ACTION values:
 *                   STAGE_FROM_CACHE, RETRIEVE_FROM_CACHE
 *                   replace ACTION values:
 *                   QUERY_ARCHIVE_TO_STAGE -> QUERY_ARCHIVE_TO_CACHE
 *                   QUERY_ARCHIVE_FROM_STAGE ->QUERY_ARCHIVE_FROM_CACHE
 *                   ARCHIVE_TO_STAGE -> ARCHIVE_TO_CACHE
 *                   ARCHIVE_FROM_STAGE -> ARCHIVE_FROM_CACHE
 *  5. 3.2004, H.G.: srawFileSystem: add cArchiveDate, cArchiveUser
 *                   new ACTION value READ_FROM_CACHE
 * 24. 6.2004, H.G.: new STATUS value STA_CACHE_FULL
 * 29. 6.2004, H.G.: new ARCH_DEVICE value ARCH_DAQ_DISK for DaqPool
 *  2. 7.2004, H.G.: new definitions: DEF_FILESIZE,
 *                   SLEEP_CACHE_FULL, MAXLOOP_CACHE_FULL 
 * 18.10.2004, H.G.: new STATUS value STA_CACHE_FILE_AWAY
 * 24.11.2004, H.G.: STATUS value ARCHIVE_AVAILABLE -> STA_ARCHIVE_AVAIL
 *                   new: STA_ARCHIVE_NOT_AVAIL, STA_NO_ACCESS
 * 26. 1.2005, H.G.: new ACTION value QUERY_RETRIEVE_STAGE
 * 20.10.2005, H.G.: increase DEF_FILESIZE to nearly 2 GByte
 * 25.11.2005, H.G.: add cNodeMasterE1/2: new entry server lxha05/4
 *  7. 3.2006, H.G.: increase MAX_NUM_DM: 10 -> 16
 * 13. 3.2006, H.G.: new STATUS value STA_SWITCH_ENTRY (for HA),
 *                   new parameter srawComm.cNodeCacheMgr (for HA)
 * 15. 3.2006, H.G.: new STATUS value STA_ENTRY_INFO (for HA)
 * 17. 3.2006, H.G.: new entry server lxgstore
 * 28. 4.2006, H.G.: cleanup cNodeMasterxx strings
 *  9. 5.2006, H.G.: new actions for 2nd level DM: QUERY_FILESERVER, 
 *                      SEND_TO_FILESERVER, RECEIVE_FROM_DM
 *                   rename QUERY_RETRIEVE_API -> QUERY_RETRIEVE_RECORD
 *                   new parameters srawComm: iPoolIdRC, cNodeRC,
 *                      iPoolIdWC, cNodeWC, iFSidWC,
 *                      iPoolIdFC, cNodeFC, iFSidFC,
 * 19. 5.2006, H.G.: new idents: IDENT_STAGE_LIST, IDENT_QUERY_FILEDB
 *  2.10.2006, H.G.: new parameters srawObjAttr: iATLServer,
 *                   iRestoHigh2, iRestoHighLow, iRestoHighLow2, 
 *  5.10.2006, H.G.: new parameter srawComm: iATLServer
 * 28. 9.2006, H.G.: add MAX_ATLSERVER
 * 24.11.2006, H.G.: new ident IDENT_QUERY_DONE
 * 20.12.2006, H.G.: add ATLSERVER_ARCHIVE for API write to gStore
 * 14. 5.2007, H.G.: in srawObjAttr: iRestoHighLow2 -> iRestoLowHigh,
 *                   iRestoHigh2 -> iRestoHighHigh
 * 17. 7.2007, H.G.: new limit: MAX_STAGE_FILE_NO = 1000
 * 29. 2.2008, H.G.: new actions (for /lustre):
 *                   COPY_TO_FILESYSTEM, RETRIEVE_TO_FILESYSTEM
 *  6. 3.2008, H.G.: new parameter srawComm: iDataFS, cDataFS
 *  7. 4.2008, H.G.: new parameter srawComm: iReservation,
 *                   new action MIGRATE_CACHE_PATH
 * 17. 4.2008, H.G.: new action ARCHIVE_FROM_FILESYSTEM
 * 21. 4.2008, H.G.: change MAX_ATLSERVER, MAX_MASTER -> 1
 * 23. 4.2008, H.G.: new status STA_FILE_CACHED
 *  8. 5.2008, H.G.: move static declaration [p]cOS from rawclin.h
 *  9. 5.2008, H.G.: add typedef srawFileList
 *                   (replaces srawArchList in rawclin.h)
 * 19. 5.2008, H.G.: remove old declarations: cNodeMaster1,
 *                   cNodeMaster2A, cNodeMaster2B
 *  2. 6.2008, H.G.: replace long->int if 64 bit client (ifdef SYSTEM64)
 *                   server still 32 bit
 *  3. 6.2008, H.G.: replace MBUF_API (unused) by MBUF_RFIO
 * 19. 6.2008, H.G.: new ident: IDENT_CACHE_STATUS
 *                   decrease MAX_NUM_DM: 16 -> 10 (no more Windows)
 * 14. 8.2008, H.G.: new ACTION SHOW_ARCHIVES
 *  3. 9.2008, H.G.: new statics cDataFSHigh, iDataFSHigh
 * 12. 1.2008, H.G.: add definition PORT_MOVER_DATAFS
 * 16. 9.2008, H.G.: cDataFSHigh -> cDataFSHigh1, cDataFSHigh2
 *                   remove iDataFSHigh
 * 17. 9.2008, H.G.: set MBUF_SOCK, MBUF_ADSM to 32768
 * 12.12.2008, H.G.: add definition CUR_QUERY_LIMIT
 * 12. 2.2009, H.G.: new identifier: IDENT_COPY_CACHE
 *                   add structure srawCopyCache to srawAPIFile
 * 23. 2.2009, H.G.: new status: STA_CACHE_COPY, STA_CACHE_COPY_ERROR
 * 24. 2.2009, H.G.: add cDataFSType
 * 10. 3.2009, H.G.: add trailing '/' to cDataFSHigh1, cDataFSHigh2
 *                   new definition MIN_DATAFS_PATH
 *  8. 4.2009, H.G.: action COPY_TO_FILESYSTEM -> COPY_RC_TO_FILESYSTEM
 *                   new action COPY_WC_TO_FILESYSTEM
 * 25. 5.2009, H.G.: new global strings cAdminUser, cForceMigPath
 *  1. 7.2009, H.G.: new IDENT_ARCHIVE_LIST, srawArchiveList
 *  3. 9.2009, H.G.: move MAX_STAGE_FILE_NO to rawservn.h
 * 28. 9.2009, H.G.: rename READ_FROM_CACHE to SEND_FROM_CACHE
 * 27.11.2009, H.G.: adds new definitions: MAX_OBJ_FS_N, MAX_OBJ_HL_N,
 *                   MAX_OBJ_LL_N, MAX_MC_N
 *  4.12.2009, H.G.: enhanced structure srawObjAttr (version 5)
 *                   keep old structure as srawObjAttrOld (version 4)
 *  7.12.2009, H.G.: new definition: VERSION_SRAWOBJATTR
 * 18.12.2009, H.G.: overall usage of new object sizes in MAX_OBJ_FS_N,
 *                   MAX_OBJ_HL_N, MAX_OBJ_LL_N (but no trailing _N)
 *                   for srawObjAttrOld:
 *                      MAX_OBJ_HL -> MAX_OBJ_HL_O,
 *                      MAX_OBJ_LL -> MAX_OBJ_LL_O
 * 14. 1.2010, H.G.: MAX_MC -> MAX_MC_O (in srawObjAttrOld),
 *                   MAX_MC_N -> MAX_MC
 *                   srawObjAttr: cMgmtClass[MAX_MC+2] (padded 8 byte)
 *                   remove unused ARCH_TAPE_MC_2, RETR_STAGE_DATE
 *                   srawObjAttr: new mem iDM (for 8 byte padding)
 * 28. 1.2010, H.G.: new definition: MAX_FULL_FILE
 *                   enhance STATUS_LEN 256 -> 512
 * 29. 1.2010, H.G.: srawFileList.cFile: MAX_FILE -> MAX_FULL_FILE 
 *                   srawComm.cDataFS: MAX_FILE -> MAX_FULL_FILE
 *                   srawCopyCache.cCopyPath: MAX_FILE -> MAX_FULL_FILE
 *  5. 2.2010, H.G.: mod. values: MAX_OBJ_HL 112, MAX_OBJ_LL 52
 *                   srawObjAttr: new mem iDummy (for 8 byte padding)
 *                   new: SIZE_CACHE_METADATA_5, SIZE_CACHE_METADATA_4
 * 11. 2.2010, H.G.: new definition: MAX_OBJ_HL_LEVEL
 * 22. 2.2010, H.G.: srawObjAttr.iFileSize[2]: two int for future 64 bit:
 *                      4 byte padding, ident struct size 32/64 bit env
 *                   if long in 64 bit env: 8 byte padding, diff struct
 *                      sizes (also in V1, V2 metadata structures) 
 * 16. 4.2010, H.G.: srawDataMoverAttr: iSynchId -> iATLServer
 *  9. 6.2010, H.G.: IDENT_PROC_INFO, MAX_ITEM added
 *  3. 9.2010, H.G.: srawComm: enable 8 byte filesize in 64 bit OS
 *                      new iFileSize2, MAX_APPLTYPE: 32 -> 28
 * 10. 9.2010, H.G.: discard SYSTEM64, srawObjAttrOld: 4 byte iFileSize 
 *  6.10.2010, H.G.: srawComm: new iClient32
 *                   new definition: MAX_FILE_SIZE_U
 * 26.11.2010, H.G.: string cTooBig: substitute for filesizes >= 4 GB
 * 19. 1.2011, H.G.: new IDENT_STAGE_PARALLEL, MAX_NUM_TAPE
 * 21. 1.2011, H.G.: new action STAGE_PARALLEL, struct srawObjNameList,
 *                   new IDENT_GLOBAL_SOCKET
 * 24. 1.2011, H.G.: new structs srawStageSocket, srawStageSocketData
 * 25. 1.2011, H.G.: add definition PORT_STAGER
 *  3. 2.2011, H.G.: new actions SET_GLOBAL_SOCKET, UNSET_GLOBAL_SOCKET
 *                   get definition PORT_STAGE_MGR from rawservn.h
 *  9. 2.2011, H.G.: new parameter srawComm: iParStageId
 *  2. 3.2011, H.G.: new IDENT_CACHEFS_ACTION
 *  1. 4.2011, H.G.: new define MAX_NUM_DM_NO (digits in DM name)
 *  4. 4.2011, H.G.: new IDENT_STAGE_SEQUENTIAL
 * 15. 4.2011, H.G.: increase MAX_NUM_DM to 12
 *  9. 2.2012, H.G.: remove MAX_MASTER, WORK_LEN (no longer needed), 
 *                   new STATUS_LEN_LONG
 * 10. 2.2012, H.G.: new IDENT_MIGR_LIMIT
 * 16. 2.2012, H.G.: get from rawservn.h: declaration cNodeStageMgr +
 *                       cNodeArchiveMgr, definition PORT_ARCHIVE_MGR 
 *                   ren PORT_STAGER -> PORT_DISTR, move to rawservn.h
 * 24. 2.2012, H.G.: srawFileSystem: add iPoolId
 * 20. 4.2012, H.G.: /hera: new base directory for InfiniBand lustre
 * 31. 8.2012, H.G.: enhanced usage, if srawComm.iDataFS > 1
 *  6. 9.2012, H.G.: new parameter srawDataMoverAttr: iPoolId, 
 *                   new define MAX_POOL_ATLSERVER,
 * 11. 9.2011, H.G.: mod define MAX_NUM_DM_NO 17 -> 20
 *                   mod define MAX_NUM_DM 12 -> 13
 * 31. 1.2013, H.G.: new IDENT_TAPE_STATUS_DM
 * 19. 3.2013, H.G.: new action SEND_FROM_TAPE
 *  3. 4.2013, H.G.: new action RETRIEVE_PARALLEL
 *  4. 4.2013, H.G.: define MAX_NUM_POOL from rawstagen.h
 * 12. 4.2013, H.G.: rename srawStageSocket -> srawGlobalSocket,
 *                      srawStageSocketData -> srawGlobalSocketData,
 *                   new srawGlobalNotice, new action SEND_GLOBAL_NOTICE
 * 15. 4.2013, H.G.: in srawComm: ren iParStageId -> iGlobalActionId
 *                   in srawGlobalSocketData: iParStageId -> iActionId
 * 19. 4.2013, H.G.: new status STA_FILE_AVAIL
 * 29. 4.2013, H.G.: srawObjNameList: add iCacheLocation (for par retr)
 * 17. 5.2013, H.G.: srawObjNameList: add iPoolIdRC, iPoolIdWC
 * 27. 6.2013, H.G.: srawGlobalNotice: add cFile (lustre file name)
 * 12. 8.2013, H.G.: sComm.iSynchId==21: test access rights
 *                   mod define MAX_NUM_DM 13 -> 20
 * 19. 8.2013, H.G.: sComm.iSynchId>100: use alternate prod TSM server
 * 22. 8.2013, H.G.: new IDENT_TAPE_STATUS_DM
 *  2. 1.2014, H.G.: srawComm.iReservation <0: request filesize check
 *                      comparing cacheDB - data mover FS
 *                   srawObjAttr: iDummy -> iObjInfo: contains 
 *                      result of filesize check
 *  3. 1.2014, H.G.: new IDENT_FILESIZE_REQUEST
 * 21. 2.2014, H.G.: get from rawclin.h: define MIN_SEQUENTIAL_FILES
 * 26. 2.2014, H.G.: new IDENT_SPACE_TREE1, IDENT_SPACE_TREE2
 * 16. 9.2014, H.G.: new STA_FILE_CACHED_OVER 
 * 12.11.2014, H.G.: new MAX_SIZE_CACHE_FILE: max file size in cache
 * 28.11.2014, H.G.: srawComm.iReservation: document new usage in API
 *                      client file loops
 * 14. 1.2015, H.G.: srawComm.iSynchId: document new usage in API
 *                      file delete
 * 21. 1.2015, H.G.: SEND_FROM_CACHE: doc. enhanced usage for any cache
 * ********************************************************************
 */

#define PORT_STAGE_MGR 1991      /* default port number StageManager */
                             /* 1992: port number SpaceManager on DM */
#define PORT_ARCHIVE_MGR 1993  /* default port number ArchiveManager */ 
#define PORT_MOVER  1994           /* default port number data mover */
                            /* 1995: port number ArchiveServer on DM */
#define PORT_MASTER 1996        /* default port number master server */
                            /* 1997: port number distribution server */
#define PORT_MOVER_DATAFS 1998   /* port number DM for lustre (root) */

#define PORT_RFIO_SERV 1974       /* default port number RFIO server */

#define ATLSERVER_ARCHIVE 1     /* API write: 1: IBM 3494, 2: SANLB2 */
#define MAX_ATLSERVER 1                    /* max no. of ATL servers */
#define MAX_POOL_ATLSERVER 10     /* max no. of pools in ATL servers */
#define MAX_NUM_POOL   5                  /* sum for all ATL servers */
#define MAX_NUM_TAPE   8        /* max no. of available tapes in ATL */
#define MAX_NUM_DM    20  /* max no. of DMs for RC + GRC or WC + GWC */
#define MAX_NUM_DM_NO 20                    /* max digits in DM name */
#define MBUF_SOCK  32768              /* default buffer size sockets */
#define MBUF_ADSM  32768                 /* default buffer size ADSM */
#define MBUF_RFIO  32768            /* max currently possible in MBS */
#define CUR_QUERY_LIMIT 0
  /* no. of entries in query buffer
     =0: current no. has default value (MBUF_SOCK - HEAD_LEN)/iObjAttr
     >0 in any case limited by default value */

#define SLEEP_CACHE_FULL 600  /* wait time/iter, if write cache full */
#define MAXLOOP_CACHE_FULL 3  /* max no of iter, if write cache full */

#define DEF_FILESIZE  2044000000   /* assume <2GByte, if unavailable */
#define MAX_FILE_SIZE 2147483647
                      /* max filesize 2GByte-1 (byte, 32 bit signed) */
#define MAX_FILE_SIZE_U 4294967295
                    /* max filesize 4GByte-1 (byte, 32 bit unsigned) */
#define MAX_SIZE_CACHE_FILE 4096000   /* max size cache file (MByte) */

#define MAX_FILE_NO 1024         /* max number of file names in list */

#define MIN_SEQUENTIAL_FILES 20
                          /* lower file limit for sequential staging */

static const char cTooBig[8] = "(>=4GB)";
               /* for 32 bit clients: substitute for filesize >= 4GB */

#define MAX_NODE    16                       /* max length node name */
static char cNodeMaster0[MAX_NODE] = "lxgstore";     /* entry server */
static char cNodeMasterE1[MAX_NODE] = "lxha05";  /* def entry server */
static char cNodeMasterE2[MAX_NODE] = "lxha04";  /* bkp entry server */
static char cNodeStageMgr[MAX_NODE] = "lxgstore";
static char cNodeArchiveMgr[MAX_NODE] = "lxgstore";

#define MIN_DATAFS_PATH 12              /* min path length in lustre */
static char cDataFSType[16] = "lustre";              /* general name */
static char cDataFSHigh1[16] = "/hera/";        /* InfiniBand lustre */
static char cDataFSHigh2[16] = "/lustre/";        /* Ethernet lustre */

#define MAX_OWNER   16                    /* max length object owner */
static char cAdminUser[MAX_OWNER] = "goeri";         /* special user */
static char cForceMigPath[32] = "/ArchiveNow";  /* special path name */

/* query client operating system */
#define MAX_OS       8           /* max length operating system name */
static char *pcOS;
#ifdef _AIX
static char cOS[MAX_OS] = "AIX    ";
#endif
#ifdef Linux
static char cOS[MAX_OS] = "Linux  ";
#endif
#ifdef Lynx
static char cOS[MAX_OS] = "Lynx   ";
#endif
#ifdef VMS
static char cOS[MAX_OS] = "VMS    ";
#endif

#define MAX_FULL_FILE   256             /* max length full file name */
#define MAX_FILE        128             /* max length rel. file name */
#define MAX_TAPE_FILE 17             /* max length file name on tape */

#define VERSION_SRAWOBJATTR 5   /* cur version of struct srawObjAttr */
#define SIZE_CACHE_METADATA_5 424       /* cur size of cacheDB entry */
#define SIZE_CACHE_METADATA_4 344       /* old size of cacheDB entry */

#define MAX_OBJ_FS    32
          /* max len filespace name, in code limited to 26 due to MC */
#define MAX_OBJ_HL   112        /* max length object high level name */
#define MAX_OBJ_LL    52         /* max length object low level name */
#define MAX_OBJ_HL_O  92        /* max length object high level name */
#define MAX_OBJ_LL_O  36         /* max length object low level name */
#define MAX_OBJ_HL_LEVEL 14
    /* max no. gStore directory levels, if MAX_OBJ_xx fully utilized */

#define MAX_MC       30 
           /* length ManagementClass name:
              limited by TSM (V5.5) to DSM_MAX_MC_NAME_LENGTH = 30
              -> length filespace name (26) + length('TAPE')         */
#define MAX_MC_O     12          /* max length Management class name */

#define MAX_DATE     20          /* max length archive date and time */
#define MAX_APPLTYPE 28               /* max length application type */
#define MAX_ITEM     32                /* max length of process item */

#define HEAD_OFFSET   3           /* offset common pre-header (ints) */
#define HEAD_LEN     12          /* length common pre-header (bytes) */
#define STATUS_LEN        512     /* max length status/error message */
#define STATUS_LEN_LONG  4096    /* max length long status/error msg */

/* identifiers for buffer headers transfered via sockets */
/* at least IDENT_STATUS must be <0 to avoid confusion with data */
#define IDENT_COMM         -1        /* communication control buffer */
#define IDENT_STATUS       -2        /* status buffer from/to client */
#define IDENT_QUERY        -3                 /* query result buffer */
#define IDENT_POOL         -4              /* stage pool info buffer */
#define IDENT_WORKSPACE    -5              /* work space info buffer */
#define IDENT_NEXT_BUFFER  -6            /* request next data buffer */
#define IDENT_MOVER_ATTR   -7               /* data mover attributes */
#define IDENT_STAGE_FILE   -8        /* stage file data (read cache) */
#define IDENT_SPACE_INFO   -9               /* data mover space info */
#define IDENT_PURGE_INFO  -10        /* purge request for data mover */
#define IDENT_SPACE_LOOK  -11             /* meta data DB space info */
#define IDENT_CLEAN_REQUEST  -12     /* clean request for stage pool */
#define IDENT_FILELIST_REQUEST -13  /* filelist request for stage FS */
#define IDENT_QUERY_ARCHDB -14         /* query result buffer archDB */
#define IDENT_FULL_ARCHDISK -15       /* buffer describing full disk */
#define IDENT_ARCHIVE_FILE -16    /* archive file data (write cache) */
#define IDENT_STAGE_LIST   -17     /* list of read cache object data */
#define IDENT_QUERY_FILEDB -18         /* query result buffer fileDB */
#define IDENT_QUERY_DONE   -19           /* last query result buffer */
#define IDENT_CACHE_STATUS -20     /* request query/set cache status */
#define IDENT_COPY_CACHE   -21    /* copy DAQ stream data from cache */
#define IDENT_ARCHIVE_LIST -22              /* list of archive names */
#define IDENT_PROC_INFO    -23     /* get/update proc no. in cacheDB */
#define IDENT_STAGE_PARALLEL -24  /* DMs and filelists for par stage */
#define IDENT_GLOBAL_SOCKET  -25  /* global socket data for CacheMgr */
#define IDENT_CACHEFS_ACTION -26      /* action request for cache FS */
#define IDENT_STAGE_SEQUENTIAL -27 /* DMs + filelists for sequ stage */
#define IDENT_MIGR_LIMIT   -28 /* info/action for migr limit WC pool */
#define IDENT_TAPE_STATUS_DM -29      /* pool DM tape connect status */
#define IDENT_RETRIEVE_PARALLEL -30    /* DMs and filelists par retr */
#define IDENT_FILESIZE_REQUEST -31  /* request object filesize on DM */
#define IDENT_SPACE_TREE1 -32 /* metadataDB space info + 1 fill tree */
#define IDENT_SPACE_TREE2 -33 /* metadataDB space info + 2 fill trees*/

enum ARCH_DEVICE                           /* logical archive device */
{
   ARCH_ANY,             /*  0: any device (for query) */ 
   ARCH_TAPE,            /*  1: standard tape class */
   ARCH_TAPE_MC,         /*  2: user specific tape class for archive */
   MGR_TAPE,             /*  3: tape for system manager: special MC */
   ARCH_DISK,            /*  4: disk in ArchivePool */
   ARCH_DAQ_DISK         /*  5: disk in DaqPool (RFIO only) */
};

enum RETR_DEVICE                           /* logical staging device */
{
   RETR_CLIENT,          /*  0: no staging */
   RETR_STAGE_TEMP,      /*  1: no expiration date (retrieve) */
   RETR_STAGE_PERM,      /*  2: with expiration date (stage) */
   RETR_FILESYSTEM       /*  3: retrieve to central file system */
};

enum FILETYPE                                      /* describes data */
{
   FIXED_INTS4,          /*  0: fixed records, 4 byte int (signed) */
   STREAM                /*  1: record length not defined */
};

enum ACTION                                                /* action */
{
   ARCHIVE,             /*  0: archive files to TSM */
   ARCHIVE_MGR,         /*  1: archive sysmgr: set owner, special MC */ 
   ARCHIVE_RECORD,      /*  2: archive file from client program to WC */
   ARCHIVE_OVER,        /*  3: archive file with overwrite (client only)
                               on server: REMOVE + ARCHIVE_TO_CACHE */
   CREATE_ARCHIVE,      /*  4: sysmgr only: create new archive */
   CLOSE,               /*  5: close tape (client) */
   REMOVE,              /*  6: delete file from mass storage */
   REMOVE_MGR,          /*  7: delete file from mass storage (sysmgr) */
   FILE_CHECK,          /*  8: only needed for client side */
   OPEN,                /*  9: open tape (client) */
   QUERY,               /* 10: query for files in mass storage */
   QUERY_ARCHIVE,       /* 11: query files before ARCHIVE */
   QUERY_ARCHIVE_MGR,   /* 12: query files before ARCHIVE (sysmgr) */
   QUERY_REMOVE,        /* 13: query for files to be deleted */
   QUERY_REMOVE_MGR,    /* 14: query for files to be deleted (sysmgr) */
   RETRIEVE,            /* 15: retrieve file to client */ 
   RETRIEVE_RECORD,     /* 16: retrieve file via client api (via RC) */
   RETRIEVE_STAGE,      /* 17: retrieve file to client via read cache */
   STAGE,               /* 18: stage file to read cache */
   QUERY_UNSTAGE,       /* 19: query for files to be unstaged */
   UNSTAGE,             /* 20: remove file from read cache */
   QUERY_POOL,          /* 21: query for status of disk pools */
   QUERY_WORKSPACE,     /* 22: query files to be staged, get RC status*/
   QUERY_STAGE,         /* 23: query for files to be staged */
   ARCHIVE_TO_CACHE,    /* 24: arch. files from client to write cache */
   ARCHIVE_FROM_CACHE,  /* 25: archive files from write cache to TSM */
   QUERY_RETRIEVE,      /* 26: query for files to be retrieved */
   QUERY_ARCHIVE_OVER,    /* 27: query for files before ARCHIVE_OVER */
   QUERY_ARCHIVE_TO_CACHE,      /* 28: query before ARCHIVE_TO_CACHE */
   SPM_REQUEST_MOVER,           /* 29: request data mover for action */
   QUERY_RETRIEVE_RECORD,        /* 30: query before RETRIEVE_RECORD */
   QUERY_ARCHIVE_FROM_CACHE,  /* 31: query before ARCHIVE_FROM_CACHE */
   END_SESSION,                         /* 32: end session on server */
   QUERY_ARCHIVE_RECORD,          /* 33: query before ARCHIVE_RECORD */
   STAGE_FROM_CACHE,                 /* 34: copy files from WC to RC */
   RETRIEVE_FROM_CACHE,                /* 35: retrieve files from WC */
   SEND_FROM_CACHE,             /* 36: read in any cache, send to DM */
   QUERY_RETRIEVE_STAGE,          /* 37: query before RETRIEVE_STAGE */
   COPY_RC_TO_FILESYSTEM,  /* 38: copy from read cache to mounted FS */
   RETRIEVE_TO_FILESYSTEM,    /* 39: retrieve from TSM to mounted FS */
   ARCHIVE_FROM_FILESYSTEM,/* 40: archive: mounted FS to write cache */
   COPY_WC_TO_FILESYSTEM,   /* 41: online copy from WC to mounted FS */
   MIGRATE_CACHE_PATH,      /* 42: migrate complete write cache path
                                   (force migration) */
   QUERY_FILESERVER,    /* 43: query files in specified 2nd level DM */
   SEND_TO_FILESERVER,       /* 44: read from RC, send to FileServer */
   RECEIVE_FROM_DM,  /* 45: on FileServer: recv from DM, write to FS */
   SHOW_ARCHIVES,               /* 46 show list of matching archives */
   STAGE_PARALLEL, /* 47 stage with several DMs in parallel or sequ. */
   SET_GLOBAL_SOCKET,  /* 48 create global socket buffer in CacheMgr
                             also lock action */
   UNSET_GLOBAL_SOCKET,/*49 mark global socket in CacheMgr as invalid
                            also lock action */
   SEND_GLOBAL_NOTICE,/*50 send notice for global socket in CacheMgr
                           also lock action */
   SEND_FROM_TAPE,             /* 51 read data from tape, send to DM */
   RETRIEVE_PARALLEL /*52 retr to lustre with several DMs in parallel*/
};

typedef struct                                 /* info for file list */
{
   char cFile[MAX_FULL_FILE];           /* fully qualified file name */
} srawFileList;

/* command buffer with object attributes */
typedef struct
{
   int iIdent;               /* IDENT_COMM identifies command buffer */
   int iAction;                          /* see definition of ACTION */
   int iCommLen;                         /* length of following data */
   char cNamefs[MAX_OBJ_FS];                       /* filespace name */
   char cNamehl[MAX_OBJ_HL];               /* object high level name */
   char cNamell[MAX_OBJ_LL];    /* file name / object low level name */
   char cOwner[MAX_OWNER];              /* account name of requestor */
   char cOS[MAX_OS];                        /* user operating system */
   char cApplType[MAX_APPLTYPE];                 /* application type */
   int iFileType;                      /* see definition of FILETYPE */
   int iBufsizeFile;                             /* buffer size file */
   unsigned int iFileSize; /* file size (byte), overlaid with 'long' */
   unsigned int iFileSize2;                  /* both together 8 byte */
   int iArchDev;                     /* = ARCH_DEVICE or RETR_DEVICE */
   unsigned int iObjHigh;              /* upper four bytes object Id */
   unsigned int iObjLow;               /* lower four bytes object Id */
   char cliNode[MAX_NODE];                       /* client node name */
   int iExecStatus;   /* execution status set by LockManager:
            =1: active, =2 waiting, =3 suppressed (--> not yet impl) */
   int iWaitTime;         /* time to wait before execution (seconds) */
   int iSynchId;
          /* = 1: RFIO write in file loop (1st: select DM)
             = 2: RFIO read in file loop (1st: select DM)
             = 3: RFIO file delete
             =11: requested action recursive (cmd client only)
             =21: (else) check access rights also for admin user
            +100: as before, but use alternate prod TSM server */
   char cTapeLib[16];            /* name of ATL: "" default, "*" all */ 
   char cNodeCacheMgr[MAX_NODE];     /* name of node with cache mgrs */
   int iATLServer;
          /* = 1: prod TSM server (aixtsm1),
             = 2: test TSM server (aixtsm2),
             < 0: gStore test system */
   int iPoolIdRC;              /* read cache PoolId (= 0: not in RC) */
   char cNodeRC[MAX_NODE];   /* read cache DM name (= "": not in RC) */
   int iStageFSid; /* stage FS no. on read cache DM (= 0: not in RC) */
   int iPoolIdWC;             /* write cache PoolId (= 0: not in WC) */
   char cNodeWC[MAX_NODE];  /* write cache DM name (= "": not in WC) */
   int iFSidWC;         /* FS no. on write cache DM (= 0: not in WC) */
   char cNodeFC[MAX_NODE]; /* file server cache node ("": not in FC) */
   int iFSidFC; /* FS no. on file server cache node (= 0: not in FC) */
   int iDataFS; /* >0: handle files on central data FS
                   >1: base hl dir level, append further subdirs to
                       data FS dir (retrieve) */ 
   char cDataFS[MAX_FULL_FILE];      /* full path in central data FS */
   int iClient32;                   /* =1: 32 bit gstore/RFIO client */
   int iReservation;
       /* <0: request filesize check comparing cacheDB - DM FS
          >0: in API client file loops: no. of open connection, 
                 appended to node name (lxgstore)
                 i>1000 => server test system, used i-1000 
              in servers: cur obj belongs to workspace with specified
                 space reservation number (--> not yet impl) */
   int iGlobalActionId; /* global actionId for parallel/sequ actions */
} srawComm;

enum STATUS                                      /* status of action */
{
   STA_BEGIN_TRANS,    /*  0: server ready for data transfer */
   STA_NEXT_BUFFER,    /*  1: API client: request next sequ. buffer */
   STA_END_OF_FILE,    /*  2: okay, handle next file */
   STA_END_OF_SESSION, /*  3: okay, end session */ 
   STA_ERROR,          /*  4: error, end session */
   STA_ERROR_EOF,      /*  5: error, handle next file */
   STA_FILE_STAGED,    /*  6: file staged (RC) or written to data FS */
   STA_END_OF_FILE_NOT_STAGED, /*  7: file retrieved, but not staged */
   STA_SEEK_BUFFER,    /*  8: API client: seek new buffer */
   STA_ARCHIVE_AVAIL,  /*  9: archive already available on server */
   STA_SWITCH_SERVER,  /* 10: switch data mover */
   STA_INFO,           /* 11: informatory message */
   STA_PURGED,         /* 12: some/all requested files purged on DM */
   STA_NOT_PURGED,     /* 13: no file could be purged on DM */
   STA_CACHE_FULL,     /* 14: write cache currently full */
   STA_CACHE_FILE_AWAY,
             /* 15: file no longer in write cache, probably archived */
   STA_ARCHIVE_NOT_AVAIL, /* 16: archive not available on server */
   STA_NO_ACCESS,      /* 17: requested access to archive not allowed */
   STA_SWITCH_ENTRY,   /* 18: switch entry server */
   STA_ENTRY_INFO,     /* 19: info: name of entry server */
   STA_FILE_CACHED,  /* 20: file archived from central data FS to WC */
   STA_CACHE_COPY, /* 21: successfull copy from WC to central dataFS */
   STA_CACHE_COPY_ERROR,
             /* 22: error status for copy from WC to central data FS */
   STA_FILE_AVAIL, /* 23: file already available in (G)RC or data FS */
   STA_FILE_CACHED_OVER
                  /* 24: file in WC overwritten from central data FS */
};

typedef struct                                      /* Status buffer */
{
   int iIdent;              /* IDENT_STATUS identifies status buffer */
   int iStatus;                          /* see definition of STATUS */
   int iStatusLen;                    /* length of following message */
   char cStatus[STATUS_LEN];                       /* status message */
} srawStatus;

typedef struct                      /* Request buffer for API client */
{
   int iIdent;        /* IDENT_NEXT_BUFFER identifies request buffer */
   int iStatus;                          /* see definition of STATUS */
   int iStatusLen;                       /* length of following info */
   int iSeekMode;    /* byte offset mode (see lseek C function):
                SEEK_SET: set offset to iOffset bytes
                SEEK_CUR: increase current offset for iOffset bytes
                SEEK_END: set offset to file size plus iOffset bytes */
   int iOffset;                             /* requested byte offset */
   int iBufferSize;                    /* length of requested buffer */
} srawRequest;

typedef struct                 /* object attribute buffer version 4 */
{
   int iVersion;               /* structure version: 4, 304 byte */
   char cNamefs[MAX_OBJ_FS];   /* filespace name */
   char cNamehl[MAX_OBJ_HL_O]; /* object high level name */
   char cNamell[MAX_OBJ_LL_O]; /* file name / object low level name  */
   int iFileType;              /* see definition of FILETYPE */
   int iBufsizeFile;           /* buffer size file */
   unsigned int iFileSize;   /* file size in bytes, old: 4 byte okay */
   char cDate[MAX_DATE];       /* archive date and time */
   char cOwner[MAX_OWNER];     /* account name of owner */
   char cOS[MAX_OS];           /* operating system archiving client */
   char cMgmtClass[MAX_MC_O];  /* management class */
   int iMediaClass;            /* media class */
   unsigned int iObjHigh;      /* upper four bytes object Id */
   unsigned int iObjLow;       /* lower four bytes object Id */
   unsigned int iRestoHigh;    /* top four bytes restore order */
   unsigned int iRestoLow;     /* lo_lo four bytes restore order */
   int iFS;                    /* filesystem number if file on disk */
   char cNode[MAX_NODE];       /* node name data mover */
   char cStageUser[MAX_OWNER]; /* account name of staging user */
   int iATLServer;
          /* = 1: prod TSM server (aixtsm1),
             = 2: test TSM server (aixtsm2),
             < 0: gStore test system */
   unsigned int iRestoHighHigh;/* hi_hi four bytes restore order */
   unsigned int iRestoHighLow; /* hi_lo four bytes restore order */
   unsigned int iRestoLowHigh; /* lo_hi four bytes restore order */
} srawObjAttrOld;

typedef struct /* query info of gStore object version 5 from server,
                          part of cacheDB buffer relevant for client */
{
   int iVersion;           /* structure version: VERSION_SRAWOBJATTR */
   int iSize;        /* size of this structure (version 5: 384 byte) */
   char cNamefs[MAX_OBJ_FS];                       /* filespace name */
   char cNamehl[MAX_OBJ_HL];               /* object high level name */
   char cNamell[MAX_OBJ_LL];   /* file name / object low level name  */
   int iFileType;                      /* see definition of FILETYPE */
   int iBufsizeFile;                                  /* buffer size */
   unsigned int iFileSize; /* file size (byte), overlaid with 'long' */
   unsigned int iFileSize2;                  /* both together 8 byte */
   char cDateCreate[MAX_DATE];       /* creation date/time in gStore */
   char cOwner[MAX_OWNER];                  /* account name of owner */
   char cOS[MAX_OS];            /* operating system archiving client */
   char cMgmtClass[MAX_MC+2]; /* TSM management class, padded 8 byte */
   int iMediaClass;                                   /* media class */
   unsigned int iObjHigh;              /* upper four bytes object Id */
   unsigned int iObjLow;               /* lower four bytes object Id */
   unsigned int iRestoHigh;          /* top four bytes restore order */
   unsigned int iRestoHighHigh;    /* hi_hi four bytes restore order */
   unsigned int iRestoHighLow;     /* hi_lo four bytes restore order */
   unsigned int iRestoLowHigh;     /* lo_hi four bytes restore order */
   unsigned int iRestoLow;         /* lo_lo four bytes restore order */
   int iATLServer;
          /* = 1: prod TSM server (aixtsm1),
             = 2: test TSM server (aixtsm2),
             < 0: gStore test system */
   int iPoolId;
      /* disk pool identifier:
          1: RetrievePool ATLServer1 (created by retrieve)
          2: StagePool ATLServer1  (created by stage)
          3: ArchivePool ATLServer1  (write cache)
          4: DAQPool ATLServer1  (write cache)
         11: RetrievePool ATLServer2 (created by retrieve)
         12: StagePool ATLServer2  (created by stage)
         13: ArchivePool ATLServer2 (write cache)
         14: DAQPool ATLServer2 (write cache)
       */
   char cNode[MAX_NODE];                     /* node name data mover */
   int iDM;             /* cache data mover number, =0: not in cache */
   int iFS;             /* cache filesystem number, =0: not in cache */
   int iFileSet;                    /* Id of file set (--> not used) */
   int iObjInfo;
          /* result of filesize check comparing cacheDB - DM FS:
             = 1: okay, = 2: diff file sizes */
   char cStageUser[MAX_OWNER];       /* account name of staging user */
} srawObjAttr;

typedef struct                 /* Query response buffer */
{
   int iIdent;  /* IDENT_QUERY, IDENT_QUERY_ARCHDB, IDENT_QUERY_DONE */
   int iObjCount;              /* no. of objects matching  */
   int iAttrLen;               /* length attributes buffer of object */
   srawObjAttr objAttr;        /* attributes of object found */
                               /* appended for each object   */
} srawQueryResult;

/* stage pool attributes for one data mover */
typedef struct
{
   char cNodeName[MAX_NODE];                 /* node name data mover */
   int iMaxSizeMover; /* max size of pool on this data mover (MByte) */
   int iFreeSizeHW;         /* current free size in hardware (MByte) */
   int iFreeSize;               /* current free size in pool (MByte) */
   int iFiles;                       /* current no. of files in pool */
   int iFileSystems; /* no. of FS on this data mover:
                        = 0: all FS, > 0: list follows (srawStageFS) */
} srawPoolStatusDMData;

/* stage pool attributes */
typedef struct
{
   char cPoolName[32];                                  /* pool name */
   char cPoolOS[MAX_OS];            /* name of OS where pool resides */
   int iPoolId;                        /* pool identification number */
   int iMaxSizeHW;                     /* overall size of HW (MByte) */
   int iFreeSizeHW;               /* current free size in HW (MByte) */
   int iMaxSize;                     /* overall size of pool (MByte) */
   int iFreeSize;               /* current free size in pool (MByte) */
   int iMaxWorkSize;       /* max size of work space in pool (MByte) */
   int iFileAvail;        /* guaranteed availability of files (days) */
   int iCheckSize;
       /* threshold work space size for check of pool status (MByte) */
   int iFiles;                       /* current no. of files in pool */
   int iDataMover;                  /* number of data movers in pool */
} srawPoolStatusData;

/* for exchange with client: status buffers stage pool + WS */
typedef struct
{
   int iIdent;           /* IDENT_POOL identifies pool status header */
   int iPoolNo;                /* no. of pool + WS buffers following */
   int iStatusLen;    /* size of following data (n pools, opt. 1 WS) */
   srawPoolStatusData sPoolStatusData;      /* stage pool attributes */
} srawPoolStatus;

typedef struct                   /* server infos on requ. work space */
{
   int iIdent;/* IDENT_WORKSPACE identifies work space status header */
   int iWorkId;                  /* work space identification number */
   int iStatusLen;                       /* length of following data */
   int iWorkSizeAll;             /* size of requ. work space (MByte) */
   int iWorkFilesAll;            /* total no. of files in work space */
   int iWorkSizeSta;      /* part of requ. work space already staged */
   int iWorkFilesSta;   /* no. of files in work space already staged */
   int iWorkSizeStaTemp;   /* part of staged work space in temp pool */
   int iWorkFilesStaTemp;        /* no. of staged files in temp pool */
   int iWorkSizeEst;      /* part of requ. work space size estimated */
   int iWorkFilesEst;
                   /* no. of files in work space with size estimated */
   int iStatus; /* work space status flag:
                   = -1: problem, can't create
                   =  0: okay, space available
                   =  1: lack of space in pool
                   =  2: above allowed limit of work space
                   =  3: work space larger than pool
                   =  9: lack of space -> clean job, no sleep
                  >= 10: lack of space -> clean job, sleep iStatus min.
                 */
} srawWorkSpace;

typedef struct                           /* attributes of data mover */
{
   char cNode[MAX_NODE];                                /* node name */
   int iPort;                                            /* port no. */
   int iSocket;                                        /* socket no. */
   int iExecStatus; /* execution status set by LockManager (unused):
                       = 1: active, = 2 waiting, = 3 suppressed      */
   int iWaitTime;         /* time to wait before execution (seconds) */
   int iATLServer;                           /* number of ATL Server */
          /* = 1: prod TSM server (aixtsm1),
             = 2: test TSM server (aixtsm2),
             < 0: gStore test system */
   int iPoolId;
          /* =1: RetrievePool, =2 StagePool,
             =3: ArchivePool, =4: DaqPool, =5: GlobalPool */
} srawDataMoverAttr;

typedef struct    /* communication buffer with data mover attributes */
{
   int iIdent;              /* IDENT_STATUS identifies status buffer */
   int iStatus;                          /* see definition of STATUS */
   int iStatusLen;                    /* length of following message */
   srawDataMoverAttr sDataMoverAttr;     /* attributes of data mover */
} srawDataMover;

typedef struct                                   /* filesystem infos */
{
   char cOS[MAX_OS];                             /* operating system */
   char cNode[MAX_NODE];                           /* datamover name */
   int iFileSystem;                     /* no. of filesystem on node */
   int iPoolId;                        /* read or write cache poolId */
   char cArchiveDate[MAX_DATE];      /* creation date in ArchivePool */
   char cArchiveUser[MAX_OWNER];   /* account name of archiving user */
} srawFileSystem;

typedef struct              /* for copy of DAQ data stream to dataFS */
{
   int iIdent;                                   /* IDENT_COPY_CACHE */
   int iCopyMode;
       /* = 0: ignore this buffer
          = 1: copy to pcCopyPath AFTER file written to WC
               (for high data rates, don't interfere writing to cache)
          = 2: for dataFS only:
               write each data buffer to WC and pcCopyPath
               (for low data rates, anyhow first buffers quickly
                available in lustre)
        */
   int iCopyLen;                     /* length following data (byte) */
   char cCopyPath[MAX_FULL_FILE];
       /* destination where to be copied
          = "/lustre..."  => fully qualified path name in lustre
          = "/hera..."    => fully qualified path name in lustre
            if not existing: will be created (see iPathConvention)
          = "RC" => read cache
        */
   int iCopyFraction;
       /* = i>0: copy each ith file to pcCopyPath
            if tape migration fails: ignore iCopyFraction, copy
            each file
        */
   int iMaxFile;
       /* for dataFS only:
          = 0: no file limit
          > 0: max no. of files to be written to directory
               files already existing are ignored
               if iMaxFile reached, new directory will be created
               (same level as previous directory)
        */
   int iPathConvention;
       /* rules for creation of initial/new path
          = 0: default convention
               initially specified .../xxx => .../xxx
               last .../xxx => create ...xxx1
               last .../xxxi => create ...xxxj j=i+1
          = 1: Hades convention: path name contains time stamp
               initially specified .../xxx => .../xxxyydddhhmm
               new => .../xxxyydddhhmm
        */
} srawCopyCache;

typedef struct                                   /* list of archives */
{
   int iIdent;                                 /* IDENT_ARCHIVE_LIST */
   int iArchCount;                /* no. of archives names following */
   int iArchSize;                 /* length of following data (byte) */
   char cArchive[MAX_OBJ_FS];                        /* archive name */
} srawArchiveList;

typedef struct    /* list of full obj names and filesizes in archive */
{
   int iCacheLocation;      /* 0: tape, 1: RC, 2: WC, 3: GRC, 4: GWC */ 
   int iPoolIdRC;
   int iPoolIdWC;        /* maybe >0 also for staged files (RC, GRC) */
   unsigned int iFileSize;                        /* 8 byte filesize */
   unsigned int iFileSize2;
   char cPath[MAX_OBJ_HL];                              /* path name */
   char cFile[MAX_OBJ_LL];                             /*  file name */
} srawObjNameList;

typedef struct            /* global socket data for parallel actions */
{
   int iSocket;    /* socket for communication client - entry server */
   int iActionId;       /* global actionId for parallel/sequ actions */
   char cClientNode[MAX_NODE];                   /* client node name */
   char cClientUser[MAX_OWNER];          /* user name on client node */
}
srawGlobalSocketData;

typedef struct          /* global socket buffer for parallel actions */
{
   int iIdent;                                /* IDENT_GLOBAL_SOCKET */
   int iAction;       /* [lock] actions with global sockets: 48 - 50 */
   int iDataLen;                 /* length of following data in byte */
   srawGlobalSocketData sGlobalSocketData;     /* global socket data */
} srawGlobalSocket;

typedef struct    /* global notification buffer for parallel actions */
{
   srawGlobalSocket sGlobalSocket;           /* global socket buffer */
   char cDataMover[MAX_NODE];     /* name of DM sending notification */
   char cNamefs[MAX_OBJ_FS];                       /* filespace name */
   char cNamehl[MAX_OBJ_HL];               /* object high level name */
   char cNamell[MAX_OBJ_LL];    /* file name / object low level name */
   char cFile[MAX_FULL_FILE];               /* full lustre file name */
   unsigned int iuFileSize;/* file size (byte), overlaid with 'long' */
   unsigned int iuFileSize2;                 /* both together 8 byte */
   int iStatus;                             /* status for cmd client */
   char cMsg[STATUS_LEN];            /* optionally: error/status msg */
} srawGlobalNotice;

