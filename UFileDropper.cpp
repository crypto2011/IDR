//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "UFileDropper.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------
TDragDropHelper::TDragDropHelper(const HWND h)
    :wndHandle(h)
{
    ::DragAcceptFiles(wndHandle, true);
}
//---------------------------------------------------------------------------
TDragDropHelper::~TDragDropHelper()
{
    ::DragAcceptFiles(wndHandle, false);
}
//---------------------------------------------------------------------------
__fastcall TFileDropper::TFileDropper(HDROP aDropHandle)
        :DropHandle(aDropHandle)
{
}
//---------------------------------------------------------------------------
__fastcall TFileDropper::~TFileDropper()
{
    ::DragFinish(DropHandle);
}
//---------------------------------------------------------------------------
String TFileDropper::GetFile(int Index)
{
    const int FileNameLength = ::DragQueryFile(DropHandle, Index, 0, 0);
    
    String res;
    res.SetLength(FileNameLength);
    ::DragQueryFile(DropHandle, Index, res.c_str(), FileNameLength + 1);

    return res;
}
//---------------------------------------------------------------------------
int TFileDropper::GetFileCount()
{
    return ::DragQueryFile(DropHandle, 0xFFFFFFFF, 0, 0);
}
//---------------------------------------------------------------------------
TPoint TFileDropper::GetPoint()
{
    TPoint res(0,0);
    DragQueryPoint(DropHandle, &res);
    return res;
}
//---------------------------------------------------------------------------
