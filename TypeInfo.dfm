object FTypeInfo_11011981: TFTypeInfo_11011981
  Left = 267
  Top = 331
  Width = 700
  Height = 434
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  PixelsPerInch = 96
  TextHeight = 13
  object memDescription: TMemo
    Left = 0
    Top = 0
    Width = 684
    Height = 363
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Fixedsys'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 0
  end
  object Panel1: TPanel
    Left = 0
    Top = 363
    Width = 684
    Height = 33
    Align = alBottom
    TabOrder = 1
    Visible = False
    object bSave: TButton
      Left = 251
      Top = 7
      Width = 61
      Height = 20
      Caption = 'Save'
      TabOrder = 0
      OnClick = bSaveClick
    end
  end
end
