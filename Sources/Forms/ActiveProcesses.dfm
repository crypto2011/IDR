object FActiveProcesses: TFActiveProcesses
  Left = 390
  Top = 440
  BorderStyle = bsSingle
  Caption = 'Active Processes (x86)'
  ClientHeight = 310
  ClientWidth = 640
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -10
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object ListViewProcesses: TListView
    Left = 0
    Top = 0
    Width = 640
    Height = 310
    Align = alClient
    BorderStyle = bsNone
    Columns = <
      item
        Caption = 'PID'
      end
      item
        Caption = 'Name'
        Width = 200
      end
      item
        Caption = 'Image Size'
        Width = 120
      end
      item
        Caption = 'Entry Point'
        Width = 120
      end
      item
        Caption = 'Base Address'
        Width = 120
      end>
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Courier New'
    Font.Style = []
    ReadOnly = True
    RowSelect = True
    ParentFont = False
    PopupMenu = PMProcess
    TabOrder = 0
    ViewStyle = vsReport
  end
  object PMProcess: TPopupMenu
    OnPopup = PMProcessPopup
    Left = 40
    Top = 40
    object PMProcessDump: TMenuItem
      Caption = 'Dump'
      OnClick = PMProcessDumpClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object PMProcessRefresh: TMenuItem
      Caption = 'Refresh'
      OnClick = PMProcessRefreshClick
    end
  end
end
