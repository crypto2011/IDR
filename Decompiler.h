//---------------------------------------------------------------------------
#ifndef DecompilerH
#define DecompilerH

#include "Main.h"
//---------------------------------------------------------------------------
//Precedence of operations
#define     PRECEDENCE_ATOM     24
#define     PRECEDENCE_UNARY    16
#define     PRECEDENCE_MULT     15  //*,/,div, mod,and,shl,shr,as
#define     PRECEDENCE_ADD      14  //+,-,or,xor
#define     PRECEDENCE_NOT      6   //@,not
#define     PRECEDENCE_CMP      9   //=,<>,<,>,<=,>=,in,is
#define     PRECEDENCE_NONE     0

#define     TAB_SIZE            2

#define     IF_ARG              1
#define     IF_VAR              2
#define     IF_STACK_PTR        4
#define     IF_CALL_RESULT      8
#define     IF_VMT_ADR          16
#define     IF_CYCLE_VAR        32
#define     IF_FIELD            64
#define     IF_ARRAY_PTR        128
#define     IF_INTVAL           256
#define     IF_INTERFACE        512
#define     IF_EXTERN_VAR       1024    //User for embedded procedures
#define     IF_RECORD_FOFS      2048    //Offset inside record

#define     CF_CONSTRUCTOR      1
#define     CF_DESTRUCTOR       2
#define     CF_FINALLY          4
#define     CF_EXCEPT           8
#define     CF_LOOP             16
#define     CF_BJL              32
#define     CF_ELSE             64

#define     CMP_FAILED          0
#define     CMP_BRANCH          1
#define     CMP_SET             2

//BJL
#define     MAXSEQNUM           1024

#define     BJL_USED            -1
#define     BJL_EMPTY           0
#define	    BJL_BRANCH  		1
#define     BJL_JUMP			2
#define     BJL_LOC				3
#define     BJL_SKIP_BRANCH     4   //branches for IntOver, BoundErr,...

typedef struct
{
    char        state;          //'U' not defined; 'B' branch; 'J' jump; '@' label; 'R' return; 'S' switch
    int         bcnt;           //branches to... count
    DWORD       address;
    String      dExpr;          //condition of direct expression
    String      iExpr;          //condition of inverse expression
    String      result;
} TBJLInfo;

typedef struct
{
	bool		branch;
	bool		loc;
    int			type;
    int			address;
    int			idx;		//IDX in BJLseq
} TBJL;
//BJL

typedef struct
{
    String  L;
    char    O;
    String  R;
} CMPITEM, *PCMPITEM;

typedef struct
{
    BYTE    Precedence;
    int     Size;       //Size in bytes
    int     Offset;     //Offset from beginning of type
    DWORD   IntValue;   //For array element size calculation
    DWORD   Flags;
    String  Value;
    String  Value1;     //For various purposes
    String  Type;
    String  Name;
} ITEM, *PITEM;

typedef struct
{
    String  Value;
    String  Name;
} WHAT, *PWHAT;

#define     itUNK   0
#define     itREG   1
#define     itLVAR  2
#define     itGVAR  3

typedef struct
{
    BYTE    IdxType;
    int     IdxValue;
    String  IdxStr;
} IDXINFO, *PIDXINFO;

class TForInfo
{
public:
    bool    NoVar;
    bool    Down;       //downto (=true)
    int     StopAdr;    //instructions are ignored from this address and to end of cycle
    String  From;
    String  To;
    IDXINFO VarInfo;
    IDXINFO CntInfo;
public:
    __fastcall TForInfo(bool ANoVar, bool ADown, int AStopAdr, String AFrom, String ATo, BYTE AVarType, int AVarIdx, BYTE ACntType, int ACntIdx);
};

typedef  TForInfo *PForInfo;

class TWhileInfo
{
public:
    bool    NoCondition;    //No condition
public:
    __fastcall TWhileInfo(bool ANoCond);
};

typedef TWhileInfo *PWhileInfo;

class TLoopInfo
{
public:
    BYTE        Kind;       //'F'- for; 'W' - while; 'T' - while true; 'R' - repeat
    DWORD       ContAdr;    //Continue address
    DWORD       BreakAdr;   //Break address
    DWORD       LastAdr;    //Last address for decompilation (skip some last instructions)
    PForInfo    forInfo;
    PWhileInfo  whileInfo;
public:
    __fastcall TLoopInfo(BYTE AKind, DWORD AContAdr, DWORD ABreakAdr, DWORD ALastAdr);
    __fastcall ~TLoopInfo();
};

typedef TLoopInfo *PLoopInfo;

//cmpStack Format: "XYYYYYYY^ZZZZ" (== YYYYYY X ZZZZ)
//'A'-JO;'B'-JNO;'C'-JB;'D'-'JNB';'E'-JZ;'F'-JNZ;'G'-JBE;'H'-JA;
//'I'-'JS';'J'-JNS;'K'-JP;'L'-JNP;'M'-JL;'N'-JGE;'O'-JLE;'P'-JG

//Only registers eax, ecx, edx, ebx, esp, ebp, esi, edi 
typedef ITEM REGS[8];

class TNamer
{
public:
    int             MaxIdx;
    TStringList     *Names;
    __fastcall TNamer();
    __fastcall ~TNamer();
    String __fastcall MakeName(String shablon);
};

struct TCaseTreeNode;
struct TCaseTreeNode
{
    TCaseTreeNode   *LNode;
    TCaseTreeNode   *RNode;
    DWORD           ZProc;
    int             FromVal;
    int             ToVal;
};

//structure for saving context of all registers
typedef struct
{
    DWORD   adr;
    REGS    gregs;  //general registers
    REGS    fregs;  //float point registers
    REGS    fregsd; //float point registers (copy)
} DCONTEXT, *PDCONTEXT;

class TDecompiler;

class TDecompileEnv
{
public:
    String      ProcName;       //Name of decompiled procedure
    DWORD       StartAdr;       //Start of decompilation area
    int         Size;           //Size of decompilation area
    int         Indent;         //For output source code
    bool        Alarm;
    bool        BpBased;
    int         LocBase;
    DWORD       StackSize;
    PITEM       Stack;
    DWORD       ErrAdr;
    String      LastResString;
    TStringList *Body;
    ITEM        RegInfo[8];
    ITEM        FStack[8];      //Floating registers stack
    TNamer      *Namer;
    int         BJLnum;
    int         BJLmaxbcnt;
    TList       *SavedContext;
    TList       *BJLseq;//TBJLInfo
    TList       *bjllist;//TBJL
    TList       *CmpStack;
    bool        Embedded;       //Is proc embedded
    TStringList *EmbeddedList;  //List of embedded procedures addresses

    __fastcall TDecompileEnv(DWORD AStartAdr, int ASize, PInfoRec recN);
    __fastcall ~TDecompileEnv();
    String __fastcall GetFieldName(PFIELDINFO fInfo);
    String __fastcall GetArgName(PARGINFO argInfo);
    String __fastcall GetGvarName(DWORD adr);
    String __fastcall GetLvarName(int Ofs, String Type);
    void __fastcall AssignItem(PITEM DstItem, PITEM SrcItem);
    void __fastcall AddToBody(String src);
    void __fastcall AddToBody(TStringList* src);
    bool __fastcall IsExitAtBodyEnd();

    void __fastcall OutputSourceCodeLine(String line);
    void __fastcall OutputSourceCode();
    void __fastcall MakePrototype();
    void __fastcall DecompileProc();
    //BJL
    bool __fastcall GetBJLRange(DWORD fromAdr, DWORD* bodyBegAdr, DWORD* bodyEndAdr, DWORD* jmpAdr, PLoopInfo loopInfo);
    void __fastcall CreateBJLSequence(DWORD fromAdr, DWORD bodyBegAdr, DWORD bodyEndAdr);
    void __fastcall UpdateBJLList();
    void __fastcall BJLAnalyze();
    bool __fastcall BJLGetIdx(int* idx, int from, int num);
    bool __fastcall BJLCheckPattern1(char* t, int from);
    bool __fastcall BJLCheckPattern2(char* t, int from);
    int __fastcall BJLFindLabel(int address, int* no);
    void __fastcall BJLSeqSetStateU(int* idx, int num);
    void __fastcall BJLListSetUsed(int from, int num);
    char __fastcall ExprGetOperation(String s);
    void __fastcall ExprMerge(String& dst, String src, char op);//dst = dst op src, op = '|' or '&'
    String __fastcall PrintBJL();
    PDCONTEXT __fastcall GetContext(DWORD Adr);
    void __fastcall SaveContext(DWORD Adr);
    void __fastcall RestoreContext(DWORD Adr);
};

class TDecompiler
{
public:
    bool            WasRet;     //Was ret instruction
    char            CmpOp;      //Compare operation
    DWORD           CmpAdr;     //Compare dest address
    int             _ESP_;      //Stack pointer
    int             _TOP_;      //Top of FStack
    DISINFO         DisInfo;
    CMPITEM         CmpInfo;
    TDecompileEnv   *Env;
    BYTE            *DeFlags;
    PITEM           Stack;

    __fastcall TDecompiler(TDecompileEnv* AEnv);
    __fastcall ~TDecompiler();
    bool __fastcall CheckPrototype(PInfoRec ARec);
    void __fastcall ClearStop(DWORD Adr);
    DWORD __fastcall Decompile(DWORD fromAdr, DWORD flags, PLoopInfo loopInfo);
    DWORD __fastcall DecompileCaseEnum(DWORD fromAdr, int N, PLoopInfo loopInfo);
    DWORD __fastcall DecompileGeneralCase(DWORD fromAdr, DWORD markAdr, PLoopInfo loopInfo, int N);
    DWORD __fastcall DecompileTry(DWORD fromAdr, DWORD flags, PLoopInfo loopInfo);
    PITEM __fastcall FGet(int idx);
    PITEM __fastcall FPop();
    void __fastcall FPush(PITEM val);
    void __fastcall FSet(int idx, PITEM val);
    void __fastcall FXch(int idx1, int idx2);
    int __fastcall GetArrayFieldOffset(String ATypeName, int AFromOfs, int AScale, String& _name, String& _type);
    int __fastcall GetCmpInfo(DWORD fromAdr);
    String __fastcall GetCycleFrom();
    void __fastcall GetCycleIdx(PIDXINFO IdxInfo, DISINFO* ADisInfo);
    String __fastcall GetCycleTo();
    void __fastcall GetFloatItemFromStack(int Esp, PITEM Dst, int FloatType);
    void __fastcall GetInt64ItemFromStack(int Esp, PITEM Dst);
    String __fastcall GetStringArgument(PITEM item);
    PLoopInfo __fastcall GetLoopInfo(int fromAdr);
    void __fastcall GetMemItem(int CurAdr, PITEM Dst, BYTE Op);
    void __fastcall GetRegItem(int Idx, PITEM Dst);
    String __fastcall GetRegType(int Idx);
    String __fastcall GetSysCallAlias(String AName);
    bool __fastcall Init(DWORD fromAdr);
    void __fastcall InitFlags();
    void __fastcall MarkCaseEnum(DWORD fromAdr);
    void __fastcall MarkGeneralCase(DWORD fromAdr);
    PITEM __fastcall Pop();
    void __fastcall Push(PITEM item);
    void __fastcall SetStackPointers(TDecompiler* ASrc);
    void __fastcall SetDeFlags(BYTE* ASrc);
    void __fastcall SetRegItem(int Idx, PITEM Val);
    void __fastcall SetStop(DWORD Adr);
    bool __fastcall SimulateCall(DWORD curAdr, DWORD callAdr, int instrLen, PMethodRec recM, DWORD AClassAdr);
    void __fastcall SimulateFloatInstruction(DWORD curAdr);
    void __fastcall SimulateFormatCall();
    void __fastcall SimulateInherited(DWORD procAdr);
    void __fastcall SimulateInstr1(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2RegImm(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2RegMem(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2RegReg(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2MemImm(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr2MemReg(DWORD curAdr, BYTE Op);
    void __fastcall SimulateInstr3(DWORD curAdr, BYTE Op);
    void __fastcall SimulatePop(DWORD curAdr);
    void __fastcall SimulatePush(DWORD curAdr, bool bShowComment);
    bool __fastcall SimulateSysCall(String name, DWORD procAdr, int instrLen);
    int __fastcall AnalyzeConditions(int brType, DWORD curAdr, DWORD sAdr, DWORD jAdr, PLoopInfo loopInfo, BOOL bFloat);
};
//---------------------------------------------------------------------------
#endif
