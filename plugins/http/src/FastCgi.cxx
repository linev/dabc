// $Id$

/************************************************************
 * The Data Acquisition Backbone Core (DABC)                *
 ************************************************************
 * Copyright (C) 2009 -                                     *
 * GSI Helmholtzzentrum fuer Schwerionenforschung GmbH      *
 * Planckstr. 1, 64291 Darmstadt, Germany                   *
 * Contact:  http://dabc.gsi.de                             *
 ************************************************************
 * This software can be used under the GPL license          *
 * agreements as stated in LICENSE.txt file                 *
 * which is part of the distribution.                       *
 ************************************************************/

#include "http/FastCgi.h"

#include <string.h>

#include "dabc/Publisher.h"

#ifndef DABC_WITHOUT_FASTCGI

#include "fcgiapp.h"
#include <fstream>

static const struct {
  const char *extension;
  int ext_len;
  const char *mime_type;
} builtin_mime_types[] = {
  {".html", 5, "text/html"},
  {".htm", 4, "text/html"},
  {".shtm", 5, "text/html"},
  {".shtml", 6, "text/html"},
  {".css", 4, "text/css"},
  {".js",  3, "application/x-javascript"},
  {".ico", 4, "image/x-icon"},
  {".gif", 4, "image/gif"},
  {".jpg", 4, "image/jpeg"},
  {".jpeg", 5, "image/jpeg"},
  {".png", 4, "image/png"},
  {".svg", 4, "image/svg+xml"},
  {".txt", 4, "text/plain"},
  {".torrent", 8, "application/x-bittorrent"},
  {".wav", 4, "audio/x-wav"},
  {".mp3", 4, "audio/x-mp3"},
  {".mid", 4, "audio/mid"},
  {".m3u", 4, "audio/x-mpegurl"},
  {".ogg", 4, "application/ogg"},
  {".ram", 4, "audio/x-pn-realaudio"},
  {".xml", 4, "text/xml"},
  {".json",  5, "text/json"},
  {".xslt", 5, "application/xml"},
  {".xsl", 4, "application/xml"},
  {".ra",  3, "audio/x-pn-realaudio"},
  {".doc", 4, "application/msword"},
  {".exe", 4, "application/octet-stream"},
  {".zip", 4, "application/x-zip-compressed"},
  {".xls", 4, "application/excel"},
  {".tgz", 4, "application/x-tar-gz"},
  {".tar", 4, "application/x-tar"},
  {".gz",  3, "application/x-gunzip"},
  {".arj", 4, "application/x-arj-compressed"},
  {".rar", 4, "application/x-arj-compressed"},
  {".rtf", 4, "application/rtf"},
  {".pdf", 4, "application/pdf"},
  {".swf", 4, "application/x-shockwave-flash"},
  {".mpg", 4, "video/mpeg"},
  {".webm", 5, "video/webm"},
  {".mpeg", 5, "video/mpeg"},
  {".mov", 4, "video/quicktime"},
  {".mp4", 4, "video/mp4"},
  {".m4v", 4, "video/x-m4v"},
  {".asf", 4, "video/x-ms-asf"},
  {".avi", 4, "video/x-msvideo"},
  {".bmp", 4, "image/bmp"},
  {".ttf", 4, "application/x-font-ttf"},
  {NULL,  0, NULL}
};

const char *FCGX_mime_type(const char *path) {
  const char *ext;
  int i, path_len;

  path_len = strlen(path);

  for (i = 0; builtin_mime_types[i].extension != NULL; i++) {
     if (path_len <= builtin_mime_types[i].ext_len) continue;
     ext = path + (path_len - builtin_mime_types[i].ext_len);
     if (strcmp(ext, builtin_mime_types[i].extension) == 0) {
       return builtin_mime_types[i].mime_type;
     }
  }

  return "text/plain";
}



void FCGX_send_file(FCGX_Request* request, const char* fname)
{
   std::ifstream is(fname);

   char* buf = 0;
   int length = 0;

   if (is) {
      is.seekg (0, is.end);
      length = is.tellg();
      is.seekg (0, is.beg);

      buf = (char*) malloc(length);
      is.read(buf, length);
      if (!is) {
         free(buf);
         buf = 0; length = 0;
      }
   }

   if (buf==0) {
      FCGX_FPrintF(request->out,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Length: 0\r\n" // Always set Content-Length
            "Connection: close\r\n\r\n");
   } else {

/*      char sbuf[100], etag[100];
      time_t curtime = time(NULL);
      strftime(sbuf, sizeof(sbuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&curtime));
      snprintf(etag, sizeof(etag), "\"%lx.%ld\"",
               (unsigned long) curtime, (long) length);

      // Send HTTP reply to the client
      FCGX_FPrintF(request->out,
             "HTTP/1.1 200 OK\r\n"
             "Date: %s\r\n"
             "Last-Modified: %s\r\n"
             "Etag: %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"     // Always set Content-Length
             "\r\n", sbuf, sbuf, etag, FCGX_mime_type(fname), length);

*/
      FCGX_FPrintF(request->out,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"     // Always set Content-Length
             "\r\n", FCGX_mime_type(fname), length);

      FCGX_PutStr(buf, length, request->out);

      // printf("Send file %s mime %s time %s etag %s \n", fname, FCGX_mime_type(fname), sbuf, etag);

      free(buf);
   }

}



#endif


http::FastCgi::FastCgi(const std::string& name, dabc::Command cmd) :
   http::Server(name, cmd),
   fCgiPort(9000),
   fSocket(0),
   fThrd(0)
{
   fCgiPort = Cfg("port", cmd).AsInt(9000);
}

http::FastCgi::~FastCgi()
{
   if (fThrd) {
      fThrd->Kill();
      delete fThrd;
      fThrd = 0;
   }

   if (fSocket>0) {
      // close opened socket
      close(fSocket);
      fSocket = 0;
   }
}


void http::FastCgi::OnThreadAssigned()
{
   std::string sport = ":9000";
   if (fCgiPort>0) sport = dabc::format(":%d",fCgiPort);

#ifndef DABC_WITHOUT_FASTCGI
   FCGX_Init();

   DOUT0("Starting FastCGI server on port %s", sport.c_str());

   fSocket = FCGX_OpenSocket(sport.c_str(), 10);

   fThrd = new dabc::PosixThread();
   fThrd->Start(http::FastCgi::RunFunc, this);

#else
   EOUT("DABC compiled without FastCgi support");
#endif

}

void* http::FastCgi::RunFunc(void* args)
{

#ifndef DABC_WITHOUT_FASTCGI

   http::FastCgi* server = (http::FastCgi*) args;

   FCGX_Request request;

   FCGX_InitRequest(&request, server->fSocket, 0);

   while (1) {

      int rc = FCGX_Accept_r(&request);

      if (rc!=0) continue;

      const char* inp_path = FCGX_GetParam("PATH_INFO", request.envp);
      const char* inp_query = FCGX_GetParam("QUERY_STRING", request.envp);

      std::string pathname, filename, query;

      if (server->IsFileRequested(inp_path, filename)) {
         FCGX_send_file(&request, filename.c_str());
         FCGX_Finish_r(&request);
         continue;
      }

      const char* rslash = strrchr(inp_path,'/');
      if (rslash==0) {
         filename = inp_path;
      } else {
         pathname.append(inp_path, rslash - inp_path);
         if (pathname=="/") pathname.clear();
         filename = rslash+1;
      }

      if (inp_query) query = inp_query;

      std::string content_type, content_str;
      dabc::Buffer content_bin;

      if (!server->Process(pathname, filename, query,
                           content_type, content_str, content_bin)) {
         FCGX_FPrintF(request.out, "HTTP/1.1 404 Not Found\r\n"
                                   "Content-Length: 0\r\n"
                                   "Connection: close\r\n\r\n");
      } else

      if (content_type=="__file__") {
         FCGX_send_file(&request, content_str.c_str());
      } else

      if (!content_bin.null()) {
         FCGX_FPrintF(request.out,
                  "HTTP/1.1 200 OK\r\n"
                  "Content-Type: %s\r\n"
                  "Content-Length: %ld\r\n"
                  "Connection: keep-alive\r\n"
                  "\r\n",
                  content_type.c_str(),
                  content_bin.GetTotalSize());

         FCGX_PutStr((const char*) content_bin.SegmentPtr(), (int) content_bin.GetTotalSize(), request.out);
      } else {

         // Send HTTP reply to the client
         FCGX_FPrintF(request.out,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"        // Always set Content-Length
             "\r\n"
             "%s",
             content_type.c_str(),
             content_str.length(),
             content_str.c_str());
      }


      FCGX_Finish_r(&request);

   } /* while */

#endif

   return 0;
}