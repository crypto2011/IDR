//---------------------------------------------------------------------------
//Create XRefs
//Scan procedure calls (include constructors and destructors)
//Calculate size of stack for arguments
void __fastcall TFMain_11011981::AnalyzeProc1(DWORD fromAdr, char xrefType, DWORD xrefAdr, int xrefOfs, bool maybeEmb)
{
	BYTE		op, b1, b2;
    bool		bpBased = false;
    WORD    	bpBase = 4;
    int         num, skipNum, instrLen, instrLen1, instrLen2, procSize;
    DWORD       b;
    int         fromPos, curPos, Pos, Pos1, Pos2;
    DWORD       curAdr, Adr, Adr1, finallyAdr, endAdr, maxAdr;
    DWORD       lastMovTarget = 0, lastCmpPos = 0, lastAdr = 0;
    PInfoRec    recN, recN1;
    PXrefRec    recX;
    DISINFO     DisInfo;

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return;

    if (IsFlagSet(cfEmbedded, fromPos)) return;

    recN = GetInfoRec(fromAdr);

    //Virtual constructor - don't analyze
    if (recN && recN->type.Pos("class of ") == 1) return;

    if (!recN)
    {
        recN = new InfoRec(fromPos, ikRefine);
    }
    else if (recN->kind == ikUnknown || recN->kind == ikData)
    {
        recN->kind = ikRefine;
        recN->procInfo = new InfoProcInfo;
    }

    //If xrefAdr != 0, add it to recN->xrefs
    if (xrefAdr)
    {
        recN->AddXref(xrefType, xrefAdr, xrefOfs);
        SetFlag(cfProcStart, Adr2Pos(xrefAdr));
    }

    //Don't analyze imports
    if (IsFlagSet(cfImport, fromPos)) return;
    //if (IsFlagSet(cfExport, fromPos)) return;

    //If Pass1 was set skip analyze
    if (IsFlagSet(cfPass1, fromPos)) return;

    if (!IsFlagSet(cfPass0, fromPos))
        AnalyzeProcInitial(fromAdr);
    SetFlag(cfProcStart | cfPass1, fromPos);

    if (maybeEmb && !(recN->procInfo->flags & PF_EMBED)) recN->procInfo->flags |= PF_MAYBEEMBED;
    procSize = GetProcSize(fromAdr);
    curPos = fromPos; curAdr = fromAdr;

    while (1)
    {
        if (curAdr >= CodeBase + TotalSize) break;
//---------------------------------- Try
//        xor reg, reg               cfTry | cfSkip
//        push ebp                   cfSkip
//        push offset @1             cfSkip
//        push fs:[reg]              cfSkip
//        mov fs:[reg], esp          cfSkip
//
//---------------------------------- OnFinally
//        xor reg, reg               cfFinally | cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        mov fs:[reg], esp          cfSkip
//Adr1-1: push offset @3             cfSkip
//@2:     ...
//        ret                        cfSkip
//---------------------------------- Finally
//@1:     jmp @HandleFinally         cfFinally | cfSkip
//        jmp @2                     cfFinally | cfSkip
//@3:     ...                        end of Finally Section
//---------------------------------- Except
//        xor reg, reg               cfExcept | cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        mov fs:[reg], esp          cfSkip
//        jmp @3 -> End of Exception Section cfSkip
//@1:     jmp @HandleAnyException    cfExcept | cfSkip
//        ...
//        call DoneExcept
//        ...
//@3:     ...
//---------------------------------- Except (another variant, rear)
//        pop fs:[0]                 cfExcept | cfSkip
//        add esp,8                  cfSkip
//        jmp @3 -> End of Exception Section cfSkip
//@1:     jmp @HandleAnyException    cfExcept | cfSkip
//        ...
//        call DoneExcept
//        ...
//@3:     ...
//---------------------------------- OnExcept
//        xor reg, reg               cfExcept | cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        pop reg                    cfSkip
//        mov fs:[reg], esp          cfSkip
//        jmp @3 -> End of Exception Section cfSkip
//@1:     jmp HandleOnException      cfExcept | cfSkip
//        dd num                     cfETable
//Table from num records:
//        dd offset ExceptionInfo
//        dd offset ExceptionProc
//        ...
//@3:     ...
//----------------------------------
        //Is it try section begin (skipNum > 0)?
        skipNum = IsTryBegin(curAdr, &finallyAdr) + IsTryBegin0(curAdr, &finallyAdr);
        if (skipNum > 0)
        {
            Adr = finallyAdr;      //Adr=@1
            Pos = Adr2Pos(Adr); if (Pos < 0) break;
            if (Adr > lastAdr) lastAdr = Adr;
            SetFlag(cfTry, curPos);
            SetFlags(cfSkip, curPos, skipNum);

            //Disassemble jmp
            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

            recN1 = GetInfoRec(DisInfo.Immediate);
            if (recN1 && recN1->HasName())
            {
                //jmp @HandleFinally
                if (recN1->SameName("@HandleFinally"))
                {
                    SetFlag(cfFinally, Pos);//@1
                    SetFlags(cfSkip, Pos - 1, instrLen1 + 1);   //ret + jmp HandleFinally

                    Pos += instrLen1; Adr += instrLen1;
                    //jmp @2
                    instrLen2 = Disasm.Disassemble(Pos2Adr(Pos), &DisInfo, 0);
                    SetFlag(cfFinally, Pos);//jmp @2
                    SetFlags(cfSkip, Pos, instrLen2);//jmp @2
                    Adr += instrLen2;
                    if (Adr > lastAdr) lastAdr = Adr;

                    Pos = Adr2Pos(DisInfo.Immediate);//@2
                    //Get prev (before Pos) instruction
                    Pos1 = GetNearestUpInstruction(Pos);
                    instrLen2 = Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //If push XXXXXXXX -> set new lastAdr
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_PUSH)
                    {
                        SetFlags(cfSkip, Pos1, instrLen2);
                        if (DisInfo.OpType[0] == otIMM && DisInfo.Immediate > lastAdr) lastAdr = DisInfo.Immediate;
                    }

                    //Find nearest up instruction with segment prefix fs:
                    Pos1 = GetNearestUpPrefixFs(Pos);
                    instrLen2 = Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //pop fs:[0]
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_POP)
                    {
                        SetFlags(cfSkip, Pos1, Pos - Pos1);
                    }
                    //mov fs:[0],reg
                    else if (DisInfo.OpType[0] == otMEM && DisInfo.BaseReg == -1 && DisInfo.Offset == 0)
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 3);
                        SetFlag(cfFinally, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }
                    //mov fs:[reg1], reg2
                    else
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 4);
                        SetFlag(cfFinally, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }
                }
                else if (recN1->SameName("@HandleAnyException") || recN1->SameName("@HandleAutoException"))
                {
                    SetFlag(cfExcept, Pos);//@1
                    //Find nearest up instruction with segment prefix fs:
                    Pos1 = GetNearestUpPrefixFs(Pos);
                    instrLen2 = Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //pop fs:[0]
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_POP)
                    {
                        SetFlags(cfSkip, Pos1, Pos - Pos1);
                    }
                    //mov fs:[0],reg
                    else if (DisInfo.OpType[0] == otMEM && DisInfo.BaseReg == -1 && DisInfo.Offset == 0)
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 3);
                        SetFlag(cfExcept, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }
                    //mov fs:[reg1], reg2
                    else
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 4);
                        SetFlag(cfExcept, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }

                    //Get prev (before Pos) instruction
                    Pos1 = GetNearestUpInstruction(Pos);
                    Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //If jmp -> set new lastAdr
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_JMP && DisInfo.Immediate > lastAdr) lastAdr = DisInfo.Immediate;
                }
                else if (recN1->SameName("@HandleOnException"))
                {
                    SetFlag(cfExcept, Pos);//@1
                    //Find nearest up instruction with segment prefix fs:
                    Pos1 = GetNearestUpPrefixFs(Pos);
                    instrLen2 = Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //pop fs:[0]
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_POP)
                    {
                        SetFlags(cfSkip, Pos1, Pos - Pos1);
                    }
                    //mov fs:[0],reg
                    else if (DisInfo.OpType[0] == otMEM && DisInfo.BaseReg == -1 && DisInfo.Offset == 0)
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 3);
                        SetFlag(cfExcept, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }
                    //mov fs:[reg1], reg2
                    else
                    {
                        Pos2 = GetNthUpInstruction(Pos1, 4);
                        SetFlag(cfExcept, Pos2);
                        SetFlags(cfSkip, Pos2, Pos1 - Pos2 + instrLen2);
                    }

                    //Get prev (before Pos) instruction
                    Pos1 = GetNearestUpInstruction(Pos);
                    Disasm.Disassemble(Pos2Adr(Pos1), &DisInfo, 0);
                    //If jmp -> set new lastAdr
                    if (Disasm.GetOp(DisInfo.Mnem) == OP_JMP && DisInfo.Immediate > lastAdr) lastAdr = DisInfo.Immediate;

                    //Next instruction
                    Pos += instrLen1; Adr += instrLen1;
                    //Set flag cfETable
                    SetFlag(cfETable, Pos);
                    //dd num
                    num = *((int*)(Code + Pos));
                    SetFlags(cfSkip, Pos, 4); Pos += 4;
                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                    for (int k = 0; k < num; k++)
                    {
                        //dd offset ExceptionInfo
                        SetFlags(cfSkip, Pos, 4); Pos += 4;
                        //dd offset ExceptionProc
                        DWORD procAdr = *((DWORD*)(Code + Pos));
                        if (IsValidCodeAdr(procAdr)) SetFlag(cfLoc, Adr2Pos(procAdr));
                        SetFlags(cfSkip, Pos, 4); Pos += 4;
                    }
                }
            }
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Is it finally section?
        skipNum = IsTryEndPush(curAdr, &endAdr);
        if (skipNum > 0)
        {
            SetFlag(cfFinally, curPos);
            SetFlags(cfSkip, curPos, skipNum);
            if (endAdr > lastAdr) lastAdr = endAdr;
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Finally section in if...then...else constructions
        skipNum = IsTryEndJump(curAdr, &endAdr);
        if (skipNum > 0)
        {
            SetFlag(cfFinally | cfExcept, curPos);
            SetFlags(cfSkip, curPos, skipNum);
            if (endAdr > lastAdr) lastAdr = endAdr;
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Int64NotEquality
        //skipNum = ProcessInt64NotEquality(curAdr, &maxAdr);
        //if (skipNum > 0)
        //{
        //    if (maxAdr > lastAdr) lastAdr = maxAdr;
        //    curPos += skipNum; curAdr += skipNum;
        //    continue;
        //}
        //Int64Equality
        //skipNum = ProcessInt64Equality(curAdr, &maxAdr);
        //if (skipNum > 0)
        //{
        //    if (maxAdr > lastAdr) lastAdr = maxAdr;
        //    curPos += skipNum; curAdr += skipNum;
        //    continue;
        //}
        //Int64Comparison
        skipNum = ProcessInt64Comparison(curAdr, &maxAdr);
        if (skipNum > 0)
        {
            if (maxAdr > lastAdr) lastAdr = maxAdr;
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Int64ComparisonViaStack1
        skipNum = ProcessInt64ComparisonViaStack1(curAdr, &maxAdr);
        if (skipNum > 0)
        {
            if (maxAdr > lastAdr) lastAdr = maxAdr;
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Int64ComparisonViaStack2
        skipNum = ProcessInt64ComparisonViaStack2(curAdr, &maxAdr);
        if (skipNum > 0)
        {
            if (maxAdr > lastAdr) lastAdr = maxAdr;
            curPos += skipNum; curAdr += skipNum;
            continue;
        }
        //Skip exception table
        if (IsFlagSet(cfETable, curPos))
        {
            //dd num
            num = *((int*)(Code + curPos));
            curPos += 4 + 8*num; curAdr += 4 + 8*num;
            continue;
        }
        b1 = Code[curPos];
        b2 = Code[curPos + 1];
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

        //Frame instructions
        if (curAdr == fromAdr && b1 == 0x55)    //push ebp
        {
            SetFlag(cfFrame, curPos);
        }
        if (b1 == 0x8B && b2 == 0xEC)           //mov ebp, esp
        {
        	bpBased = true;
            recN->procInfo->flags |= PF_BPBASED;
            recN->procInfo->bpBase = bpBase;

            SetFlags(cfFrame, curPos, instrLen);
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (b1 == 0x8B && b2 == 0xE5)           //mov esp, ebp
        {
            SetFlags(cfFrame, curPos, instrLen);
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (op == OP_JMP)
        {
            if (curAdr == fromAdr) break;
            if (DisInfo.OpType[0] == otMEM)
            {
                if (Adr2Pos(DisInfo.Offset) < 0 && (!lastAdr || curAdr == lastAdr)) break;
            }
            if (DisInfo.OpType[0] == otIMM)
            {
                Adr = DisInfo.Immediate; Pos = Adr2Pos(Adr);
                if (Pos < 0 && (!lastAdr || curAdr == lastAdr)) break;
                if (GetSegmentNo(Adr) != 0 && GetSegmentNo(fromAdr) != GetSegmentNo(Adr) && (!lastAdr || curAdr == lastAdr)) break;
                SetFlag(cfLoc, Pos);
                recN1 = GetInfoRec(Adr);
                if (!recN1) recN1 = new InfoRec(Pos, ikUnknown);
                recN1->AddXref('J', fromAdr, curAdr - fromAdr);

                if (Adr < fromAdr && (!lastAdr || curAdr == lastAdr)) break;
            }
        }

        //End of procedure
        if (DisInfo.Ret)
        {
            if (!lastAdr || curAdr == lastAdr)
            {
                //Proc end
                //SetFlag(cfProcEnd, curPos + instrLen - 1);
                recN->procInfo->procSize = curAdr - fromAdr + instrLen;
                recN->procInfo->retBytes = 0;
                //ret N
                if (DisInfo.OpNum)
                {
                    recN->procInfo->retBytes = DisInfo.Immediate;//num;
                }
                break;
            }
        }
        //push
        if (op == OP_PUSH)
        {
            SetFlag(cfPush, curPos);
            bpBase += 4;
        }
        //pop
        if (op == OP_POP)  SetFlag(cfPop, curPos);
        //add (sub) esp,...
        if (DisInfo.OpRegIdx[0] == 20 && DisInfo.OpType[1] == otIMM)
        {
            if (op == OP_ADD) bpBase -= (int)DisInfo.Immediate;
            if (op == OP_SUB) bpBase += (int)DisInfo.Immediate;
            //skip
            SetFlags(cfSkip, curPos, instrLen);
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        ////fstp [esp]
        //if (!memcmp(DisInfo.Mnem, "fst", 3) && DisInfo.BaseReg == 20) SetFlag(cfFush, curPos);

        //skip
        if (!memcmp(DisInfo.Mnem, "sahf", 4) || !memcmp(DisInfo.Mnem, "wait", 4))
        {
            SetFlags(cfSkip, curPos, instrLen);
            curPos += instrLen; curAdr += instrLen;
            continue;
        }

        if (op == OP_MOV) lastMovTarget = DisInfo.Offset;
        if (op == OP_CMP) lastCmpPos = curPos;

        //Is instruction (not call and jmp), that contaibs operand [reg+xxx], where xxx is negative value
        if (maybeEmb && !DisInfo.Call && !DisInfo.Branch && (int)DisInfo.Offset < 0 &&
        	(DisInfo.IndxReg == -1 && (DisInfo.BaseReg >= 16 && DisInfo.BaseReg <= 23 && DisInfo.BaseReg != 20 && DisInfo.BaseReg != 21)))
        {
        	//May be add condition that all Xrefs must points to one subroutine!!!!!!!!!!!!!
        	if ((bpBased && DisInfo.BaseReg != 21) || (!bpBased && DisInfo.BaseReg != 20))
            {
                recN->procInfo->flags |= PF_EMBED;
            }
        }

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset))
            {
                //SetFlag(cfProcEnd, curPos + instrLen - 1);
                recN->procInfo->procSize = curAdr - fromAdr + instrLen;
            	break;
            }
            DWORD cTblAdr = 0, jTblAdr = 0;
            SetFlag(cfSwitch, lastCmpPos);
            SetFlag(cfSwitch, curPos);
            
            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Table address  - last 4 bytes of instruction
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Scan gap to find table cTbl
            if (Adr <= lastMovTarget && lastMovTarget < jTblAdr) cTblAdr = lastMovTarget;
            //If exists cTblAdr, skip it
            BYTE CTab[256];
            if (cTblAdr)
            {
                int CNum = jTblAdr - cTblAdr;
                SetFlags(cfSkip, Pos, CNum);
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
                SetFlags(cfSkip, Pos, 4);
                Pos += 4; Adr += 4;
                if (Adr1 > lastAdr) lastAdr = Adr1;
            }
            if (Adr > lastAdr) lastAdr = Adr;
            curPos = Pos; curAdr = Adr;
            continue;
        }
        if (b1 == 0x68)		//try block	(push loc_TryBeg)
        {
            DWORD NPos = curPos + instrLen;
            //Check that next instruction is push fs:[reg] or retn
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
                            if (Code[NPos + 2] == 0x35)
                            {
                                SetFlag(cfTry, NPos - 6);
                                SetFlags(cfSkip, NPos - 6, 20);
                            }
                            else
                            {
                                SetFlag(cfTry, NPos - 8);
                                SetFlags(cfSkip, NPos - 8, 14);
                            }
                            //Disassemble jmp
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);

                            recN1 = GetInfoRec(DisInfo.Immediate);
                            if (recN1 && recN1->HasName())
                            {
                                //jmp @HandleFinally
                                if (recN1->SameName("@HandleFinally"))
                                {
                                    SetFlag(cfFinally, Pos);
                                
                                    SetFlags(cfSkip, Pos - 1, instrLen1 + 1);   //ret + jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    SetFlag(cfFinally, Pos);
                                    SetFlags(cfSkip, Pos, instrLen2);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                    //int hfEndPos = Adr2Pos(Adr);

                                    int hfStartPos = Adr2Pos(DisInfo.Immediate); assert(hfStartPos >= 0);

                                    Pos = hfStartPos - 5;

                                    if (Code[Pos] == 0x68)  //push offset @3       //Flags[Pos] & cfInstruction must be != 0
                                    {
                                        hfStartPos = Pos - 8;
                                        SetFlags(cfSkip, hfStartPos, 13);
                                    }
                                    SetFlag(cfFinally, hfStartPos);
                                }
                                else if (recN1->SameName("@HandleAnyException") || recN1->SameName("@HandleAutoException"))
                                {
                                    SetFlag(cfExcept, Pos);

                                    int hoStartPos = Pos - 10;
                                    SetFlags(cfSkip, hoStartPos, instrLen1 + 10);
                                    Disasm.Disassemble(Code + Pos - 10, (__int64)(Adr - 10), &DisInfo, 0);
                                    if (Disasm.GetOp(DisInfo.Mnem) != OP_XOR || DisInfo.OpRegIdx[0] != DisInfo.OpRegIdx[1])
                                    {
                                        hoStartPos = Pos - 13;
                                        SetFlags(cfSkip, hoStartPos, instrLen1 + 13);
                                    }
                                    //Find prev jmp
                                    Pos1 = hoStartPos; Adr1 = Pos2Adr(Pos1);
                                    for (int k = 0; k < 6; k++)
                                    {
                                        instrLen2 = Disasm.Disassemble(Code + Pos1, (__int64)Adr1, &DisInfo, 0);
                                        Pos1 += instrLen2;
                                        Adr1 += instrLen2;
                                    }
                                    if (DisInfo.Immediate > lastAdr) lastAdr = DisInfo.Immediate;
                                    //int hoEndPos = Adr2Pos(DisInfo.Immediate);

                                    SetFlag(cfExcept, hoStartPos);
                                }
                                else if (recN1->SameName("@HandleOnException"))
                                {
                                    SetFlag(cfExcept, Pos);

                                    int hoStartPos = Pos - 10;
                                    SetFlags(cfSkip, hoStartPos, instrLen1 + 10);
                                    Disasm.Disassemble(Code + Pos - 10, (__int64)(Adr - 10), &DisInfo, 0);
                                    if (Disasm.GetOp(DisInfo.Mnem) != OP_XOR || DisInfo.OpRegIdx[0] != DisInfo.OpRegIdx[1])
                                    {
                                        hoStartPos = Pos - 13;
                                        SetFlags(cfSkip, hoStartPos, instrLen1 + 13);
                                    }
                                    //Find prev jmp
                                    Pos1 = hoStartPos; Adr1 = Pos2Adr(Pos1);
                                    for (int k = 0; k < 6; k++)
                                    {
                                        instrLen2 = Disasm.Disassemble(Code + Pos1, (__int64)Adr1, &DisInfo, 0);
                                        Pos1 += instrLen2;
                                        Adr1 += instrLen2;
                                    }
                                    if (DisInfo.Immediate > lastAdr) lastAdr = DisInfo.Immediate;
                                    //int hoEndPos = Adr2Pos(DisInfo.Immediate);

                                    SetFlag(cfExcept, hoStartPos);

                                    //Next instruction
                                    Pos += instrLen1; Adr += instrLen1;
                                    //Set flag cfETable
                                    SetFlag(cfETable, Pos);
                                    //dd num
                                    num = *((int*)(Code + Pos));
                                    SetFlags(cfSkip, Pos, 4); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        SetFlags(cfSkip, Pos, 4); Pos += 4;
                                        //dd offset ExceptionProc
                                        DWORD procAdr = *((DWORD*)(Code + Pos));
                                        if (IsValidCodeAdr(procAdr)) SetFlag(cfLoc, Adr2Pos(procAdr));
                                        SetFlags(cfSkip, Pos, 4); Pos += 4;
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

        if (DisInfo.Call)
        {
            SetFlag(cfCall, curPos);
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr) && Adr2Pos(Adr) >= 0)
            {
                SetFlag(cfLoc, Adr2Pos(Adr));
                //If after call exists instruction pop ecx, it may be embedded procedure
                bool mbemb = (Code[curPos + instrLen] == 0x59);
                AnalyzeProc1(Adr, 'C', fromAdr, curAdr - fromAdr, mbemb);

                recN1 = GetInfoRec(Adr);
                if (recN1 && recN1->procInfo)
                {
                    //After embedded proc instruction pop ecx must be skipped
                    if (mbemb && recN1->procInfo->flags & PF_EMBED)
                        SetFlag(cfSkip, curPos + instrLen);

                    if (recN1->HasName())
                    {
                        if (recN1->SameName("@Halt0"))
                        {
                            SetFlags(cfSkip, curPos, instrLen);
                            if (fromAdr == EP && !lastAdr)
                            {
                                //SetFlag(cfProcEnd, curPos + instrLen - 1);
                                recN->procInfo->procSize = curAdr - fromAdr + instrLen;
                                recN->SetName("EntryPoint");
                                recN->procInfo->retBytes = 0;
                                break;
                            }
                        }

                        int begPos, endPos;
                        //If called procedure is @ClassCreate, then current procedure is constructor
                        if (recN1->SameName("@ClassCreate"))
                        {
                            recN->kind = ikConstructor;
                            //Code from instruction test... until this call is not sufficient (mark skipped)
                            begPos = GetNearestUpInstruction1(curPos, fromPos, "test");
                            if (begPos != -1) SetFlags(cfSkip, begPos, curPos + instrLen - begPos);
                        }
                        else if (recN1->SameName("@AfterConstruction"))
                        {
                            begPos = GetNearestUpInstruction2(curPos, fromPos, "test", "cmp");
                            endPos = GetNearestDownInstruction(curPos, "add");
                            if (begPos != -1 && endPos != -1) SetFlags(cfSkip, begPos, endPos - begPos);
                        }
                        else if (recN1->SameName("@BeforeDestruction"))
                            SetFlag(cfSkip, curPos);
                        //If called procedure is @ClassDestroy, then current procedure is destructor
                        else if (recN1->SameName("@ClassDestroy"))
                        {
                            recN->kind = ikDestructor;
                            begPos = GetNearestUpInstruction2(curPos, fromPos, "test", "cmp");
                            if (begPos != -1) SetFlags(cfSkip, begPos, curPos + instrLen - begPos);
                        }
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
                Pos = Adr2Pos(Adr);
                if (!IsFlagSet(cfEmbedded, Pos))//Possible branch to start of Embedded proc (for ex. in proc TextToFloat)) 
                {
                    SetFlag(cfLoc, Pos);
                    //Mark possible start of Loop
                    if (Adr < curAdr && !IsFlagSet(cfFinally, Adr2Pos(curAdr)) && !IsFlagSet(cfExcept, Adr2Pos(curAdr)))
                        SetFlag(cfLoop, Pos);
                    recN1 = GetInfoRec(Adr);
                    if (!recN1) recN1 = new InfoRec(Pos, ikUnknown);
                    recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                    if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
                }
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (b1 == 0xE9)    //relative abs jmp or cond jmp
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                Pos = Adr2Pos(Adr);
                SetFlag(cfLoc, Pos);
                //Mark possible start of Loop
                if (Adr < curAdr && !IsFlagSet(cfFinally, Adr2Pos(curAdr)) && !IsFlagSet(cfExcept, Adr2Pos(curAdr)))
                    SetFlag(cfLoop, Pos);
                recN1 = GetInfoRec(Adr);
                if (recN1 && recN1->HasName())
                {
                    if (recN1->SameName("@HandleFinally")      ||
                        recN1->SameName("@HandleAnyException") ||
                        recN1->SameName("@HandleOnException")  ||
                        recN1->SameName("@HandleAutoException"))
                    {
                        recN1->AddXref('J', fromAdr, curAdr - fromAdr);
                    }
                }
                if (!recN1 && Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        //Second operand - immediate and is valid address
        if (DisInfo.OpType[1] == otIMM)
        {
            Pos = Adr2Pos(DisInfo.Immediate);
            //imm32 must be valid code address outside current procedure
            if (Pos >= 0 && IsValidCodeAdr(DisInfo.Immediate) && (DisInfo.Immediate < fromAdr || DisInfo.Immediate >= fromAdr + procSize))
            {
                //Position must be free
                if (!Flags[Pos])
                {
                    //No Name
                    if (!Infos[Pos])
                    {
                        //Address must be outside current procedure
                        if (DisInfo.Immediate < fromAdr || DisInfo.Immediate >= fromAdr + procSize)
                        {
                            //If valid code lets user decide later
                            int codeValidity = IsValidCode(DisInfo.Immediate);

                            if (codeValidity == 1)  //Code
                                AnalyzeProc1(DisInfo.Immediate, 'D', fromAdr, curAdr - fromAdr, false);
                        }
                    }
                }
                //If slot is not free (procedure is already loaded)
                else if (IsFlagSet(cfProcStart, Pos))
                    AnalyzeProc1(DisInfo.Immediate, 'D', fromAdr, curAdr - fromAdr, false);
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        curPos += instrLen; curAdr += instrLen;
    }
}
//---------------------------------------------------------------------------

