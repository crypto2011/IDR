object FHex2DoubleDlg_11011981: TFHex2DoubleDlg_11011981
  Left = 384
  Top = 341
  BorderStyle = bsDialog
  Caption = 'Hex->Double Converter'
  ClientHeight = 95
  ClientWidth = 806
  Color = clBtnFace
  ParentFont = True
  OldCreateOrder = True
  Position = poScreenCenter
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object rgDataViewStyle: TRadioGroup
    Left = 0
    Top = 0
    Width = 806
    Height = 49
    Align = alTop
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
  object edtValue: TLabeledEdit
    Left = 80
    Top = 56
    Width = 697
    Height = 28
    EditLabel.Width = 60
    EditLabel.Height = 20
    EditLabel.Caption = 'Value:'
    EditLabel.Font.Charset = RUSSIAN_CHARSET
    EditLabel.Font.Color = clWindowText
    EditLabel.Font.Height = -13
    EditLabel.Font.Name = 'Fixedsys'
    EditLabel.Font.Style = []
    EditLabel.ParentFont = False
    Font.Charset = RUSSIAN_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Fixedsys'
    Font.Style = []
    LabelPosition = lpLeft
    LabelSpacing = 3
    ParentFont = False
    TabOrder = 1
    OnEnter = edtValueEnter
  end
end
