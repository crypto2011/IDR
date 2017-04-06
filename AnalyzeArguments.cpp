//---------------------------------------------------------------------------
String __fastcall TFMain_11011981::AnalyzeArguments(DWORD fromAdr)
{
	BYTE		op;
    bool		kb;
    bool        bpBased;
    bool        emb, lastemb;
    bool        argA, argD, argC;
    bool        inA, inD, inC;
    bool		spRestored = false;
    WORD        bpBase, retBytes;
    int         num, instrLen, instrLen1, instrLen2, _procSize;
    int			firstPopRegIdx = -1, popBytes = 0, procStackSize = 0;
    int         fromPos, curPos, Pos, reg1Idx, reg2Idx, sSize;
    DWORD       b, curAdr, Adr, Adr1;
    DWORD       lastAdr = 0, lastCallAdr = 0, lastMovAdr = 0, lastMovImm = 0;
    PInfoRec    recN, recN1;
    PARGINFO    argInfo;
    DWORD		stack[256];
    int			sp = -1;
    int			macroFrom, macroTo;
    String 		minClassName, className = "", retType = "";
    DISINFO     DisInfo, DisInfo1;

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return "";
    if (IsFlagSet(cfEmbedded, fromPos)) return "";
    if (IsFlagSet(cfExport, fromPos)) return "";
    
    recN = GetInfoRec(fromAdr);
    if (!recN || !recN->procInfo) return "";

    //If cfPass is set exit
    if (IsFlagSet(cfPass, fromPos)) return recN->type;

    //Proc start
    SetFlag(cfProcStart | cfPass, fromPos);

    //Skip Imports
    if (IsFlagSet(cfImport, fromPos)) return recN->type;

    kb = (recN->procInfo->flags & PF_KBPROTO);
    bpBased = (recN->procInfo->flags & PF_BPBASED);
    emb = (recN->procInfo->flags & PF_EMBED);
    bpBase = recN->procInfo->bpBase;
    retBytes = recN->procInfo->retBytes;

    //If constructor or destructor get class name
    if (recN->kind == ikConstructor || recN->kind == ikDestructor)
        className = ExtractClassName(recN->GetName());
    //If ClassName not given and proc is dynamic, try to find minimal class
    if (className == "" && (recN->procInfo->flags & PF_DYNAMIC) && recN->xrefs)
    {
        minClassName = "";
        for (int n = 0; n < recN->xrefs->Count; n++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[n];
            if (recX->type == 'D')
            {
                className = GetClsName(recX->adr);
                if (minClassName == "" || !IsInheritsByClassName(className, minClassName)) minClassName = className;
            }
        }
        if (minClassName != "") className = minClassName;
    }
    //If ClassName not given and proc is virtual, try to find minimal class
    if (className == "" && (recN->procInfo->flags & PF_VIRTUAL) && recN->xrefs)
    {
        minClassName = "";
        for (int n = 0; n < recN->xrefs->Count; n++)
        {
            PXrefRec recX = (PXrefRec)recN->xrefs->Items[n];
            if (recX->type == 'D')
            {
                className = GetClsName(recX->adr);
                if (minClassName == "" || !IsInheritsByClassName(className, minClassName)) minClassName = className;
            }
        }
        if (minClassName != "") className = minClassName;
    }

    argA = argD = argC = true;  //On entry
    inA = inD = inC = false;    //No arguments

    _procSize = GetProcSize(fromAdr);
    curPos = fromPos; curAdr = fromAdr;

    while (1)
    {
        if (curAdr >= CodeBase + TotalSize) break;
        //Skip exception table
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            curPos += 4 + 8*num; curAdr += 4 + 8*num;
            continue;
        }

        BYTE b1 = Code[curPos];
        BYTE b2 = Code[curPos + 1];
        if (!b1 && !b2 && !lastAdr) break;

        instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo, 0);
        //if (!instrLen) break;
        if (!instrLen)
        {
            curPos++; curAdr++;
            continue;
        }

        op = Disasm.GetOp(DisInfo.Mnem);
        //Code
        SetFlags(cfCode, curPos, instrLen);
        //Instruction begin
        SetFlag(cfInstruction, curPos);

        if (curAdr >= lastAdr) lastAdr = 0;

        if (op == OP_JMP)
        {
            if (curAdr == fromAdr) break;
            if (DisInfo.OpType[0] == otMEM)
            {
                if (Adr2Pos(DisInfo.Offset) < 0 && (!lastAdr || curAdr == lastAdr)) break;
            }
            if (DisInfo.OpType[0] == otIMM)
            {
                Adr = DisInfo.Immediate;
                if (Adr2Pos(Adr) < 0 && (!lastAdr || curAdr == lastAdr)) break;
                if (GetSegmentNo(Adr) != 0 && GetSegmentNo(fromAdr) != GetSegmentNo(Adr) && (!lastAdr || curAdr == lastAdr)) break;
                if (Adr < fromAdr && (!lastAdr || curAdr == lastAdr)) break;
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }

        if (DisInfo.Ret)
        {
            //End of proc
            if (!lastAdr || curAdr == lastAdr)
            {
                //Get last instruction
                curPos -= instrLen;
                if ((DisInfo.Ret && !IsFlagSet(cfSkip, curPos)) ||   //ret not in SEH
                    IsFlagSet(cfCall, curPos))                       //@Halt0
                {
                    if (IsFlagSet(cfCall, curPos)) spRestored = true;  //acts like mov esp, ebp

                    sp = -1;
                    SetFlags(cfFrame, curPos, instrLen);
                    //ret - stop analyze output regs
                    lastCallAdr = 0; firstPopRegIdx = -1; popBytes = 0;
                    //define all pop registers (ret skipped)
                    for (Pos = curPos - 1; Pos >= fromPos; Pos--)
                    {
                        b = Flags[Pos];
                        if (b & cfInstruction)
                        {
                            //pop instruction
                            if (b & cfPop)
                            {
                                //pop ecx
                                if (b & cfSkip) break;
                                instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                                firstPopRegIdx = DisInfo1.OpRegIdx[0];
                                popBytes += 4;
                                SetFlags(cfFrame, Pos, instrLen1);
                            }
                            else
                            {
                                //skip frame instruction
                                if (b & cfFrame) continue;
                                //set eax - function
                                if (b & cfSetA)
                                {
                                    recN1 = GetInfoRec(fromAdr);
                                    recN1->procInfo->flags |= PF_OUTEAX;
                                    if (!kb && !(recN1->procInfo->flags & (PF_EVENT | PF_DYNAMIC)) &&
                                        recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                                    {
                                        recN1->kind = ikFunc;
                                        Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                                        op = Disasm.GetOp(DisInfo1.Mnem);
                                        //if setXX - return type is Boolean
                                        if (op == OP_SET) recN1->type = "Boolean";
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
                if (firstPopRegIdx!= -1)
                {
                    //Skip pushed regs
                    if (spRestored)
                    {
                        for (Pos = fromPos;;Pos++)
                        {
                            b = Flags[Pos];
                            //Proc end
                            //if (Pos != fromPos && (b & cfProcEnd))
                            if (_procSize && Pos - fromPos + 1 >= _procSize) break;
                            if (b & cfInstruction)
                            {
                                instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                                SetFlags(cfFrame, Pos, instrLen1);
                                if ((b & cfPush) && (DisInfo1.OpRegIdx[0] == firstPopRegIdx)) break;
                            }
                        }
                    }
                    else
                    {
                        for (Pos = fromPos;;Pos++)
                        {
                            if (!popBytes) break;

                            b = Flags[Pos];
                            //End of proc
                            //if (Pos != fromPos && (b & cfProcEnd))
                            if (_procSize && Pos - fromPos + 1 >= _procSize) break;
                            if (b & cfInstruction)
                            {
                                instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                                op = Disasm.GetOp(DisInfo1.Mnem);
                                SetFlags(cfFrame, Pos, instrLen1);
                                //add esp,...
                                if (op == OP_ADD && DisInfo1.OpRegIdx[0] == 20)
                                {
                                    popBytes += (int)DisInfo1.Immediate;
                                    continue;
                                }
                                //sub esp,...
                                if (op == OP_SUB && DisInfo1.OpRegIdx[0] == 20)
                                {
                                    popBytes -= (int)DisInfo1.Immediate;
                                    continue;
                                }
                                if (b & cfPush) popBytes -= 4;
                            }
                        }
                    }
                }
                //If no prototype, try add information from analyze arguments result
                if (!recN->HasName() && className != "")
                {
                    //dynamic
                    if (recN->procInfo->flags & PF_DYNAMIC)
                    {
                    }
                    else
                    {
                    }
                }
                if (!kb)
                {
                    if (inA)
                    {
                        if (className != "")
                            recN->procInfo->AddArg(0x21, 0, 4, "Self", className);
                        else
                            recN->procInfo->AddArg(0x21, 0, 4, "", "");
                    }
                    if (inD)
                    {
                        if (recN->kind == ikConstructor || recN->kind == ikDestructor)
                            recN->procInfo->AddArg(0x21, 1, 4, "_Dv__", "Boolean");
                        else
                            recN->procInfo->AddArg(0x21, 1, 4, "", "");
                    }
                    if (inC)
                    {
                        recN->procInfo->AddArg(0x21, 2, 4, "", "");
                    }
                }
                break;
            }
            if (!IsFlagSet(cfSkip, curPos)) sp = -1;
        }

        if (op == OP_MOV)
        {
            lastMovAdr = DisInfo.Offset;
            lastMovImm = DisInfo.Immediate;
        }
        //init stack via loop
        if (IsFlagSet(cfLoc, curPos))
        {
            recN1 = GetInfoRec(curAdr);
            if (recN1 && recN1->xrefs && recN1->xrefs->Count == 1)
            {
                PXrefRec recX = (PXrefRec)recN1->xrefs->Items[0];
                Adr = recX->adr + recX->offset;
                if (Adr > curAdr)
                {
                    sSize = IsInitStackViaLoop(curAdr, Adr);
                    if (sSize)
                    {
                        procStackSize += sSize * lastMovImm;
                        //skip jne
                        instrLen = Disasm.Disassemble(Code + Adr2Pos(Adr), (__int64)Adr, 0, 0);
                        curAdr = Adr + instrLen; curPos = Adr2Pos(curAdr);
                        SetFlags(cfFrame, fromPos, curAdr - fromAdr);
                        continue;
                    }
                }
            }
        }
        //add (sub) esp,...
        if (DisInfo.OpRegIdx[0] == 20 && DisInfo.OpType[1] == otIMM)
        {
            if (op == OP_ADD)
            {
                if ((int)DisInfo.Immediate < 0) procStackSize -= (int)DisInfo.Immediate;
            }
            if (op == OP_SUB)
            {
                if ((int)DisInfo.Immediate > 0) procStackSize += (int)DisInfo.Immediate;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        /*
        //dec reg
        if (op == OP_DEC && DisInfo.Op1Type == otREG)
        {
            //Save (dec reg) address
            Adr = curAdr;
            //Look next instruction
            curPos += instrLen;
            curAdr += instrLen;
            instrLen = Disasm.Disassemble(Code + curPos, (__int64)curAdr, &DisInfo1, 0);
            //If jne @1, where @1 < adr of jne, when make frame from begin if Stack inited via loop
            if (DisInfo1.Conditional && DisInfo1.Immediate < curAdr && DisInfo1.Immediate >= fromAdr)
            {
                if (IsInitStackViaLoop(DisInfo1.Immediate, Adr))
                    SetFlags(cfFrame, fromPos, curPos + instrLen - fromPos + 1);
            }
            continue;
        }
        */

        //mov esp, ebp
        if (b1 == 0x8B && b2 == 0xE5) spRestored = true;

        if (!IsFlagSet(cfSkip, curPos))
        {
            if (IsFlagSet(cfPush, curPos))
            {
                if (curPos != fromPos && sp < 255)
                {
                    sp++;
                    stack[sp] = curPos;
                }
            }
            if (IsFlagSet(cfPop, curPos) && sp >= 0)
            {
                macroFrom = stack[sp];
                SetFlag(cfBracket, macroFrom);
                macroTo = curPos;
                SetFlag(cfBracket, macroTo);
                sp--;
            }
        }

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Taqble address - last 4 bytes of instruction
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Analyze gap to find table cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //If cTblAdr exists, skip it
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                Pos += CNum; Adr += CNum;
            }
            for (int k = 0; k < 4096; k++)
            {
                //Loc - end of table
                if (IsFlagSet(cfLoc, Pos)) break;

                Adr1 = *((DWORD*)(Code + Pos));
                //Validate Adr1
                if (!IsValidCodeAdr(Adr1) || Adr1 < fromAdr) break;
                //Set cfLoc
                SetFlag(cfLoc, Adr2Pos(Adr1));

                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }
//----------------------------------
//PosTry: xor reg, reg
//        push ebp
//        push offset @1
//        push fs:[reg]
//        mov fs:[reg], esp
//        ...
//@2:     ...
//At @1 we have various situations:
//----------------------------------
//@1:     jmp @HandleFinally
//        jmp @2
//----------------------------------
//@1:     jmp @HandleAnyException
//        call DoneExcept
//----------------------------------
//@1:     jmp HandleOnException
//        dd num
//Follow the table of num records like:
//        dd offset ExceptionInfo
//        dd offset ExceptionProc
//----------------------------------
        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //check that next instruction is push fs:[reg] or retn
            if ((Code[NPos] == 0x64 &&
                Code[NPos + 1] == 0xFF &&
                ((Code[NPos + 2] >= 0x30 && Code[NPos + 2] <= 0x37) || Code[NPos + 2] == 0x75)) ||
                Code[NPos] == 0xC3)
            {
                Adr = DisInfo.Immediate;      //Adr=@1
                if (IsValidCodeAdr(Adr))
                {
                    if (Adr > lastAdr) lastAdr = Adr;
                    Pos = Adr2Pos(Adr);
                    if (Pos >= 0 && Pos - NPos < MAX_DISASSEMBLE)
                    {
                        if (Code[Pos] == 0xE9) //jmp Handle...
                        {
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

                            recN1 = GetInfoRec(DisInfo.Immediate);
                            if (recN1)
                            {
                                if (recN1->SameName("@HandleFinally"))
                                {
                                    //ret + jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN1->SameName("@HandleAnyException") || recN1->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN1->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Pos += 4;
                                        //dd offset ExceptionProc
                                        Pos += 4;
                                    }
                                }
                            }
                        }
                    }
                }
            	curPos += instrLen; curAdr += instrLen;
            	continue;
            }
        }
        while (1)
        {
            //Call - stop analyze of arguments
            if (DisInfo.Call)
            {
                lastemb = false;
                Adr = DisInfo.Immediate;
                if (IsValidCodeAdr(Adr))
                {
                    recN1 = GetInfoRec(Adr);
                    if (recN1 && recN1->procInfo)
                    {
                        lastemb = (recN1->procInfo->flags & PF_EMBED);
                        WORD retb;
                        //@XStrCatN
                        if (recN1->SameName("@LStrCatN") ||
                            recN1->SameName("@WStrCatN") ||
                            recN1->SameName("@UStrCatN") ||
                            recN1->SameName("Format"))
                        {
                            retb = 0;
                        }
                        else
                            retb = recN1->procInfo->retBytes;

                        if (retb && sp >= retb)
                            sp -= retb;
                        else
                            sp = -1;
                    }
                    else
                        sp = -1;
                    //call not always preserve registers eax, edx, ecx
                    if (recN1)
                    {
                        //@IntOver, @BoundErr nothing change
                        if (!recN1->SameName("@IntOver") &&
                            !recN1->SameName("@BoundErr"))
                        {
                            argA = argD = argC = false;
                        }
                    }
                    else
                    {
                        argA = argD = argC = false;
                    }
                }
                else
                {
                    argA = argD = argC = false;
                    sp = -1;
                }
                break;
            }
            //jmp - stop analyze of output arguments
            if (op == OP_JMP)
            {
                lastCallAdr = 0;
                break;
            }
            //cop ..., [ebp + Offset]
            if (!kb && bpBased && DisInfo.BaseReg == 21 && DisInfo.IndxReg == -1 && (int)DisInfo.Offset > 0)
            {
                recN1 = GetInfoRec(fromAdr);
                //For embedded procs we have on1 additional argument (pushed on stack first), that poped from stack by instrcution pop ecx
                if (!emb || (int)DisInfo.Offset != retBytes + bpBase)
                {
                    int argSize = DisInfo.OpSize;
                    String argType = "";
                    if (argSize == 10) argType = "Extended";
                    //Each argument in stack has size 4*N bytes
                    if (argSize < 4) argSize = 4;
                    argSize = ((argSize + 3) / 4) * 4;
                    recN1->procInfo->AddArg(0x21, (int)DisInfo.Offset, argSize, "", argType);
                }
            }
            //Instruction pop reg always change reg
            if (op == OP_POP && DisInfo.OpType[0] == otREG)
            {
                //eax
                if (DisInfo.OpRegIdx[0] == 16)
                {
                    //Forget last call and set flag cfSkip, if it was call of embedded proc
                    if (lastCallAdr)
                    {
                        if (lastemb)
                        {
                            SetFlag(cfSkip, curPos);
                            lastemb = false;
                        }
                        lastCallAdr = 0;
                    }

                    if (argA && !inA)
                    {
                        argA = false;
                        if (!inD) argD = false;
                        if (!inC) argC = false;
                    }
                }
                //edx
                if (DisInfo.OpRegIdx[0] == 18)
                {
                    if (argD && !inD)
                    {
                        argD = false;
                        if (!inC) argC = false;
                    }
                }
                //ecx
                if (DisInfo.OpRegIdx[0] == 17)
                {
                    if (argC && !inC) argC = false;
                }
                break;
            }
            //cdq always change edx; eax may be output argument of last call
            if (op == OP_CDQ)
            {
                if (lastCallAdr)
                {
                    recN1 = GetInfoRec(lastCallAdr);
                    if (recN1 && recN1->procInfo &&
                        !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                        recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                    {
                        recN1->procInfo->flags |= PF_OUTEAX;
                        recN1->kind = ikFunc;
                    }
                    lastCallAdr = 0;
                }
                if (argD && !inD)
                {
                    argD = false;
                    if (!inC) argC = false;
                }
                break;
            }
            if (DisInfo.Float)
            {
                //fstsw, fnstsw always change eax
                if (!memcmp(DisInfo.Mnem + 1, "stsw", 4) || !memcmp(DisInfo.Mnem + 1, "nstsw", 5))
                {
                    if (DisInfo.OpType[0] == otREG && (DisInfo.OpRegIdx[0] == 16 || DisInfo.OpRegIdx[0] == 8 || DisInfo.OpRegIdx[0] == 4 || DisInfo.OpRegIdx[0] == 0))
                    {
                        if (lastCallAdr) lastCallAdr = 0;

                        if (argA && !inA)
                        {
                            argA = false;
                            if (!inD) argD = false;
                            if (!inC) argC = false;
                        }
                    }
                    SetFlags(cfSkip, curPos, instrLen);
                    break;
                }
                //Instructions fst, fstp after call means that it was call of function
                if (!memcmp(DisInfo.Mnem + 1, "st", 2))
                {
                    Pos = GetNearestUpInstruction(curPos, fromPos, 1);
                    if (Pos != -1 && IsFlagSet(cfCall, Pos))
                    {
                        if (lastCallAdr)
                        {
                            recN1 = GetInfoRec(lastCallAdr);
                            if (recN1 && recN1->procInfo &&
                                !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                                recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                            {
                                recN1->procInfo->flags |= PF_OUTEAX;
                                recN1->kind = ikFunc;
                            }
                            lastCallAdr = 0;
                        }
                    }
                    break;
                }
            }
    //mul, div ?????????????????????????????????????????????????????????????????????
            //xor reg, reg always change register
            if (op == OP_XOR && DisInfo.OpType[0] == DisInfo.OpType[1] && DisInfo.OpRegIdx[0] == DisInfo.OpRegIdx[1])
            {
                if (DisInfo.OpRegIdx[0] == 16 || DisInfo.OpRegIdx[0] == 8 || DisInfo.OpRegIdx[0] == 4 || DisInfo.OpRegIdx[0] == 0)
                {
                    if (lastCallAdr) lastCallAdr = 0;
                }
                if (DisInfo.OpRegIdx[0] == 18 || DisInfo.OpRegIdx[0] == 10 || DisInfo.OpRegIdx[0] == 6 || DisInfo.OpRegIdx[0] == 2)
                {
                    if (lastCallAdr) lastCallAdr = 0;
                }

                //eax, ax, ah, al
                if (DisInfo.OpRegIdx[0] == 16 || DisInfo.OpRegIdx[0] == 8 || DisInfo.OpRegIdx[0] == 4 || DisInfo.OpRegIdx[0] == 0)
                {
                    SetFlag(cfSetA, curPos);
                    if (argA && !inA)
                    {
                        argA = false;
                        if (!inD) argD = false;
                        if (!inC) argC = false;
                    }
                }
                //edx, dx, dh, dl
                if (DisInfo.OpRegIdx[0] == 18 || DisInfo.OpRegIdx[0] == 10 || DisInfo.OpRegIdx[0] == 6 || DisInfo.OpRegIdx[0] == 2)
                {
                    SetFlag(cfSetD, curPos);
                    if (argD && !inD)
                    {
                        argD = false;
                        if (!inC) argC = false;
                    }
                }
                //ecx, cx, ch, cl
                if (DisInfo.OpRegIdx[0] == 17 || DisInfo.OpRegIdx[0] == 9 || DisInfo.OpRegIdx[0] == 5 || DisInfo.OpRegIdx[0] == 1)
                {
                    SetFlag(cfSetC, curPos);
                    if (argC && !inC) argC = false;
                }
                break;
            }
            //If eax, edx, ecx in memory address - always used as registers
            if (DisInfo.BaseReg != -1 || DisInfo.IndxReg != -1)
            {
                if (DisInfo.BaseReg == 16 || DisInfo.IndxReg == 16)
                {
                    if (lastCallAdr)
                    {
                        recN1 = GetInfoRec(lastCallAdr);
                        if (recN1 && recN1->procInfo &&
                            !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                            recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                        {
                            recN1->procInfo->flags |= PF_OUTEAX;
                            recN1->kind = ikFunc;
                        }
                        lastCallAdr = 0;
                    }
                }
                if (DisInfo.BaseReg == 16 || DisInfo.IndxReg == 16)
                {
                    if (argA && !inA)
                    {
                        inA = true;
                        argA = false;
                    }
                }
                if (DisInfo.BaseReg == 18 || DisInfo.IndxReg == 18)
                {
                    if (argD && !inD)
                    {
                        inD = true;
                        argD = false;
                        if (!inA)
                        {
                            inA = true;
                            argA = false;
                        }
                    }
                }
                if (DisInfo.BaseReg == 17 || DisInfo.IndxReg == 17)
                {
                    if (argC && !inC)
                    {
                        inC = true;
                        argC = false;
                        if (!inA)
                        {
                            inA = true;
                            argA = false;
                        }
                        if (!inD)
                        {
                            inD = true;
                            argD = false;
                        }
                    }
                }
            }
            //xchg
            if (op == OP_XCHG)
            {
                if (DisInfo.OpType[0] == otREG)
                {
                    if (DisInfo.OpRegIdx[0] == 16) //eax
                    {
                        if (lastCallAdr)
                        {
                            recN1 = GetInfoRec(lastCallAdr);
                            if (recN1 && recN1->procInfo &&
                                !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                                recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                            {
                                recN1->procInfo->flags |= PF_OUTEAX;
                                recN1->kind = ikFunc;
                            }
                            lastCallAdr = 0;
                        }
                    }
                    if (DisInfo.OpRegIdx[0] == 16) //eax
                    {
                        SetFlag(cfSetA, curPos);
                        if (argA && !inA)
                        {
                            inA = true;
                            argA = false;
                        }
                    }
                    if (DisInfo.OpRegIdx[0] == 18) //edx
                    {
                        SetFlag(cfSetD, curPos);
                        if (argD && !inD)
                        {
                            inD = true;
                            argD = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                        }
                    }
                    if (DisInfo.OpRegIdx[0] == 17) //ecx
                    {
                        SetFlag(cfSetC, curPos);
                        //xchg ecx, [ebp...] - ecx used as argument
                        if (DisInfo.BaseReg == 21)
                        {
                            inC = true;
                            argC = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                            if (!inD)
                            {
                                inD = true;
                                argD = false;
                            }
                            //Set cfFrame upto start of procedure
                            SetFlags(cfFrame, fromPos, (curPos + instrLen - fromPos));
                        }
                        else if (argC && !inC)
                        {
                            inC = true;
                            argC = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                            if (!inD)
                            {
                                inD = true;
                                argD = false;
                            }
                        }
                    }
                }
                if (DisInfo.OpType[1] == otREG)
                {
                    if (DisInfo.OpRegIdx[1] == 16)
                    {
                        if (lastCallAdr)
                        {
                            recN1 = GetInfoRec(lastCallAdr);
                            if (recN1 && recN1->procInfo &&
                                !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                                recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                            {
                                recN1->procInfo->flags |= PF_OUTEAX;
                                recN1->kind = ikFunc;
                            }
                            lastCallAdr = 0;
                        }
                    }
                    if (DisInfo.OpRegIdx[1] == 16)
                    {
                        SetFlag(cfSetA, curPos);
                        if (argA && !inA)
                        {
                            inA = true;
                            argA = false;
                        }
                    }
                    if (DisInfo.OpRegIdx[1] == 18)
                    {
                        SetFlag(cfSetD, curPos);
                        if (argD && !inD)
                        {
                            inD = true;
                            argD = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                        }
                    }
                    if (DisInfo.OpRegIdx[1] == 17)
                    {
                        SetFlag(cfSetC, curPos);
                        if (argC && !inC)
                        {
                            inC = true;
                            argC = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                            if (!inD)
                            {
                                inD = true;
                                argD = false;
                            }
                        }
                    }
                }
                break;
            }
            //cop ..., reg
            if (DisInfo.OpType[1] == otREG)
            {
                if (DisInfo.OpRegIdx[1] == 16 || DisInfo.OpRegIdx[1] == 8 || DisInfo.OpRegIdx[1] == 0)
                {
                    if (lastCallAdr)
                    {
                        recN1 = GetInfoRec(lastCallAdr);
                        if (recN1 && recN1->procInfo &&
                            !(recN1->procInfo->flags & (PF_KBPROTO | PF_EVENT | PF_DYNAMIC)) &&
                            recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                        {
                            recN1->procInfo->flags |= PF_OUTEAX;
                            recN1->kind = ikFunc;
                        }
                        lastCallAdr = 0;
                    }
                }
                //eax, ax, ah, al
                if (DisInfo.OpRegIdx[1] == 16 || DisInfo.OpRegIdx[1] == 8 || DisInfo.OpRegIdx[1] == 4 || DisInfo.OpRegIdx[1] == 0)
                {
                    if (argA && !inA)
                    {
                        inA = true;
                        argA = false;
                    }
                }
                //edx, dx, dh, dl
                if (DisInfo.OpRegIdx[1] == 18 || DisInfo.OpRegIdx[1] == 10 || DisInfo.OpRegIdx[1] == 6 || DisInfo.OpRegIdx[1] == 2)
                {
                    if (argD && !inD)
                    {
                        inD = true;
                        argD = false;
                        if (!inA)
                        {
                            inA = true;
                            argA = false;
                        }
                    }
                }
                //ecx, cx, ch, cl
                if (DisInfo.OpRegIdx[1] == 17 || DisInfo.OpRegIdx[1] == 9 || DisInfo.OpRegIdx[1] == 5 || DisInfo.OpRegIdx[1] == 1)
                {
                    if (argC && !inC)
                    {
                        inC = true;
                        argC = false;
                        if (!inA)
                        {
                            inA = true;
                            argA = false;
                        }
                        if (!inD)
                        {
                            inD = true;
                            argD = false;
                        }
                    }
                }
            }

            if (DisInfo.OpType[0] == otREG && op != OP_UNK && op != OP_PUSH)
            {
                if (op != OP_MOV && op != OP_LEA && op != OP_SET)
                {
                    //eax, ax, ah, al
                    if (DisInfo.OpRegIdx[0] == 16 || DisInfo.OpRegIdx[0] == 8 || DisInfo.OpRegIdx[0] == 4 || DisInfo.OpRegIdx[0] == 0)
                    {
                        if (argA && !inA)
                        {
                            inA = true;
                            argA = false;
                        }
                    }
                    //edx, dx, dh, dl
                    if (DisInfo.OpRegIdx[0] == 18 || DisInfo.OpRegIdx[0] == 10 || DisInfo.OpRegIdx[0] == 6 || DisInfo.OpRegIdx[0] == 2)
                    {
                        if (argD && !inD)
                        {
                            inD = true;
                            argD = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                        }
                    }
                    //ecx, cx, ch, cl
                    if (DisInfo.OpRegIdx[0] == 17 || DisInfo.OpRegIdx[0] == 9 || DisInfo.OpRegIdx[0] == 5 || DisInfo.OpRegIdx[0] == 1)
                    {
                        if (argC && !inC)
                        {
                            inC = true;
                            argC = false;
                            if (!inA)
                            {
                                inA = true;
                                argA = false;
                            }
                            if (!inD)
                            {
                                inD = true;
                                argD = false;
                            }
                        }
                    }
                }
                else
                {
                    //eax, ax, ah, al
                    if (DisInfo.OpRegIdx[0] == 16 || DisInfo.OpRegIdx[0] == 8 || DisInfo.OpRegIdx[0] == 4 || DisInfo.OpRegIdx[0] == 0)
                    {
                        SetFlag(cfSetA, curPos);
                        if (argA && !inA)
                        {
                            argA = false;
                            if (!inD) argD = false;
                            if (!inC) argC = false;
                        }
                        if (lastCallAdr) lastCallAdr = 0;
                    }
                    //edx, dx, dh, dl
                    if (DisInfo.OpRegIdx[0] == 18 || DisInfo.OpRegIdx[0] == 10 || DisInfo.OpRegIdx[0] == 6 || DisInfo.OpRegIdx[0] == 2)
                    {
                        SetFlag(cfSetD, curPos);
                        if (argD && !inD)
                        {
                            argD = false;
                            if (!inC) argC = false;
                        }
                        if (lastCallAdr) lastCallAdr = 0;
                    }
                    //ecx, cx, ch, cl
                    if (DisInfo.OpRegIdx[0] == 17 || DisInfo.OpRegIdx[0] == 9 || DisInfo.OpRegIdx[0] == 5 || DisInfo.OpRegIdx[0] == 1)
                    {
                        SetFlag(cfSetC, curPos);
                        if (argC && !inC) argC = false;
                    }
                }
            }
            break;
        }

        if (DisInfo.Call)  //call sub_XXXXXXXX
        {
            lastCallAdr = 0;
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                retType = AnalyzeArguments(Adr);
                lastCallAdr = Adr;

                recN1 = GetInfoRec(Adr);
                if (recN1 && recN1->HasName())
                {
                    //Hide some procedures
                    //@Halt0 is not executed
                    if (recN1->SameName("@Halt0"))
                    {
                        SetFlags(cfSkip, curPos, instrLen);
                        if (fromAdr == EP && !lastAdr) break;
                    }
                    //Procs together previous unstruction
                    else if (recN1->SameName("@IntOver")     ||
                             recN1->SameName("@InitImports") ||
                             recN1->SameName("@InitResStringImports"))
                    {
                        Pos = GetNearestUpInstruction(curPos, fromPos, 1);
                        if (Pos != -1) SetFlags(cfSkip, Pos, (curPos - Pos) + instrLen);
                    }
                    //@BoundErr
                    else if (recN1->SameName("@BoundErr"))
                    {
                    	if (IsFlagSet(cfLoc, curPos))
                        {
                            Pos = GetNearestUpInstruction(curPos, fromPos, 1);
                            Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                            if (DisInfo1.Branch)
                            {
                            	Pos = GetNearestUpInstruction(Pos, fromPos, 3);
                                SetFlags(cfSkip, Pos, (curPos - Pos) + instrLen);
                            }
                        }
                        else
                        {
                            Pos = GetNearestUpInstruction(curPos, fromPos, 1);
                            Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo1, 0);
                            if (DisInfo1.Branch)
                            {
                                Pos = GetNearestUpInstruction(Pos, fromPos, 1);
                                if (IsFlagSet(cfPop, Pos))
                                {
                                 	Pos = GetNearestUpInstruction(Pos, fromPos, 3);
                                }
                              	SetFlags(cfSkip, Pos, (curPos - Pos) + instrLen);
                            }
                        }
                    }
                    //Not in source code
                    else if (recN1->SameName("@_IOTest") ||
                             recN1->SameName("@InitExe") ||
                             recN1->SameName("@InitLib") ||
                    		 recN1->SameName("@DoneExcept"))
                    {
                        SetFlags(cfSkip, curPos, instrLen);
                    }
                }
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (b1 == 0xEB ||				 //short relative abs jmp or cond jmp
        	(b1 >= 0x70 && b1 <= 0x7F) ||
            (b1 == 0xF && b2 >= 0x80 && b2 <= 0x8F))
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                recN1 = GetInfoRec(Adr);
                if (!recN1 && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        curPos += instrLen; curAdr += instrLen;
    }
    //Check matching ret bytes and summary size of stack arguments
    if (bpBased)
    {
        if (recN->procInfo->args)
        {
            int delta = retBytes;
            for (int n = 0; n < recN->procInfo->args->Count; n++)
            {
                argInfo = (PARGINFO)recN->procInfo->args->Items[n];
                if (argInfo->Ndx > 2) delta -= argInfo->Size;
            }
            if (delta < 0)
            {
            	//If delta between N bytes (in "ret N" instruction) and total size of argumnets != 0
                recN->procInfo->flags |= PF_ARGSIZEG;
                //delta = 4 and proc can be embbedded - allow that it really embedded
                if (delta == 4 && (recN->procInfo->flags & PF_MAYBEEMBED))
                {
                	//Ставим флажок PF_EMBED
                    recN->procInfo->flags |= PF_EMBED;
                    //Skip following after call instrcution "pop ecx"
                    for (int n = 0; n < recN->xrefs->Count; n++)
                    {
                    	PXrefRec recX = (PXrefRec)recN->xrefs->Items[n];
                        if (recX->type == 'C')
                        {
                        	Adr = recX->adr + recX->offset; Pos = Adr2Pos(Adr);
                    		instrLen = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                    		Pos += instrLen;
                    		if (Code[Pos] == 0x59) SetFlag(cfSkip, Pos);
                        }
                    }
                }
            }
            //If delta < 0, then part of arguments can be unusable
            else if (delta < 0)
            {
            	recN->procInfo->flags |= PF_ARGSIZEL;
            }
        }
    }
    recN = GetInfoRec(fromAdr);
    //if PF_OUTEAX not set - Procedure
    if (!kb && !(recN->procInfo->flags & PF_OUTEAX))
    {
        if (recN->kind != ikConstructor && recN->kind != ikDestructor)
        {
            recN->kind = ikProc;
        }
    }

    recN->procInfo->stackSize = procStackSize + 0x1000;
    if (lastCallAdr) return retType;
    return "";
}
//---------------------------------------------------------------------------
