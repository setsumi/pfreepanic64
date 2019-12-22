//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------
class TFormPfreepanic64 : public TForm
{
__published:	// IDE-managed Components
	TMemo *Memo1;
	TEdit *edtKey;
	TLabel *Label1;
	TLabel *Label3;
	TEdit *edtTermKey;
	TButton *btnInfo;
	TRadioGroup *rdgVoice;
	TGroupBox *GroupBox1;
	TCheckBox *chkOSDEnabled;
	TButton *btnOSDHelp;
	TButton *btnOSDTest;
	TEdit *edtOSDDuration;
	TUpDown *udOSDDuration;
	TLabel *Label2;
	TTimer *TimerConsole;
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall edtKeyEnter(TObject *Sender);
	void __fastcall edtKeyExit(TObject *Sender);
	void __fastcall edtTermKeyEnter(TObject *Sender);
	void __fastcall edtTermKeyExit(TObject *Sender);
	void __fastcall btnInfoClick(TObject *Sender);
	void __fastcall btnOSDHelpClick(TObject *Sender);
	void __fastcall btnOSDTestClick(TObject *Sender);
	void __fastcall TimerConsoleTimer(TObject *Sender);
private:	// User declarations
	void __fastcall WndProc(TMessage& Message);
public:		// User declarations
	void Load();
	void Save();
	void MemoResetStyle();

	__fastcall TFormPfreepanic64(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TFormPfreepanic64 *FormPfreepanic64;
//---------------------------------------------------------------------------
#endif
