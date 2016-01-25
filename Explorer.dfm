object FExplorer_11011981: TFExplorer_11011981
  Left = 422
  Top = 205
  BorderStyle = bsToolWindow
  Caption = 'Explorer'
  ClientHeight = 649
  ClientWidth = 806
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object pc1: TPageControl
    Left = 0
    Top = 0
    Width = 806
    Height = 649
    ActivePage = tsCode
    Align = alClient
    PopupMenu = pm1
    TabIndex = 0
    TabOrder = 0
    object tsCode: TTabSheet
      Caption = 'Code'
      object lbCode: TListBox
        Left = 0
        Top = 0
        Width = 798
        Height = 568
        Align = alClient
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -15
        Font.Name = 'Fixedsys'
        Font.Style = []
        ItemHeight = 20
        ParentFont = False
        PopupMenu = pm1
        TabOrder = 0
      end
      object Panel3: TPanel
        Left = 0
        Top = 568
        Width = 798
        Height = 50
        Align = alBottom
        TabOrder = 1
        object btnDefCode: TButton
          Left = 275
          Top = 11
          Width = 109
          Height = 30
          Cursor = crHandPoint
          Caption = 'Define'
          TabOrder = 0
          OnClick = btnDefCodeClick
        end
        object btnUndefCode: TButton
          Left = 423
          Top = 11
          Width = 108
          Height = 30
          Cursor = crHandPoint
          Caption = 'Undefine'
          TabOrder = 1
          OnClick = btnUndefCodeClick
        end
      end
    end
    object tsData: TTabSheet
      Caption = 'Data'
      ImageIndex = 1
      object lbData: TListBox
        Left = 0
        Top = 50
        Width = 798
        Height = 568
        Align = alClient
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -15
        Font.Name = 'Fixedsys'
        Font.Style = []
        ItemHeight = 20
        ParentFont = False
        PopupMenu = pm1
        TabOrder = 0
      end
      object Panel2: TPanel
        Left = 0
        Top = 0
        Width = 798
        Height = 50
        Align = alTop
        TabOrder = 1
        object rgDataViewStyle: TRadioGroup
          Left = 1
          Top = 1
          Width = 796
          Height = 48
          Align = alClient
          Columns = 6
          ItemIndex = 0
          Items.Strings = (
            'Hex'
            'Single (32bit)'
            'Double (64)'
            'Extended (80)'
            'Real (32)'
            'Comp (64)')
          TabOrder = 0
          OnClick = rgDataViewStyleClick
        end
      end
    end
    object tsString: TTabSheet
      Caption = 'String'
      ImageIndex = 2
      object rgStringViewStyle: TRadioGroup
        Left = 0
        Top = 441
        Width = 798
        Height = 127
        Align = alClient
        Caption = ' String Styles '
        Columns = 2
        ItemIndex = 0
        Items.Strings = (
          'PAnsiChar'
          'PWideChar'
          'ShortString'
          'AnsiString'
          'WideString')
        TabOrder = 0
        OnClick = rgStringViewStyleClick
      end
      object lbString: TMemo
        Left = 0
        Top = 0
        Width = 798
        Height = 441
        Align = alTop
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -15
        Font.Name = 'Fixedsys'
        Font.Style = []
        ParentFont = False
        TabOrder = 1
      end
      object Panel1: TPanel
        Left = 0
        Top = 568
        Width = 798
        Height = 50
        Align = alBottom
        TabOrder = 2
        object btnDefString: TButton
          Left = 275
          Top = 11
          Width = 109
          Height = 30
          Cursor = crHandPoint
          Caption = 'Define'
          Enabled = False
          TabOrder = 0
          OnClick = btnDefStringClick
        end
        object btnUndefString: TButton
          Left = 423
          Top = 11
          Width = 108
          Height = 30
          Cursor = crHandPoint
          Caption = 'Undefine'
          Enabled = False
          TabOrder = 1
        end
      end
    end
    object tsText: TTabSheet
      Caption = 'Text'
      ImageIndex = 4
      object lbText: TListBox
        Left = 0
        Top = 0
        Width = 798
        Height = 618
        Align = alClient
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -15
        Font.Name = 'Fixedsys'
        Font.Style = []
        ItemHeight = 20
        ParentFont = False
        PopupMenu = pm1
        TabOrder = 0
      end
    end
  end
  object pm1: TPopupMenu
    Left = 576
    Top = 336
    object miCopy2Clipboard: TMenuItem
      Caption = 'Copy to Clipboard'
      OnClick = miCopy2ClipboardClick
    end
  end
end
