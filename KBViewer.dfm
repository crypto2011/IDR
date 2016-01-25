object FKBViewer_11011981: TFKBViewer_11011981
  Left = 227
  Top = 178
  BorderStyle = bsToolWindow
  Caption = 'KBViewer'
  ClientHeight = 696
  ClientWidth = 1257
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object Splitter1: TSplitter
    Left = 465
    Top = 33
    Width = 3
    Height = 693
    Cursor = crHSplit
    Align = alNone
  end
  object lbKB: TListBox
    Left = 0
    Top = 41
    Width = 590
    Height = 655
    AutoComplete = False
    Align = alLeft
    Color = 15597544
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Fixedsys'
    Font.Style = []
    ItemHeight = 20
    MultiSelect = True
    ParentFont = False
    TabOrder = 0
  end
  object lbIDR: TListBox
    Left = 590
    Top = 41
    Width = 590
    Height = 655
    AutoComplete = False
    Align = alClient
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Fixedsys'
    Font.Style = []
    ItemHeight = 20
    MultiSelect = True
    ParentFont = False
    TabOrder = 1
  end
  object Panel3: TPanel
    Left = 1180
    Top = 41
    Width = 77
    Height = 655
    Align = alRight
    TabOrder = 3
    object lPosition: TLabel
      Left = 8
      Top = 292
      Width = 64
      Height = 16
      Alignment = taCenter
      AutoSize = False
      Caption = 'Position'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object lKBIdx: TLabel
      Left = 5
      Top = 12
      Width = 64
      Height = 16
      Alignment = taCenter
      AutoSize = False
      Caption = 'KB Index'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -15
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object bPrev: TButton
      Left = 8
      Top = 229
      Width = 64
      Height = 30
      Caption = 'Prev'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      OnClick = bPrevClick
    end
    object bNext: TButton
      Left = 8
      Top = 349
      Width = 64
      Height = 30
      Caption = 'Next'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 1
      OnClick = bNextClick
    end
    object btnOk: TButton
      Left = 8
      Top = 587
      Width = 64
      Height = 30
      Cursor = crHandPoint
      Caption = 'Use'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 3
      OnClick = btnOkClick
    end
    object btnCancel: TButton
      Left = 8
      Top = 619
      Width = 64
      Height = 30
      Cursor = crHandPoint
      Caption = 'Cancel'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
      TabOrder = 4
      OnClick = btnCancelClick
    end
    object edtCurrIdx: TEdit
      Left = 6
      Top = 32
      Width = 65
      Height = 24
      TabOrder = 2
      OnChange = edtCurrIdxChange
    end
  end
  object Panel1: TPanel
    Left = 0
    Top = 0
    Width = 1257
    Height = 41
    Align = alTop
    TabOrder = 2
    object Label1: TLabel
      Left = 176
      Top = 11
      Width = 55
      Height = 16
      Caption = 'KB Unit:'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object lblKbIdxs: TLabel
      Left = 776
      Top = 8
      Width = 3
      Height = 16
    end
    object cbUnits: TComboBox
      Left = 240
      Top = 8
      Width = 529
      Height = 24
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ItemHeight = 16
      ParentFont = False
      TabOrder = 0
      OnChange = cbUnitsChange
    end
  end
end
