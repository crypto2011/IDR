//---------------------------------------------------------------------------
#ifndef KnowledgeBaseH
#define KnowledgeBaseH
//---------------------------------------------------------------------------
#include <stdio.h>
//---------------------------------------------------------------------------
//Информация о смещениях имен и данных
typedef struct
{
    DWORD	Offset;
    DWORD	Size;
    int     ModId;    //Modules
    int     NamId;    //Names
} OFFSETSINFO, *POFFSETSINFO;
//Fixup info
typedef struct
{
    BYTE        Type;           //'A' - ADR, 'J' - JMP, 'D' - DAT
    DWORD       Ofs;            //Смещение относительно начала дампа
    char        *Name;
} FIXUPINFO, *PFIXUPINFO;

/*
ModuleDataTable
---------------
//Состоит из ModuleCount записей вида
WORD 	ID;
PSTR 	ModuleName;
PSTR	Filename;
WORD 	UsesNum;
WORD 	UsesID[UsesNum];	//Массив идентификаторов модулей
PSTR	UsesNames[UsesNum];	//Массив имен модулей

ConstDataTable
--------------
//Состоит из ModuleCount записей вида
WORD 	ModuleID;
PSTR 	ConstName;
BYTE 	Type; //'C'-ConstDecl, 'P'-PDecl (VMT), 'V'-VarCDecl
PSTR 	TypeDef; //Тип
PSTR 	Value; //Значение
DWORD	DumpTotal;	//Общий размер дампа (дамп+релоки+фиксапы)
DWORD 	DumpSize; //Размер бинарного дампа (RTTI)
DWORD 	FixupNum; //Количество фиксапов дампа
BYTE 	Dump[DumpSize]; //Бинарный дамп (RTTI)
BYTE 	Relocs[DumpSize];
FIXUPINFO Fixups[FixupNum]; //Массив фиксапов

TypeDataTable
-------------
//Состоит из TypeCount записей вида
DWORD   Size; //Size of Type
WORD 	ModuleID;
PSTR 	TypeName;
BYTE 	Kind; //drArrayDef,...,drVariantDef (см. начало)
DWORD 	VMCnt; //Количество элементов VMT (начиная с 0)
PSTR 	Decl; //Декларация
DWORD	DumpTotal;	//Общий размер дампа (дамп+релоки+фиксапы)
DWORD 	DumpSize; //Размер бинарного дампа (RTTI)
DWORD 	FixupNum; //Количество фиксапов дампа
BYTE 	Dump[DumpSize]; //Бинарный дамп (RTTI)
BYTE 	Relocs[DumpSize];
FIXUPINFO Fixups[FixupNum]; //Фиксапы
DWORD	FieldsTotal;	//Общий размер данных полей
WORD 	FieldsNum; //Количество полей (class, interface, record)
FIELDINFO Fields[FieldNum]; //Поля
DWORD	PropsTotal;	//Общий размер данных свойств
WORD 	PropsNum; //Количество свойств (class, interface)
PROPERTYINFO Props[PropNum]; //Свойства
DWORD	MethodsTotal;	//Общий размер данных методов
WORD 	MethodsNum; //Количество методов (class, interface)
METHODINFO Methods[MethodNum]; //Методы

VarDataTable
------------
//Состоит из VarCount записей вида
WORD 	ModuleID;
PSTR 	VarName;
BYTE 	Type; //'V'-Var;'A'-AbsVar;'S'-SpecVar;'T'-ThreadVar
PSTR 	TypeDef;
PSTR 	AbsName; //Для ключевого слова absolute

ResStrDataTable
---------------
//Состоит из ResStrCount записей вида
WORD 	ModuleID;
PSTR 	ResStrName;
PSTR 	TypeDef;
PSTR	Context;

ProcDataTable
-------------
//Contains ProcCount structures:
WORD 	ModuleID;
PSTR 	ProcName;
BYTE 	Embedded; //Contains embedded procs
BYTE 	DumpType; //'C' - code, 'D' - data
BYTE 	MethodKind; //'M'-method,'P'-procedure,'F'-function,'C'-constructor,'D'-destructor
BYTE 	CallKind; //1-'cdecl', 2-'pascal', 3-'stdcall', 4-'safecall'
int 	VProc; //Flag for "overload" (if Delphi version > verD3 and VProc&0x1000 != 0)
PSTR 	TypeDef; //Type of Result for function
DWORD	DumpTotal;	//Total size of dump (dump+relocs+fixups)
DWORD 	DumpSz; //Dump size
DWORD 	FixupNum; //Dump fixups number
BYTE 	Dump[DumpSz]; //Binary dump
BYTE 	Relocs[DumpSize];
FIXUPINFO Fixups[FixupNum]; //Fixups
DWORD	ArgsTotal;	//Total size of arguments
WORD 	ArgsNum; //Arguments number
ARGINFO Args[ArgNum]; //Arguments
DWORD	LocalsTotal;	//Total size of local vars
WORD 	LocalsNum; //Local vars number
LOCALINFO Locals[LocalNum]; //Local vars
*/

#define SCOPE_TMP   32  //Temp struct FIELDINFO, to be deleted
typedef struct FIELDINFO
{
    FIELDINFO():xrefs(0){}
    ~FIELDINFO();
    BYTE 	Scope;      //9-private, 10-protected, 11-public, 12-published
    int 	Offset;     //Offset in class instance
    int 	Case;       //Case for record (in other cases 0xFFFFFFFF)
    String  Name;       //Field Name
    String  Type;       //Field Type
    TList	*xrefs;		//Xrefs from code
} FIELDINFO, *PFIELDINFO;

typedef struct
{
    BYTE 	Scope;      //9-private, 10-protected, 11-public, 12-published
    int 	Index;      //readonly, writeonly в зависимости от установки бит 1 и 2
    int 	DispID;     //???
    String 	Name;       //Имя свойства
    String 	TypeDef;    //Тип свойства
    String 	ReadName;   //Процедура для чтения свойства или соответствующее поле
    String 	WriteName;  //Процедура для записи свойства или соответствующее поле
    String 	StoredName; //Процедура для проверки свойства или соответствующее значение
} PROPINFO, *PPROPINFO;

typedef struct
{
    BYTE 	Scope;      //9-private, 10-protected, 11-public, 12-published
    BYTE 	MethodKind; //'M'-method, 'P'-procedure, 'F'-function, 'C'-constructor, 'D'-destructor
    String 	Prototype;  //Prototype full name
} METHODINFO, *PMETHODINFO;

typedef struct
{
    BYTE 	Tag;        //0x21-"val", 0x22-"var"
    bool 	Register;   //If true - argument is in register, else - in stack
    int 	Ndx;        //Register number and offset (XX-number, XXXXXX-offset) (0-EAX, 1-ECX, 2-EDX)
    int		Size;		//Argument Size
    String 	Name;       //Argument Name
    String 	TypeDef;    //Argument Type
} ARGINFO, *PARGINFO;

typedef struct
{
    int 	Ofs;        //Offset of local var (from ebp or EP)
    int     Size;       //Size of local var
    String 	Name;       //Local var Name
    String 	TypeDef;    //Local var Type
} LOCALINFO, *PLOCALINFO;

typedef struct
{
    char        type;       //'C'-call; 'J'-jmp; 'D'-data
    DWORD       adr;        //address of procedure
    int         offset;     //offset in procedure
} XrefRec, *PXrefRec;


//Флажки для заполнения членов классов
#define INFO_DUMP       1
#define INFO_ARGS       2
#define INFO_LOCALS     4
#define INFO_FIELDS     8
#define INFO_PROPS      16
#define INFO_METHODS    32
#define INFO_ABSNAME    64

class MConstInfo
{
public:
    __fastcall MConstInfo();
    __fastcall ~MConstInfo();
public:
    WORD 	    ModuleID;
    String 	    ConstName;
    BYTE 	    Type;       //'C'-ConstDecl, 'P'-PDecl (VMT), 'V'-VarCDecl
    String 	    TypeDef;    //Тип
    String 	    Value;      //Значение
    DWORD 	    DumpSz;     //Размер бинарного дампа
    DWORD 	    FixupNum;   //Количество фиксапов дампа
    BYTE 	    *Dump;      //Бинарный дамп
};

//Значения байта Kind информации о типе
#define drArrayDef          0x4C    //'L'
#define drClassDef          0x46    //'F'
#define drFileDef           0x4F    //'O'
#define drFloatDef          0x49    //'I'
#define drInterfaceDef      0x54    //'T'
#define drObjVMTDef         0x47    //'G'
#define drProcTypeDef       0x48    //'H'
#define drPtrDef            0x45    //'E'
#define drRangeDef          0x44    //'D'
#define drRecDef            0x4D    //'M'
#define drSetDef            0x4A    //'J'
#define drShortStrDef       0x4B    //'K'
#define drStringDef         0x52    //'R'
#define drTextDef           0x50    //'P'
#define drVariantDef        0x53    //'S'
#define drAliasDef          0x41    //'Z'

class MTypeInfo
{
public:
    __fastcall MTypeInfo();
    __fastcall ~MTypeInfo();
public:
    DWORD       Size;
    WORD        ModuleID;
    String      TypeName;
    BYTE 	    Kind;       //drArrayDef,...,drVariantDef
    WORD 	    VMCnt;      //VMT elements number (from 0)
    String 	    Decl;       //Declaration
    DWORD 	    DumpSz;     //Binary dump size
    DWORD 	    FixupNum;   //Binary dump fixup number
    BYTE 	    *Dump;      //Binary dump
    WORD 	    FieldsNum;  //Fields number (class, interface, record)
    BYTE        *Fields;
    WORD 	    PropsNum;   //Properties number (class, interface)
    BYTE        *Props;
    WORD 	    MethodsNum; //Methods number (class, interface)
    BYTE        *Methods;
};
//Var Type field
#define VT_VAR          'V'
#define VT_ABSVAR       'A'
#define VT_SPECVAR      'S'
#define VT_THREADVAR    'T'

class MVarInfo
{
public:
    __fastcall MVarInfo();
    __fastcall ~MVarInfo();
public:
    WORD 	ModuleID;
    String 	VarName;
    BYTE 	Type; //'V'-Var;'A'-AbsVar;'S'-SpecVar;'T'-ThreadVar
    String 	TypeDef;
    String 	AbsName; //Для ключевого слова absolute
};

class MResStrInfo
{
public:
    __fastcall MResStrInfo();
    __fastcall ~MResStrInfo();
public:
    WORD        ModuleID;
    String      ResStrName;
    String      TypeDef;
    //String      Context;
};

class MProcInfo
{
public:
    __fastcall MProcInfo();
    __fastcall ~MProcInfo();
public:
    WORD        ModuleID;
    String      ProcName;
    bool        Embedded;   //true-содержит вложенные процедуры
    char        DumpType;   //'C' - код, 'D' - данные
    char        MethodKind; //'M'-method,'P'-procedure,'F'-function,'C'-constructor,'D'-destructor
    BYTE 	    CallKind;   //1-'cdecl', 2-'pascal', 3-'stdcall', 4-'safecall'
    int 	    VProc;      //флажок для "overload" (если версия Дельфи > verD3 и VProc&0x1000 != 0)
    String      TypeDef;    //Тип
    DWORD 	    DumpSz;     //Размер бинарного дампа
    DWORD 	    FixupNum;   //Количество фиксапов дампа
    BYTE 	    *Dump;      //Бинарный дамп (включает в себя собственно дамп, релоки и фиксапы)
    WORD 	    ArgsNum;    //Количество аргументов процедуры
    BYTE        *Args;      //Список аргументов
    //WORD 	    LocalsNum;  //Количество локальных переменных процедуры
    //BYTE        *Locals;    //Список локальных переменных
};
//Секции базы знаний
#define     KB_NO_SECTION       0
#define     KB_CONST_SECTION    1
#define     KB_TYPE_SECTION     2
#define     KB_VAR_SECTION      4
#define     KB_RESSTR_SECTION   8
#define     KB_PROC_SECTION     16

class MKnowledgeBase
{
public:
    __fastcall  MKnowledgeBase();
    __fastcall ~MKnowledgeBase();

    bool __fastcall Open(char* filename);
    void __fastcall Close();

    const BYTE* __fastcall GetKBCachePtr(DWORD Offset, DWORD Size);
    WORD __fastcall GetModuleID(char* ModuleName);
    String __fastcall GetModuleName(WORD ModuleID);
    void __fastcall GetModuleIdsByProcName(char* ProcName);
    int __fastcall GetItemSection(WORD* ModuleIDs, char* ItemName);
    int __fastcall GetConstIdx(WORD* ModuleID, char* ConstName);
    int __fastcall GetConstIdxs(char* ConstName, int* ConstIdx);
    int __fastcall GetTypeIdxByModuleIds(WORD* ModuleIDs, char* TypeName);
    int __fastcall GetTypeIdxsByName(char* TypeName, int* TypeIdx);
    int __fastcall GetTypeIdxByUID(char* UID);
    int __fastcall GetVarIdx(WORD* ModuleIDs, char* VarName);
    int __fastcall GetResStrIdx(int from, char* ResStrContext);
    int __fastcall GetResStrIdx(WORD ModuleID, char* ResStrContext);
    int __fastcall GetResStrIdx(WORD* ModuleIDs, char* ResStrName);
    int __fastcall GetProcIdx(WORD ModuleID, char* ProcName);
    int __fastcall GetProcIdx(WORD ModuleID, char* ProcName, BYTE* code);
    int __fastcall GetProcIdx(WORD* ModuleIDs, char* ProcName, BYTE* code);
    bool __fastcall GetProcIdxs(WORD ModuleID, int* FirstProcIdx, int* LastProcIdx);
    bool __fastcall GetProcIdxs(WORD ModuleID, int* FirstProcIdx, int* LastProcIdx, int* DumpSize);
    MConstInfo* __fastcall GetConstInfo(int AConstIdx, DWORD AFlags, MConstInfo* cInfo);
    MProcInfo* __fastcall GetProcInfo(char* ProcName, DWORD AFlags, MProcInfo* pInfo, int* procIdx);
    MProcInfo* __fastcall GetProcInfo(int AProcIdx, DWORD AFlags, MProcInfo* pInfo);
    MTypeInfo* __fastcall GetTypeInfo(int ATypeIdx, DWORD AFlags, MTypeInfo* tInfo);
    MVarInfo* __fastcall GetVarInfo(int AVarIdx, DWORD AFlags, MVarInfo* vInfo);
    MResStrInfo* __fastcall GetResStrInfo(int AResStrIdx, DWORD AFlags, MResStrInfo* rsInfo);
    int __fastcall ScanCode(BYTE* code, DWORD* CodeFlags, DWORD CodeSz, MProcInfo* pInfo);
    WORD* __fastcall GetModuleUses(WORD ModuleID);
    int __fastcall GetProcUses(char* ProcName, WORD* uses);
    WORD* __fastcall GetTypeUses(char* TypeName);
    WORD* __fastcall GetConstUses(char* ConstName);
    String __fastcall GetProcPrototype(int ProcIdx);
    String __fastcall GetProcPrototype(MProcInfo* pInfo);
    bool __fastcall IsUsedProc(int AIdx);
    void __fastcall SetUsedProc(int AIdx);
    bool __fastcall GetKBProcInfo(String typeName, MProcInfo* procInfo, int* procIdx);
    bool __fastcall GetKBTypeInfo(String typeName, MTypeInfo* typeInfo);
    bool __fastcall GetKBPropertyInfo(String className, String propName, MTypeInfo* typeInfo);
    String __fastcall IsPropFunction(String className, String procName);
    DWORD       Version;
    int         ModuleCount;
    OFFSETSINFO *ModuleOffsets;
    WORD        *Mods;
    BYTE        *UsedProcs;
    const OFFSETSINFO *ConstOffsets;
    const OFFSETSINFO *TypeOffsets;
    const OFFSETSINFO *VarOffsets;
    const OFFSETSINFO *ResStrOffsets;
    const OFFSETSINFO *ProcOffsets;

private:
    bool        Inited;
    FILE        *Handle;
    bool __fastcall CheckKBFile();

    long        SectionsOffset;
    //Modules
    int         MaxModuleDataSize;
    //Consts
    int         ConstCount;
    int         MaxConstDataSize;
    //Types
    int         TypeCount;
    int         MaxTypeDataSize;
    //Vars
    int         VarCount;
    int         MaxVarDataSize;
    //ResStr
    int         ResStrCount;
    int         MaxResStrDataSize;
    //Procs
    int         MaxProcDataSize;
    int         ProcCount;

    //as temp test (global KB file cache in mem)
    const BYTE  *KBCache;
    long        SizeKBFile;
    String      NameKBFile;

};
//---------------------------------------------------------------------------
#endif

