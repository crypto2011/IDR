//---------------------------------------------------------------------------
#ifndef MiscH
#define MiscH
//---------------------------------------------------------------------------
#include "Decompiler.h"
//---------------------------------------------------------------------------
//Float Type
#define     FT_SINGLE       1
#define     FT_DOUBLE       2
#define     FT_EXTENDED     3
#define     FT_REAL         4
#define     FT_COMP         5
#define     FT_CURRENCY     6
//---------------------------------------------------------------------------
//global API
void __fastcall ScaleForm(TForm* AForm);
int __fastcall Adr2Pos(DWORD adr);
String __fastcall Val2Str0(DWORD Val);
String __fastcall Val2Str1(DWORD Val);
String __fastcall Val2Str2(DWORD Val);
String __fastcall Val2Str4(DWORD Val);
String __fastcall Val2Str5(DWORD Val);
String __fastcall Val2Str8(DWORD Val);
DWORD __fastcall Pos2Adr(int Pos);
PInfoRec __fastcall AddToBSSInfos(DWORD Adr, String AName, String ATypeName);
void __fastcall AddClassAdr(DWORD Adr, const String& AName);
void __fastcall AddFieldXref(PFIELDINFO fInfo, DWORD ProcAdr, int ProcOfs, char type);
void __fastcall AddPicode(int Pos, BYTE Op, String Name, int Ofs);
int __fastcall ArgsCmpFunction(void *item1, void *item2);
int __fastcall BranchGetPrevInstructionType(DWORD fromAdr, DWORD* jmpAdr, PLoopInfo loopInfo);
bool __fastcall CanReplace(const String& fromName, const String& toName);
void __fastcall ClearFlag(DWORD flag, int pos);
void __fastcall ClearFlags(DWORD flag, int pos, int num);
void __fastcall Copy2Clipboard(TStrings* items, int leftMargin, bool asmCode);
int __fastcall ExportsCmpFunction(void *item1, void *item2);
String __fastcall ExtractClassName(const String& AName);
String __fastcall ExtractProcName(const String& AName);
String __fastcall ExtractName(const String& AName);
String __fastcall ExtractType(const String& AName);
void __fastcall FillArgInfo(int k, BYTE callkind, PARGINFO argInfo, BYTE** p, int* s);
DWORD __fastcall FindClassAdrByName(const String& AName);
int __fastcall FloatNameToFloatType(String AName);
String __fastcall GetArrayElementType(String arrType);
int __fastcall GetArrayElementTypeSize(String arrType);
bool __fastcall GetArrayIndexes(String arrType, int ADim, int* LowIdx, int* HighIdx);
int __fastcall GetArraySize(String arrType);
DWORD __fastcall GetChildAdr(DWORD Adr);
DWORD __fastcall GetClassAdr(const String& AName);
int __fastcall GetClassSize(DWORD adr);
String __fastcall GetClsName(DWORD adr);
String __fastcall GetDefaultProcName(DWORD adr);
String __fastcall GetDynaInfo(DWORD adr, WORD id, DWORD* dynAdr);
String __fastcall GetDynArrayTypeName(DWORD adr);
String __fastcall GetEnumerationString(String TypeName, Variant Val);
String __fastcall GetImmString(int Val);
String __fastcall GetImmString(String TypeName, int Val);
PInfoRec __fastcall GetInfoRec(DWORD adr);
int __fastcall GetLastLocPos(int fromAdr);
String __fastcall GetModuleVersion(const String& module);
int __fastcall GetNearestArgA(int fromPos);
int __fastcall GetNearestDownInstruction(int fromPos);
int __fastcall GetNearestDownInstruction(int fromPos, char* Instruction);
int __fastcall GetNearestUpPrefixFs(int fromPos);
int __fastcall GetNearestUpInstruction(int fromPos);
int __fastcall GetNthUpInstruction(int fromPos, int N);
int __fastcall GetNearestUpInstruction(int fromPos, int toPos);
int __fastcall GetNearestUpInstruction(int fromPos, int toPos, int no);
int __fastcall GetNearestUpInstruction1(int fromPos, int toPos, char* Instruction);
int __fastcall GetNearestUpInstruction2(int fromPos, int toPos, char* Instruction1, char* Instruction2);
DWORD __fastcall GetParentAdr(DWORD Adr);
String __fastcall GetParentName(DWORD adr);
String __fastcall GetParentName(const String& ClassName);
int __fastcall GetParentSize(DWORD Adr);
int __fastcall GetProcRetBytes(MProcInfo* pInfo);
int __fastcall GetProcSize(DWORD fromAdr);
int __fastcall StrGetRecordSize(String str);
int __fastcall StrGetRecordFieldOffset(String str);
String __fastcall StrGetRecordFieldName(String str);
String __fastcall StrGetRecordFieldType(String str);
int __fastcall GetRecordSize(String AName);
String __fastcall GetRecordFields(int AOfs, String AType);
String __fastcall GetAsmRegisterName(int Idx);
String __fastcall GetDecompilerRegisterName(int Idx);
String __fastcall GetSetString(String TypeName, BYTE* ValAdr);
DWORD __fastcall GetStopAt(DWORD vmtAdr);
DWORD __fastcall GetOwnTypeAdr(String AName);
PTypeRec __fastcall GetOwnTypeByName(String AName);
String __fastcall GetTypeDeref(String ATypeName);
BYTE __fastcall GetTypeKind(String AName, int* size);
int __fastcall GetRTTIRecordSize(DWORD adr);
int __fastcall GetPackedTypeSize(String AName);
String __fastcall GetTypeName(DWORD TypeAdr);
int __fastcall GetTypeSize(String AName);
int __fastcall ImportsCmpFunction(void *item1, void *item2);
bool __fastcall IsADC(int Idx);
int __fastcall IsBoundErr(DWORD fromAdr);
bool __fastcall IsConnected(DWORD fromAdr, DWORD toAdr);
bool __fastcall IsBplByExport(const char* bpl);
bool __fastcall IsDefaultName(String AName);
DWORD __fastcall IsGeneralCase(DWORD fromAdr, int retAdr);
bool __fastcall IsExit(DWORD fromAdr);
bool __fastcall IsInheritsByAdr(const DWORD Adr1, const DWORD Adr2);
bool __fastcall IsInheritsByClassName(const String& Name1, const String& Name2);
bool __fastcall IsInheritsByProcName(const String& Name1, const String& Name2);
int __fastcall IsInitStackViaLoop(DWORD fromAdr, DWORD toAdr);
bool __fastcall IsSameRegister(int Idx1, int Idx2);
bool __fastcall IsValidCodeAdr(DWORD Adr);
bool __fastcall IsValidCString(int pos);
bool __fastcall IsValidImageAdr(DWORD Adr);
bool __fastcall IsValidModuleName(int len, int pos);
bool __fastcall IsValidName(int len, int pos);
bool __fastcall IsValidString(int len, int pos);
bool __fastcall IsXorMayBeSkipped(DWORD fromAdr);
void __fastcall MakeGvar(PInfoRec recN, DWORD adr, DWORD xrefAdr);
String __fastcall MakeGvarName(DWORD adr);
int __fastcall MethodsCmpFunction(void *item1, void *item2);
void __fastcall OutputDecompilerHeader(FILE* f);
bool __fastcall IsFlagSet(DWORD flag, int pos);
void __fastcall SetFlag(DWORD flag, int pos);
void __fastcall SetFlags(DWORD flag, int pos, int num);
int __fastcall SortUnitsByAdr(void *item1, void* item2);
int __fastcall SortUnitsByNam(void *item1, void* item2);
int __fastcall SortUnitsByOrd(void *item1, void* item2);
String __fastcall TransformString(char* str, int len);
String __fastcall TransformUString(WORD codePage, wchar_t* data, int len);
String __fastcall TrimTypeName(const String& TypeName);
String __fastcall TypeKind2Name(BYTE kind);
String __fastcall UnmangleName(String Name);
//Decompiler
int __fastcall IsAbs(DWORD fromAdr);
int _fastcall IsIntOver(DWORD fromAdr);
int __fastcall IsInlineLengthCmp(DWORD fromAdr);
int __fastcall IsInlineLengthTest(DWORD fromAdr);
int __fastcall IsInlineDiv(DWORD fromAdr, int* div);
int __fastcall IsInlineMod(DWORD fromAdr, int* mod);
int __fastcall ProcessInt64Equality(DWORD fromAdr, DWORD* maxAdr);
int __fastcall ProcessInt64NotEquality(DWORD fromAdr, DWORD* maxAdr);
int __fastcall ProcessInt64Comparison(DWORD fromAdr, DWORD* maxAdr);
int __fastcall ProcessInt64ComparisonViaStack1(DWORD fromAdr, DWORD* maxAdr);
int __fastcall ProcessInt64ComparisonViaStack2(DWORD fromAdr, DWORD* maxAdr);
int __fastcall IsInt64Equality(DWORD fromAdr, int* skip1, int* skip2, bool *immVal, __int64* Val);
int __fastcall IsInt64NotEquality(DWORD fromAdr, int* skip1, int* skip2, bool *immVal, __int64* Val);
int __fastcall IsInt64Comparison(DWORD fromAdr, int* skip1, int* skip2, bool *immVal, __int64* Val);
int __fastcall IsInt64ComparisonViaStack1(DWORD fromAdr, int* skip1, DWORD* simEnd);
int __fastcall IsInt64ComparisonViaStack2(DWORD fromAdr, int* skip1, int* skip2, DWORD* simEnd);
int __fastcall IsInt64Shr(DWORD fromAdr);
int __fastcall IsInt64Shl(DWORD fromAdr);
String __fastcall InputDialogExec(String caption, String labelText, String text);
String __fastcall ManualInput(DWORD procAdr, DWORD curAdr, String caption, String labelText);
bool __fastcall MatchCode(BYTE* code, MProcInfo* pInfo);
void __fastcall SaveCanvas(TCanvas* ACanvas);
void __fastcall RestoreCanvas(TCanvas* ACanvas);
void __fastcall DrawOneItem(String AItem, TCanvas* ACanvas, TRect &ARect, TColor AColor, int flags);
int __fastcall IsTryBegin(DWORD fromAdr, DWORD* endAdr);
int __fastcall IsTryBegin0(DWORD fromAdr, DWORD* endAdr);
int __fastcall IsTryEndPush(DWORD fromAdr, DWORD* endAdr);
int __fastcall IsTryEndJump(DWORD fromAdr, DWORD* endAdr);

PFIELDINFO __fastcall GetClassField(String TypeName, int Offset);
int __fastcall GetRecordField(String ARecType, int AOfs, String& name, String& type);
int __fastcall GetField(String TypeName, int Offset, String& name, String& type);
//---------------------------------------------------------------------------
#endif
