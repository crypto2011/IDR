//---------------------------------------------------------------------------
#ifndef UFileDropperH
#define UFileDropperH

#include <classes.hpp>

//
//simple class that allow/disallow drag-drop support for window
//
class TDragDropHelper
{
public:
    TDragDropHelper(const HWND);
    ~TDragDropHelper();
private:
    const HWND wndHandle;
};

//
//simple class that accepts dropped files and stores names + droppoint
//
class TFileDropper : public TObject
{
public:
    __fastcall TFileDropper(HDROP );
    virtual __fastcall ~TFileDropper();

    __property int FileCount                = {read=GetFileCount};
    __property String Files[int Index]      = {read=GetFile};
    __property TPoint DropPoint             = {read=GetPoint};
    
private:
    HDROP       DropHandle;
    String      GetFile(int Index);
    int         GetFileCount();
    TPoint      GetPoint();
};

//---------------------------------------------------------------------------
#endif

