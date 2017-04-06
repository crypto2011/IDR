object FProgressBar: TFProgressBar
  Left = 449
  Top = 484
  BorderIcons = []
  BorderStyle = bsToolWindow
  ClientHeight = 43
  ClientWidth = 480
  Color = clBtnFace
  DefaultMonitor = dmMainForm
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -10
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poScreenCenter
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object pb: TProgressBar
    Left = 0
    Top = 0
    Width = 480
    Height = 24
    Align = alClient
    BorderWidth = 4
    Min = 0
    Max = 256
    Smooth = True
    TabOrder = 0
  end
  object sb: TStatusBar
    Left = 0
    Top = 24
    Width = 480
    Height = 19
    Panels = <
      item
        Text = '000'
        Width = 200
      end
      item
        Text = '111'
        Width = 280
      end>
    SimplePanel = False
  end
end
