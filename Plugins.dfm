object FPlugins: TFPlugins
  Left = 550
  Top = 365
  Width = 548
  Height = 287
  Caption = 'Plugins'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 120
  TextHeight = 16
  object cklbPluginsList: TCheckListBox
    Left = 0
    Top = 0
    Width = 540
    Height = 209
    OnClickCheck = cklbPluginsListClickCheck
    Align = alTop
    ItemHeight = 16
    TabOrder = 0
    OnDblClick = cklbPluginsListDblClick
  end
  object bOk: TButton
    Left = 144
    Top = 216
    Width = 75
    Height = 25
    Caption = 'Ok'
    TabOrder = 1
    OnClick = bOkClick
  end
  object bCancel: TButton
    Left = 320
    Top = 216
    Width = 75
    Height = 25
    Caption = 'Cancel'
    TabOrder = 2
    OnClick = bCancelClick
  end
end
