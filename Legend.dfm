object FLegend_11011981: TFLegend_11011981
  Left = 452
  Top = 306
  BorderStyle = bsDialog
  Caption = 'Legend'
  ClientHeight = 280
  ClientWidth = 302
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
  object gb1: TGroupBox
    Left = 5
    Top = 4
    Width = 289
    Height = 129
    Caption = 'Unit colors'
    TabOrder = 0
    object Label1: TLabel
      Left = 97
      Top = 23
      Width = 135
      Height = 16
      Caption = 'Standard unit (from KB)'
    end
    object Label2: TLabel
      Left = 97
      Top = 49
      Width = 52
      Height = 16
      Caption = 'User unit'
    end
    object Label3: TLabel
      Left = 97
      Top = 75
      Width = 60
      Height = 16
      Caption = 'Trivial unit'
    end
    object Label5: TLabel
      Left = 97
      Top = 102
      Width = 156
      Height = 16
      Caption = 'Unrecognized bytes in unit'
    end
    object lblUnitStd: TLabel
      Left = 9
      Top = 23
      Width = 80
      Height = 20
      AutoSize = False
      Caption = 'system'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object lblUnitUser: TLabel
      Left = 9
      Top = 48
      Width = 80
      Height = 20
      AutoSize = False
      Caption = 'Unit5'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object lblUnitTrivial: TLabel
      Left = 9
      Top = 74
      Width = 80
      Height = 20
      AutoSize = False
      Caption = 'Unit25'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object lblUnitUserUnk: TLabel
      Left = 9
      Top = 100
      Width = 80
      Height = 19
      AutoSize = False
      Caption = 'dlgPass'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
  end
  object gb2: TGroupBox
    Left = 5
    Top = 138
    Width = 289
    Height = 103
    Caption = 'Unit types'
    TabOrder = 1
    object Label4: TLabel
      Left = 97
      Top = 23
      Width = 160
      Height = 16
      Caption = 'Non trivial Initializaiton proc'
    end
    object Label6: TLabel
      Left = 97
      Top = 49
      Width = 159
      Height = 16
      Caption = 'Non trivial Finalization proc'
    end
    object Label7: TLabel
      Left = 97
      Top = 75
      Width = 54
      Height = 16
      Caption = 'ask me :)'
    end
    object lblInit: TLabel
      Left = 9
      Top = 23
      Width = 80
      Height = 20
      AutoSize = False
      Caption = 'I'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object lblFin: TLabel
      Left = 9
      Top = 48
      Width = 80
      Height = 20
      AutoSize = False
      Caption = 'F'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
    object lblUnk: TLabel
      Left = 9
      Top = 74
      Width = 80
      Height = 20
      AutoSize = False
      Caption = '?'
      Color = clWhite
      Font.Charset = RUSSIAN_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'Fixedsys'
      Font.Style = []
      ParentColor = False
      ParentFont = False
    end
  end
  object btnOK: TButton
    Left = 101
    Top = 246
    Width = 92
    Height = 31
    Cursor = crHandPoint
    Caption = 'OK'
    ModalResult = 1
    TabOrder = 2
  end
end
