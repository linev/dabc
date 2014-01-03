#include "TRootSnifferStore.h"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRootSnifferStore                                                    //
//                                                                      //
// Used to store different results of objects scanning by TRootSniffer  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////


TRootSnifferStore::TRootSnifferStore() :
   TObject(),
   fResPtr(0),
   fResClass(0),
   fResNumChilds(-1)
{
   // normal constructor
}

TRootSnifferStore::~TRootSnifferStore()
{
   // destructor
}

void TRootSnifferStore::SetResult(void* _res, TClass* _rescl, Int_t _res_chld)
{
   // set pointer on found element, class and number of childs

   fResPtr = _res;
   fResClass = _rescl;
   fResNumChilds = _res_chld;
}

// =================================================================================

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRootSnifferStoreXml                                                 //
//                                                                      //
// Used to store scanned objects hierarchy in XML form                  //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

void TRootSnifferStoreXml::CreateNode(Int_t lvl, const char* nodename)
{
   // starts new xml node, will be closed by CloseNode

   buf->Append(TString::Format("%*s<%s", lvl*2, "", nodename));
}

void TRootSnifferStoreXml::SetField(Int_t, const char* field, const char* value, Int_t)
{
   // set field (xml attribute) in current node

   buf->Append(TString::Format(" %s=\"%s\"", field, value));
}

void TRootSnifferStoreXml::BeforeNextChild(Int_t, Int_t nchld, Int_t)
{
   // called before next child node created

   if (nchld==0) buf->Append(">\n");
}

void TRootSnifferStoreXml::CloseNode(Int_t lvl, const char* nodename, Int_t numchilds)
{
   // called when node should be closed
   // depending from number of childs different xml format is applied

   if (numchilds > 0)
      buf->Append(TString::Format("%*s</%s>\n", lvl*2, "", nodename));
   else
      buf->Append(TString::Format("/>\n"));
}

// ============================================================================

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TRootSnifferStoreXml                                                 //
//                                                                      //
// Used to store scanned objects hierarchy in JSON form                 //
//                                                                      //
//////////////////////////////////////////////////////////////////////////

void TRootSnifferStoreJson::CreateNode(Int_t lvl, const char* nodename)
{
   // starts new json object, will be closed by CloseNode

   buf->Append(TString::Format("%*s\"%s\" : {", lvl*4, "", nodename));
}

void TRootSnifferStoreJson::SetField(Int_t lvl, const char* field, const char* value, Int_t nfld)
{
   // set field (json field) in current node

   if (nfld==0) buf->Append("\n"); else buf->Append(",\n");
   buf->Append(TString::Format("%*s\"%s\" : \"%s\"", lvl*4+2, "", field, value));
}

void TRootSnifferStoreJson::BeforeNextChild(Int_t lvl, Int_t nchld, Int_t nfld)
{
   // called before next child node created

   if (nchld==0) {
      if (nfld==0) buf->Append("\n"); else buf->Append(",\n");
      buf->Append(TString::Format("%*s\"childs\" : [\n", lvl*4+2, ""));
   } else {
      buf->Append(",\n");
   }
}

void TRootSnifferStoreJson::CloseNode(Int_t lvl, const char*, Int_t numchilds)
{
   // called when node should be closed
   // depending from number of childs different xml format is applied

   if (numchilds > 0)
      buf->Append(TString::Format("\n%*s]", lvl*4+2, ""));
   buf->Append(TString::Format("\n%*s}", lvl*4, ""));
}

