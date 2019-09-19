object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'pfree panic 64'
  ClientHeight = 268
  ClientWidth = 391
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  OnDestroy = FormDestroy
  DesignSize = (
    391
    268)
  PixelsPerInch = 96
  TextHeight = 13
  object Label1: TLabel
    Left = 16
    Top = 40
    Width = 167
    Height = 13
    Caption = 'PFree (Unlimited plays) toggle key:'
  end
  object Label3: TLabel
    Left = 16
    Top = 68
    Width = 101
    Height = 13
    Caption = 'Terminate game key:'
  end
  object Memo1: TMemo
    Left = 8
    Top = 183
    Width = 375
    Height = 77
    Anchors = [akLeft, akTop, akRight, akBottom]
    ReadOnly = True
    ScrollBars = ssVertical
    TabOrder = 0
  end
  object edtKey: TEdit
    Left = 200
    Top = 38
    Width = 101
    Height = 21
    ReadOnly = True
    TabOrder = 2
    Text = 'Press single key'
    OnEnter = edtKeyEnter
    OnExit = edtKeyExit
  end
  object edtTermKey: TEdit
    Left = 200
    Top = 65
    Width = 101
    Height = 21
    ReadOnly = True
    TabOrder = 3
    Text = 'Press single key'
    OnEnter = edtTermKeyEnter
    OnExit = edtTermKeyExit
  end
  object btnInfo: TButton
    Left = 16
    Top = 8
    Width = 94
    Height = 25
    Caption = 'Information...'
    TabOrder = 1
    OnClick = btnInfoClick
  end
  object rdgVoice: TRadioGroup
    Left = 16
    Top = 91
    Width = 163
    Height = 41
    Caption = 'Notification voice'
    Columns = 2
    Enabled = False
    Items.Strings = (
      'English'
      'Japanese')
    TabOrder = 4
  end
  object GroupBox1: TGroupBox
    Left = 200
    Top = 92
    Width = 183
    Height = 85
    Caption = 'OSD'
    TabOrder = 5
    object Label2: TLabel
      Left = 24
      Top = 56
      Width = 45
      Height = 13
      Caption = 'Duration:'
    end
    object chkOSDEnabled: TCheckBox
      Left = 8
      Top = 16
      Width = 73
      Height = 17
      Caption = 'Enabled'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Tahoma'
      Font.Style = [fsBold]
      ParentFont = False
      TabOrder = 0
    end
    object btnOSDHelp: TButton
      Left = 87
      Top = 16
      Width = 25
      Height = 25
      Caption = '?'
      TabOrder = 1
      OnClick = btnOSDHelpClick
    end
    object btnOSDTest: TButton
      Left = 118
      Top = 16
      Width = 52
      Height = 25
      Caption = 'Test'
      TabOrder = 2
      OnClick = btnOSDTestClick
    end
    object edtOSDDuration: TEdit
      Left = 88
      Top = 56
      Width = 65
      Height = 21
      TabOrder = 3
      Text = '0'
    end
    object udOSDDuration: TUpDown
      Left = 153
      Top = 56
      Width = 16
      Height = 21
      Associate = edtOSDDuration
      Max = 30000
      TabOrder = 4
    end
  end
  object TimerConsole: TTimer
    Enabled = False
    Interval = 200
    OnTimer = TimerConsoleTimer
    Left = 336
    Top = 24
  end
end
