//---------------------------------------------------------------------------
#ifndef IDCGenH
#define IDCGenH

#include "Main.h"

typedef struct
{
    int     index;      //Index of name in names
    int     counter;    //Counter
} REPNAMEINFO, *PREPNAMEINFO;

class TIDCGen
{
public:
    __fastcall TIDCGen(FILE* FIdc, int splitSize);
    __fastcall ~TIDCGen();
    void __fastcall NewIDCPart(FILE* FIdc);
    void __fastcall DeleteName(int pos);
    int __fastcall MakeByte(int pos);
    int __fastcall MakeWord(int pos);
    int __fastcall MakeDword(int pos);
    int __fastcall MakeQword(int pos);
    int __fastcall MakeArray(int pos, int num);
    int __fastcall MakeShortString(int pos);
    int __fastcall MakeCString(int pos);
    void __fastcall MakeLString(int pos);
    void __fastcall MakeWString(int pos);
    void __fastcall MakeUString(int pos);
    int __fastcall MakeCode(int pos);
    void __fastcall MakeFunction(DWORD adr);
    void __fastcall MakeComment(int pos, String text);
    int __fastcall OutputAttrData(int pos);
    void __fastcall OutputHeaderFull();
    void __fastcall OutputHeaderShort();
    int __fastcall OutputRTTIHeader(BYTE kind, int pos);
    void __fastcall OutputRTTIInteger(BYTE kind, int pos);
    void __fastcall OutputRTTIChar(BYTE kind, int pos);
    void __fastcall OutputRTTIEnumeration(BYTE kind, int pos, DWORD adr);
    void __fastcall OutputRTTIFloat(BYTE kind, int pos);
    void __fastcall OutputRTTIString(BYTE kind, int pos);
    void __fastcall OutputRTTISet(BYTE kind, int pos);
    void __fastcall OutputRTTIClass(BYTE kind, int pos);
    void __fastcall OutputRTTIMethod(BYTE kind, int pos);
    void __fastcall OutputRTTIWChar(BYTE kind, int pos);
    void __fastcall OutputRTTILString(BYTE kind, int pos);
    void __fastcall OutputRTTIWString(BYTE kind, int pos);
    void __fastcall OutputRTTIVariant(BYTE kind, int pos);
    void __fastcall OutputRTTIArray(BYTE kind, int pos);
    void __fastcall OutputRTTIRecord(BYTE kind, int pos);
    void __fastcall OutputRTTIInterface(BYTE kind, int pos);
    void __fastcall OutputRTTIInt64(BYTE kind, int pos);
    void __fastcall OutputRTTIDynArray(BYTE kind, int pos);
    void __fastcall OutputRTTIUString(BYTE kind, int pos);
    void __fastcall OutputRTTIClassRef(BYTE kind, int pos);
    void __fastcall OutputRTTIPointer(BYTE kind, int pos);
    void __fastcall OutputRTTIProcedure(BYTE kind, int pos);
    void __fastcall OutputVMT(int pos, PInfoRec recN);
    int __fastcall OutputVMTHeader(int pos, String vmtName);
    void __fastcall OutputIntfTable(int pos);
    void __fastcall OutputIntfVTable(int pos, DWORD stopAdr);
    void __fastcall OutputAutoTable(int pos);
    void __fastcall OutputAutoPTable(int pos);
    void __fastcall OutputInitTable(int pos);
    void __fastcall OutputFieldTable(int pos);
    void __fastcall OutputFieldTTable(int pos);
    void __fastcall OutputMethodTable(int pos);
    void __fastcall OutputVmtMethodEntry(int pos);
    int __fastcall OutputVmtMethodEntryTail(int pos);
    void __fastcall OutputDynamicTable(int pos);
    void __fastcall OutputResString(int pos, PInfoRec recN);
    int __fastcall OutputProc(int pos, PInfoRec recN, bool imp);
    void __fastcall OutputData(int pos, PInfoRec recN);
    PREPNAMEINFO __fastcall GetNameInfo(int idx);
    FILE*           idcF;
    String          unitName;
    String          itemName;
    TStringList*    names;
    TList*          repeated;
    int             SplitSize;//Maximum output bytes if idc splitted
    int             CurrentPartNo;//Current part number (filename looks like XXX_NN.idc)
    int             CurrentBytes;//Current part output bytes
};

class TSaveIDCDialog : public TOpenDialog
{
public:
	__fastcall TSaveIDCDialog(TComponent* AOwner, char* TemplateName);
protected:
	virtual void __fastcall WndProc(Messages::TMessage &Message);
};
//---------------------------------------------------------------------------
#endif


