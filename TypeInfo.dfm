object FTypeInfo_11011981: TFTypeInfo_11011981
  Left = 267
  Top = 331
  Width = 700
  Height = 434
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -14
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  PixelsPerInch = 120
  TextHeight = 16
  object memDescription: TMemo
    Left = 0
    Top = 0
    Width = 692
    Height = 353
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -15
    Font.Name = 'Fixedsys'
    Font.Style = []
    ParentFont = False
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 0
  end
  object Panel1: TPanel
    Left = 0
    Top = 353
    Width = 692
    Height = 41
    Align = alBottom
    TabOrder = 1
    Visible = False
    object bSave: TButton
      Left = 309
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Save'
      TabOrder = 0
      OnClick = bSaveClick
    end
  end
end
