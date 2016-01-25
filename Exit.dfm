object FExit_11011981: TFExit_11011981
  Left = 482
  Top = 333
  BorderStyle = bsDialog
  Caption = 'Save Project'
  ClientHeight = 156
  ClientWidth = 385
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 120
  TextHeight = 16
  object Bevel1: TBevel
    Left = 10
    Top = 10
    Width = 365
    Height = 100
    Shape = bsFrame
  end
  object OKBtn: TButton
    Left = 78
    Top = 123
    Width = 92
    Height = 31
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
    OnClick = OKBtnClick
  end
  object CancelBtn: TButton
    Left = 215
    Top = 123
    Width = 93
    Height = 31
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
    OnClick = CancelBtnClick
  end
  object cbDontSaveProject: TCheckBox
    Left = 64
    Top = 49
    Width = 257
    Height = 21
    Caption = 'Don'#39't save Project'
    TabOrder = 2
  end
end
