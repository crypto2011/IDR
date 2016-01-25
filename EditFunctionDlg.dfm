object FEditFunctionDlg_11011981: TFEditFunctionDlg_11011981
  Left = 374
  Top = 200
  BorderStyle = bsToolWindow
  Caption = 'Edit Prototype'
  ClientHeight = 626
  ClientWidth = 877
  Color = clBtnFace
  ParentFont = True
  KeyPreview = True
  OldCreateOrder = True
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnKeyDown = FormKeyDown
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object Panel1: TPanel
    Left = 0
    Top = 581
    Width = 877
    Height = 45
    Align = alBottom
    TabOrder = 0
    object bEdit: TButton
      Left = 25
      Top = 6
      Width = 92
      Height = 31
      Caption = 'Edit'
      TabOrder = 0
      OnClick = bEditClick
    end
    object bAdd: TButton
      Left = 143
      Top = 6
      Width = 92
      Height = 31
      Caption = 'Add'
      Default = True
      Enabled = False
      TabOrder = 1
      OnClick = bAddClick
    end
    object bRemove: TButton
      Left = 261
      Top = 6
      Width = 92
      Height = 31
      Caption = 'Remove'
      TabOrder = 2
      OnClick = bRemoveClick
    end
    object bOk: TButton
      Left = 769
      Top = 6
      Width = 92
      Height = 31
      Caption = 'Ok'
      ModalResult = 1
      TabOrder = 3
      OnClick = bOkClick
    end
  end
  object pc: TPageControl
    Left = 0
    Top = 0
    Width = 877
    Height = 581
    ActivePage = tsType
    Align = alClient
    TabIndex = 0
    TabOrder = 1
    OnChange = pcChange
    object tsType: TTabSheet
      Caption = 'Type'
      object Label1: TLabel
        Left = 24
        Top = 496
        Width = 68
        Height = 16
        Caption = 'RetBytes:'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object lRetBytes: TLabel
        Left = 96
        Top = 496
        Width = 5
        Height = 16
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object Label2: TLabel
        Left = 16
        Top = 528
        Width = 76
        Height = 16
        Caption = 'ArgsBytes:'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object lArgsBytes: TLabel
        Left = 96
        Top = 528
        Width = 5
        Height = 16
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
      end
      object cbEmbedded: TCheckBox
        Left = 37
        Top = 390
        Width = 136
        Height = 21
        Caption = 'Embedded'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 3
      end
      object mType: TMemo
        Left = 176
        Top = 40
        Width = 689
        Height = 465
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'Fixedsys'
        Font.Style = []
        ParentFont = False
        TabOrder = 0
      end
      object rgCallKind: TRadioGroup
        Left = 13
        Top = 209
        Width = 156
        Height = 177
        Caption = 'Function Call Kind: '
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ItemIndex = 0
        Items.Strings = (
          'fastcall'
          'cdecl'
          'pascal'
          'stdcall'
          'safecall')
        ParentFont = False
        TabOrder = 2
      end
      object bApplyType: TButton
        Left = 400
        Top = 512
        Width = 92
        Height = 30
        Caption = 'Apply'
        Default = True
        TabOrder = 8
        OnClick = bApplyTypeClick
      end
      object bCancelType: TButton
        Left = 560
        Top = 512
        Width = 92
        Height = 31
        Caption = 'Cancel'
        Default = True
        TabOrder = 9
        OnClick = bCancelTypeClick
      end
      object rgFunctionKind: TRadioGroup
        Left = 13
        Top = 17
        Width = 156
        Height = 177
        Caption = 'Function Kind: '
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        Items.Strings = (
          'constructor'
          'destructor'
          'procedure'
          'function')
        ParentFont = False
        TabOrder = 1
      end
      object cbVmtCandidates: TComboBox
        Left = 344
        Top = 8
        Width = 521
        Height = 24
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ItemHeight = 16
        ParentFont = False
        Sorted = True
        TabOrder = 7
      end
      object cbMethod: TCheckBox
        Left = 192
        Top = 8
        Width = 137
        Height = 25
        Caption = 'Method of Class:'
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        ParentFont = False
        TabOrder = 6
        OnClick = cbMethodClick
      end
      object lEndAdr: TLabeledEdit
        Left = 53
        Top = 424
        Width = 113
        Height = 24
        EditLabel.Width = 36
        EditLabel.Height = 16
        EditLabel.Caption = 'End: '
        EditLabel.Font.Charset = DEFAULT_CHARSET
        EditLabel.Font.Color = clWindowText
        EditLabel.Font.Height = -13
        EditLabel.Font.Name = 'MS Sans Serif'
        EditLabel.Font.Style = [fsBold]
        EditLabel.ParentFont = False
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        LabelPosition = lpLeft
        LabelSpacing = 3
        ParentFont = False
        TabOrder = 4
      end
      object lStackSize: TLabeledEdit
        Left = 53
        Top = 456
        Width = 113
        Height = 24
        EditLabel.Width = 40
        EditLabel.Height = 16
        EditLabel.Caption = 'Stack'
        EditLabel.Font.Charset = DEFAULT_CHARSET
        EditLabel.Font.Color = clWindowText
        EditLabel.Font.Height = -13
        EditLabel.Font.Name = 'MS Sans Serif'
        EditLabel.Font.Style = [fsBold]
        EditLabel.ParentFont = False
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'MS Sans Serif'
        Font.Style = [fsBold]
        LabelPosition = lpLeft
        LabelSpacing = 3
        ParentFont = False
        TabOrder = 5
      end
    end
    object tsArgs: TTabSheet
      Caption = 'Arguments'
      object lbArgs: TListBox
        Left = 0
        Top = 0
        Width = 869
        Height = 550
        Align = alClient
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'Fixedsys'
        Font.Style = []
        ItemHeight = 20
        ParentFont = False
        TabOrder = 0
      end
    end
    object tsVars: TTabSheet
      Caption = 'Var'
      ImageIndex = 1
      object lbVars: TListBox
        Left = 0
        Top = 0
        Width = 956
        Height = 169
        Font.Charset = RUSSIAN_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'Fixedsys'
        Font.Style = []
        ItemHeight = 20
        ParentFont = False
        TabOrder = 0
        OnClick = lbVarsClick
      end
      object pnlVars: TPanel
        Left = 0
        Top = 176
        Width = 956
        Height = 180
        TabOrder = 1
        object rgLocBase: TRadioGroup
          Left = 320
          Top = 8
          Width = 320
          Height = 35
          Columns = 2
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'MS Sans Serif'
          Font.Style = [fsBold]
          Items.Strings = (
            'ESP-based'
            'EBP-based')
          ParentFont = False
          TabOrder = 0
        end
        object edtVarOfs: TLabeledEdit
          Left = 136
          Top = 48
          Width = 576
          Height = 24
          EditLabel.Width = 45
          EditLabel.Height = 16
          EditLabel.Caption = 'Offset:'
          EditLabel.Font.Charset = RUSSIAN_CHARSET
          EditLabel.Font.Color = clWindowText
          EditLabel.Font.Height = -13
          EditLabel.Font.Name = 'MS Sans Serif'
          EditLabel.Font.Style = [fsBold]
          EditLabel.ParentFont = False
          Font.Charset = RUSSIAN_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'MS Sans Serif'
          Font.Style = [fsBold]
          LabelPosition = lpLeft
          LabelSpacing = 3
          ParentFont = False
          TabOrder = 1
        end
        object edtVarSize: TLabeledEdit
          Left = 136
          Top = 80
          Width = 576
          Height = 24
          EditLabel.Width = 35
          EditLabel.Height = 16
          EditLabel.Caption = 'Size:'
          EditLabel.Font.Charset = RUSSIAN_CHARSET
          EditLabel.Font.Color = clWindowText
          EditLabel.Font.Height = -13
          EditLabel.Font.Name = 'MS Sans Serif'
          EditLabel.Font.Style = [fsBold]
          EditLabel.ParentFont = False
          Font.Charset = RUSSIAN_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'MS Sans Serif'
          Font.Style = [fsBold]
          LabelPosition = lpLeft
          LabelSpacing = 3
          ParentFont = False
          TabOrder = 2
        end
        object edtVarName: TLabeledEdit
          Left = 136
          Top = 112
          Width = 576
          Height = 24
          EditLabel.Width = 46
          EditLabel.Height = 16
          EditLabel.Caption = 'Name:'
          EditLabel.Font.Charset = RUSSIAN_CHARSET
          EditLabel.Font.Color = clWindowText
          EditLabel.Font.Height = -13
          EditLabel.Font.Name = 'MS Sans Serif'
          EditLabel.Font.Style = [fsBold]
          EditLabel.ParentFont = False
          Font.Charset = RUSSIAN_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'MS Sans Serif'
          Font.Style = [fsBold]
          LabelPosition = lpLeft
          LabelSpacing = 3
          ParentFont = False
          TabOrder = 3
        end
        object edtVarType: TLabeledEdit
          Left = 136
          Top = 144
          Width = 576
          Height = 24
          EditLabel.Width = 41
          EditLabel.Height = 16
          EditLabel.Caption = 'Type:'
          EditLabel.Font.Charset = RUSSIAN_CHARSET
          EditLabel.Font.Color = clWindowText
          EditLabel.Font.Height = -13
          EditLabel.Font.Name = 'MS Sans Serif'
          EditLabel.Font.Style = [fsBold]
          EditLabel.ParentFont = False
          Font.Charset = RUSSIAN_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'MS Sans Serif'
          Font.Style = [fsBold]
          LabelPosition = lpLeft
          LabelSpacing = 3
          ParentFont = False
          TabOrder = 4
        end
        object bApplyVar: TButton
          Left = 728
          Top = 72
          Width = 92
          Height = 30
          Caption = 'Apply'
          Default = True
          TabOrder = 5
          OnClick = bApplyVarClick
        end
        object bCancelVar: TButton
          Left = 728
          Top = 112
          Width = 92
          Height = 31
          Caption = 'Cancel'
          Default = True
          TabOrder = 6
          OnClick = bCancelVarClick
        end
      end
    end
  end
end
