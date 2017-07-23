//////////////////////////
//Highlight SDK         //
//Version: 1.2.0        //
//Date   : 03.03.2015   //
//Author : Error13Tracer//
//////////////////////////

#ifndef HighlightH
#define HighlightH

const LPCSTR HighlightDll = "Highlight.dll";

//Языки
const ID_UNKNOWN   = 0xFFFFFFFF;
const ID_DELPHI    = 0x00000000;
const ID_ASM       = 0x00000001;
const ID_HEX       = 0x00000002;
//Тема из конфига
const ID_CFG_THEME = 0xFFFFFFFF;

//Установка подсветки
typedef  DWORD (__stdcall * PCreateHighlight)  (HWND, DWORD);
//Хендл ListBox
//Язык  подсветки
//Возврат - ID контрола

//Отрисовка в событии OnDrawItem
typedef  void  (__stdcall * PHighlightDrawItem)(DWORD, int, TRect, bool);
//ID контрола
//Index
//Rect
//Lighten (true - осветление, false - рисовать согласно теме)

//Перерисовка контрола
typedef  void  (__stdcall * PHighlightRedraw)  (DWORD);
//ID контрола

//Установка языка
typedef  void  (__stdcall * PChangeLanguage)   (DWORD, DWORD);
//ID контрола
//Язык

//Установка темы
typedef  void  (__stdcall * PChangeTheme)      (DWORD, DWORD);
//ID контрола
//ID темы

//Установка глабальной темы с сохранением в конфиг
typedef  void  (__stdcall * PChangeGlobalTheme)(DWORD, DWORD);
//Язык
//ID темы

//Удаление подсветки
typedef  void  (__stdcall * PDeleteHighlight)  (DWORD);
//ID контрола

//ID текущей темы
typedef  DWORD (__stdcall * PGetThemeId)       (DWORD);
//ID контрола
//Возврат - ID темы

//Кол-во тем для языка
typedef  DWORD (__stdcall * PGetThemesCount)   (DWORD);
//Язык
//Возврат - кол-во тем

//Имя темы
typedef  DWORD (__stdcall * PGetThemeName)     (DWORD, DWORD, LPCSTR, DWORD);
//Язык
//ID темы
//Буфер
//Размер буфера
//Возврат - длина имени темы; Буфер - имя темы

//Диалог настроек
typedef  int   (__stdcall * PSettingsShowModal)(HWND);
//Хендл окна, поверх которого отображать
//Возврат - ModalResult

PCreateHighlight   CreateHighlight;
PHighlightDrawItem HighlightDrawItem;
PHighlightRedraw   HighlightRedraw;
PChangeLanguage    ChangeLanguage;
PChangeTheme       ChangeTheme;
PChangeGlobalTheme ChangeGlobalTheme;
PDeleteHighlight   DeleteHighlight;
PGetThemeId        GetThemeId;
PGetThemesCount    GetThemesCount;
PGetThemeName      GetThemeName;
PSettingsShowModal SettingsShowModal;

HMODULE hHighlight = NULL;

static bool InitHighlight(){
  if (hHighlight == NULL){
    hHighlight = LoadLibraryA(HighlightDll);
  }
  if (hHighlight != NULL) {
    CreateHighlight   = (PCreateHighlight)  GetProcAddress(hHighlight, /*0x01*/"CreateHighlight@8");
    HighlightDrawItem = (PHighlightDrawItem)GetProcAddress(hHighlight, /*0x02*/"HighlightDrawItem@16");
    HighlightRedraw   = (PHighlightRedraw)  GetProcAddress(hHighlight, /*0x03*/"HighlightRedraw@4");
    ChangeLanguage    = (PChangeLanguage)   GetProcAddress(hHighlight, /*0x04*/"ChangeLanguage@8");
    ChangeTheme       = (PChangeTheme)      GetProcAddress(hHighlight, /*0x05*/"ChangeTheme@8");
    ChangeGlobalTheme = (PChangeGlobalTheme)GetProcAddress(hHighlight, /*0x06*/"ChangeGlobalTheme@8");
    DeleteHighlight   = (PDeleteHighlight)  GetProcAddress(hHighlight, /*0x07*/"DeleteHighlight@4");
    GetThemeId        = (PGetThemeId)       GetProcAddress(hHighlight, /*0x08*/"GetThemeId@4");
    GetThemesCount    = (PGetThemesCount)   GetProcAddress(hHighlight, /*0x09*/"GetThemesCount@4");
    GetThemeName      = (PGetThemeName)     GetProcAddress(hHighlight, /*0x0A*/"GetThemeName@16");
    SettingsShowModal = (PSettingsShowModal)GetProcAddress(hHighlight, /*0x0B*/"SettingsShowModal@4");
    return ((CreateHighlight != NULL)&(ChangeLanguage    != NULL)&(ChangeTheme       != NULL)&
            (GetThemesCount  != NULL)&(DeleteHighlight   != NULL)&(HighlightDrawItem != NULL)&
            (HighlightRedraw != NULL)&(GetThemeName      != NULL)&(ChangeGlobalTheme != NULL)&
            (GetThemeId      != NULL)&(SettingsShowModal != NULL));
  }else
    return false;
}

static void FreeHighlight(){
  if (hHighlight != 0) {
    FreeLibrary(hHighlight);
  }
}

#endif
