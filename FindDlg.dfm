object FindDlg_11011981: TFindDlg_11011981
  Left = 487
  Top = 325
  BorderStyle = bsToolWindow
  Caption = 'Find'
  ClientHeight = 118
  ClientWidth = 473
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object Bevel1: TBevel
    Left = 10
    Top = 10
    Width = 424
    Height = 60
    Shape = bsFrame
  end
  object Label1: TLabel
    Left = 30
    Top = 30
    Width = 67
    Height = 16
    Caption = 'Text to find:'
  end
  object OKBtn: TButton
    Left = 142
    Top = 84
    Width = 92
    Height = 30
    Cursor = crHandPoint
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
  end
  object CancelBtn: TButton
    Left = 240
    Top = 84
    Width = 92
    Height = 30
    Cursor = crHandPoint
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object cbText: TComboBox
    Left = 108
    Top = 25
    Width = 307
    Height = 24
    ItemHeight = 16
    TabOrder = 2
    OnEnter = cbTextEnter
  end
end
