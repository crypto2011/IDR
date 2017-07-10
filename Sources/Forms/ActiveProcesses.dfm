object FActiveProcesses: TFActiveProcesses
  Left = 390
  Top = 440
  BorderStyle = bsSingle
  Caption = 'Active Processes'
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
        Caption = 'EP'
        Width = 120
      end
      item
        Caption = 'Base'
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
    ExplicitWidth = 609
    ExplicitHeight = 300
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
