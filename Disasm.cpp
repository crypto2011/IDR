//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

//---------------------------------------------------------------------------
#include <windows.h>
#include <winbase.h>
#include <mem.h>
#include <string.h>
#include <stdio.h>
#include <SyncObjs.hpp>
#include "Disasm.h"
//---------------------------------------------------------------------------
extern  BYTE *Code;
extern  int __fastcall Adr2Pos(DWORD Adr);
extern  TCriticalSection* CrtSection;

DWORD* (__stdcall* PdisNew)(int);
DWORD (_stdcall* CchFormatInstr)(char*, DWORD);
DWORD (_stdcall* Dist)();
DWORD   *DIS;
char*   Reg8Tab[8] =
{
    //0     1     2     3     4     5     6     7
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh"
};
char*   Reg16Tab[8] =
{
    //8     9    10    11    12    13    14    15
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di"
};
char*   Reg32Tab[8] =
{
    //16     17     18     19     20     21     22     23
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"
};
char*   SegRegTab[8] =
{
    //24   25    26    27    28    29    30    31
    "es", "cs", "ss", "ds", "fs", "gs", "??", "??"
};
char*   RegCombTab[8] =
{
    "bx+si", "bx+di", "bp+si", "bp+di", "si", "di", "bp", "bx"
};
char*   RepPrefixTab[4] =
{
    "lock", "repne", "repe", "rep"
};
//---------------------------------------------------------------------------
__fastcall MDisasm::MDisasm()
{
    hModule = 0;
    PdisNew = 0;
    CchFormatInstr = 0;
    Dist = 0;
    DIS = 0;
}
//---------------------------------------------------------------------------
__fastcall MDisasm::~MDisasm()
{
    if (hModule) FreeLibrary(hModule);
    hModule = 0;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::Init()
{
    hModule = LoadLibrary("dis.dll");
    if (!hModule) return 0;
    PdisNew = (DWORD* (__stdcall*)(int))GetProcAddress(hModule, "?PdisNew@DIS@@SGPAV1@W4DIST@1@@Z");
    CchFormatInstr = (DWORD (_stdcall*)(char*, DWORD))GetProcAddress(hModule, "?CchFormatInstr@DIS@@QBEIPADI@Z");
    Dist = (DWORD (_stdcall*)())GetProcAddress(hModule, "?Dist@DIS@@QBE?AW4DIST@1@XZ");
    DIS = PdisNew(1);
    return 1;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetOp(char* mnem)
{
    DWORD   dd = *((DWORD*)mnem);

    if ((dd & 0xFFFFFF) == 'vom')
    {
        if (mnem[3] == 0 || mnem[4] == 'x') return OP_MOV;
        return OP_MOVS;
    }
    if (dd == 'hsup') return OP_PUSH;
    if (dd == 'pop')  return OP_POP;
    if (dd == 'pmj')  return OP_JMP;
    if (dd == 'rox')  return OP_XOR;
    if (dd == 'pmc')  return OP_CMP;
    if (dd == 'tset') return OP_TEST;
    if (dd == 'ael')  return OP_LEA;
    if (dd == 'dda')  return OP_ADD;
    if (dd == 'bus')  return OP_SUB;
    if (dd == 'ro')   return OP_OR;
    if (dd == 'dna')  return OP_AND;
    if (dd == 'cni')  return OP_INC;
    if (dd == 'ced')  return OP_DEC;
    if (dd == 'lum')  return OP_MUL;
    if (dd == 'vid')  return OP_DIV;
    if (dd == 'lumi') return OP_IMUL;
    if (dd == 'vidi') return OP_IDIV;
    if (dd == 'lhs' || dd == 'dlhs')  return OP_SHL;
    if (dd == 'rhs' || dd == 'drhs')  return OP_SHR;
    if (dd == 'las')  return OP_SAL;
    if (dd == 'ras')  return OP_SAR;
    if (dd == 'gen')  return OP_NEG;
    if (dd == 'ton')  return OP_NOT;
    if (dd == 'cda')  return OP_ADC;
    if (dd == 'bbs')  return OP_SBB;
    if (dd == 'qdc')  return OP_CDQ;
    if (dd == 'ghcx') return OP_XCHG;
    if (dd == 'tb')   return OP_BT;
    if (dd == 'ctb')  return OP_BTC;
    if (dd == 'rtb')  return OP_BTR;
    if (dd == 'stb')  return OP_BTS;

    if ((dd & 0xFFFFFF) == 'tes') return OP_SET;

    return OP_UNK;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::GetOpType(char* Op)
{
    char c = Op[0];
    if (c == 0) return otUND;
    if (strchr(Op, '[')) return otMEM;
    if (c >= '0' && c <= '9') return otIMM;
    if (c == 's' && Op[1] == 't') return otFST;
    return otREG;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::GetRegister(char* reg)
{
    for (int n = 0; n < 8; n++)
    {
        if (!stricmp(Reg32Tab[n], reg)) return n;
    }
    return -1;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::Disassemble(DWORD fromAdr, PDISINFO pDisInfo, char* disLine)
{
    int     _res;

    CrtSection->Enter();

    if (Adr2Pos(fromAdr))
        _res = Disassemble(Code + Adr2Pos(fromAdr), (__int64)fromAdr, pDisInfo, disLine);
    else
        _res = 0;

    CrtSection->Leave();
    return _res;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::Disassemble(BYTE* from, __int64 address, PDISINFO pDisInfo, char* disLine)
{
	int	    InstrLen, _res;
	char    *p, *q;
    char    Instr[1024];

    CrtSection->Enter();

    asm
    {
        push    64h
        mov     ecx, [DIS]
        mov     edx, [ecx]
        push    [from]
        push    dword ptr [address + 4]
        push    dword ptr [address]
        call    dword ptr [edx+18h]
        mov     [InstrLen], eax
    }

    //If address of structure DISINFO not given, return only instruction length
    if (pDisInfo)
    {
        if (InstrLen)
        {
            memset(pDisInfo, 0, sizeof(DISINFO));
            pDisInfo->OpRegIdx[0] = -1;
            pDisInfo->OpRegIdx[1] = -1;
            pDisInfo->OpRegIdx[2] = -1;
            pDisInfo->BaseReg = -1;
            pDisInfo->IndxReg = -1;
            pDisInfo->RepPrefix = -1;
            pDisInfo->SegPrefix = -1;
            /*
            asm
            {
                push    400h
                lea     eax, [Instr]
                push    eax
                mov     ecx, [DIS]
                call    CchFormatInstr
            }
            */
            FormatInstr(pDisInfo, disLine);
            if (pDisInfo->IndxReg != -1 && !pDisInfo->Scale) pDisInfo->Scale = 1;

            DWORD   dd = *((DWORD*)pDisInfo->Mnem);

            if (pDisInfo->Mnem[0] == 'f' || dd == 'tiaw')
            {
                pDisInfo->Float = true;
            }
            else if (pDisInfo->Mnem[0] == 'j')
            {
                pDisInfo->Branch = true;
                if (pDisInfo->Mnem[1] != 'm') pDisInfo->Conditional = true;
            }
            else if (dd == 'llac')
            {
                pDisInfo->Call = true;
            }
            else if (dd == 'ter')
            {
                pDisInfo->Ret = true;
            }
            _res = InstrLen;
        }
        else
        {
            _res = 0;
        }
    }
    else
    {
        _res = InstrLen;
    }

    CrtSection->Leave();
	return _res;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::FormatInstr(PDISINFO pDisInfo, char* disLine)
{
    BYTE    _repPrefix, p, *OpName, *ArgInfo;
    int     i, Bytes = 0;
    DWORD   Cmd, Arg;

    if (disLine) *disLine = 0;
    _repPrefix = GetRepPrefix();
    if (_repPrefix)
    {
        switch (_repPrefix)
        {
        case 0xF0:
            if (disLine) strcat(disLine, "lock ");
            pDisInfo->RepPrefix = 0;
            Bytes = 5;
            break;
        case 0xF2:
            if (disLine) strcat(disLine, "repne ");
            pDisInfo->RepPrefix = 1;
            Bytes = 6;
            break;
        case 0xF3:
            if ((GetCop() & 0xF6) == 0xA6)
            {
                if (disLine) strcat(disLine, "repe ");
                pDisInfo->RepPrefix = 2;
                Bytes = 5;
            }
            else
            {
                if (disLine) strcat(disLine, "rep ");
                pDisInfo->RepPrefix = 3;
                Bytes = 4;
            }
            break;
        }
    }

    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+4Ch]
        mov     eax, [edx]
        mov     [OpName], eax
    }

    if (!GetOperandSize())
    {
        switch (GetCop())
        {
        case 0x60:
            OpName = "pusha";
            break;
        case 0x61:
            OpName = "popa";
            break;
        case 0x98:
            OpName = "cbw";
            break;
        case 0x99:
            OpName = "cwd";
            break;
        case 0x9C:
            OpName = "pushf";
            break;
        case 0x9D:
            OpName = "popf";
            break;
        case 0xCF:
            OpName = "iret";
            break;
        }
    }

    if (!GetAddressSize() && GetCop() == 0xE3) OpName = "jcxz";

    if (disLine) strcat(disLine, OpName);
    strcpy(pDisInfo->Mnem, OpName);
    Bytes += strlen(OpName);

    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+4Ch]
        mov     eax, [edx+4]
        add     eax, 9
        mov     [ArgInfo], eax
    }

    for (i = 0; i < 3; i++)
    {
        if (!*ArgInfo) break;
        if (disLine)
        {
            if (!i)
                for (; Bytes < ASMMAXCOPLEN; Bytes++) strcat(disLine, " ");
            else
                strcat(disLine, ",");
        }
        asm
        {
            mov     ecx, [ArgInfo]
            xor     edx, edx
            xor     eax, eax
            mov     dx, [ecx+1]
            mov     [Arg], edx
            mov     al, [ecx]
            mov     [Cmd], eax
            add     ecx, 4
            mov     [ArgInfo], ecx
        }

        FormatArg(i, Cmd, Arg, pDisInfo, disLine);
        pDisInfo->OpNum++;
    }
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::OutputGeneralRegister(char *dst, int reg, int size)
{
BYTE        OperandSize;

    if (size == 1)
    {
        strcat(dst, Reg8Tab[reg]);
        return 0;
    }

    if (size == 2)
    {
        strcat(dst, Reg16Tab[reg]);
        return 8;
    }

    OperandSize = GetOperandSize();
    if (size != 4 && !OperandSize)
    {
        strcat(dst, Reg16Tab[reg]);
        return 8;
    }

    strcat(dst, Reg32Tab[reg]);
    return 16;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::OutputHex(char *dst, DWORD val)
{
    BYTE    b;
    char    buf[12];

    if (val <= 9)
    {
        sprintf(dst + strlen(dst), "%ld", val);
        return;
    }
    sprintf(buf, "%lX", val);
    b = buf[0];
    if (!isdigit(b)) strcat(dst, "0");
    strcat(dst, buf);
}
//---------------------------------------------------------------------------
DWORD   __fastcall MDisasm::GetAddress()
{
int         n;
DWORD       res = 0;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+68h]
        mov     [n], eax
    }

    switch (n)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 7:
    case 8:
    case 9:
    case 0x11:
        asm
        {
            xor     eax, eax
            mov     dword ptr [res], eax
        }
        break;
    case 4:
    case 0xA:
    case 0xC:
    case 0xD:
        asm
        {
            mov     ecx, [DIS]
            mov     eax, [ecx+64h]
            movsx   eax, byte ptr [eax+ecx+3Ch]
            mov     edi, [ecx+38h]
            mov     ebx, [ecx+28h]
            cdq
            xor     esi, esi
            add     edi, eax
            mov     al, [ecx+51h]
            push    ebp
            mov     ebp, [ecx+2Ch]
            adc     esi, edx
            add     edi, ebx
            adc     esi, ebp
            pop     ebp
            test    al, al
            jnz     @GA1
            and     edi, 0FFFFh
            and     esi, 0
        @GA1:
            mov     eax, [ecx+8]
            test    eax, eax
            jnz     @GA2
            and     ebx, 0FFFF0000h
            and     edi, 0FFFFh
            or      ebx, edi
            xor     esi, esi
            xor     ecx, ecx
            mov     edi, ebx
            or      esi, ecx
        @GA2:
            mov     eax, edi
            mov     edx, esi
            mov     dword ptr [res], eax
        }
        break;
    case 5:
    case 0xB:
    case 0xE:
    case 0xF:
        asm
        {
            mov     al, [ecx+51h]
            test    al, al
            jz      @GA3
            mov     edx, [ecx+64h]
            mov     eax, [edx+ecx+3Ch]
            jmp     @GA4
        @GA3:
            mov     eax, [ecx+64h]
            movsx   eax, word ptr [eax+ecx+3Ch]
        @GA4:
            mov     edi, [ecx+38h]
            mov     ebx, [ecx+28h]
            cdq
            xor     esi, esi
            add     edi, eax
            mov     al, [ecx+51h]
            push    ebp
            mov     ebp, [ecx+2Ch]
            adc     esi, edx
            add     edi, ebx
            adc     esi, ebp
            pop     ebp
            test    al, al
            jnz     @GA5
            and     edi, 0FFFFh
            and     esi, 0
        @GA5:
            mov     eax, [ecx+8]
            test    eax, eax
            jnz     @GA6
            and     ebx, 0FFFF0000h
            and     edi, 0FFFFh
            or      ebx, edi
            xor     esi, esi
            xor     ecx, ecx
            mov     edi, ebx
            or      esi, ecx
        @GA6:
            mov     eax, edi
            mov     edx, esi
            mov     dword ptr [res], eax
        }
        break;
    case 6:
    case 0x10:
        asm
        {
            mov     edx, [ecx+64h]
            mov     eax, [edx+ecx+3Ch]
            mov     dword ptr [res], eax
        }
        break;
    }
    return res;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::OutputSegPrefix(char* dst, PDISINFO pDisInfo)
{
    BYTE    _segPrefix;
    char    *sptr = NULL;

    _segPrefix = GetSegPrefix();
    switch (_segPrefix)
    {
    case 0x26:
        sptr = "es:";
        pDisInfo->SegPrefix = 0;
        break;
    case 0x2E:
        sptr = "cs:";
        pDisInfo->SegPrefix = 1;
        break;
    case 0x36:
        sptr = "ss:";
        pDisInfo->SegPrefix = 2;
        break;
    case 0x3E:
        sptr = "ds:";
        pDisInfo->SegPrefix = 3;
        break;
    case 0x64:
        sptr = "fs:";
        pDisInfo->SegPrefix = 4;
        break;
    case 0x65:
        sptr = "gs:";
        pDisInfo->SegPrefix = 5;
        break;
    }

    if (sptr) strcat(dst, sptr);
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::EvaluateOperandSize()
{
    BYTE    OperandSize;
    DWORD   Ofs;
    int     OpSize;

    OperandSize = GetOperandSize();
    asm
    {
        mov     ecx, [DIS]
        mov     ecx, [ecx+4Ch]
        mov     ecx, [ecx+4]
        mov     [Ofs], ecx
    }

    OpSize = (!OperandSize) ? 2: 4;
    if (Ofs == 0x1041BB30 ||    //INVLPG, PREFETCH, PREFETCHW
        Ofs == 0x1041C370)      //LEA
        return 0;
    if (Ofs == 0x1041BC38)      //BOUND
        return 2*OpSize;
    if (Ofs == 0x1041BCB0 ||    //CALL, JMP
        Ofs == 0x1041C3E8)      //LES, LDS, LSS, LFS, LGS
        return OpSize + 2;
    if (Ofs == 0x1041BC98 ||    //FLDENV
        Ofs == 0x1041C460)      //FNSTENV
        return (!OperandSize) ? 14: 28;
    if (Ofs == 0x1041BCF8 ||    //FRSTOR
        Ofs == 0x1041C4C0)      //FNSAVE
        return (!OperandSize) ? 94: 108;
    return OpSize;
}
//---------------------------------------------------------------------------
char* __fastcall MDisasm::GetSizeString(int size)
{
    if (size == 1) return "byte";
    if (size == 2) return "word";
    if (size == 4) return "dword";
    if (size == 6) return "fword";
    if (size == 8) return "qword";
    if (size == 10) return "tbyte";
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::OutputSizePtr(int size, bool mm, PDISINFO pDisInfo, char* disLine)
{
    char*   sptr = NULL;

    if (!size) size = EvaluateOperandSize();
    switch (size)
    {
        case 1:
            sptr = "byte";
            break;
        case 2:
            sptr = "word";
            break;
        case 4:
            sptr = "dword";
            break;
        case 6:
            sptr = "fword";
            break;
        case 8:
            if (mm)
                sptr = "mmword";
            else
                sptr = "qword";
            break;
        case 10:
            sptr = "tbyte";
            break;
        case 16:
            if (mm) sptr = "xmmword";
            break;
    }
    if (sptr)
    {
        if (disLine)
        {
            strcat(disLine, sptr);
            strcat(disLine, " ptr ");
        }
        pDisInfo->OpSize = size;
        strcpy(pDisInfo->sSize, sptr);
    }
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::OutputMemAdr32(int argno, char* dst, DWORD arg, bool f1, bool f2, PDISINFO pDisInfo, char* disLine)
{
    BYTE    PostByte, SegPrefix, mod, *pos, sib, b;
    bool    ofs, ofs1, ib, mm;
    int     ss, index, base, idxofs, idxval;
    DWORD   offset32;

    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+5Ch]
        mov     al, [edx+ecx+3Ch]
        mov     [PostByte], al
    }
    mod = PostByte & 0xC0;
    if (mod == 0xC0)
    {
        if (!f1 && !f2)
        {
            idxval = PostByte & 7;
            idxofs = OutputGeneralRegister(dst, idxval, arg);
            pDisInfo->OpRegIdx[argno] = idxofs + idxval;
            return;
        }
        if (f2) strcat(dst, "x");
        sprintf(dst + strlen(dst), "mm%d", PostByte & 7);
        return;
    }
    ofs = false;
    index = -1;

    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+5Ch]
        lea     eax, [edx+ecx+3Dh]
        mov     [pos], eax
    }

    base = PostByte & 7;

    if (base != 4)
    {
        if ((PostByte & 0xC7) == 5) //mod=00;r/m=101
        {
            ofs = true;
            base = -1;
        }
    }
    else    //sib
    {
        sib = *pos++;
        if ((sib & 7) == 5 && !mod)
            base = - 1;
        else
            base = sib & 7;
        index = (sib >> 3) & 7;
        if (index != 4)
            ss = 1 << (sib >> 6);
        else
            index = -1;
        if ((sib & 7) == 5 && !mod)
            ofs = true;
    }
    
    offset32 = 0;
    if ((PostByte & 0xC0) == 0x40)  //mod=01
    {
        b = *pos;
        offset32 = b;
        if ((b & 0x80) != 0)
            offset32 |= 0xFFFFFF00;
    }
    else if ((PostByte & 0xC0) == 0x80) //mod=10
        ofs = true;

    if (ofs) offset32 = *((DWORD*)pos);

    mm = (f1 || f2);
    OutputSizePtr(arg, mm, pDisInfo, disLine);
    OutputSegPrefix(dst, pDisInfo);
    ib = (base != -1 || index != -1);

    if (!GetSegPrefix() && !ib)
    {
        pDisInfo->SegPrefix = 3;
        strcat(dst, "ds:");
    }
    strcat(dst, "[");
    ofs1 = (offset32 != 0);

    if (ib)
    {
        if (base != -1)
        {
            strcat(dst, Reg32Tab[base]);
            pDisInfo->BaseReg = base + 16;
        }
        if (index != -1)
        {
            if (base != -1) strcat(dst, "+");
            strcat(dst, Reg32Tab[index]);
            pDisInfo->IndxReg = index + 16;
            if (ss != 1)
            {
                sprintf(dst + strlen(dst), "*%d", ss);
                pDisInfo->Scale = ss;
            }
        }
    }
    else
        ofs1 = true;

    if (ofs)
    {
        pDisInfo->Offset = offset32;
        if (ib)
        {
            if ((int)offset32 < 0)
            {
                strcat(dst, "-");
                offset32 = -(int)offset32;
            }
            else
                strcat(dst, "+");
        }
        OutputHex(dst, offset32);
        strcat(dst, "]");
        return;
    }

    if (ofs1)
    {
        pDisInfo->Offset = offset32;
        if (ib)
        {
            if ((int)offset32 < 0)
            {
                strcat(dst, "-");
                offset32 = -(int)offset32;
            }
            else
                strcat(dst, "+");
        }
        OutputHex(dst, offset32);
    }
    strcat(dst, "]");
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::OutputMemAdr16(int argno, char* dst, DWORD arg, bool f1, bool f2, PDISINFO pDisInfo, char* disLine)
{
    BYTE    PostByte, SegPrefix, b;
    bool    ofs, mm;
    char    *regcomb, sign;
    int     idxofs, idxval;
    DWORD   offset16, dval;

    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+5Ch]
        mov     al, [edx+ecx+3Ch]
        mov     [PostByte], al
    }
    if ((PostByte & 0xC0) == 0xC0)  //mod=11
    {
        if (!f1 && !f2)
        {
            idxval = PostByte & 7;
            idxofs = OutputGeneralRegister(dst, idxval, arg);
            pDisInfo->OpRegIdx[argno] = idxofs + idxval;
            return;
        }
        if (f2) strcat(dst, "x");
        sprintf(dst + strlen(dst), "mm%d", PostByte & 7);
        return;
    }

    ofs = false;
    regcomb = 0;

    if ((PostByte & 0xC7) == 6) //mod=00;r/m=110
        ofs = true;
    else
    {
        b = PostByte & 7;
        regcomb = RegCombTab[b];
        if (b == 0 || b == 1 || b == 7)
            pDisInfo->BaseReg = 11;
        if (b == 2 || b == 3 || b == 6)
            pDisInfo->BaseReg = 13;
        if (b == 0 || b == 2 || b == 4)
            pDisInfo->IndxReg = 14;
        if (b == 1 || b == 3 || b == 5)
            pDisInfo->IndxReg = 15;
    }
    sign = 0;

    if ((PostByte & 0xC0) == 0x40)  //mod=01
    {
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+5Ch]
            mov     al, [edx+ecx+3Dh]
            test    al, 80h
            mov     byte ptr [offset16], al
            jnz     @OM16_1
            mov     eax, [offset16]
            mov     [sign], '+'
            jmp     @OM16_2
        @OM16_1:
            mov     [sign], '-'
            neg     eax
        @OM16_2:
            and     eax, 0FFh
            mov     [offset16], eax
        }
    }
    else if ((PostByte & 0xC0) == 0x80) //mod=10
    {
        ofs = true;
    }

    mm = (f1 || f2);
    OutputSizePtr(arg, mm, pDisInfo, disLine);
    OutputSegPrefix(dst, pDisInfo);

    if (!GetSegPrefix() && !regcomb)
    {
        strcat(dst, "ds:");
        pDisInfo->SegPrefix = 3;
    }
    strcat(dst, "[");
    if (regcomb) strcat(dst, regcomb);

    if (ofs)
    {
        if (regcomb) strcat(dst, "+");
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+5Ch]
            xor     eax, eax
            mov     ax, [ecx+edx+3Dh]
            mov     [dval], eax
        }
        pDisInfo->Offset = dval;
        sprintf(dst + strlen(dst), "%04lX]", dval);
        return;
    }
    if (sign)
    {
        pDisInfo->Offset = offset16;
        sprintf(dst + strlen(dst), "%c", sign);
        OutputHex(dst, offset16);
        strcat(dst, "]");
    }
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::FormatArg(int argno, DWORD cmd, DWORD arg, PDISINFO pDisInfo, char* disLine)
{
    BYTE    AddressSize, OperandSize;
    DWORD   dval, adr;
    int     ival, idxofs, idxval, stno;
    char    Op[64], *p = Op;

    *p = 0;

    switch (cmd)
    {
    //segment:offset
    case 1:
        OperandSize = GetOperandSize();
        if (OperandSize)
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                xor     eax, eax
                mov     ax, [edx+ecx+40h]
                mov     [dval], eax
            }
            p += sprintf(p, "%04lX:", dval);
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                mov     eax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
            sprintf(p, "%08lX", dval);
        }
        else
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                xor     eax, eax
                mov     ax, [edx+ecx+3Eh]
                mov     [dval], eax
            }
            p += sprintf(p, "%04lX:", dval);
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                xor     eax, eax
                mov     ax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
            sprintf(p, "%04lX", dval);
        }
        break;
    //cr#
    case 2:
        sprintf(Op, "cr%d", GetPostByteReg());
        break;
    //Integer value (sar, shr)
    case 3:
        sprintf(Op, "%d", arg);
        pDisInfo->Immediate = arg;
        //pDisInfo->ImmPresent = true;
        break;
    //dr#
    case 4:
        sprintf(Op, "dr%d", GetPostByteReg());
        break;
    //General register
    case 5:
        idxval = GetCop() & 7;
        idxofs = OutputGeneralRegister(Op, idxval, arg);
        pDisInfo->OpRegIdx[argno] = idxofs + idxval;
        break;
    //General register
    case 6:
        idxval = GetCop1() & 7;
        idxofs = OutputGeneralRegister(Op, idxval, arg);
        pDisInfo->OpRegIdx[argno] = idxofs + idxval;
        break;
    //Immediate byte
    case 7:
        if (GetCop() == 0x83)
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                movsx   eax, byte ptr [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        else
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                movzx   eax, byte ptr [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        pDisInfo->Immediate = dval;
        //pDisInfo->ImmPresent = true;
        //pDisInfo->ImmSize = 1;
        OutputHex(Op, dval);
        break;
    //Immediate byte
    case 8:
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+64h]
            xor     eax, eax
            mov     al, [edx+ecx+3Eh]
            mov     [dval], eax
        }
        pDisInfo->Immediate = dval;
        //pDisInfo->ImmPresent = true;
        //pDisInfo->ImmSize = 1;
        OutputHex(Op, dval);
        break;
    //Immediate dword
    case 9:
        OperandSize = GetOperandSize();
        if (OperandSize)
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                mov     eax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        else
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                xor     eax, eax
                mov     ax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        pDisInfo->Immediate = dval;
        //pDisInfo->ImmPresent = true;
        //pDisInfo->ImmSize = 4;
        OutputHex(Op, dval);
        break;
    //Immediate word (ret)
    case 0xA:
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+64h]
            xor     eax, eax
            mov     ax, [edx+ecx+3Ch]
            mov     [dval], eax
        }
        pDisInfo->Immediate = dval;
        //pDisInfo->ImmPresent = true;
        //pDisInfo->ImmSize = 2;
        OutputHex(Op, dval);
        break;
    //Address (jmp, jcond, call)
    case 0xB:
    case 0xC:
        adr = GetAddress();
        pDisInfo->Immediate = adr;
        //pDisInfo->ImmPresent = true;
        sprintf(Op, "%08lX", adr);
        break;
    //Memory
    case 0xD:
    case 0xF:
    case 0x1B:
        AddressSize = GetAddressSize();
        if (AddressSize)
            OutputMemAdr32(argno, Op, arg, (cmd == 0xD), (cmd == 0x1B), pDisInfo, disLine);
        else
            OutputMemAdr16(argno, Op, arg, (cmd == 0xD), (cmd == 0x1B), pDisInfo, disLine);
        break;
    //mm#
    case 0xE:
        sprintf(Op, "mm%d", GetPostByteReg());
        break;
    //General register
    case 0x10:
        idxval = GetPostByteReg();
        idxofs = OutputGeneralRegister(Op, idxval, arg);
        pDisInfo->OpRegIdx[argno] = idxofs + idxval;
        break;
    //Segment register
    case 0x11:
        idxval = GetPostByteReg();
        pDisInfo->OpRegIdx[argno] = idxval + 24;
        sprintf(Op, "%s", SegRegTab[idxval]);
        break;
    //sreg:memory
    case 0x12:
        OutputSegPrefix(Op, pDisInfo);
        AddressSize = GetAddressSize();
        if (AddressSize)
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                mov     eax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        else
        {
            asm
            {
                mov     ecx, [DIS]
                mov     edx, [ecx+64h]
                xor     eax, eax
                mov     ax, [edx+ecx+3Ch]
                mov     [dval], eax
            }
        }
        sprintf(Op + strlen(Op), "[%08lX]", dval);
        pDisInfo->Offset = dval;    //!
        break;
    //8-bit register
    case 0x13:
        strcpy(Op, Reg8Tab[arg]);
        pDisInfo->OpRegIdx[argno] = arg;
        break;
    //General register
    case 0x14:
        idxval = arg;
        idxofs = OutputGeneralRegister(Op, idxval, 0);
        pDisInfo->OpRegIdx[argno] = idxofs + idxval;
        break;
    //8-bit register
    case 0x15:
        strcpy(Op, Reg8Tab[arg]);
        pDisInfo->OpRegIdx[argno] = arg;
        break;
    //st
    case 0x16:
        strcpy(Op, "st");
        pDisInfo->OpRegIdx[argno] = 30;
        break;
    //st(#)
    case 0x17:
        stno = GetPostByteRm();
        sprintf(Op, "st(%d)", stno);
        pDisInfo->OpRegIdx[argno] = stno + 30;
        break;
    //Segment register
    case 0x18:
        strcpy(Op, SegRegTab[arg]);
        pDisInfo->OpRegIdx[argno] = arg + 24;
        break;
    //tr#
    case 0x19:
        sprintf(Op, "tr%d", GetPostByteReg());
        break;
    //[esi] or [si]
    case 0x1A:
        OutputSizePtr(arg, false, pDisInfo, disLine);
        OutputSegPrefix(Op, pDisInfo);
        AddressSize = GetAddressSize();
        if (AddressSize)
        {
            pDisInfo->BaseReg = 22;
            sprintf(Op + strlen(Op), "[%s]", "esi");
        }
        else
        {
            pDisInfo->BaseReg = 14;
            strcat(Op, "[si]");
        }
        break;
    //xmm#
    case 0x1C:
        sprintf(Op, "xmm%d", GetPostByteReg());
        break;
    //[edi] or es:[di]
    case 0x1D:
        OutputSizePtr(arg, false, pDisInfo, disLine);
        AddressSize = GetAddressSize();
        if (AddressSize)
        {
            pDisInfo->BaseReg = 23;
            sprintf(Op, "[%s]", "edi");
        }
        else
        {
            pDisInfo->SegPrefix = 0;
            pDisInfo->BaseReg = 15;
            strcpy(Op, "es:[di]");
        }
        break;
    //[ebx] or [bx]
    case 0x1E:
        OutputSizePtr(1, false, pDisInfo, disLine);
        OutputSegPrefix(Op, pDisInfo);
        AddressSize = GetAddressSize();
        if (AddressSize)
        {
            pDisInfo->BaseReg = 19;
            sprintf(Op + strlen(Op), "[%s]", "ebx");
        }
        else
        {
            pDisInfo->BaseReg = 11;
            strcat(Op, "[bx]");
        }
        break;
    }

    if (argno == 0)
    {
        strcpy(pDisInfo->Op1, Op);
        pDisInfo->OpType[0] = GetOpType(Op);
    }
    else if (argno == 1)
    {
        //strcpy(pDisInfo->Op2, Op);
        pDisInfo->OpType[1] = GetOpType(Op);
    }
    else
    {
        //strcpy(pDisInfo->Op3, Op);
        pDisInfo->OpType[2] = GetOpType(Op);
    }

    if (disLine) strcat(disLine, Op);
}
//---------------------------------------------------------------------------
bool __fastcall MDisasm::GetAddressSize()
{
bool        res;

    asm
    {
        mov     ecx, [DIS]
        mov     al, [ecx+50h]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
bool __fastcall MDisasm::GetOperandSize()
{
bool        res;

    asm
    {
        mov     ecx, [DIS]
        mov     al, [ecx+51h]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetSegPrefix()
{
BYTE        res;

    asm
    {
        mov     ecx, [DIS]
        mov     al, [ecx+52h]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetRepPrefix()
{
BYTE        res;

    asm
    {
        mov     ecx, [DIS]
        mov     al, [ecx+53h]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetCop()
{
BYTE        res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+58h]
        mov     al, [eax+ecx+3Ch]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetCop1()
{
BYTE        res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+58h]
        mov     al, [eax+ecx+3Dh]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetPostByte()
{
BYTE         res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+5Ch]
        mov     al, [eax+ecx+3Ch]
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::SetPostByte(BYTE b)
{
    asm
    {
        mov     ecx, [DIS]
        mov     edx, [ecx+5Ch]
        mov     al, [b]
        mov     [ecx+edx+3Ch], al
    }
}
//---------------------------------------------------------------------------
BYTE __fastcall MDisasm::GetPostByteMod()
{
BYTE         res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+5Ch]
        mov     al, [eax+ecx+3Ch]
        and     al, 0C0h
        mov     [res], al
    }
    return res;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::GetPostByteReg()
{
int         res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+5Ch]
        mov     al, [eax+ecx+3Ch]
        shr     eax, 3
        and     eax, 7
        mov     [res], eax
    }
    return res;
}
//---------------------------------------------------------------------------
int __fastcall MDisasm::GetPostByteRm()
{
int         res;

    asm
    {
        mov     ecx, [DIS]
        mov     eax, [ecx+5Ch]
        mov     al, [eax+ecx+3Ch]
        and     eax, 7
        mov     [res], eax
    }
    return res;
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::SetOffset(DWORD ofs)
{
    BYTE AddressSize = GetAddressSize();
    if (AddressSize)
    {
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+64h]
            mov     eax, [ofs]
            mov     [edx+ecx+3Ch], eax
        }
    }
    else
    {
        asm
        {
            mov     ecx, [DIS]
            mov     edx, [ecx+64h]
            mov     eax, [ofs]
            mov     [edx+ecx+3Ch], ax
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall MDisasm::GetInstrBytes(BYTE* dst)
{
    asm
    {
        mov     ecx, [DIS]
        lea     esi, [ecx+3Ch]
        mov     edi, [dst]
        mov     ecx, [ecx+38h]
        rep     movsb
    }
}
//---------------------------------------------------------------------------
