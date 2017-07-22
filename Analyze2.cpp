void __fastcall AddFieldXref(PFIELDINFO fInfo, DWORD ProcAdr, int ProcOfs, char type);
//---------------------------------------------------------------------------
//structure for saving context of all registers (branch instruction)
typedef struct
{
    int     sp;
    DWORD   adr;
    RINFO   registers[32];
} RCONTEXT, *PRCONTEXT;
//---------------------------------------------------------------------------
PRCONTEXT __fastcall GetCtx(TList* Ctx, DWORD Adr)
{
    for (int n = 0; n < Ctx->Count; n++)
    {
        PRCONTEXT rinfo = (PRCONTEXT)Ctx->Items[n];
        if (rinfo->adr == Adr) return rinfo;
    }
    return 0;
}
//---------------------------------------------------------------------------
void __fastcall SetRegisterValue(PRINFO regs, int Idx, DWORD Value)
{
    if (Idx >= 16 && Idx <= 19)
    {
    	regs[Idx - 16].value = Value;
        regs[Idx - 12].value = Value;
    	regs[Idx - 8].value = Value;
    	regs[Idx].value = Value;
        return;
    }
    if (Idx >= 20 && Idx <= 23)
    {
    	regs[Idx - 8].value = Value;
    	regs[Idx].value = Value;
        return;
    }
    if (Idx >= 0 && Idx <= 3)
    {
    	regs[Idx].value = Value;
     	regs[Idx + 8].value = Value;
        regs[Idx + 16].value = Value;
        return;
    }
    if (Idx >= 4 && Idx <= 7)
    {
    	regs[Idx].value = Value;
    	regs[Idx + 4].value = Value;
        regs[Idx + 12].value = Value;
        return;
    }
    if (Idx >= 8 && Idx <= 11)
    {
    	regs[Idx - 8].value = Value;
        regs[Idx - 4].value = Value;
    	regs[Idx].value = Value;
        regs[Idx + 8].value = Value;
        return;
    }
    if (Idx >= 12 && Idx <= 15)
    {
    	regs[Idx].value = Value;
     	regs[Idx + 8].value = Value;
        return;
    }
}
//---------------------------------------------------------------------------
//Possible values
//'V' - Virtual table base (for calls processing)
//'v' - var
//'L' - lea local var
//'l' - local var
//'A' - lea argument
//'a' - argument
//'I' - Integer
void __fastcall SetRegisterSource(PRINFO regs, int Idx, char Value)
{
    if (Idx >= 16 && Idx <= 19)
    {
    	regs[Idx - 16].source = Value;
        regs[Idx - 12].source = Value;
    	regs[Idx - 8].source = Value;
    	regs[Idx].source = Value;
        return;
    }
    if (Idx >= 20 && Idx <= 23)
    {
    	regs[Idx - 8].source = Value;
    	regs[Idx].source = Value;
        return;
    }
    if (Idx >= 0 && Idx <= 3)
    {
    	regs[Idx].source = Value;
     	regs[Idx + 8].source = Value;
        regs[Idx + 16].source = Value;
        return;
    }
    if (Idx >= 4 && Idx <= 7)
    {
    	regs[Idx].source = Value;
    	regs[Idx + 4].source = Value;
        regs[Idx + 12].source = Value;
        return;
    }
    if (Idx >= 8 && Idx <= 11)
    {
    	regs[Idx - 8].source = Value;
        regs[Idx - 4].source = Value;
    	regs[Idx].source = Value;
        regs[Idx + 8].source = Value;
        return;
    }
    if (Idx >= 12 && Idx <= 15)
    {
	    regs[Idx].source = Value;
     	regs[Idx + 8].source = Value;
        return;
    }
}
//---------------------------------------------------------------------------
void __fastcall SetRegisterType(PRINFO regs, int Idx, String Value)
{
    if (Idx >= 16 && Idx <= 19)
    {
    	regs[Idx - 16].type = Value;
        regs[Idx - 12].type = Value;
    	regs[Idx - 8].type = Value;
    	regs[Idx].type = Value;
        return;
    }
    if (Idx >= 20 && Idx <= 23)
    {
    	regs[Idx - 8].type = Value;
    	regs[Idx].type = Value;
        return;
    }
    if (Idx >= 0 && Idx <= 3)
    {
    	regs[Idx].type = Value;
     	regs[Idx + 8].type = Value;
        regs[Idx + 16].type = Value;
        return;
    }
    if (Idx >= 4 && Idx <= 7)
    {
    	regs[Idx].type = Value;
    	regs[Idx + 4].type = Value;
        regs[Idx + 12].type = Value;
        return;
    }
    if (Idx >= 8 && Idx <= 11)
    {
    	regs[Idx - 8].type = Value;
        regs[Idx - 4].type = Value;
    	regs[Idx].type = Value;
        regs[Idx + 8].type = Value;
        return;
    }
    if (Idx >= 12 && Idx <= 15)
    {
    	regs[Idx].type = Value;
     	regs[Idx + 8].type = Value;
        return;
    }
}
//---------------------------------------------------------------------------
void __fastcall TFMain_11011981::AnalyzeProc2(DWORD fromAdr, bool addArg, bool AnalyzeRetType)
{
    //saved context
    TList *sctx = new TList;
    for (int n = 0; n < 3; n++)
    {
        if (!AnalyzeProc2(fromAdr, addArg, AnalyzeRetType, sctx)) break;
    }
    //delete sctx
    CleanupList<RCONTEXT>(sctx);
}
//---------------------------------------------------------------------------
bool __fastcall TFMain_11011981::AnalyzeProc2(DWORD fromAdr, bool addArg, bool AnalyzeRetType, TList *sctx)
{
	BYTE		    op, b1, b2;
    char            source;
	bool		    reset, bpBased, vmt, fContinue = false;
    WORD            bpBase;
    int             n, num, instrLen, instrLen1, instrLen2, _ap, _procSize;
    int             reg1Idx, reg2Idx;
    int			    sp = -1, fromIdx = -1; 	//fromIdx - index of register in instruction mov eax,reg (for processing call @IsClass)
    DWORD           b;
    int             fromPos, curPos, Pos;
    DWORD           curAdr;
    DWORD           lastMovAdr = 0;
    DWORD           procAdr, Val, Adr, Adr1;
    DWORD           reg, varAdr, classAdr, vmtAdr, lastAdr = 0;
    PInfoRec        recN, recN1;
    PLOCALINFO      locInfo;
    PARGINFO        argInfo;
    PFIELDINFO 	    fInfo = 0;
    PRCONTEXT       rinfo;
    RINFO     	    rtmp;
    String		    comment, typeName, className = "", varName, varType;
    String          _eax_Type, _edx_Type, _ecx_Type, sType;
    RINFO     	    registers[32];
    RINFO   	    stack[256];
    DISINFO         DisInfo, DisInfo1;

    fromPos = Adr2Pos(fromAdr);
    if (fromPos < 0) return false;
    if (IsFlagSet(cfPass2, fromPos)) return false;
    if (IsFlagSet(cfEmbedded, fromPos)) return false;
    if (IsFlagSet(cfExport, fromPos)) return false;

    //b1 = Code[fromPos];
    //b2 = Code[fromPos + 1];
    //if (!b1 && !b2) return false;

    //Import - return ret type of function
    if (IsFlagSet(cfImport, fromPos)) return false;
    recN = GetInfoRec(fromAdr);

    //if recN = 0 (Interface Methods!!!) then return
    if (!recN || !recN->procInfo) return false;

    //Procedure from Knowledge Base not analyzed
    if (recN && recN->kbIdx != -1) return false;

    //if (!IsFlagSet(cfPass1, fromPos))
    //???

    SetFlag(cfProcStart | cfPass2, fromPos);

    //If function name contains class name get it
    className = ExtractClassName(recN->GetName());
    bpBased = (recN->procInfo->flags & PF_BPBASED);
    bpBase = (recN->procInfo->bpBase);

    rtmp.result = 0; rtmp.source = 0; rtmp.value = 0; rtmp.type = "";
    for (n = 0; n < 32; n++) registers[n] = rtmp;

    //Get args
    _eax_Type = _edx_Type = _ecx_Type = "";
    BYTE callKind = recN->procInfo->flags & 7;
    if (recN->procInfo->args && !callKind)
    {
    	for (n = 0; n < recN->procInfo->args->Count; n++)
        {
        	PARGINFO argInfo = (PARGINFO)recN->procInfo->args->Items[n];
            if (argInfo->Ndx == 0)
            {
            	if (className != "")
                	registers[16].type = className;
                else
            		registers[16].type = argInfo->TypeDef;
                _eax_Type = registers[16].type;
                //var
                if (argInfo->Tag == 0x22) registers[16].source = 'v';
                continue;
            }
            if (argInfo->Ndx == 1)
            {
            	registers[18].type = argInfo->TypeDef;
                _edx_Type = registers[18].type;
                //var
                if (argInfo->Tag == 0x22) registers[18].source = 'v';
                continue;
            }
            if (argInfo->Ndx == 2)
            {
           		registers[17].type = argInfo->TypeDef;
                _ecx_Type = registers[17].type;
                //var
                if (argInfo->Tag == 0x22) registers[17].source = 'v';
                continue;
            }
          	break;
        }
    }
    else if (className != "")
    {
    	registers[16].type = className;
    }

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
                if (AnalyzeRetType)
                {
                    //Если тип регистра eax не пустой, находим ближайшую сверху инструкцию его инциализации
                    if (registers[16].type != "")
                    {
                        for (Pos = curPos - 1; Pos >= fromPos; Pos--)
                        {
                            b = Flags[Pos];
                            if ((b & cfInstruction) & !(b & cfSkip))
                            {
                                Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo, 0);
                                //If branch - break
                                if (DisInfo.Branch) break;
                                //If call
                                //Other cases (call [reg+Ofs]; call [Adr]) need to add
                                if (DisInfo.Call)
                                {
                                    Adr = DisInfo.Immediate;
                                    if (IsValidCodeAdr(Adr))
                                    {
                                        recN1 = GetInfoRec(Adr);
                                        if (recN1 && recN1->procInfo/*recN1->kind == ikFunc*/)
                                        {
                                            typeName = recN1->type;
                                            recN1 = GetInfoRec(fromAdr);
                                            if (!(recN1->procInfo->flags & (PF_EVENT | PF_DYNAMIC)) &&
                                                recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                                            {
                                                recN1->kind = ikFunc;
                                                recN1->type = typeName;
                                            }
                                        }
                                    }
                                }
                                else if (b & cfSetA)
                                {
                                    recN1 = GetInfoRec(fromAdr);
                                    if (!(recN1->procInfo->flags & (PF_EVENT | PF_DYNAMIC)) &&
                                        recN1->kind != ikConstructor && recN1->kind != ikDestructor)
                                    {
                                        recN1->kind = ikFunc;
                                        recN1->type = registers[16].type;
                                    }
                                }
                            }
                        }
                    }
                }
                break;
            }
            if (!IsFlagSet(cfSkip, curPos)) sp = -1;
        }

        //cfBracket
        if (IsFlagSet(cfBracket, curPos))
        {
        	if (op == OP_PUSH && sp < 255)
            {
                reg1Idx = DisInfo.OpRegIdx[0];
                sp++;
                stack[sp] = registers[reg1Idx];
            }
            else if (op == OP_POP && sp >= 0)
            {
                reg1Idx = DisInfo.OpRegIdx[0];
                registers[reg1Idx] = stack[sp];
                sp--;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        //Проверим, не попал ли внутрь инструкции Fixup или ThreadVar
        bool    NameInside = false;
        for (int k = 1; k < instrLen; k++)
        {
            if (Infos[curPos + k])
            {
                NameInside = true;
                break;
            }
        }

        reset = ((op & OP_RESET) != 0);

        if (op == OP_MOV) lastMovAdr = DisInfo.Offset;

        //If loc then try get context
        if (curAdr != fromAdr && IsFlagSet(cfLoc, curPos))
        {
            rinfo = GetCtx(sctx, curAdr);
            if (rinfo)
            {
                sp = rinfo->sp;
                for (n = 0; n < 32; n++) registers[n] = rinfo->registers[n];
            }
            //context not found - set flag to continue on the next step
            else
            {
                fContinue = true;
            }
        }

        if (b1 == 0xFF && (b2 & 0x38) == 0x20 && DisInfo.OpType[0] == otMEM && IsValidImageAdr(DisInfo.Offset)) //near absolute indirect jmp (Case)
        {
            if (!IsValidCodeAdr(DisInfo.Offset)) break;
            DWORD cTblAdr = 0, jTblAdr = 0;

            Pos = curPos + instrLen;
            Adr = curAdr + instrLen;
            //Адрес таблицы - последние 4 байта инструкции
            jTblAdr = *((DWORD*)(Code + Pos - 4));
            //Анализируем промежуток на предмет таблицы cTbl
            if (Adr <= lastMovAdr && lastMovAdr < jTblAdr) cTblAdr = lastMovAdr;
            //Если есть cTblAdr, пропускаем эту таблицу
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
                //Save context
                if (!GetCtx(sctx, Adr1))
                {
                    rinfo = new RCONTEXT;
                    rinfo->sp = sp;
                    rinfo->adr = Adr1;
                    for (n = 0; n < 32; n++) rinfo->registers[n] = registers[n];
                    sctx->Add((void*)rinfo);
                }

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

                            recN = GetInfoRec(DisInfo.Immediate);
                            if (recN)
                            {
                                if (recN->SameName("@HandleFinally"))
                                {
                                    //jmp HandleFinally
                                    Pos += instrLen1; Adr += instrLen1;
                                    //jmp @2
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleAnyException") || recN->SameName("@HandleAutoException"))
                                {
                                    //jmp HandleAnyException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //call DoneExcept
                                    instrLen2 = Disasm.Disassemble(Code + Pos, (__int64)Adr, 0, 0);
                                    Adr += instrLen2;
                                    if (Adr > lastAdr) lastAdr = Adr;
                                }
                                else if (recN->SameName("@HandleOnException"))
                                {
                                    //jmp HandleOnException
                                    Pos += instrLen1; Adr += instrLen1;
                                    //dd num
                                    num = *((int*)(Code + Pos)); Pos += 4;
                                    if (Adr + 4 + 8 * num > lastAdr) lastAdr = Adr + 4 + 8 * num;

                                    for (int k = 0; k < num; k++)
                                    {
                                        //dd offset ExceptionInfo
                                        Adr = *((DWORD*)(Code + Pos)); Pos += 4;
                                        if (IsValidImageAdr(Adr))
                                        {
                                            recN1 = GetInfoRec(Adr);
                                            if (recN1 && recN1->kind == ikVMT) className = recN1->GetName();
                                        }
                                        //dd offset ExceptionProc
                                        procAdr = *((DWORD*)(Code + Pos)); Pos += 4;
                                        if (IsValidImageAdr(procAdr))
                                        {
                                            //Save context
                                            if (!GetCtx(sctx, procAdr))
                                            {
                                                rinfo = new RCONTEXT;
                                                rinfo->sp = sp;
                                                rinfo->adr = procAdr;
                                                for (n = 0; n < 32; n++) rinfo->registers[n] = registers[n];
                                                //eax
                                                rinfo->registers[16].value = GetClassAdr(className);
                                                rinfo->registers[16].type = className;

                                                sctx->Add((void*)rinfo);
                                            }
                                        }
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
        //branch
        if (DisInfo.Branch)
        {
            Adr = DisInfo.Immediate;
            if (IsValidCodeAdr(Adr))
            {
                _ap = Adr2Pos(Adr);
                //SetFlag(cfLoc, _ap);
                //recN1 = GetInfoRec(Adr);
                //if (!recN1) recN1 = new InfoRec(_ap, ikUnknown);
                //recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                //Save context
                if (!GetCtx(sctx, Adr))
                {
                    rinfo = new RCONTEXT;
                    rinfo->sp = sp;
                    rinfo->adr = Adr;
                    for (n = 0; n < 32; n++) rinfo->registers[n] = registers[n];
                    sctx->Add((void*)rinfo);
                }
                if (Adr >= fromAdr && Adr > lastAdr) lastAdr = Adr;
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        if (registers[16].type != "" && registers[16].type[1] == '#')
        {
            DWORD dd = *((DWORD*)(registers[16].type.c_str()));
            //Если был вызов функции @GetTls, смотрим след. инструкцию вида [eax+N]
            if (dd == 'SLT#')
            {
                //Если нет внутреннего имени (Fixup, ThreadVar)
                if (!NameInside)
                {
                    //Destination (GlobalLists := TList.Create)
                    //Source (GlobalLists.Add)
                    if ((DisInfo.OpType[0] == otMEM || DisInfo.OpType[1] == otMEM) && DisInfo.BaseReg == 16)
                    {
                        _ap = Adr2Pos(curAdr); assert(_ap >= 0);
                        recN1 = GetInfoRec(curAdr + 1);
                        if (!recN1) recN1 = new InfoRec(_ap + 1, ikThreadVar);
                        if (!recN1->HasName()) recN1->SetName(String("threadvar_") + DisInfo.Offset);
                    }
                }
                SetRegisterValue(registers, 16, 0xFFFFFFFF);
                registers[16].type = "";
                curPos += instrLen; curAdr += instrLen;
                continue;
            }
        }
        //Call
        if (DisInfo.Call)
        {
            Adr = DisInfo.Immediate;
            if (IsValidImageAdr(Adr))
            {
            	recN = GetInfoRec(Adr);
                if (recN && recN->procInfo)
                {
                    int retBytes = (int)recN->procInfo->retBytes;
                    if (retBytes != -1 && sp >= retBytes)
                        sp -= retBytes;
                    else
                        sp = -1;

					//for constructor type is in eax
                    if (recN->kind == ikConstructor)
                    {
                        //Если dl = 1, регистр eax после вызова используется
                        if (registers[2].value == 1)
                        {
                            classAdr = GetClassAdr(registers[16].type);
                            if (IsValidImageAdr(classAdr))
                            {
                                //Add xref to vmt info
                                recN1 = GetInfoRec(classAdr);
                                recN1->AddXref('D', Adr, 0);

                                comment = registers[16].type + ".Create";
                                AddPicode(curPos, OP_CALL, comment, 0);
                                AnalyzeTypes(fromAdr, curPos, Adr, registers);
                            }
                        }
                        SetFlag(cfSetA, curPos);
                    }
                    else
                    {
                        //Found @Halt0 - exit
                        if (recN->SameName("@Halt0") && fromAdr == EP && !lastAdr) break;

                    	DWORD dynAdr;
                        if (recN->SameName("@ClassCreate"))
                        {
                             SetRegisterType(registers, 16, className);
                             SetFlag(cfSetA, curPos);
                        }
                        else if (recN->SameName("@CallDynaInst") ||
                                 recN->SameName("@CallDynaClass"))
                        {
                            if (DelphiVersion <= 5)
                                comment = GetDynaInfo(GetClassAdr(registers[16].type), registers[11].value, &dynAdr);	//bx
                            else
                                comment = GetDynaInfo(GetClassAdr(registers[16].type), registers[14].value, &dynAdr);	//si
                            AddPicode(curPos, OP_CALL, comment, dynAdr);
                        	SetRegisterType(registers, 16, "");
                        }
                        else if (recN->SameName("@FindDynaInst") ||
                                 recN->SameName("@FindDynaClass"))
                        {
                            comment = GetDynaInfo(GetClassAdr(registers[16].type), registers[10].value, &dynAdr);	//dx
                            AddPicode(curPos, OP_CALL, comment, dynAdr);
                            SetRegisterType(registers, 16, "");
                        }
                        //@XStrArrayClr
                        else if (recN->SameName("@LStrArrayClr") || recN->SameName("@WStrArrayClr") || recN->SameName("@UStrArrayClr"))
                        {
                            DWORD arrAdr = registers[16].value;
                            int cnt = registers[18].value;
                            //Direct address???
                            if (IsValidImageAdr(arrAdr))
                            {
                            }
                            //Local vars
                            else if ((registers[16].source & 0xDF) == 'L')
                            {
                                recN1 = GetInfoRec(fromAdr);
                                int aofs = registers[16].value;
                                for (int aa = 0; aa < cnt; aa++, aofs += 4)
                                {
                                    if (recN->SameName("@LStrArrayClr"))
                                        recN1->procInfo->AddLocal(aofs, 4, "", "AnsiString");
                                    else if (recN->SameName("@WStrArrayClr"))
                                        recN1->procInfo->AddLocal(aofs, 4, "", "WideString");
                                    else if (recN->SameName("@UStrArrayClr"))
                                        recN1->procInfo->AddLocal(aofs, 4, "", "UString");
                                }
                            }
                            SetRegisterType(registers, 16, "");
                        }
                        //@TryFinallyExit
                        else if (recN->SameName("@TryFinallyExit"))
                        {
                            //Find first jxxx
                            for (Pos = curPos - 1; Pos >= fromPos; Pos--)
                            {
                                b = Flags[Pos];
                                if (b & cfInstruction)
                                {
                                    instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Pos2Adr(Pos), &DisInfo, 0);
                                    if (DisInfo.Conditional) break;
                                    SetFlags(cfSkip | cfFinallyExit, Pos, instrLen1);
                                }
                            }
                            //@TryFinallyExit + jmp XXXXXXXX
                            instrLen += Disasm.Disassemble(Code + curPos + instrLen, (__int64)(Pos2Adr(curPos) + instrLen), &DisInfo, 0);
                            SetFlags(cfSkip | cfFinallyExit, curPos, instrLen);
                        }
                        else
                        {
                            String retType = AnalyzeTypes(fromAdr, curPos, Adr, registers);
                            recN1 = GetInfoRec(fromAdr);
                            for (int mm = 16; mm <= 18; mm++)
                            {
                            	if (registers[mm].result == 1)
                                {
                                	if ((registers[mm].source & 0xDF) == 'L')
                                    {
                                    	recN1->procInfo->AddLocal((int)registers[mm].value, 4, "", registers[mm].type);
                                    }
                                    else if ((registers[mm].source & 0xDF) == 'A')
                                    	recN1->procInfo->AddArg(0x21, (int)registers[mm].value, 4, "", registers[mm].type);
                                }
                            }
                            SetRegisterType(registers, 16, retType);
                        }
                    }
                }
                else
                {
                    sp = -1;
            		SetRegisterType(registers, 16, "");
                }
            }
            //call Memory
            else if (DisInfo.OpType[0] == otMEM && DisInfo.IndxReg == -1)
            {
                sp = -1;
                //call [Offset]
                if (DisInfo.BaseReg == -1)
                {
                }
                //call [BaseReg + Offset]
                else
                {
                    classAdr = registers[DisInfo.BaseReg].value;
                    SetRegisterType(registers, 16, "");
                    if (IsValidCodeAdr(classAdr) && registers[DisInfo.BaseReg].source == 'V')
                    {
                        recN = GetInfoRec(classAdr);
                        if (recN && recN->vmtInfo && recN->vmtInfo->methods)
                        {
                            for (int mm = 0; mm < recN->vmtInfo->methods->Count; mm++)
                            {
                                PMethodRec recM = (PMethodRec)recN->vmtInfo->methods->Items[mm];
                                if (recM->kind == 'V' && recM->id == (int)DisInfo.Offset)
                                {
                                    recN1 = GetInfoRec(recM->address);

                                    if (recM->name != "")
                                        comment = recM->name;
                                    else
                                    {
                                        if (recN1->HasName())
                                            comment = recN1->GetName();
                                        else
                                            comment = GetClsName(classAdr) + ".sub_" + Val2Str8(recM->address);
                                    }
                                    AddPicode(curPos, OP_CALL, comment, recM->address);

                                    recN1->AddXref('V', fromAdr, curAdr - fromAdr);
                                    if (recN1->kind == ikFunc) SetRegisterType(registers, 16, recN1->type);
                                    break;
                                }
                            }
                        }
                        registers[DisInfo.BaseReg].source = 0;
                    }
                    else
                    {
                    	int callOfs = DisInfo.Offset;
                    	typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                        if (typeName != "" && callOfs > 0)
                        {
                        	Pos = GetNearestUpInstruction(curPos, fromPos, 1); Adr = Pos2Adr(Pos);
                            instrLen1 = Disasm.Disassemble(Code + Pos, (__int64)Adr, &DisInfo, 0);
                            if (DisInfo.Offset == callOfs + 4)
                            {
                                fInfo = GetField(typeName, callOfs, &vmt, &vmtAdr, "");
                                if (fInfo)
                                {
                                    if (fInfo->Name != "") AddPicode(curPos, OP_CALL, typeName + "." + fInfo->Name, 0);
                                    if (vmt)
                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                    else
                                        delete fInfo;
                                }
                                else if (vmt)
                                {
                                    fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, callOfs, -1, "", "");
                                    if (fInfo) AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                }
                            }
                        }
                    }
                }
            }
            SetRegisterSource(registers, 16, 0);
            SetRegisterSource(registers, 17, 0);
            SetRegisterSource(registers, 18, 0);
            SetRegisterValue(registers, 16, 0xFFFFFFFF);
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        sType = String(DisInfo.sSize);
        //floating point operations
        if (DisInfo.Float)
        {
            float       singleVal;
            long double extendedVal;
            String		fVal = "";

            switch (DisInfo.OpSize)
            {
            case 4:
                sType = "Single";
                break;
            //Double or Comp???
            case 8:
            	sType = "Double";
                break;
            case 10:
                sType = "Extended";
                break;
            default:
            	sType = "Float";
                break;
            }

        	Adr = DisInfo.Offset;
            _ap = Adr2Pos(Adr);
        	//fxxx [Adr]
        	if (DisInfo.BaseReg == -1 && DisInfo.IndxReg == -1)
            {
                if (IsValidImageAdr(Adr))
                {
                    if (_ap >= 0)
                    {
                        switch (DisInfo.OpSize)
                        {
                        case 4:
                            singleVal = 0; memmove((void*)&singleVal, Code + _ap, 4);
                            fVal = FloatToStr(singleVal);
                            break;
                        //Double or Comp???
                        case 8:
                            break;
                        case 10:
                            try
                            {
                                extendedVal = 0; memmove((void*)&extendedVal, Code + _ap, 10);
                                fVal = FloatToStr(extendedVal);
                            }
                            catch (Exception &E)
                            {
                                fVal = "Impossible!";
                            }
                            break;
                        }
                        SetFlags(cfData, _ap, DisInfo.OpSize);

                        recN = GetInfoRec(Adr);
                        if (!recN) recN = new InfoRec(_ap, ikData);
                        if (!recN->HasName()) recN->SetName(fVal);
                        if (recN->type == "") recN->type = sType;
                        if (!IsValidCodeAdr(Adr)) recN->AddXref('D', fromAdr, curAdr - fromAdr);
                    }
                    else
                    {
                        recN = AddToBSSInfos(Adr, MakeGvarName(Adr), sType);
                        if (recN) recN->AddXref('C', fromAdr, curAdr - fromAdr);
                    }
                }
            }
            else if (DisInfo.BaseReg != -1)
            {
            	//fxxxx [BaseReg + Offset]
            	if (DisInfo.IndxReg == -1)
                {
                    //fxxxx [ebp - Offset]
                    if (bpBased && DisInfo.BaseReg == 21 && (int)DisInfo.Offset < 0)
                    {
                        recN1 = GetInfoRec(fromAdr);
                        recN1->procInfo->AddLocal((int)DisInfo.Offset, DisInfo.OpSize, "", sType);
                    }
                    //fxxx [esp + Offset]
                    else if (DisInfo.BaseReg == 20)
                    {
                        dummy = 1;
                    }
                    else
                    {
                        //fxxxx [BaseReg]
                        if (!DisInfo.Offset)
                        {
                            varAdr = registers[DisInfo.BaseReg].value;
                            if (IsValidImageAdr(varAdr))
                            {
                                _ap = Adr2Pos(varAdr);
                                if (_ap >= 0)
                                {
                                    recN1 = GetInfoRec(varAdr);
                                    if (!recN1) recN1 = new InfoRec(_ap, ikData);
                                    MakeGvar(recN1, varAdr, curAdr);
                                    recN1->type = sType;
                                    if (!IsValidCodeAdr(varAdr)) recN1->AddXref('D', fromAdr, curAdr - fromAdr);
                                }
                                else
                                {
                                    recN1 = AddToBSSInfos(varAdr, MakeGvarName(varAdr), sType);
                                    if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                }
                            }
                        }
                        //fxxxx [BaseReg + Offset]
                        else if ((int)DisInfo.Offset > 0)
                        {
                            typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                            if (typeName != "")
                            {
                                fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                                if (fInfo)
                                {
                                    if (vmt)
                                    {
                                        if (CanReplace(fInfo->Type, sType)) fInfo->Type = sType;
                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                    }
                                    else
                                        delete fInfo;
                                    //if (vmtAdr) typeName = GetClsName(vmtAdr);
                                    AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                }
                                else if (vmt)
                                {
                                    fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                                    if (fInfo)
                                    {
                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                    	AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                    }
                                }
                            }
                        }
                        //fxxxx [BaseReg - Offset]
                        else
                        {
                        }
                    }
                }
                //fxxxx [BaseReg + IndxReg*Scale + Offset]
                else
                {
                }
            }
            curPos += instrLen; curAdr += instrLen;
            continue;
        }
        //No operands
        if (DisInfo.OpNum == 0)
        {
        	//cdq
            if (op == OP_CDQ)
            {
                SetRegisterSource(registers, 16, 'I');
            	SetRegisterValue(registers, 16, 0xFFFFFFFF);
                SetRegisterType(registers, 16, "Integer");
                SetRegisterSource(registers, 18, 'I');
                SetRegisterValue(registers, 18, 0xFFFFFFFF);
                SetRegisterType(registers, 18, "Integer");
            }
        }
        //1 operand
        else if (DisInfo.OpNum == 1)
        {
        	//op Imm
            if (DisInfo.OpType[0] == otIMM)
            {
            	if (IsValidImageAdr(DisInfo.Immediate))
                {
                    _ap = Adr2Pos(DisInfo.Immediate);
                    if (_ap >= 0)
                    {
                        recN1 = GetInfoRec(DisInfo.Immediate);
                        if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                    }
                    else
                    {
                        recN1 = AddToBSSInfos(DisInfo.Immediate, MakeGvarName(DisInfo.Immediate), "");
                        if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                    }
                }
            }
            //op reg
            else if (DisInfo.OpType[0] == otREG && op != OP_UNK && op != OP_PUSH)
            {
                reg1Idx = DisInfo.OpRegIdx[0];
                SetRegisterSource(registers, reg1Idx, 0);
                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                SetRegisterType(registers, reg1Idx, "");
            }
            //op [BaseReg + Offset]
            else if (DisInfo.OpType[0] == otMEM)
            {
                if (DisInfo.BaseReg != -1 && DisInfo.IndxReg == -1 && (int)DisInfo.Offset > 0)
                {
                    typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                    if (typeName != "")
                    {
                        fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                        if (fInfo)
                        {
                            if (vmt)
                                AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                            else
                                delete fInfo;
                            AddPicode(curPos, 0, typeName, DisInfo.Offset);
                        }
                        else if (vmt)
                        {
                            fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                            if (fInfo)
                            {
                                AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                            	AddPicode(curPos, 0, typeName, DisInfo.Offset);
                            }
                        }
                    }
                }
            	if (op == OP_IMUL || op == OP_IDIV)
                {
                    SetRegisterSource(registers, 16, 0);
                    SetRegisterValue(registers, 16, 0xFFFFFFFF);
                    SetRegisterType(registers, 16, "Integer");
                    SetRegisterSource(registers, 18, 0);
                    SetRegisterValue(registers, 18, 0xFFFFFFFF);
                    SetRegisterType(registers, 18, "Integer");
                }
            }
        }
        //2 or 3 operands
        else if (DisInfo.OpNum >= 2)
        {
            if (op & OP_A2)
            //if (op == OP_MOV || op == OP_CMP || op == OP_LEA || op == OP_XOR || op == OP_ADD || op == OP_SUB ||
            //    op == OP_AND || op == OP_TEST || op == OP_XCHG || op == OP_IMUL || op == OP_IDIV || op == OP_OR ||
            //    op == OP_BT || op == OP_BTC || op == OP_BTR || op == OP_BTS)
            {
                if (DisInfo.OpType[0] == otREG)	//cop reg,...
                {
                    reg1Idx = DisInfo.OpRegIdx[0];
                    source = registers[reg1Idx].source;
                    SetRegisterSource(registers, reg1Idx, 0);

                    if (DisInfo.OpType[1] == otIMM)	//cop reg, Imm
                    {
                    	if (reset)
                        {
                            typeName = TrimTypeName(registers[reg1Idx].type);
                            SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                            SetRegisterType(registers, reg1Idx, "");

                            if (op == OP_ADD)
                            {
                            	if (typeName != "" && source != 'v')
                                {
                                    fInfo = GetField(typeName, (int)DisInfo.Immediate, &vmt, &vmtAdr, "");
                                    if (fInfo)
                                    {
                                        registers[reg1Idx].type = fInfo->Type;
                                        if (vmt)
                                            AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                        else
                                            delete fInfo;
                                        //if (vmtAdr) typeName = GetClsName(vmtAdr);
                                        AddPicode(curPos, 0, typeName, DisInfo.Immediate);
                                    }
                                    else if (vmt)
                                    {
                                        fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Immediate, -1, "", "");
                                        if (fInfo)
                                        {
                                            AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                        	AddPicode(curPos, 0, typeName, DisInfo.Immediate);
                                        }
                                    }
                                }
                            }
                            else
                            {
                            	if (op == OP_MOV) SetRegisterValue(registers, reg1Idx, DisInfo.Immediate);
                                SetRegisterSource(registers, reg1Idx, 'I');
                                if (IsValidImageAdr(DisInfo.Immediate))
                                {
                                    _ap = Adr2Pos(DisInfo.Immediate);
                                    if (_ap >= 0)
                                    {
                                        recN1 = GetInfoRec(DisInfo.Immediate);
                                        if (recN1)
                                        {
                                            SetRegisterType(registers, reg1Idx, recN1->type);
                                            bool _addXref = false;
                                            switch (recN1->kind)
                                            {
                                            case ikString:
                                                SetRegisterType(registers, reg1Idx, "ShortString");
                                                _addXref = true;
                                                break;
                                            case ikLString:
                                                SetRegisterType(registers, reg1Idx, "AnsiString");
                                                _addXref = true;
                                                break;
                                            case ikWString:
                                                SetRegisterType(registers, reg1Idx, "WideString");
                                                _addXref = true;
                                                break;
                                            case ikCString:
                                                SetRegisterType(registers, reg1Idx, "PAnsiChar");
                                                _addXref = true;
                                                break;
                                            case ikWCString:
                                                SetRegisterType(registers, reg1Idx, "PWideChar");
                                                _addXref = true;
                                                break;
                                            case ikUString:
                                                SetRegisterType(registers, reg1Idx, "UString");
                                                _addXref = true;
                                                break;
                                            }
                                            if (_addXref) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                        }
                                    }
                                    else
                                    {
                                        recN1 = AddToBSSInfos(DisInfo.Immediate, MakeGvarName(DisInfo.Immediate), "");
                                        if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                    }
                                }
                            }
                        }
                    }
                    else if (DisInfo.OpType[1] == otREG)	//cop reg, reg
                    {
                        reg2Idx = DisInfo.OpRegIdx[1];
                    	if (reset)
                        {
                            if (op == OP_MOV)
                            {
                            	SetRegisterSource(registers, reg1Idx, registers[reg2Idx].source);
                                SetRegisterValue(registers, reg1Idx, registers[reg2Idx].value);
                                SetRegisterType(registers, reg1Idx, registers[reg2Idx].type);
                                if (reg1Idx == 16) fromIdx = reg2Idx;
                            }
                            else if (op == OP_XOR)
                            {
                                SetRegisterValue(registers, reg1Idx, registers[reg1Idx].value ^ registers[reg2Idx].value);
                                SetRegisterType(registers, reg1Idx, "");
                            }
                            else if (op == OP_XCHG)
                            {
                                rtmp = registers[reg1Idx]; registers[reg1Idx] = registers[reg2Idx]; registers[reg2Idx] = rtmp;
                            }
                            else if (op == OP_IMUL || op == OP_IDIV)
                            {
                            	SetRegisterSource(registers, reg1Idx, 0);
                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                SetRegisterType(registers, reg1Idx, "Integer");
                                if (reg1Idx != reg2Idx)
                                {
                                	SetRegisterSource(registers, reg2Idx, 0);
                                	SetRegisterValue(registers, reg2Idx, 0xFFFFFFFF);
                                	SetRegisterType(registers, reg2Idx, "Integer");
                            	}
                            }
                            else
                            {
                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                SetRegisterType(registers, reg1Idx, "");
                            }
                        }
                    }
                    else if (DisInfo.OpType[1] == otMEM)	//cop reg, Memory
                    {
                    	if (DisInfo.BaseReg == -1)
                        {
                        	if (DisInfo.IndxReg == -1)	//cop reg, [Offset]
                            {
                            	if (reset)
                                {
                                    if (op == OP_IMUL)
                                    {
                                        SetRegisterSource(registers, reg1Idx, 0);
                                        SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        SetRegisterType(registers, reg1Idx, "Integer");
                                    }
                                    else
                                    {
                                        SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        SetRegisterType(registers, reg1Idx, "");
                                    }
                                }
                                Adr = DisInfo.Offset;
                                if (IsValidImageAdr(Adr))
                                {
                                    _ap = Adr2Pos(Adr);
                                    if (_ap >= 0)
                                    {
                                        recN = GetInfoRec(Adr);
                                        if (recN)
                                        {
                                            MakeGvar(recN, Adr, curAdr);
                                            if (recN->kind == ikVMT)
                                            {
                                                if (reset)
                                                {
                                                    SetRegisterType(registers, reg1Idx, recN->GetName());
                                                    SetRegisterValue(registers, reg1Idx, Adr - VmtSelfPtr);
                                                }
                                            }
                                            else
                                            {
                                                if (reset) registers[reg1Idx].type = recN->type;
                                                if (reg1Idx < 24)
                                                {
                                                    if (reg1Idx >= 16)
                                                    {
                                                        if (IsFlagSet(cfImport, _ap))
                                                        {
                                                            recN1 = GetInfoRec(Adr);
                                                            AddPicode(curPos, OP_COMMENT, recN1->GetName(), 0);
                                                        }
                                                        else if (!IsFlagSet(cfRTTI, _ap))
                                                        {
                                                            Val = *((DWORD*)(Code + _ap));
                                                            if (reset) SetRegisterValue(registers, reg1Idx, Val);
                                                            if (IsValidImageAdr(Val))
                                                            {
                                                                _ap = Adr2Pos(Val);
                                                                if (_ap >= 0)
                                                                {
                                                                    recN1 = GetInfoRec(Val);
                                                                    if (recN1)
                                                                    {
                                                                        MakeGvar(recN1, Val, curAdr);
                                                                        varName = recN1->GetName();
                                                                        if (varName != "") recN->SetName("^" + varName);
                                                                        if (recN->type != "") registers[reg1Idx].type = recN->type;
                                                                        varType = recN1->type;
                                                                        if (varType != "")
                                                                        {
                                                                            recN->type = varType;
                                                                            registers[reg1Idx].type = varType;
                                                                        }
                                                                    }
                                                                    else
                                                                    {
                                                                        recN1 = new InfoRec(_ap, ikData);
                                                                        MakeGvar(recN1, Val, curAdr);
                                                                        varName = recN1->GetName();
                                                                        if (varName != "") recN->SetName("^" + varName);
                                                                        if (recN->type != "") registers[reg1Idx].type = recN->type;
                                                                    }
                                                                    if (recN) recN->AddXref('C', fromAdr, curAdr - fromAdr);
                                                                }
                                                                else
                                                                {
                                                                    if (recN->HasName())
                                                                        recN1 = AddToBSSInfos(Val, recN->GetName(), recN->type);
                                                                    else
                                                                        recN1 = AddToBSSInfos(Val, MakeGvarName(Val), recN->type);
                                                                    if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                                                }
                                                            }
                                                            else
                                                            {
                                                                AddPicode(curPos, OP_COMMENT, "0x" + Val2Str0(Val), 0);
                                                                SetFlags(cfData, _ap, 4);
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        if (reg1Idx <= 7)
                                                        {
                                                            Val = *(Code + _ap);
                                                        }
                                                        else if (reg1Idx <= 15)
                                                        {
                                                            Val = *((WORD*)(Code + _ap));
                                                        }
                                                        AddPicode(curPos, OP_COMMENT, "0x" + Val2Str0(Val), 0);
                                                        SetFlags(cfData, _ap, 4);
                                                    }
                                                }
                                            }
                                        }
                                        else
                                        {
                                            recN = new InfoRec(_ap, ikData);
                                            MakeGvar(recN, Adr, curAdr);
                                            if (reg1Idx < 24)
                                            {
                                                if (reg1Idx >= 16)
                                                {
                                                    Val = *((DWORD*)(Code + _ap));
                                                    if (reset) SetRegisterValue(registers, reg1Idx, Val);
                                                    if (IsValidImageAdr(Val))
                                                    {
                                                        _ap = Adr2Pos(Val);
                                                        if (_ap >= 0)
                                                        {
                                                            recN->kind = ikPointer;
                                                            recN1 = GetInfoRec(Val);
                                                            if (recN1)
                                                            {
                                                                MakeGvar(recN1, Val, curAdr);
                                                                varName = recN1->GetName();
                                                                if (varName != "" && (recN1->kind == ikLString || recN1->kind == ikWString || recN1->kind == ikUString))
                                                                    varName = "\"" + varName + "\"";
                                                                if (varName != "") recN->SetName("^" + varName);
                                                                if (recN->type != "") registers[reg1Idx].type = recN->type;
                                                                varType = recN1->type;
                                                                if (varType != "")
                                                                {
                                                                    recN->type = varType;
                                                                    registers[reg1Idx].type = varType;
                                                                }
                                                            }
                                                            else
                                                            {
                                                                recN1 = new InfoRec(_ap, ikData);
                                                                MakeGvar(recN1, Val, curAdr);
                                                                varName = recN1->GetName();
                                                                if (varName != "" && (recN1->kind == ikLString || recN1->kind == ikWString || recN1->kind == ikUString))
                                                                    varName = "\"" + varName + "\"";
                                                                if (varName != "") recN->SetName("^" + varName);
                                                                if (recN->type != "") registers[reg1Idx].type = recN->type;
                                                            }
                                                            if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                                        }
                                                        else
                                                        {
                                                            recN1 = AddToBSSInfos(Val, MakeGvarName(Val), "");
                                                            if (recN1)
                                                            {
                                                                recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                                                if (recN->type != "") recN->type = recN1->type;
                                                            }
                                                        }
                                                    }
                                                    else
                                                    {
                                                        AddPicode(curPos, OP_COMMENT, "0x" + Val2Str0(Val), 0);
                                                        SetFlags(cfData, _ap, 4);
                                                    }
                                                }
                                                else
                                                {
                                                    if (reg1Idx <= 7)
                                                    {
                                                        Val = *(Code + _ap);
                                                    }
                                                    else if (reg1Idx <= 15)
                                                    {
                                                        Val = *((WORD*)(Code + _ap));
                                                    }
                                                    AddPicode(curPos, OP_COMMENT, "0x" + Val2Str0(Val), 0);
                                                    SetFlags(cfData, _ap, 4);
                                                }
                                            }
                                        }
                                    }
                                    else
                                    {
                                        recN1 = AddToBSSInfos(Adr, MakeGvarName(Adr), "");
                                        if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                    }

                                }
                            }
                            else	//cop reg, [Offset + IndxReg*Scale]
                            {
                            	if (reset)
                                {
                                    if (op == OP_IMUL)
                                    {
                                        SetRegisterSource(registers, reg1Idx, 0);
                                        SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        SetRegisterType(registers, reg1Idx, "Integer");
                                    }
                                    else
                                    {
                                        SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        SetRegisterType(registers, reg1Idx, "");
                                    }
                                }

                                Adr = DisInfo.Offset;
                                if (IsValidImageAdr(Adr))
                                {
                                    _ap = Adr2Pos(Adr);
                                    if (_ap >= 0)
                                    {
                                        recN = GetInfoRec(Adr);
                                        if (recN)
                                        {
                                            if (recN->kind == ikVMT)
                                                typeName = recN->GetName();
                                            else
                                                typeName = recN->type;

                                            if (reset) SetRegisterType(registers, reg1Idx, typeName);
                                            if (!IsValidCodeAdr(Adr)) recN->AddXref('C', fromAdr, curAdr - fromAdr);
                                        }
                                    }
                                    else
                                    {
                                        recN1 = AddToBSSInfos(Adr, MakeGvarName(Adr), "");
                                        if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                    }
                                }
                            }
                        }
                        else
                        {
                        	if (DisInfo.IndxReg == -1)
                            {
                            	if (bpBased && DisInfo.BaseReg == 21)	//cop reg, [ebp + Offset]
                                {
                                    if ((int)DisInfo.Offset < 0)	//cop reg, [ebp - Offset]
                                    {
                                        if (reset)
                                        {
                                            if (op == OP_IMUL)
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer");
                                            }
                                            else
                                            {
                                                SetRegisterSource(registers, reg1Idx, (op == OP_LEA) ? 'L' : 'l');
                                                SetRegisterValue(registers, reg1Idx, DisInfo.Offset);
                                                SetRegisterType(registers, reg1Idx, "");
                                            }
                                        }
                                        //xchg ecx, [ebp-4] (ecx = 0, [ebp-4] = _ecx_)
                                        if ((int)DisInfo.Offset == -4 && reg1Idx == 17)
                                        {
                                            recN1 = GetInfoRec(fromAdr);
                                            locInfo = recN1->procInfo->AddLocal((int)DisInfo.Offset, 4, "", "");
                                            SetRegisterType(registers, reg1Idx, _ecx_Type);
                                        }
                                        else
                                        {
                                            recN1 = GetInfoRec(fromAdr);
                                            locInfo = recN1->procInfo->AddLocal((int)DisInfo.Offset, DisInfo.OpSize, "", "");
                                            //mov, xchg
                                            if (op == OP_MOV || op == OP_XCHG)
                                            {
                                                SetRegisterType(registers, reg1Idx, locInfo->TypeDef);
                                            }
                                            else if (op == OP_LEA && locInfo->TypeDef != "")
                                            {
                                            	SetRegisterType(registers, reg1Idx, locInfo->TypeDef);
                                            }
                                        }
                                    }
                                    else	//cop reg, [ebp + Offset]
                                    {
                                        if (reset)
                                        {
                                            if (op == OP_IMUL)
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer");
                                            }
                                            else
                                            {
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "");
                                            }
                                        }
                                        if (bpBased && addArg)
                                        {
                                            recN1 = GetInfoRec(fromAdr);
                                            argInfo = recN1->procInfo->AddArg(0x21, DisInfo.Offset, 4, "", "");
                                            if (op == OP_MOV || op == OP_LEA || op == OP_XCHG)
                                            {
                                                SetRegisterSource(registers, reg1Idx, (op == OP_LEA) ? 'A' : 'a');
                                                SetRegisterValue(registers, reg1Idx, DisInfo.Offset);
                                                SetRegisterType(registers, reg1Idx, argInfo->TypeDef);
                                            }
                                        }
                                    }
                                }
                                else if (DisInfo.BaseReg == 20)	//cop reg, [esp + Offset]
                                {
                                    if (reset)
                                    {
                                    	if (op == OP_IMUL)
                                        {
                                        	SetRegisterSource(registers, reg1Idx, 0);
                                            SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            SetRegisterType(registers, reg1Idx, "Integer");
                                        }
                                        else
                                        {
                                        	SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        	SetRegisterType(registers, reg1Idx, "");
                                        }
                                    }
                                }
                                else	//cop reg, [BaseReg + Offset]
                                {
                                    if (!DisInfo.Offset)	//cop reg, [BaseReg]
                                    {
                                        Adr = registers[DisInfo.BaseReg].value;
                                        typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                        if (reset)
                                        {
                                            if (op == OP_IMUL)
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer");
                                            }
                                            else
                                            {
                                            	SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            	SetRegisterType(registers, reg1Idx, "");
                                            }
                                            
                                            if (typeName != "")
                                            {
                                            	if (typeName[1] == '^') typeName = typeName.SubString(2, typeName.Length() - 1);
                                               	SetRegisterValue(registers, reg1Idx, GetClassAdr(typeName));
                                                SetRegisterType(registers, reg1Idx, typeName); //???
                                               	SetRegisterSource(registers, reg1Idx, 'V');	//Virtual table base (for calls processing)
                                            }
                                            if (IsValidImageAdr(Adr))
                                            {
                                                _ap = Adr2Pos(Adr);
                                                if (_ap >= 0)
                                                {
                                                    recN = GetInfoRec(Adr);
                                                    if (recN)
                                                    {
                                                        if (recN->kind == ikVMT)
                                                        {
                                                            SetRegisterType(registers, reg1Idx, recN->GetName());
                                                            SetRegisterValue(registers, reg1Idx, Adr - VmtSelfPtr);
                                                        }
                                                        else
                                                        {
                                                            SetRegisterType(registers, reg1Idx, recN->type);
                                                            if (recN->type != "")
                                                                SetRegisterValue(registers, reg1Idx, GetClassAdr(recN->type));
                                                        }
                                                    }
                                                }
                                                else
                                                {
                                                    AddToBSSInfos(Adr, MakeGvarName(Adr), "");
                                                }
                                            }
                                        }
                                    }
                                    else if ((int)DisInfo.Offset > 0)	//cop reg, [BaseReg + Offset]
                                    {
                                        typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                        if (reset)
                                        {
                                            if (op == OP_IMUL)
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer"); sType = "Integer";
                                            }
                                            else
                                            {
                                        		SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            	SetRegisterType(registers, reg1Idx, "");
                                            }
                                        }
                                        if (typeName != "")
                                        {
                                            fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                                            if (fInfo)
                                            {
                                                if (op == OP_MOV || op == OP_XCHG)
                                                {
                                                    registers[reg1Idx].type = fInfo->Type;
                                                }
                                                if (vmt)
                                                {
                                                    if (CanReplace(fInfo->Type, sType)) fInfo->Type = sType;
                                                    AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                                }
                                                else
                                                    delete fInfo;
                                                //if (vmtAdr) typeName = GetClsName(vmtAdr);
                                                AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                            }
                                            else if (vmt)
                                            {
                                                fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                                                if (fInfo)
                                                {
                                                    AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                                	AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                                }
                                            }
                                        }
                                    }
                                    else	//cop reg, [BaseReg - Offset]
                                    {
                                    	if (reset)
                                        {
                                            if (op == OP_IMUL)
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer");
                                            }
                                            else
                                            {
                                        		SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        		SetRegisterType(registers, reg1Idx, "");
                                            }
                                        }
                                    }
                                }
                            }
                            else	//cop reg, [BaseReg + IndxReg*Scale + Offset]
                            {
                            	if (DisInfo.BaseReg == 21)	//cop reg, [ebp + IndxReg*Scale + Offset]
                                {
                                    if (reset)
                                    {
                                        if (op == OP_IMUL)
                                        {
                                            SetRegisterSource(registers, reg1Idx, 0);
                                            SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            SetRegisterType(registers, reg1Idx, "Integer");
                                        }
                                        else
                                        {
                                        	SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        	SetRegisterType(registers, reg1Idx, "");
                                        }
                                    }
                                }
                                else if (DisInfo.BaseReg == 20)	//cop reg, [esp + IndxReg*Scale + Offset]
                                {
                                    if (reset)
                                    {
                                        if (op == OP_IMUL)
                                        {
                                            SetRegisterSource(registers, reg1Idx, 0);
                                            SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            SetRegisterType(registers, reg1Idx, "Integer");
                                        }
                                        else
                                        {
                                        	SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        	SetRegisterType(registers, reg1Idx, "");
                                        }
                                    }
                                }
                                else	//cop reg, [BaseReg + IndxReg*Scale + Offset]
                                {
                                	typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                	if (reset)
                                    {
                                    	if (op == OP_LEA)
                                        {
                                        	//BaseReg - points to class
                                            if (typeName != "")
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "");
                                            }
                                            //Else - general arifmetics
                                            else
                                            {
                                                SetRegisterSource(registers, reg1Idx, 0);
                                                SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                                SetRegisterType(registers, reg1Idx, "Integer");
                                            }
                                        }
                                        else if (op == OP_IMUL)
                                        {
                                            SetRegisterSource(registers, reg1Idx, 0);
                                            SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                            SetRegisterType(registers, reg1Idx, "Integer");
                                        }
                                        else
                                        {
                                        	SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                                        	SetRegisterType(registers, reg1Idx, "");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                //cop Mem,...
                else
                {
                    //cop Mem, Imm
                    if (DisInfo.OpType[1] == otIMM)
                    {
                        //cop [Offset], Imm
                        if (DisInfo.BaseReg == -1 && DisInfo.IndxReg == -1)
                        {
                            Adr = DisInfo.Offset;
                            if (IsValidImageAdr(Adr))
                            {
                                _ap = Adr2Pos(Adr);
                                if (_ap >= 0)
                                {
                                    recN1 = GetInfoRec(Adr);
                                    if (!recN1) recN1 = new InfoRec(_ap, ikData);
                                    MakeGvar(recN1, Adr, curAdr);
                                    if (!IsValidCodeAdr(Adr)) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                }
                                else
                                {
                                    recN1 = AddToBSSInfos(Adr, MakeGvarName(Adr), "");
                                    if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                }
                            }
                        }
                        //cop [BaseReg + IndxReg*Scale + Offset], Imm
                        else if (DisInfo.BaseReg != -1)
                        {
                            //cop [BaseReg + Offset], Imm
                            if (DisInfo.IndxReg == -1)
                            {
                                //cop [ebp - Offset], Imm
                                if (bpBased && DisInfo.BaseReg == 21 && (int)DisInfo.Offset < 0)
                                {
                                    recN1 = GetInfoRec(fromAdr);
                                    recN1->procInfo->AddLocal((int)DisInfo.Offset, DisInfo.OpSize, "", "");
                                }
                                //cop [esp], Imm
                                else if (DisInfo.BaseReg == 20)
                                {
                                    dummy = 1;
                                }
                                //other registers
                                else
                                {
                                    //cop [BaseReg], Imm
                                    if (!DisInfo.Offset)
                                    {
                                        Adr = registers[DisInfo.BaseReg].value;
                                        if (IsValidImageAdr(Adr))
                                        {
                                            _ap = Adr2Pos(Adr);
                                            if (_ap >= 0)
                                            {
                                                recN = GetInfoRec(Adr);
                                                if (!recN) recN = new InfoRec(_ap, ikData);
                                                MakeGvar(recN, Adr, curAdr);
                                                if (!IsValidCodeAdr(Adr)) recN->AddXref('C', fromAdr, curAdr - fromAdr);
                                            }
                                            else
                                            {
                                                recN1 = AddToBSSInfos(Adr, MakeGvarName(Adr), "");
                                                if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                            }
                                        }
                                    }
                                    //cop [BaseReg + Offset], Imm
                                    else if ((int)DisInfo.Offset > 0)
                                    {
                                        typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                        if (typeName != "")
                                        {
                                            fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                                            if (fInfo)
                                            {
                                                if (vmt)
                                                {
                                                    if (op != OP_CMP && op != OP_TEST)
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                    else
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                                }
                                                else
                                                    delete fInfo;
                                                //if (vmtAdr) typeName = GetClsName(vmtAdr);
                                                AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                            }
                                            else if (vmt)
                                            {
                                                fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                                                if (fInfo)
                                                {
                                                    if (op != OP_CMP && op != OP_TEST)
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                    else
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'C');
                                                	AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                                }
                                            }
                                        }
                                    }
                                    //cop [BaseReg - Offset], Imm
                                    else
                                    {
                                    }
                                }
                            }
                            //cop [BaseReg + IndxReg*Scale + Offset], Imm
                            else
                            {
                            }
                        }
                        //Other instructions
                        else
                        {
                        }
                    }
                    //cop Mem, reg
                    else if (DisInfo.OpType[1] == otREG)
                    {
                        reg2Idx = DisInfo.OpRegIdx[1];
                        //op [Offset], reg
                        if (DisInfo.BaseReg == -1 && DisInfo.IndxReg == -1)
                        {
                            varAdr = DisInfo.Offset;
                            if (IsValidImageAdr(varAdr))
                            {
                                _ap = Adr2Pos(varAdr);
                                if (_ap >= 0)
                                {
                                    recN1 = GetInfoRec(varAdr);
                                    if (!recN1) recN1 = new InfoRec(_ap, ikData);
                                    MakeGvar(recN1, varAdr, curAdr);
                                    if (op == OP_MOV)
                                    {
                                        if (registers[reg2Idx].type != "") recN1->type = registers[reg2Idx].type;
                                    }
                                    if (!IsValidCodeAdr(varAdr)) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                }
                                else
                                {
                                    recN1 = AddToBSSInfos(varAdr, MakeGvarName(varAdr), registers[reg2Idx].type);
                                    if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                }
                            }
                        }
                        //cop [BaseReg + IndxReg*Scale + Offset], reg
                        else if (DisInfo.BaseReg != -1)
                        {
                            if (DisInfo.IndxReg == -1)
                            {
                                //cop [ebp - Offset], reg
                                if (bpBased && DisInfo.BaseReg == 21 && (int)DisInfo.Offset < 0)
                                {
                                    recN1 = GetInfoRec(fromAdr);
                                    recN1->procInfo->AddLocal((int)DisInfo.Offset, 4, "", registers[reg2Idx].type);
                                }
                                //esp
                                else if (DisInfo.BaseReg == 20)
                                {
                                }
                                //other registers
                                else
                                {
                                    //cop [BaseReg], reg
                                    if (!DisInfo.Offset)
                                    {
                                        varAdr = registers[DisInfo.BaseReg].value;
                                        if (IsValidImageAdr(varAdr))
                                        {
                                            _ap = Adr2Pos(varAdr);
                                            if (_ap >= 0)
                                            {
                                                recN1 = GetInfoRec(varAdr);
                                                if (!recN1) recN1 = new InfoRec(_ap, ikData);
                                                MakeGvar(recN1, varAdr, curAdr);
                                                if (recN1->type == "") recN1->type = registers[reg2Idx].type;
                                                if (!IsValidCodeAdr(varAdr)) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                            }
                                            else
                                            {
                                                recN1 = AddToBSSInfos(varAdr, MakeGvarName(varAdr), registers[reg2Idx].type);
                                                if (recN1) recN1->AddXref('C', fromAdr, curAdr - fromAdr);
                                            }
                                        }
                                        else
                                        {
                                            typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                            if (typeName != "")
                                            {
                                                if (registers[reg2Idx].type != "") sType = registers[reg2Idx].type;
                                                fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                                                if (fInfo)
                                                {
                                                    if (vmt)
                                                    {
                                                        if (CanReplace(fInfo->Type, sType)) fInfo->Type = sType;
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                    }
                                                    else
                                                        delete fInfo;
                                                    AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                                }
                                                else if (vmt)
                                                {
                                                    fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                                                    if (fInfo)
                                                    {
                                                        AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                        AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    //cop [BaseReg + Offset], reg
                                    else if ((int)DisInfo.Offset > 0)
                                    {
                                        typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                        if (typeName != "")
                                        {
                                        	if (registers[reg2Idx].type != "") sType = registers[reg2Idx].type;
                                            fInfo = GetField(typeName, (int)DisInfo.Offset, &vmt, &vmtAdr, "");
                                            if (fInfo)
                                            {
                                                if (vmt)
                                                {
                                                    if (CanReplace(fInfo->Type, sType)) fInfo->Type = sType;
                                                    AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                }
                                                else
                                                    delete fInfo;
                                                //if (vmtAdr) typeName = GetClsName(vmtAdr);
                                                AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                            }
                                            else if (vmt)
                                            {
                                                fInfo = AddField(fromAdr, curAdr - fromAdr, typeName, FIELD_PUBLIC, (int)DisInfo.Offset, -1, "", sType);
                                                if (fInfo)
                                                {
                                                    AddFieldXref(fInfo, fromAdr, curAdr - fromAdr, 'c');
                                                	AddPicode(curPos, 0, typeName, DisInfo.Offset);
                                                }
                                            }
                                        }
                                    }
                                    //cop [BaseReg - Offset], reg
                                    else
                                    {
                                    }
                                }
                            }
                            //cop [BaseReg + IndxReg*Scale + Offset], reg
                            else
                            {
                                //cop [BaseReg + IndxReg*Scale + Offset], reg
                                if (bpBased && DisInfo.BaseReg == 21 && (int)DisInfo.Offset < 0)
                                {
                                }
                                //esp
                                else if (DisInfo.BaseReg == 20)
                                {
                                }
                                //other registers
                                else
                                {
                                    //[BaseReg]
                                    if (!DisInfo.Offset)
                                    {
                                    }
                                    //cop [BaseReg + IndxReg*Scale + Offset], reg
                                    else if ((int)DisInfo.Offset > 0)
                                    {
                                        typeName = TrimTypeName(registers[DisInfo.BaseReg].type);
                                    }
                                    //cop [BaseReg - Offset], reg
                                    else
                                    {
                                    }
                                }
                            }
                        }
                        //Other instructions
                        else
                        {
                        }
                    }
                }
            }
            else if (op == OP_ADC || op == OP_SBB)
            {
                if (DisInfo.OpType[0] == otREG)
                {
                    reg1Idx = DisInfo.OpRegIdx[0];
                    SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                    registers[reg1Idx].type = "";
                }
            }
            else if (op == OP_MUL || op == OP_DIV)
            {
                //Clear register eax
                SetRegisterValue(registers, 16, 0xFFFFFFFF);
                for (n = 0; n <=16; n += 4)
                {
                    if (n == 12) continue;
                    registers[n].type = "";
                }
                //Clear register edx
                SetRegisterValue(registers, 18, 0xFFFFFFFF);
                for (n = 2; n <= 18; n += 4)
                {
                    if (n == 14) continue;
                    registers[n].type = "";
                }
            }
            else
            {
                if (DisInfo.OpType[0] == otREG)
                {
                    reg1Idx = DisInfo.OpRegIdx[0];
                    if ((registers[reg1Idx].source & 0xDF) != 'L') SetRegisterValue(registers, reg1Idx, 0xFFFFFFFF);
                    registers[reg1Idx].type = "";
                }
            }
            //SHL??? SHR???
        }
        curPos += instrLen; curAdr += instrLen;
    }
    return fContinue;
}
//---------------------------------------------------------------------------
String __fastcall TFMain_11011981::AnalyzeTypes(DWORD parentAdr, int callPos, DWORD callAdr, PRINFO registers)
{
    WORD        codePage, elemSize = 1;
    int         n, wBytes, pos, pushn, itemPos, refcnt, len, regIdx;
    int         _idx, _ap, _kind, _size, _pos;
    DWORD		itemAdr, strAdr;
    char        *tmpBuf;
    PInfoRec    recN, recN1;
    PARGINFO	argInfo;
    String      typeDef, typeName, retName, _vs;
    DISINFO		_disInfo;
    char        buf[1024];  //for LoadStr function

    _ap = Adr2Pos(callAdr);
    if (_ap < 0) return "";

    retName = "";
    recN = GetInfoRec(callAdr);
    //If procedure is skipped return
    if (IsFlagSet(cfSkip, callPos))
    {
        //@BeforeDestruction
        if (recN->SameName("@BeforeDestruction")) return registers[16].type;

        return recN->type;
    }

    //cdecl, stdcall
    if (recN->procInfo->flags & 1)
    {
    	if (!recN->procInfo->args || !recN->procInfo->args->Count)
        {
            return recN->type;
        }

        for (pos = callPos, pushn = -1;; pos--)
        {
            if (!IsFlagSet(cfInstruction, pos)) continue;
            if (IsFlagSet(cfProcStart, pos)) break;
            //I cannot yet handle this situation
            if (IsFlagSet(cfCall, pos) && pos != callPos) break;
            if (IsFlagSet(cfPush, pos))
            {
                pushn++;
                if (pushn < recN->procInfo->args->Count)
                {
                    Disasm.Disassemble(Code + pos, (__int64)Pos2Adr(pos), &_disInfo, 0);
                    itemAdr = _disInfo.Immediate;
                    if (IsValidImageAdr(itemAdr))
                    {
                        itemPos = Adr2Pos(itemAdr);
                        argInfo = (PARGINFO)recN->procInfo->args->Items[pushn];
                        typeDef = argInfo->TypeDef;

                        if (SameText(typeDef, "PAnsiChar") || SameText(typeDef, "PChar"))
                        {
                            if (itemPos >= 0)
                            {
                                recN1 = GetInfoRec(itemAdr);
                                if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                                //var - use pointer
                                if (argInfo->Tag == 0x22)
                                {
                                    strAdr = *((DWORD*)(Code + itemPos));
                                    if (!strAdr)
                                    {
                                        SetFlags(cfData, itemPos, 4);
                                        MakeGvar(recN1, itemAdr, Pos2Adr(pos));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                    else
                                    {
                                        _ap = Adr2Pos(strAdr);
                                        if (_ap >= 0)
                                        {
                                            len = strlen((char*)(Code + _ap));
                                            SetFlags(cfData, _ap, len + 1);
                                        }
                                        else if (_ap == -1)
                                        {
                                            recN1 = AddToBSSInfos(strAdr, MakeGvarName(strAdr), typeDef);
                                            if (recN1) recN1->AddXref('C', callAdr, callPos);
                                        }
                                    }
                                }
                                //val
                                else if (argInfo->Tag == 0x21)
                                {
                                    recN1->kind = ikCString;
                                    len = strlen(Code + itemPos);
                                    if (!recN1->HasName())
                                    {
                                        if (IsValidCodeAdr(itemAdr))
                                        {
                                            recN1->SetName(TransformString(Code + itemPos, len));
                                        }
                                        else
                                        {
                                            recN1->SetName(MakeGvarName(itemAdr));
                                            if (typeDef != "") recN1->type = typeDef;
                                        }
                                    }
                                    SetFlags(cfData, itemPos, len + 1);
                                }
                                if (recN1) recN1->ScanUpItemAndAddRef(callPos, itemAdr, 'C', parentAdr);
                            }
                            else
                            {
                                recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                                if (recN1) recN1->AddXref('C', callAdr, callPos);
                            }
                        }
                        else if (SameText(typeDef, "PWideChar"))
                        {
                            if (itemPos)
                            {
                                recN1 = GetInfoRec(itemAdr);
                                if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                                //var - use pointer
                                if (argInfo->Tag == 0x22)
                                {
                                    strAdr = *((DWORD*)(Code + itemPos));
                                    if (!strAdr)
                                    {
                                        SetFlags(cfData, itemPos, 4);
                                        MakeGvar(recN1, itemAdr, Pos2Adr(pos));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                    else
                                    {
                                        _ap = Adr2Pos(strAdr);
                                        if (_ap >= 0)
                                        {
                                            len = wcslen((wchar_t*)(Code + Adr2Pos(strAdr)));
                                            SetFlags(cfData, Adr2Pos(strAdr), 2*len + 1);
                                        }
                                        else if (_ap == -1)
                                        {
                                            recN1 = AddToBSSInfos(strAdr, MakeGvarName(strAdr), typeDef);
                                            if (recN1) recN1->AddXref('C', callAdr, callPos);
                                        }
                                    }
                                }
                                //val
                                else if (argInfo->Tag == 0x21)
                                {
                                    recN1->kind = ikWCString;
                                    len = wcslen((wchar_t*)(Code + itemPos));
                                    if (!recN1->HasName())
                                    {
                                        if (IsValidCodeAdr(itemAdr))
                                        {
                                            WideString wStr = WideString((wchar_t*)(Code + itemPos));
                                            int size = WideCharToMultiByte(CP_ACP, 0, wStr, len, 0, 0, 0, 0);
                                            if (size)
                                            {
                                                tmpBuf = new char[size + 1];
                                                WideCharToMultiByte(CP_ACP, 0, wStr, len, (LPSTR)tmpBuf, size, 0, 0);
                                                recN1->SetName(TransformString(tmpBuf, size));
                                                delete[] tmpBuf;
                                                if (recN->SameName("GetProcAddress")) retName = recN1->GetName();
                                            }
                                        }
                                        else
                                        {
                                            recN1->SetName(MakeGvarName(itemAdr));
                                            if (typeDef != "") recN1->type = typeDef;
                                        }
                                    }
                                    SetFlags(cfData, itemPos, 2*len + 1);
                                }
                                recN1->AddXref('C', callAdr, callPos);
                            }
                            else
                            {
                                recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                                if (recN1) recN1->AddXref('C', callAdr, callPos);
                            }
                        }
                        else if (SameText(typeDef, "TGUID"))
                        {
                            if (itemPos)
                            {
                                recN1 = GetInfoRec(itemAdr);
                                if (!recN1) recN1 = new InfoRec(itemPos, ikGUID);
                                recN1->kind = ikGUID;
                                SetFlags(cfData, itemPos, 16);
                                if (!recN1->HasName())
                                {
                                    if (IsValidCodeAdr(itemAdr))
                                    {
                                        recN1->SetName(Guid2String(Code + itemPos));
                                    }
                                    else
                                    {
                                        recN1->SetName(MakeGvarName(itemAdr));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                }
                                recN1->AddXref('C', callAdr, callPos);
                            }
                            else
                            {
                                recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                                if (recN1) recN1->AddXref('C', callAdr, callPos);
                            }
                        }
                    }
                    if (pushn == recN->procInfo->args->Count - 1) break;
                }
            }
        }
        return recN->type;
    }
    if (recN->HasName())
    {
        if (recN->SameName("LoadStr") || recN->SameName("FmtLoadStr") || recN->SameName("LoadResString"))
        {
            int ident = registers[16].value;  //eax = string ID
            if (ident != -1)
            {
                HINSTANCE hInst = LoadLibraryEx(SourceFile.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
                if (hInst)
                {
                    int bytes = LoadString(hInst, (UINT)ident, buf, 1024);
                    if (bytes) AddPicode(callPos, OP_COMMENT, "'" + String(buf, bytes) + "'", 0);
                    FreeLibrary(hInst);
                }
            }
            return "";
        }
        if (recN->SameName("TApplication.CreateForm"))
        {
            DWORD vmtAdr = registers[18].value + VmtSelfPtr; //edx

            DWORD refAdr = registers[17].value;        //ecx
            if (IsValidImageAdr(refAdr))
            {
                typeName = GetClsName(vmtAdr);
                _ap = Adr2Pos(refAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(refAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikData);
                    MakeGvar(recN1, refAdr, 0);
                    if (typeName != "") recN1->type = typeName;
                }
                else
                {
                    _vs = Val2Str8(refAdr);
                    _idx = BSSInfos->IndexOf(_vs);
                    if (_idx != -1)
                    {
                        recN1 = (PInfoRec)BSSInfos->Objects[_idx];
                        if (typeName != "") recN1->type = typeName;
                    }
                }
            }
            return "";
        }
        if (recN->SameName("@FinalizeRecord"))
        {
            DWORD recAdr = registers[16].value;        //eax
            DWORD recTypeAdr = registers[18].value;    //edx
            typeName = GetTypeName(recTypeAdr);
            //Address given directly
            if (IsValidImageAdr(recAdr))
            {
                _ap = Adr2Pos(recAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(recAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikRecord);
                    MakeGvar(recN1, recAdr, 0);
                    if (typeName != "") recN1->type = typeName;
                    if (!IsValidCodeAdr(recAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(recAdr, MakeGvarName(recAdr), typeName);
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            //Local variable
            else if ((registers[16].source & 0xDF) == 'L')
            {
                if (registers[16].type == "" && typeName != "") registers[16].type = typeName;
                registers[16].result = 1;
            }
            return "";
        }
        if (recN->SameName("@DynArrayAddRef"))
        {
            DWORD arrayAdr = registers[16].value;      //eax
            //Address given directly
            if (IsValidImageAdr(arrayAdr))
            {
                _ap = Adr2Pos(arrayAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(arrayAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikDynArray);
                    MakeGvar(recN1, arrayAdr, 0);
                    if (!IsValidCodeAdr(arrayAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(arrayAdr, MakeGvarName(arrayAdr), "");
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            //Local variable
            else if ((registers[16].source & 0xDF) == 'L')
            {
                if (registers[16].type == "") registers[16].type = "array of ?";
                registers[16].result = 1;
            }
            return "";
        }
        if (recN->SameName("DynArrayClear")     ||
            recN->SameName("@DynArrayClear")    ||
            recN->SameName("DynArraySetLength") ||
            recN->SameName("@DynArraySetLength"))
        {
            DWORD arrayAdr = registers[16].value;      //eax
            DWORD elTypeAdr = registers[18].value;     //edx
            typeName = GetTypeName(elTypeAdr);
            //Address given directly
            if (IsValidImageAdr(arrayAdr))
            {
                _ap = Adr2Pos(arrayAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(arrayAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikDynArray);
                    MakeGvar(recN1, arrayAdr, 0);
                    if (recN1->type == "" && typeName != "") recN1->type = typeName;
                    if (!IsValidCodeAdr(arrayAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(arrayAdr, MakeGvarName(arrayAdr), typeName);
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            //Local variable
            else if ((registers[16].source & 0xDF) == 'L')
            {
                if (registers[16].type == "" && typeName != "") registers[16].type = typeName;
                registers[16].result = 1;
            }
            return "";
        }
        if (recN->SameName("@DynArrayCopy"))
        {
            DWORD arrayAdr = registers[16].value;      //eax
            DWORD elTypeAdr = registers[18].value;     //edx
            DWORD dstArrayAdr = registers[17].value;   //ecx
            typeName = GetTypeName(elTypeAdr);
            //Address given directly
            if (IsValidImageAdr(arrayAdr))
            {
                _ap = Adr2Pos(arrayAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(arrayAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikDynArray);
                    MakeGvar(recN1, arrayAdr, 0);
                    if (typeName != "") recN1->type = typeName;
                    if (!IsValidCodeAdr(arrayAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(arrayAdr, MakeGvarName(arrayAdr), typeName);
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            //Local variable
            else if ((registers[16].source & 0xDF) == 'L')
            {
                if (registers[16].type == "" && typeName != "") registers[16].type = typeName;
                registers[16].result = 1;
            }
            //Address given directly
            if (IsValidImageAdr(dstArrayAdr))
            {
                _ap = Adr2Pos(dstArrayAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(dstArrayAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikDynArray);
                    MakeGvar(recN1, dstArrayAdr, 0);
                    if (typeName != "") recN1->type = typeName;
                    if (!IsValidCodeAdr(dstArrayAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(dstArrayAdr, MakeGvarName(dstArrayAdr), typeName);
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            //Local variable
            else if ((registers[17].source & 0xDF) == 'L')
            {
                if (registers[17].type == "" && typeName != "") registers[17].type = typeName;
                registers[17].result = 1;
            }
            return "";
        }
        if (recN->SameName("@IntfClear"))
        {
            DWORD intfAdr = registers[16].value;       //eax

            if (IsValidImageAdr(intfAdr))
            {
                _ap = Adr2Pos(intfAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(intfAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikInterface);
                    MakeGvar(recN1, intfAdr, 0);
                    recN1->type = "IInterface";
                    if (!IsValidCodeAdr(intfAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(intfAdr, MakeGvarName(intfAdr), "IInterface");
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            return "";
        }
        if (recN->SameName("@FinalizeArray"))
        {
            DWORD arrayAdr = registers[16].value;      //eax
            int elNum = registers[17].value;           //ecx
            DWORD elTypeAdr = registers[18].value;     //edx

            if (IsValidImageAdr(arrayAdr))
            {
                typeName = "array[" + String(elNum) + "] of " + GetTypeName(elTypeAdr);
                _ap = Adr2Pos(arrayAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(arrayAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikArray);
                    MakeGvar(recN1, arrayAdr, 0);
                    recN1->type = typeName;
                    if (!IsValidCodeAdr(arrayAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(arrayAdr, MakeGvarName(arrayAdr), typeName);
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            return "";
        }
        if (recN->SameName("@VarClr"))
        {
            DWORD strAdr = registers[16].value;        //eax
            if (IsValidImageAdr(strAdr))
            {
                _ap = Adr2Pos(strAdr);
                if (_ap >= 0)
                {
                    recN1 = GetInfoRec(strAdr);
                    if (!recN1) recN1 = new InfoRec(_ap, ikVariant);
                    MakeGvar(recN1, strAdr, 0);
                    recN1->type = "Variant";
                    if (!IsValidCodeAdr(strAdr)) recN1->AddXref('C', callAdr, callPos);
                }
                else
                {
                    recN1 = AddToBSSInfos(strAdr, MakeGvarName(strAdr), "Variant");
                    if (recN1) recN1->AddXref('C', callAdr, callPos);
                }
            }
            return "";
        }
        //@AsClass
        if (recN->SameName("@AsClass"))
        {
            return registers[18].type;
        }
        //@IsClass
        if (recN->SameName("@IsClass"))
        {
            return "";
        }
        //@GetTls
        if (recN->SameName("@GetTls"))
        {
            return "#TLS";
        }
        //@AfterConstruction
        if (recN->SameName("@AfterConstruction")) return "";
    }
    //try prototype
    BYTE callKind = recN->procInfo->flags & 7;
    if (recN->procInfo->args && !callKind)
    {
        registers[16].result = 0;
        registers[17].result = 0;
        registers[18].result = 0;

        for (n = 0; n < recN->procInfo->args->Count; n++)
        {
            argInfo = (PARGINFO)recN->procInfo->args->Items[n];
            regIdx = -1;
            if (argInfo->Ndx == 0)         //eax
                regIdx = 16;
            else if (argInfo->Ndx == 1)    //edx
                regIdx = 18;
            else if (argInfo->Ndx == 2)    //ecx
                regIdx = 17;
            if (regIdx == -1) continue;
            if (argInfo->TypeDef == "")
            {
            	if (registers[regIdx].type != "")
                    argInfo->TypeDef = TrimTypeName(registers[regIdx].type);
            }
            else
            {
            	if (registers[regIdx].type == "")
                {
                    registers[regIdx].type = argInfo->TypeDef;
                    //registers[regIdx].result = 1;
                }
                else
                {
                	typeName = GetCommonType(argInfo->TypeDef, TrimTypeName(registers[regIdx].type));
                    if (typeName != "") argInfo->TypeDef = typeName;
                }
                //Aliases ???????????
            }

            typeDef = argInfo->TypeDef;
            //Local var (lea - remove ^ before type)
            if (registers[regIdx].source == 'L')
            {
            	if (SameText(typeDef, "Pointer"))
                	registers[regIdx].type = "Byte";
                else if (SameText(typeDef, "PAnsiChar") || SameText(typeDef, "PChar"))
                    registers[regIdx].type = typeDef.SubString(2, typeDef.Length() - 1);
                else if (DelphiVersion >= 2009 && SameText(typeDef, "AnsiString"))
                	registers[regIdx].type = "UnicodeString";

                registers[regIdx].result = 1;
                continue;
            }
            //Local var
            if (registers[regIdx].source == 'l')
            {
            	continue;
            }
            //Arg
            if ((registers[regIdx].source & 0xDF) == 'A')
            {
                continue;
            }
            itemAdr = registers[regIdx].value;
            if (IsValidImageAdr(itemAdr))
            {
                itemPos = Adr2Pos(itemAdr);
                if (itemPos >= 0)
                {
                    recN1 = GetInfoRec(itemAdr);
                    if (!recN1 || recN1->kind != ikVMT)
                    {
                        registers[regIdx].result = 1;

                        if (SameText(typeDef, "PShortString") || SameText(typeDef, "ShortString"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                            //var - use pointer
                            if (argInfo->Tag == 0x22)
                            {
                                strAdr = *((DWORD*)(Code + itemPos));
                                if (IsValidCodeAdr(strAdr))
                                {
                                    _ap = Adr2Pos(strAdr);
                                    len = *(Code + _ap);
                                    SetFlags(cfData, _ap, len + 1);
                                }
                                else
                                {
                                    SetFlags(cfData, itemPos, 4);
                                    MakeGvar(recN1, itemAdr, 0);
                                    if (typeDef != "") recN1->type = typeDef;
                                }
                            }
                            //val
                            else if (argInfo->Tag == 0x21)
                            {
                                recN1->kind = ikString;
                                len = *(Code + itemPos);
                                if (!recN1->HasName())
                                {
                                    if (IsValidCodeAdr(itemAdr))
                                    {
                                        recN1->SetName(TransformString(Code + itemPos + 1, len));
                                    }
                                    else
                                    {
                                        recN1->SetName(MakeGvarName(itemAdr));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                }
                                SetFlags(cfData, itemPos, len + 1);
                            }
                        }
                        else if (SameText(typeDef, "PAnsiChar") || SameText(typeDef, "PChar"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                            //var - use pointer
                            if (argInfo->Tag == 0x22)
                            {
                                strAdr = *((DWORD*)(Code + itemPos));
                                if (IsValidCodeAdr(strAdr))
                                {
                                    _ap = Adr2Pos(strAdr);
                                    len = strlen(Code + _ap);
                                    SetFlags(cfData, _ap, len + 1);
                                }
                                else
                                {
                                    SetFlags(cfData, itemPos, 4);
                                    MakeGvar(recN1, itemAdr, 0);
                                    if (typeDef != "") recN1->type = typeDef;
                                }
                            }
                            //val
                            else if (argInfo->Tag == 0x21)
                            {
                                recN1->kind = ikCString;
                                len = strlen(Code + itemPos);
                                if (!recN1->HasName())
                                {
                                    if (IsValidCodeAdr(itemAdr))
                                    {
                                        recN1->SetName(TransformString(Code + itemPos, len));
                                    }
                                    else
                                    {
                                        recN1->SetName(MakeGvarName(itemAdr));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                }
                                SetFlags(cfData, itemPos, len + 1);
                            }
                        }
                        else if (SameText(typeDef, "AnsiString") ||
                                 SameText(typeDef, "String")     ||
                                 SameText(typeDef, "UString")    ||
                                 SameText(typeDef, "UnicodeString"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                            //var - use pointer
                            if (argInfo->Tag == 0x22)
                            {
                                strAdr = *((DWORD*)(Code + itemPos));
                                _ap = Adr2Pos(strAdr);
                                if (IsValidCodeAdr(strAdr))
                                {
                                    refcnt = *((int*)(Code + _ap - 8));
                                    len = *((int*)(Code + _ap - 4));
                                    if (refcnt == -1 && len >= 0 && len < 25000)
                                    {
                                        if (DelphiVersion < 2009)
                                        {
                                            SetFlags(cfData, _ap - 8, (8 + len + 1 + 3) & (-4));
                                        }
                                        else
                                        {
                                            codePage = *((WORD*)(Code + _ap - 12));
                                            elemSize = *((WORD*)(Code + _ap - 10));
                                            SetFlags(cfData, _ap - 12, (12 + (len + 1)*elemSize + 3) & (-4));
                                        }
                                    }
                                    else
                                    {
                                        SetFlags(cfData, _ap, 4);
                                    }
                                }
                                else
                                {
                                    if (_ap >= 0)
                                    {
                                        SetFlags(cfData, itemPos, 4);
                                        MakeGvar(recN1, itemAdr, 0);
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                    else if (_ap == -1)
                                    {
                                        recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                                    }
                                }
                            }
                            //val
                            else if (argInfo->Tag == 0x21)
                            {
                                refcnt = *((int*)(Code + itemPos - 8));
                                len = wcslen((wchar_t*)(Code + itemPos));
                                if (DelphiVersion < 2009)
                                {
                                    recN1->kind = ikLString;
                                }
                                else
                                {
                                    codePage = *((WORD*)(Code + itemPos - 12));
                                    elemSize = *((WORD*)(Code + itemPos - 10));
                                    recN1->kind = ikUString;
                                }
                                if (refcnt == -1 && len >= 0 && len < 25000)
                                {
                                    if (!recN1->HasName())
                                    {
                                        if (IsValidCodeAdr(itemAdr))
                                        {
                                            if (DelphiVersion < 2009)
                                                recN1->SetName(TransformString(Code + itemPos, len));
                                            else
                                                recN1->SetName(TransformUString(codePage, (wchar_t*)(Code + itemPos), len));
                                        }
                                        else
                                        {
                                            recN1->SetName(MakeGvarName(itemAdr));
                                            if (typeDef != "") recN1->type = typeDef;
                                        }
                                    }
                                    if (DelphiVersion < 2009)
                                        SetFlags(cfData, itemPos - 8, (8 + len + 1 + 3) & (-4));
                                    else
                                        SetFlags(cfData, itemPos - 12, (12 + (len + 1)*elemSize + 3) & (-4));
                                }
                                else
                                {
                                    if (!recN1->HasName())
                                    {
                                        if (IsValidCodeAdr(itemAdr))
                                        {
                                            recN1->SetName("");
                                        }
                                        else
                                        {
                                            recN1->SetName(MakeGvarName(itemAdr));
                                            if (typeDef != "") recN1->type = typeDef;
                                        }
                                    }
                                    SetFlags(cfData, itemPos, 4);
                                }
                            }
                        }
                        else if (SameText(typeDef, "WideString"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                            //var - use pointer
                            if (argInfo->Tag == 0x22)
                            {
                                strAdr = *((DWORD*)(Code + itemPos));
                                _ap = Adr2Pos(strAdr);
                                if (IsValidCodeAdr(strAdr))
                                {
                                    len = *((int*)(Code + _ap - 4));
                                    SetFlags(cfData, _ap - 4, (4 + len + 1 + 3) & (-4));
                                }
                                else
                                {
                                    if (_ap >= 0)
                                    {
                                        SetFlags(cfData, itemPos, 4);
                                        MakeGvar(recN1, itemAdr, 0);
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                    else if (_ap == -1)
                                    {
                                        recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                                    }
                                }
                            }
                            //val
                            else if (argInfo->Tag == 0x21)
                            {
                                recN1->kind = ikWString;
                                len = wcslen((wchar_t*)(Code + itemPos));
                                if (!recN1->HasName())
                                {
                                    if (IsValidCodeAdr(itemAdr))
                                    {
                                        WideString wStr = WideString((wchar_t*)(Code + itemPos));
                                        int size = WideCharToMultiByte(CP_ACP, 0, wStr, len, 0, 0, 0, 0);
                                        if (size)
                                        {
                                            tmpBuf = new BYTE[size + 1];
                                            WideCharToMultiByte(CP_ACP, 0, wStr, len, (LPSTR)tmpBuf, size, 0, 0);
                                            recN1->SetName(TransformString(tmpBuf, size));    //???size - 1
                                            delete[] tmpBuf;
                                        }
                                    }
                                    else
                                    {
                                        recN1->SetName(MakeGvarName(itemAdr));
                                        if (typeDef != "") recN1->type = typeDef;
                                    }
                                }
                                SetFlags(cfData, itemPos - 4, (4 + len + 1 + 3) & (-4));
                            }
                        }
                        else if (SameText(typeDef, "TGUID"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikGUID);
                            recN1->kind = ikGUID;
                            SetFlags(cfData, itemPos, 16);
                            if (!recN1->HasName())
                            {
                                if (IsValidCodeAdr(itemAdr))
                                {
                                    recN1->SetName(Guid2String(Code + itemPos));
                                }
                                else
                                {
                                    recN1->SetName(MakeGvarName(itemAdr));
                                    if (typeDef != "") recN1->type = typeDef;
                                }
                            }
                        }
                        else if (SameText(typeDef, "PResStringRec"))
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1)
                            {
                                recN1 = new InfoRec(itemPos, ikResString);
                                recN1->type = "TResStringRec";
                                recN1->ConcatName("SResString" + String(LastResStrNo));
                                LastResStrNo++;
                                //Set Flags
                                SetFlags(cfData, itemPos, 8);
                                //Get Context
                                HINSTANCE hInst = LoadLibraryEx(SourceFile.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
                                if (hInst)
                                {
                                    DWORD resid = *((DWORD*)(Code + itemPos + 4));
                                    if (resid < 0x10000)
                                    {
                                        int Bytes = LoadString(hInst, (UINT)resid, buf, 1024);
                                        recN1->rsInfo->value = String(buf, Bytes);
                                    }
                                    FreeLibrary(hInst);
                                }
                            }
                        }
                        else
                        {
                            recN1 = GetInfoRec(itemAdr);
                            if (!recN1) recN1 = new InfoRec(itemPos, ikData);
                            if (!recN1->HasName()            &&
                                recN1->kind != ikProc        &&
                                recN1->kind != ikFunc        &&
                                recN1->kind != ikConstructor &&
                                recN1->kind != ikDestructor  &&
                                recN1->kind != ikRefine)
                            {
                                if (typeDef != "") recN1->type = typeDef;
                            }
                        }
                    }
                }
                else
                {
                    _kind = GetTypeKind(typeDef, &_size);
                    if (_kind == ikInteger     ||
                        _kind == ikChar        ||
                        _kind == ikEnumeration ||
                        _kind == ikFloat       ||
                        _kind == ikSet         ||
                        _kind == ikWChar)
                    {
                        _idx = BSSInfos->IndexOf(Val2Str8(itemAdr));
                        if (_idx != -1)
                        {
                            recN1 = (PInfoRec)BSSInfos->Objects[_idx];
                            delete recN1;
                            BSSInfos->Delete(_idx);
                        }
                    }
                    else
                    {
                        recN1 = AddToBSSInfos(itemAdr, MakeGvarName(itemAdr), typeDef);
                    }
                }
            }
        }
    }
    if (recN->kind == ikFunc)
    {
        return recN->type;
    }
    return "";
}
//---------------------------------------------------------------------------
