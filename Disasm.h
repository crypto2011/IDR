//---------------------------------------------------------------------------
#ifndef DisasmH
#define DisasmH
//---------------------------------------------------------------------------
#define     ASMMAXCOPLEN 12
//Instruction type
#define 	itUnknown   0   //Unknown instruction
#define 	itTransfer  1   //Data transfer instruction
#define 	itArifm     2   //Ariphmetical instruction
#define 	itLogic     3   //Logical instruction
#define 	itControl   4   //Control flow instruction
#define 	itString    5   //String instruction
#define 	itFloat     6   //Coprocessor instruction

typedef struct
{
	char    Mnem[32];
    char    Op1[64];
	//char    Op2[64];
	//char    Op3[64];
    //BYTE    InstrType;
    bool    Float;
	bool	Call;
	bool	Branch;
	bool	Conditional;
    bool    Ret;
    //Register indexes, used as operands
    int     OpRegIdx[3];
    //[BaseReg + IndxReg*Scale + Offset]
    int     BaseReg;
    int     IndxReg;
    int     Scale;
    DWORD   Offset;
    //
    //bool    ImmPresent;
    DWORD   Immediate;
	BYTE	OpSize;//Operand size
    //BYTE    ImmSize;//size of immediate operand
    char	sSize[32];
	int 	RepPrefix;
    int     SegPrefix;
	BYTE	OpNum;
    BYTE    OpType[3];
	//BYTE	Op1Type;
	//BYTE	Op2Type;
	//BYTE	Op3Type;
} DISINFO, *PDISINFO;

#define		otUND   0
#define 	otIMM   1
#define 	otREG	2
#define 	otMEM   3
#define     otFST   4

#define     OP_RESET    0x80
#define     OP_A2       0x40    //2 or 3 operands

#define		OP_UNK		0
#define		OP_ADC      0x81    //1 OP_RESET
#define		OP_ADD		0xC2    //2 OP_RESET OP_A2
#define		OP_AND		0xC3    //3 OP_RESET OP_A2
#define     OP_BT       0x44    //4 OP_A2
#define     OP_BTC      0x45    //5 OP_A2
#define     OP_BTR      0x46    //6 OP_A2
#define     OP_BTS      0x47    //7 OP_A2
#define		OP_CDQ		0x88    //8 OP_RESET
#define		OP_CMP		0x49    //9 OP_A2
#define		OP_DEC		0x8A    //A OP_RESET
#define		OP_DIV      0x8B    //B OP_RESET
#define		OP_IDIV		0xCC    //C OP_RESET OP_A2
#define		OP_IMUL		0xCD    //D OP_RESET OP_A2
#define		OP_INC		0x8E    //E OP_RESET
#define		OP_JMP		0x8F    //F OP_RESET
#define		OP_LEA		0xD0    //10 OP_RESET OP_A2
#define		OP_MOV		0xD1    //11 OP_RESET OP_A2
#define		OP_MOVS		0x92    //12 OP_RESET
#define		OP_MUL      0x93    //13 OP_RESET
#define     OP_NEG      0x94    //14 OP_RESET
#define		OP_OR		0xD5    //15 OP_RESET OP_A2
#define		OP_POP		0x96    //16 OP_RESET
#define		OP_PUSH		0x97    //17 OP_RESET
#define     OP_SAR      0x98    //18 OP_RESET
#define		OP_SBB      0x99    //19 OP_RESET
#define     OP_SET      0x9A    //1A OP_RESET
#define		OP_SUB		0x9B    //1B OP_RESET
#define		OP_TEST		0x5C    //1C OP_A2
#define		OP_XCHG		0xDD    //1D OP_RESET OP_A2
#define		OP_XOR		0xDE    //1E OP_RESET OP_A2
#define     OP_SHR      0x9F    //1F OP_RESET
#define     OP_SAL      0xA0    //20 OP_RESET
#define     OP_SHL      0xA1    //21 OP_RESET
#define     OP_NOT      0xA2    //22 OP_RESET

class   MDisasm
{
public:
    __fastcall MDisasm();
    __fastcall ~MDisasm();
    int __fastcall Init();
    int __fastcall Disassemble(DWORD fromAdr, PDISINFO pDisInfo, char* disLine);
    int __fastcall Disassemble(BYTE* from, __int64 address, PDISINFO pDisInfo, char* disLine);
    int __fastcall GetRegister(char* reg);
    HINSTANCE   hModule;
    BYTE __fastcall GetOp(char* mnem);
    BYTE __fastcall GetCop();
    BYTE __fastcall GetCop1();
    BYTE __fastcall GetPostByte();
    void __fastcall SetPostByte(BYTE b);
    void __fastcall SetOffset(DWORD ofs);
    void __fastcall GetInstrBytes(BYTE* dst);
    char* __fastcall GetSizeString(int size);
private:
    bool __fastcall GetAddressSize();
    bool __fastcall GetOperandSize();
    BYTE __fastcall GetSegPrefix();
    BYTE __fastcall GetRepPrefix();
    BYTE __fastcall GetPostByteMod();
    int __fastcall GetPostByteReg();
    int __fastcall GetPostByteRm();
    int __fastcall GetOpType(char* Op);
    void __fastcall FormatInstr(PDISINFO pDisInfo, char* disLine);
    void __fastcall FormatArg(int argno, DWORD cmd, DWORD arg, PDISINFO pDisInfo, char* disLine);
    int __fastcall OutputGeneralRegister(char *dst, int reg, int size);
    void __fastcall OutputHex(char *dst, DWORD val);
    DWORD __fastcall GetAddress();
    void __fastcall OutputSegPrefix(char* dst, PDISINFO pDisInfo);
    int __fastcall EvaluateOperandSize();
    void __fastcall OutputSizePtr(int size, bool mm, PDISINFO pDisInfo, char* disLine);
    void __fastcall OutputMemAdr16(int argno, char* dst, DWORD arg, bool f1, bool f2, PDISINFO pDisInfo, char* disLine);
    void __fastcall OutputMemAdr32(int argno, char* dst, DWORD arg, bool f1, bool f2, PDISINFO pDisInfo, char* disLine);
};
//---------------------------------------------------------------------------
#endif
