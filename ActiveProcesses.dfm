object FActiveProcesses: TFActiveProcesses
  Left = 390
  Top = 440
  BorderStyle = bsSingle
  Caption = 'Active Processes'
  ClientHeight = 310
  ClientWidth = 792
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
  object btnDump: TButton
    Left = 171
    Top = 286
    Width = 61
    Height = 20
    Caption = 'Dump'
    TabOrder = 1
    OnClick = btnDumpClick
  end
  object btnCancel: TButton
    Left = 561
    Top = 286
    Width = 61
    Height = 20
    Caption = 'Cancel'
    TabOrder = 2
    OnClick = btnCancelClick
  end
  object lvProcesses: TListView
    Left = 0
    Top = 0
    Width = 792
    Height = 280
    Align = alTop
    Columns = <
      item
        Caption = 'PID'
        Width = 65
      end
      item
        Caption = 'Name'
        Width = 325
      end
      item
        Caption = 'Image Size'
        Width = 122
      end
      item
        Caption = 'EP'
        Width = 122
      end
      item
        Caption = 'Base'
        Width = 122
      end>
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Courier New'
    Font.Style = []
    RowSelect = True
    ParentFont = False
    TabOrder = 0
    ViewStyle = vsReport
    OnClick = lvProcessesClick
  end
end
