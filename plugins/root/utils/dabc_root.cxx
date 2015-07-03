// dedicated to convert DABC files to ROOT files
// for instance, hierarchy stored in binary file, can be converted to ROOT histograms

#include <stdio.h>
#include <string.h>

#include "dabc/version.h"
#include "dabc/Buffer.h"
#include "dabc/Hierarchy.h"
#include "dabc/BinaryFileIO.h"
#include "dabc/Iterator.h"

#include "TFile.h"
#include "TH1.h"
#include "TH2.h"

int main(int argc, char* argv[]) {

   printf("dabc_root utility, v %s\n", DABC_RELEASE);

   const char* inpfile(0), *outfile(0);

   for (int n=1;n<argc;n++) {
      if ((strcmp(argv[n],"-h")==0) && (n+1 < argc)) {
         inpfile = argv[++n];
         continue;
      }
      if ((strcmp(argv[n],"-o")==0) && (n+1 < argc)) {
         outfile = argv[++n];
         continue;
      }
   }

   if ((inpfile==0) || (outfile==0)) {
      printf("Arguments missing\n");
      printf("-h filename     -   input file with stored hierarchy\n");
      printf("-o filename.root -  output ROOT file with histograms\n");
      return 1;
   }

   dabc::BinaryFile inp;

   if (!inp.OpenReading(inpfile)) {
      printf("Cannot open file %s for reading\n", inpfile);
      return 1;
   }

   uint64_t size, typ;

   if (!inp.ReadBufHeader(&size,&typ)) {
      printf("Cannot read buffer header from file %s\n", inpfile);
      return 1;
   }

   dabc::Buffer buf = dabc::Buffer::CreateBuffer(size);
   buf.SetTypeId(typ);

   if (!inp.ReadBufPayload(buf.SegmentPtr(), size)) {
      printf("Cannot read buffer %lu from file %s\n", (long unsigned) size, inpfile);
      return 1;
   }

   dabc::Hierarchy h;
   if (!h.ReadFromBuffer(buf)) {
      printf("Cannot decode hierarchy from buffer\n");
      return 1;
   }

   TFile* f = TFile::Open(outfile,"recreate");
   if (f==0) {
      printf("Cannot create %s for output\n", outfile);
      return 1;
   }

   int cnt = 0;
   TString lastdir;
   dabc::Iterator iter(h);
   while (iter.next()) {
      dabc::Hierarchy item = iter.ref();
      if (item.null()) continue;

      if (!item.HasField("_dabc_hist")) continue;

      TString dirname;

      for (unsigned n=1;n<iter.level();n++) {
         dabc::Object* prnt = iter.parent(n);
         if (prnt==0) break;

         if (n>1) dirname.Append("/");
         dirname.Append(prnt->GetName());
      }

      if (lastdir!=dirname) {
         f->mkdir(dirname.Data());
         f->cd(dirname.Data());
      }

      lastdir = dirname;

      if (item.Field("_kind").AsStr() == "ROOT.TH1D") {
         int nbins = item.Field("nbins").AsInt();
         TH1D* hist = new TH1D(item.GetName(), item.Field("_title").AsStr().c_str(),
                               nbins, item.Field("left").AsDouble(), item.Field("right").AsDouble());
         std::vector<double> bins = item.Field("bins").AsDoubleVect();

         for (int n=0;n<nbins+2;n++) hist->SetBinContent(n,bins[3+n]);

         hist->Write();
      } else
      if (item.Field("_kind").AsStr() == "ROOT.TH2D") {
         int nbins1 = item.Field("nbins1").AsInt();
         int nbins2 = item.Field("nbins2").AsInt();
         TH2D* hist = new TH2D(item.GetName(), item.Field("_title").AsStr().c_str(),
                               nbins1, item.Field("left1").AsDouble(), item.Field("right1").AsDouble(),
                               nbins2, item.Field("left2").AsDouble(), item.Field("right2").AsDouble());

         std::vector<double> bins = item.Field("bins").AsDoubleVect();

         for (int n=0;n<(nbins1+2)*(nbins2+2);n++) hist->SetBinContent(n,bins[6+n]);
         hist->Write();
      }

      cnt++;
   }

   printf("TOTAL %d histograms converted\n", cnt);

   delete f;

   return 0;
}
