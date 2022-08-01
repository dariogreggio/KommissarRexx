/*****************************************************************
*                           kommissarRexx                        *
*                        by Dario Greggio                        *
*                           version 1.0                          *
*                                                                *
*                  PIX32MZ  by  Dario Greggio       *
*                           22/7/2022  
*
 * la EmptyString potrebbe non servire più qua...
 * uno statement (tipo = al posto di ==) in IF o WHEN schianta
 * 
*****************************************************************/


#include "../at_winc1500.h"
#include <ctype.h>
#include <float.h>
#include <sys/kmem.h>

#include "../genericTypedefs.h"
#include "../pc_pic_cpu.h"

#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif

#include "fat_sd/fsconfig.h"
#include "fat_sd/fsio.h"
#include "fat_ram/ramfsio.h"
#include "fat_ide/idefsio.h"
#include "fat_ide/fdc.h"
//#include "../../pc_pic_video.X/Adafruit_GFX.h"
#include "../../pc_pic_video.X/Adafruit_colors.h"
#include "../../pc_pic_video.X/gfxfont.h"
#include "kommissarrexx.h"

#include "harmony_app.h"
#include "superfile.h"

extern APP_DATA appData;
#define KOMMISSARREXX_COPYRIGHT_STRING "KommissarRexx for PIC32MZ v0.1.4 - 1/8/2022\n"
extern const char _PC_PIC_CPU_C[];
extern const char _PC_PIC_COMMAND_C[];


#undef stricmp    // per at_winc1500 ... £$%@#
#undef strnicmp

extern BYTE eventReceived;
extern struct KEYPRESS keypress;
extern SIZE Screen,ScreenText;
extern BYTE errorLevel;
extern BYTE powerState;
extern signed char currDrive;
extern enum FILE_DEVICE m_stdout,m_stdin,m_stderr;

extern DWORD now;
#ifdef USA_WIFI
extern BYTE myRSSI;
extern Ipv4Addr myIp;
extern uint8_t internetBuffer[256];
extern uint8_t rxBuffer[1536];
#endif

extern const char months[13][4];
extern const char wdays[7][4];
extern const unsigned char days_month[2][12];


char getDrive(const char *s,SUPERFILE *f,char **filename);
BOOL hitCtrlC(BOOL);
BOOL isCtrlS(void);

extern DWORD extRAMtot;


/* tokens defined */
enum __attribute((packed)) {
  B_ERROR=20,
  EOL,
  EOS,
  VALUE,
  NOT,
  EQUALS,
  GREATER,
  GREATEREQUALS,
  LESS,
  LESSEQUALS,
  EQUALS_STRICT,
  GREATER_STRICT,
  GREATEREQUALS_STRICT,
  LESS_STRICT,
  LESSEQUALS_STRICT,
  DIMSTRID,
  QUOTE,
  SEMICOLON,
  
	DIV=80,
	DIVINT,
	MULT,
  POW,
	OPAREN,
  CPAREN,
  PLUS,
  MINUS,
  COMMA,
  COLON,
  DOT,
  MOD,
  REM,
  REM2,
  AND,
  OR,
  XOR,

  ADDRESS=100,
  WITH,
	INPUTTOK,
  OUTPUTTOK,
  STREAM,
  ARG,
	CALL,
  ON,
	ERRORTOK,
  NAME,
	DO,
  END,
  WHILE,
  UNTIL,
  TO,
  BY,
  FOR,
  FOREVER,
	DROP,
  EXIT,
  IF,
	THEN,
	ELSE,
	INTERPRET,
  ITERATE,
  LEAVE,
  NOP,
  NUMERIC,
  DIGITS,
  FORM,
  SCIENTIFIC,
  ENGINEERING,
  FUZZ,
  OPTIONS,
  PARSE,
//  LINEIN,
  UPPER,
  SOURCE,
  VERSION,
  VALUETOK,
  VAR,
  PROCEDURE,
  EXPOSE,
  PULL,
  PUSH,
  QUEUE,
	RETURN,
	SAY,      //46 su address
  SELECT,
  WHEN,
  OTHERWISE,
  SIGNAL,
  FAILURE,
  HALT,
  NOTREADY,
  NOVALUE,
  SYNTAX,
  OFF,
  TRACE,
#ifdef REGINA_REXX
  BEEP,
  CHDIR,
  DIRECTORY_TOK,
  SLEEP,
  SEEK,
#endif
  
	ABBREV,
	ABS,
//  ADDRESS,
//  ARG,
  BITAND,
  BITOR,
  BITXOR,
  B2X,
  CENTER,
  CENTRE,   // che delirio :D
  CHANGESTR,
  CHARIN,
  CHAROUT,
  CHARS,
  COMPARE,
  CONDITION,
  COPIES,
  COUNTSTR,
  C2D,
  C2X,
  DATATYPE,
  DATE,
  DELSTR,
  DELWORD,
//  DIGITS,
  D2C,
  D2X,
  ERRORTEXT,
//  FORM,
  FORMAT,
//  FUZZ,
  INSERT,
  LASTPOS,
  LEFT,
  LENGTH,
  LINEIN,
  LINEOUT,
  LINES,
  MAX,
#ifdef USA_BREAKTHROUGH
	{ "MENUKEY$",8 },
#endif
  MIN,
  OVERLAY,
  POS,
  QUALIFY,
  QUEUED,
  RANDOM,
  REVERSE,
  RIGHT,
  SIGN,
  SOURCELINE,
  SPACE,
//  STREAM,
  STRIP,
  SUBSTR,
  SUBWORD,
  SYMBOL,
  TIME,
//  TRACE,
  TRANSLATE,
  TRUNC,
//  VALUETOK,
  VERIFY,
  WORDTOK,
  WORDINDEX,
  WORDLENGTH,
  WORDPOS,
  WORDS,
  XRANGE,
  X2B,
  X2C,
  X2D,
#ifdef REGINA_REXX
  B2C,
  BITCHG,
  BITCLR,
  BITCOMP,
  BITSET,
  BITTST,
  C2B,
  EXISTS,
  FREESPACE,
  GETENV,
  GETPID,
  GETSPACE,
  IMPORT,
  JUSTIFY,
  TRIM,
  UNAME,
  WRITECH,
  WRITELN,
#endif
  
	};


/* relational operators defined */
enum REL_OPS {
	ROP_EQ=1,         // equals 
	ROP_NEQ,	        // doesn't equal 
	ROP_LT,		        // less than 
	ROP_LTE,         // less than or equals 
	ROP_GT,          // greater than 
	ROP_GTE,          // greater than or equals 
	ROP_EQ_S,          // stricts
	ROP_NEQ_S,	       // doesn't equal 
	ROP_LT_S,	        // less than 
	ROP_LTE_S,         // less than or equals 
	ROP_GT_S,          // greater than 
	ROP_GTE_S          // greater than or equals 
	};



BSTATIC int inkey(void);
  
#warning OVVIAMENTE personalizzare errori!
static const char * const error_msgs[] = {
	"",
	"Syntax error",
	"Out of memory",
	"Identifier too long",
	"No such variable",
	"Bad subscript", 
	"Too many dimensions",
	"Too many initialisers",
	"Illegal type",
	"Too many nested DO",
	"DO without matching END",    //10
	"END without matching DO",
	"Divide by zero",
	"Negative logarithm",
	"Negative square root",
	"Sine or cosine out of range",
	"End of input file",
	"Illegal offset",
	"Type mismatch",
	"Input too long",
	"Bad value",
	"Not an integer",
	"Too many CALL",
	"RETURN without CALL",
	"Formula too complex",
	"File error",
	"Network error",
	"BREAKPOINT (press C to continue or ESC to stop)",
	"Fatal internal error"
	};



static const char * const EmptyString="";
static uint8_t digitSize=32;   // v.DIGITS
static uint8_t digitSizeDecimal=6;   // v.DIGITS
// METTERE in instance!
static uint8_t digitFuzz=0,digitForm=0;

static TOKEN_LIST const tl[] = {
	{ "ADDRESS",7 },
	{ "WITH",4 },
	{ "INPUT",5 },
	{ "OUTPUT",6 },
	{ "STREAM",6 },
	{ "ARG",3 },
	{ "CALL",4 },
	{ "ON",2 },
	{ "ERROR",5 },
	{ "NAME",4 },
	{ "DO",2 },
	{ "END",3 },
	{ "WHILE",5 },
	{ "UNTIL",5 },
	{ "TO",2 },
	{ "BY",2 },
	{ "FOR",3 },
	{ "FOREVER",7 },
	{ "DROP",4 },
	{ "EXIT",4 },
	{ "IF",2 },
	{ "THEN",4 },
	{ "ELSE",4 },
	{ "INTERPRET",9 },
	{ "ITERATE",7 },
	{ "LEAVE",5 },
	{ "NOP",3 },
	{ "NUMERIC",7 },
	{ "DIGITS",6 },
	{ "FORM",4 },
	{ "SCIENTIFIC",10 },
	{ "ENGINEERING",11 },
	{ "FUZZ",4 },
	{ "OPTIONS",7 },
	{ "PARSE",5 },
	{ "UPPER",5 },
	{ "SOURCE",6 },
	{ "VERSION",7 },
	{ "VALUE",5 },
	{ "VAR",3 },
	{ "PROCEDURE",9 },
	{ "EXPOSE",6 },
	{ "PULL",4 },
	{ "PUSH",4 },
	{ "QUEUE",5 },
	{ "RETURN",6 },
	{ "SAY",3 },
	{ "SELECT",6 },
	{ "WHEN",4 },
	{ "OTHERWISE",9 },
	{ "SIGNAL",6 },
  { "FAILURE",7 },
  { "HALT",4 },
  { "NOTREADY",8 },
  { "NOVALUE",7 },
  { "SYNTAX",6 },
	{ "OFF",3 },
	{ "TRACE",5 },
#ifdef REGINA_REXX
  // bah dice cmq che sono funzioni anche queste...
  { "BEEP",4 },
  { "CHDIR",5 },
  { "DIRECTORY",9 },
  { "SLEEP",5 },
  { "SEEK",4 },
#endif


	{ "ABBREV",6 },
	{ "ABS",3 },
	{ "BITAND",6 },
	{ "BITOR",5 },
	{ "BITXOR",6 },
	{ "B2X",3 },
	{ "CENTER",6 },
	{ "CENTRE",6 },
	{ "CHANGESTR",9 },
	{ "CHARIN",6 },
	{ "CHAROUT",7 },
	{ "CHARS",5 },
	{ "COMPARE",7 },
	{ "CONDITION",9 },
	{ "COPIES",6 },
	{ "COUNTSTR",8 },
	{ "C2D",3 },
	{ "C2X",3 },
	{ "DATATYPE",8 },
	{ "DATE",4 },
	{ "DELSTR",6 },
	{ "DELWORD",7 },
	{ "D2C",3 },
	{ "D2X",3 },
	{ "ERRORTEXT",9 },
	{ "FORMAT",6 },
	{ "INSERT",6 },
	{ "LASTPOS",7 },
	{ "LEFT",4 },
	{ "LENGTH",6 },
	{ "LINEIN",6 },
	{ "LINEOUT",7 },
	{ "LINES",5 },
	{ "MAX",3 },
#ifdef USA_BREAKTHROUGH
	{ "MENUKEY$",8 },
#endif
	{ "MIN",3 },
	{ "OVERLAY",7 },
	{ "POS",3 },
	{ "QUALIFY",7 },
	{ "QUEUED",6 },
	{ "RANDOM",6 },
	{ "REVERSE",7 },
	{ "RIGHT",5 },
	{ "SIGN",4 },
	{ "SOURCELINE",10 },
	{ "SPACE",5 },
	{ "STRIP",5 },
	{ "SUBSTR",6 },
	{ "SUBWORD",7 },
	{ "SYMBOL",6 },
	{ "TIME",4 },
	{ "TRANSLATE",9 },
	{ "TRUNC",5 },
	{ "VERIFY",6 },
	{ "WORD",4 },
	{ "WORDINDEX",9 },
	{ "WORDLENGTH",10 },
	{ "WORDPOS",7 },
	{ "WORDS",5 },
	{ "XRANGE",6 },
	{ "X2B",3 },
	{ "X2C",3 },
	{ "X2D",3 },
#ifdef REGINA_REXX
  { "B2C",3 },
  { "BITCHG",6 },
  { "BITCLR",6 },
  { "BITCOMP",7 },
  { "BITSET",6 },
  { "BITTST",6 },
  { "C2B",3 },
  { "EXISTS",6 },
  { "FREESPACE",9 },
  { "GETENV",6 },
  { "GETPID",6 },
  { "GETSPACE",8 },
  { "IMPORT",6 },
  { "JUSTIFY",7 },
  { "TRIM",4 },
  { "UNAME",5 },
  { "WRITECH",7 },
  { "WRITELN",7 }
#endif
  };


#define basicAssert(x) //myTextOut(mInstance,"assert!") //assert((x))



BSTATIC int setup(KOMMISSARREXX *,const char *script);
BSTATIC void cleanup(KOMMISSARREXX *,uint8_t);
BSTATIC void subClose(KOMMISSARREXX *,int f);

BSTATIC void reportError(KOMMISSARREXX *,LINE_NUMBER_TYPE lineno);
BSTATIC int16_t findLine(KOMMISSARREXX *,LINE_NUMBER_TYPE no);
BSTATIC int16_t findProcedure(KOMMISSARREXX *mInstance,const char *name);
BSTATIC int16_t findBlockEnd(KOMMISSARREXX *mInstance,uint8_t level);
BSTATIC int8_t gotoBlockEnd(KOMMISSARREXX *mInstance,uint8_t level);

BSTATIC VARIABLESTRING *addNewVar(KOMMISSARREXX *mInstance,const char *id,void *parent);
int16_t to_int(const char *);
LNUM_TYPE to_num(const char *);
char *to_string(LNUM_TYPE,char *);
char *to_string_i(int16_t,char *);
DWORD to_time(const char *);

BSTATIC LINE_NUMBER_TYPE line(KOMMISSARREXX *);
BSTATIC void doSay(KOMMISSARREXX *);
BSTATIC BYTE get_args(const char *args,BYTE w,char *ret);
BSTATIC void doArg(KOMMISSARREXX *);
BSTATIC void doParse(KOMMISSARREXX *);
BSTATIC void doLet(KOMMISSARREXX *);
BSTATIC BYTE deleteVariable(KOMMISSARREXX *,const char *,void *);
BSTATIC void doDrop(KOMMISSARREXX *);
BSTATIC void doDim(KOMMISSARREXX *);
BSTATIC int8_t doIf(KOMMISSARREXX *,uint8_t *);
BSTATIC LINE_NUMBER_TYPE doCall(KOMMISSARREXX *);
BSTATIC LINE_NUMBER_TYPE doReturn(KOMMISSARREXX *);
BSTATIC void doSelect(KOMMISSARREXX *);
BSTATIC int8_t doWhen(KOMMISSARREXX *,uint8_t *);
BSTATIC int8_t doOtherwise(KOMMISSARREXX *,uint8_t *);
BSTATIC LINE_NUMBER_TYPE doSignal(KOMMISSARREXX *);
BSTATIC void doInterpret(KOMMISSARREXX *);
BSTATIC void doAddress(KOMMISSARREXX *);
BSTATIC void doPull(KOMMISSARREXX *);
BSTATIC void doRem(KOMMISSARREXX *);
BSTATIC void doDo(KOMMISSARREXX *);
BSTATIC LINE_NUMBER_TYPE doIterate(KOMMISSARREXX *);
BSTATIC LINE_NUMBER_TYPE doLeave(KOMMISSARREXX *);
BSTATIC LINE_NUMBER_TYPE doEnd(KOMMISSARREXX *);
BSTATIC void doOpen(KOMMISSARREXX *);
BSTATIC void doClose(KOMMISSARREXX *);
BSTATIC void doError(KOMMISSARREXX *);
BSTATIC LINE_NUMBER_TYPE doRun(KOMMISSARREXX *);
BSTATIC void doContinue(KOMMISSARREXX *);
BSTATIC void doList(KOMMISSARREXX *);
BSTATIC void doLoad(KOMMISSARREXX *);
BSTATIC void doSave(KOMMISSARREXX *);
BSTATIC void doTrace(KOMMISSARREXX *);
BSTATIC void doExit(KOMMISSARREXX *);
BSTATIC void doQueue(KOMMISSARREXX *);
BSTATIC void doPush(KOMMISSARREXX *);
BSTATIC void doNop(KOMMISSARREXX *);
BSTATIC void doNumeric(KOMMISSARREXX *);
BSTATIC void doOptions(KOMMISSARREXX *);

#ifdef REGINA_REXX
BSTATIC void doBeep(KOMMISSARREXX *);
BSTATIC void doChdir(KOMMISSARREXX *);
BSTATIC void doDirectory(KOMMISSARREXX *);
BSTATIC uint16_t doBitchg(KOMMISSARREXX *);
BSTATIC uint16_t doBitset(KOMMISSARREXX *);
BSTATIC uint16_t doBitclr(KOMMISSARREXX *);
BSTATIC uint16_t doBittst(KOMMISSARREXX *);
BSTATIC int16_t doBitcomp(KOMMISSARREXX *);
BSTATIC void doSleep(KOMMISSARREXX *);
BSTATIC void doSeek(KOMMISSARREXX *);
#endif

BSTATIC void lvalue(KOMMISSARREXX *,LLVALUE *lv);
BSTATIC int subLoad(const char *,char *);

BSTATIC signed char boolExpr(KOMMISSARREXX *);
BSTATIC signed char rop(signed char, LNUM_TYPE, LNUM_TYPE);
BSTATIC signed char strRop(signed char, const char *, const char *);
BSTATIC signed char boolFactor(KOMMISSARREXX *);
BSTATIC enum REL_OPS relOp(KOMMISSARREXX *);


BSTATIC char *expr(KOMMISSARREXX *);
BSTATIC char *term(KOMMISSARREXX *);
BSTATIC char *factor(KOMMISSARREXX *);
BSTATIC LNUM_TYPE variable(KOMMISSARREXX *);
BSTATIC int16_t ivariable(KOMMISSARREXX *);
BSTATIC LNUM_TYPE dimVariable(KOMMISSARREXX *);


BSTATIC VARIABLESTRING *findVariable(KOMMISSARREXX *,const char *id,void *parent,BYTE private);
BSTATIC LDIMVARSTRING *findDimVar(KOMMISSARREXX *,const char *id,void *parent,BYTE private);
BSTATIC LDIMVARSTRING *dimension(KOMMISSARREXX *,const char *id, BYTE ndims, ...);
BSTATIC void *getDimVar(KOMMISSARREXX *,LDIMVARSTRING *dv, ...);
BSTATIC VARIABLESTRING *addVar(KOMMISSARREXX *,const char *id,void *parent);
BSTATIC LDIMVARSTRING *addDimVar(KOMMISSARREXX *,const char *id,void *parent);

enum DATATYPE datatype(const char *);
signed char isString(TOKEN_NUM);
char *mystrdup(KOMMISSARREXX *,const char *);
char *mystrdupint(KOMMISSARREXX *,int16_t);
char *mystrdupnum(KOMMISSARREXX *,LNUM_TYPE);
long double strtold(const char *string, char **endPtr);
void ldtoa(long double n, char* res, BYTE afterpoint);
BYTE isvalidrexx(char);
BYTE isvalidrexx2(char);

// diciamo che sarebbero tutte CONST , volendo..
BSTATIC char *leftString(KOMMISSARREXX *);
BSTATIC char *rightString(KOMMISSARREXX *);
BSTATIC char *subString(KOMMISSARREXX *);
BSTATIC char *translateString(KOMMISSARREXX *);
BSTATIC char *reverseString(KOMMISSARREXX *);
BSTATIC char *delString(KOMMISSARREXX *);
BSTATIC char *countString(KOMMISSARREXX *);
BSTATIC char *stringString(KOMMISSARREXX *);
BSTATIC char *changeString(KOMMISSARREXX *);
BSTATIC char *hexString(KOMMISSARREXX *);
BSTATIC char *stripString(KOMMISSARREXX *);
BSTATIC char *abbrevString(KOMMISSARREXX *);
#ifdef REGINA_REXX
BSTATIC char *trimString(KOMMISSARREXX *);
#endif
BSTATIC char *dateString(KOMMISSARREXX *);
BSTATIC char *timeString(KOMMISSARREXX *);
BSTATIC char *inkeyString(KOMMISSARREXX *);
BSTATIC char *lineinString(KOMMISSARREXX *);
BSTATIC char *lineoutString(KOMMISSARREXX *);
BSTATIC char *datatypeString(KOMMISSARREXX *);
BSTATIC char *argString(KOMMISSARREXX *);
BSTATIC char *words(KOMMISSARREXX *);
BSTATIC char *word(KOMMISSARREXX *);
BSTATIC char *wordpos(KOMMISSARREXX *);
BSTATIC char *delword(KOMMISSARREXX *);
BSTATIC char *xrange(KOMMISSARREXX *);
BSTATIC char *charin(KOMMISSARREXX *);
BSTATIC char *charout(KOMMISSARREXX *);
BSTATIC char *instr(KOMMISSARREXX *);
BSTATIC char *instr2(KOMMISSARREXX *);
BSTATIC char *compareStrings(KOMMISSARREXX *);
#ifdef USA_BREAKTHROUGH
BSTATIC char *menukeyString(KOMMISSARREXX *);
#endif
BSTATIC char *errorString(KOMMISSARREXX *);
BSTATIC char *digits(KOMMISSARREXX *);
BSTATIC char *form(KOMMISSARREXX *);
BSTATIC char *fuzz(KOMMISSARREXX *);
BSTATIC char *b2x(KOMMISSARREXX *);
BSTATIC char *c2d(KOMMISSARREXX *);
BSTATIC char *c2x(KOMMISSARREXX *);
BSTATIC char *d2c(KOMMISSARREXX *);
BSTATIC char *d2x(KOMMISSARREXX *);
BSTATIC char *x2b(KOMMISSARREXX *);
BSTATIC char *x2c(KOMMISSARREXX *);
BSTATIC char *x2d(KOMMISSARREXX *);
#ifdef REGINA_REXX
BSTATIC char *c2b(KOMMISSARREXX *);
BSTATIC char *exists(KOMMISSARREXX *);
BSTATIC char *freespace(KOMMISSARREXX *);
BSTATIC char *mygetenv(KOMMISSARREXX *);
BSTATIC char *getpid(KOMMISSARREXX *);
BSTATIC char *getspace(KOMMISSARREXX *);
BSTATIC char *import(KOMMISSARREXX *);
BSTATIC char *justify(KOMMISSARREXX *);
BSTATIC char *uname(KOMMISSARREXX *);
BSTATIC char *writech(KOMMISSARREXX *);
BSTATIC char *writeln(KOMMISSARREXX *);
#endif
BSTATIC char *addressString(KOMMISSARREXX *);
BSTATIC char *verString(KOMMISSARREXX *);
BSTATIC char *stringDimVar(KOMMISSARREXX *);
BSTATIC const char *stringVar(KOMMISSARREXX *);
BSTATIC char *stringLiteral(KOMMISSARREXX *);
BSTATIC char *dirString(KOMMISSARREXX *);

BSTATIC void match(KOMMISSARREXX *,TOKEN_NUM tok);
BSTATIC void setError(KOMMISSARREXX *,enum INTERPRETER_ERRORS errorcode);
LINE_NUMBER_TYPE getNextStatement(KOMMISSARREXX *,const char *str);
BSTATIC TOKEN_NUM getToken(const char *str);
BSTATIC uint8_t tokenLen(KOMMISSARREXX *,const char *str, TOKEN_NUM token);

BSTATIC LNUM_TYPE getValue(const char *str, IDENT_LEN *len);
BSTATIC void getId(KOMMISSARREXX *,const char *str, char *out, IDENT_LEN *len);
BSTATIC void subAssignVar(KOMMISSARREXX *mInstance, char **v, LVARIABLE *d);

static void mystrgrablit(char *dest, const char *src);
static const char *mystrend(const char *str, char quote);
static unsigned int myStrCount(const char *str, char ch);
static char *myStrConcat(const char *str, const char *cat);

static const char *skipSpaces(const char *);

const char KommissarRexxCopyrightString[]= {KOMMISSARREXX_COPYRIGHT_STRING};
static char *scriptAllocated=NULL;
char scriptArgs[128];   // anche questi forse andrebbero in Instance...

extern SIZE Screen;

BSTATIC int myTextOut(KOMMISSARREXX *mInstance,const char *s);
BSTATIC int myCR(KOMMISSARREXX *mInstance);


//====================================================== MAIN ===================================================================
int kommissarrexx(KOMMISSARREXX *hInstance,const char *scriptName) {
  BYTE ch;
  int i;
  char *cmdPointer,*parmsPointer;
  char commandLine[128]={0},lastCommandLine[128]={0};
  int commandLineCnt=0;

    
/*	static const char rom greeting[] = 
		"Minibasic-PIC 0.1\n"
#if !defined(__18CXX)
		"R-Run\n"
		"L-List\n"
		#ifdef EDITOR
		"E-Edit\n"
		#endif
		"P-Program\n\n";
#else
		"\n";
#endif
		*/


//------------------------------------------------------------- Send CopyRight Message ----------------------------------------------------------
  hInstance->incomingChar[0]=hInstance->incomingChar[1]=0;
  hInstance->stack=NULL;
	hInstance->nfiles=0;
  hInstance->variables = NULL;
  hInstance->nvariables = 0;
  hInstance->dimVariables = NULL;
  hInstance->ndimVariables = 0;

  hInstance->lines = 0;
  hInstance->nlines = 0;
	hInstance->ngosubs=hInstance->ndos=hInstance->inBlock=0;
  hInstance->curline=0;
  hInstance->errorFlag=0;
	hInstance->errorHandler.handler=0;
	hInstance->errorHandler.errorline=0;
	hInstance->errorHandler.errorcode=0;
  
#ifdef USA_BREAKTHROUGH
  HDC hDC;
  GetDC(hInstance->hWnd,&hDC);
	SetTextColor(&hDC,BLUE);
 	TextOut(&hDC,0,0,KommissarRexxCopyrightString);  // c'è anche in WM_CREATE di là, unire...
#else
	SetColors(BLUE,WHITE);
  Cls();
  hInstance->Cursor.x=0; hInstance->Cursor.y=0;
 	myTextOut(hInstance,KommissarRexxCopyrightString);  // 
  myCR(hInstance);
  sprintf(commandLine,"%u Bytes free.", getTotRAM());
  myTextOut(hInstance,commandLine);
  *commandLine=0;
#endif
  myCR(hInstance);
  myTextOut(hInstance,"]");
//  myCR(hInstance); /*deve/devono dipendere da xsize.. */

  hInstance->traceLevel='O';
  *scriptArgs=0;

  keypress.key=keypress.modifier=0;
  
  
 

//------------------------------------------------------------- Run Script -----------------------------------------------------------------------
  
  do {
    
    handle_events();
    hInstance->incomingChar[0]=keypress.key;
    hInstance->incomingChar[1]=keypress.modifier;
    KBClear();
    
#if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    SYS_Tasks();
#ifdef USA_USB_HOST_MSD
    SYS_FS_Tasks();
#endif
#else
    APP_Tasks();
#endif

#ifdef USA_WIFI
  	m2m_wifi_handle_events(NULL);
#endif
#ifdef USA_ETHERNET
    StackTask();
    StackApplications();
#endif

    if(hInstance->incomingChar[0]) {

     if((hInstance->incomingChar[0]>=' ' && hInstance->incomingChar[0]<=0x7e) ||
      hInstance->incomingChar[0]=='\r' || hInstance->incomingChar[0]=='\n' || hInstance->incomingChar[0]=='\x8') {

      ch=hInstance->incomingChar[0];
      hInstance->incomingChar[0]=0;

      if(commandLineCnt<sizeof(commandLine)) {
        commandLine[commandLineCnt++]=ch;
        commandLine[commandLineCnt]=0;
        hInstance->Cursor.x++;
        if(hInstance->Cursor.x >= ScreenText.cx) {
          myCR(hInstance);
          }
        
        switch(ch) {
          default:
            if(isprint(ch))
              putchar(ch);
            break;
          case '\x8':
            // backspace
            commandLine[--commandLineCnt]=0;    // xché cmq l'ho messo in commandline...
            hInstance->Cursor.x--;
            if(commandLineCnt>0) {
              commandLine[--commandLineCnt]=0;
              hInstance->Cursor.x--;
              putchar('\x8');
              }
//            SetXY(1,hInstance->Cursor.x*1 /*textsize*/ ,hInstance->Cursor.y*1 /*textsize*/ );
//            putchar(' ');   // o mandare 0x08 e far fare alla scheda video??
            break;
          case '\r':
            commandLine[commandLineCnt--]=0;            // pulisco cmq il successivo, in caso di prec. comando lungo..
            commandLine[commandLineCnt]=0;            // tolgo LF
            break;
          case '\n':
            commandLine[--commandLineCnt]=0;            // tolgo CR NO! compatibilità con minibasic-script
            myCR(hInstance);
  
            cmdPointer=&commandLine[0];   // per warning...
//            myTextOut(hInstance,cmdPointer);

            hInstance->errorFlag=0;
            hInstance->ngosubs=hInstance->ndos=hInstance->inBlock=0;
            
            strcpy(lastCommandLine,commandLine);

            if(!strnicmp(commandLine,"RUN",3)) {
              parmsPointer=(char *)skipSpaces(cmdPointer+3);
              if(*parmsPointer=='\"') {
                SUPERFILE f;
                char buf[128],*p,*filename;
                // OVVIAMENTE andare anche su C: E: !
                
                strncpy(buf,parmsPointer+1,120);
                if(p=strchr(buf,'\"'))
                  *p=0;
                if(!strchr(buf,'.'))
                  strcat(buf,".REX");

                getDrive(buf,&f,&filename);
                p=theScript;
                if(SuperFileOpen(&f,filename,'r')) { 
                  while(1) {
                    if(SuperFileRead(&f,p,1) == 1) {  // o FSfgets??
                      if(*p != 13 && *p != 9)    // cmq tolgo CR e TAB
                        p++;
                      }
                    else
                      break;
                    }
                  *p=0;
                  SuperFileClose(&f);
                  i=rexx(hInstance,theScript,"",0);
  //                if(!i)
  //                  break;
                  goto ready2;
                  }
                else {
                  myTextOut(hInstance,"Script not found");
                  myCR(hInstance);
                  goto ready;
                  }
                }     // se nomefile..
              else {
                goto execline;
                }
              }
            else if(!strnicmp(commandLine,"EXIT",4)) {
              if(getToken(hInstance->string) == VALUE)
                errorLevel=to_int(expr(hInstance));
              goto fine;
              break;
              }
            else {
          		if(*cmdPointer) {
execline:
                hInstance->string = cmdPointer;
                hInstance->token = getToken(hInstance->string);
                hInstance->errorFlag = 0;
                i = line(hInstance);
                if(hInstance->errorFlag) {
                  myTextOut(hInstance,error_msgs[hInstance->errorFlag]);
                  ErrorBeep(200);
                  hInstance->errorFlag=0;
                  myCR(hInstance);
                  }
ready2:
                myCR(hInstance);
ready:
                myTextOut(hInstance,"]");
                }
              else {
//                putchar('\n'); 
                myCR(hInstance);
                }
              }
//            myCR(hInstance);
            
            commandLineCnt=0;
            memset(commandLine,0,sizeof(commandLine));;

            break;
          }

        }
      }     // char accepted
    else {
      switch(hInstance->incomingChar[0]) {
        case 0xa1: // F1
//          strcpy(commandLine,lastCommandLine);
//          commandLineCnt=strlen(commandLine);
//          myTextOut(hInstance,commandLine);
          if(commandLineCnt<strlen(lastCommandLine)) {
            char buf[2];
            commandLine[commandLineCnt]=lastCommandLine[commandLineCnt];
            buf[0]=commandLine[commandLineCnt]; buf[1]=0;
            commandLine[++commandLineCnt]=0;
            myTextOut(hInstance,buf);
            }
          break;
        case 0xa3: // F3
          while(commandLineCnt<strlen(lastCommandLine)) {
            char buf[2];
            commandLine[commandLineCnt]=lastCommandLine[commandLineCnt];
            buf[0]=commandLine[commandLineCnt]; buf[1]=0;
            commandLine[++commandLineCnt]=0;
            myTextOut(hInstance,buf);
            }
          break;
        case 0x94: // up, 
          break;
        case 0x91: // right
          // gestire frecce qua?
          break;
        }
      }
      }
    
//    handleWinTimers();
//    manageWindows(i);    // fare ogni mS

    } while(1);


//------------------------------------------------------------- Finished running Basic Script ---------------------------------------------------
fine:
          
#ifdef USA_BREAKTHROUGH
 	do_print(&hDC,"\nPress a key to restart");
  //compila ma non esiste???!!! 2/1/22
#else
#endif
  
  if(scriptAllocated) {
    free((void *)scriptAllocated);
    scriptAllocated=NULL;
    }
  cleanup(hInstance,0);
  
#ifdef USA_BREAKTHROUGH
  ReleaseDC(hInstance->hWnd,&hDC);
#else
#endif

  SetCursorMode(1,1);

  
	return 0;
	}



//============================= Rexx Interpreter Start ================================
/*
  Interpret a REXX script

  Params: script - the script to run
  Returns: 0 on success, 1 on error condition.
*/

int rexx(KOMMISSARREXX *mInstance,const char *script,const char *args,BYTE where) {
  int16_t /*LINE_NUMBER_TYPE per marker fine*/ nextline;
  int answer = 0;

  mInstance->incomingChar[0]=mInstance->incomingChar[1]=0;
  mInstance->stack=NULL;
	mInstance->nfiles=0;
  mInstance->variables = NULL;
  mInstance->nvariables = 0;
  mInstance->dimVariables = 0;
  mInstance->ndimVariables = 0;

  mInstance->lines = 0;
  mInstance->nlines = 0;
	mInstance->ngosubs=mInstance->ndos=mInstance->inBlock=0;
  mInstance->curline=0;
  mInstance->errorFlag=0;
	mInstance->errorHandler.handler=0;
	mInstance->errorHandler.errorline=0;
	mInstance->errorHandler.errorcode=0;
  
  mInstance->ColorPaletteBK=textColors[0] /*BLACK*/;
  mInstance->ColorBK=ColorRGB(mInstance->ColorPaletteBK);
	mInstance->ColorPalette=textColors[7] /*WHITE*/;
  mInstance->Color=ColorRGB(mInstance->ColorPalette);
//	setTextColorBG(textColors[mInstance->ColorPalette],textColors[mInstance->ColorPaletteBK]); 
#ifndef USING_SIMULATOR
  Cls();
	mInstance->Cursor.x=0;
  mInstance->Cursor.y=0;
  SetCursorMode(0,0);
#endif
 	
  if(where) {
    subLoad((char *)script,theScript);
    }
  else {
    strcpy(theScript,script);
    }

  if(setup(mInstance,(char *)theScript) == -1) {				//Run setup Function to extract data from the Script
    answer=-1;										//Script error END
    goto end_all;
    }
  
  *scriptArgs=0;
  if(args) {
    strncpy(scriptArgs,args,sizeof(scriptArgs)-1);  // ovviamente sono globali per ev. script nidificati...
    scriptArgs[sizeof(scriptArgs)-1]=0;
    }
  mInstance->traceLevel='O';
  
 	while(mInstance->curline != -1) {			//Loop through Script Running each Line

		mInstance->string = mInstance->lines[mInstance->curline].str;
		mInstance->token = getToken(mInstance->string);
    switch(mInstance->traceLevel) {
      case 'A':
      case 'C':
      case 'I':   // aggiungere..
      {
        const char *p=mInstance->string;
        while(*p && *p!='\n')
          err_printf("%c",*p++);    // si potrebbero evitare i commenti?
        err_printf("\r\n");
        err_printf("--%u\r\n",mInstance->inBlock);
      }
        break;
      case '?':
        inputKey();
        break;
      default:
        break;
      }

		mInstance->errorFlag = 0;
		
		nextline = line(mInstance);
    
		if(mInstance->errorFlag)	{
			if(!mInstance->errorHandler.handler)
				reportError(mInstance,mInstance->curline);

      if(mInstance->errorFlag == ERR_STOP)	{
        int i;

#if 0
        do {
          i=mInstance->incomingChar;
          Yield(mInstance->threadID);
          ClrWdt();
          }	while(i != 'C' && i != '\x1b');
#else
//          SYS_Tasks();
          goto handle_error2;
#endif

        if(hitCtrlC(TRUE))	{ 
//        if(i == '\x1b')	{     // usare CTRL-C ....
          goto handle_error2;
          }
        }
      else	{
//handle_error:
        if(mInstance->errorHandler.handler)	{
          nextline = mInstance->errorHandler.handler;
          mInstance->errorHandler.errorcode=mInstance->errorFlag;
          mInstance->errorHandler.errorline=mInstance->lines[mInstance->curline].no;
          }
        else	{
handle_error2:
          answer=1;
          break;
          }
        }
      }

		if(nextline == -1)			// p.es. comando END
	  	break;

		if(nextline == 0)	{				//Line increment from 0 to 1
    	mInstance->curline++;

 			if(mInstance->curline == mInstance->nlines)			//check if last line has been read
   			break;
   		}
		else {							//find next line
    	mInstance->curline = findLine(mInstance,nextline);
 			if(mInstance->curline == (int16_t)-1) {
        char buf[32];
        sprintf(buf,"line %u not found", nextline);		// QUESTO NON viene trappato.. non ha senso..!
		    myTextOut(mInstance,buf);
        myCR(mInstance);
				goto handle_error2;
  			}
   		}
		ClrWdt();
    
    Yield(mInstance->threadID);
    
    mLED_1 ^= 1;     //  

//    if(mInstance->incomingChar == '\x1b')
//      if(mInstance->incomingChar[0] =='c' && mInstance->incomingChar[1] == 0b00000001)
#warning FINIRE modifier qua!
//        break;
      if(hitCtrlC(TRUE))    // opp. così...
        break;
    
      setError(mInstance,ERR_STOP);
      

    handle_events();
    mInstance->incomingChar[0]=keypress.key; mInstance->incomingChar[1]=keypress.modifier;
    KBClear();

#if defined(USA_USB_HOST) || defined(USA_USB_SLAVE_CDC)
    SYS_Tasks();
#ifdef USA_USB_HOST_MSD
    SYS_FS_Tasks();
#endif
#else
    APP_Tasks();
#endif

#ifdef USA_WIFI
  	m2m_wifi_handle_events(NULL);
#endif
#ifdef USA_ETHERNET
    StackTask();
    StackApplications();
#endif

  	} //While finish


end_all:
  cleanup(mInstance,1);
  SetCursorMode(1,1);
  KBClear();
  
  return answer;
	}

//===============================================================================================================================================================

int setup(KOMMISSARREXX *mInstance,const char *script) {
  int i;

  mInstance->nlines = myStrCount(script,'\n');						//Count the lines in the Basic Script by counting \n
  mInstance->lines = (LINE *)malloc(mInstance->nlines * sizeof(LINE));
  if(!mInstance->lines) {
    myTextOut(mInstance,"Out of memory");
    myCR(mInstance);
		return -1;
  	}
  for(i=0; i<mInstance->nlines; i++) {
    mInstance->lines[i].str = script;
		script = (char *)strchr((char *)script,'\n') +1;
  	}
  if(!mInstance->nlines) {
    myTextOut(mInstance,"Can't read script");
    myCR(mInstance);
    free(mInstance->lines);
		return -1;
  	}

  mInstance->nvariables = 0;
  mInstance->variables = NULL;

  mInstance->dimVariables = 0;
  mInstance->ndimVariables = 0;

  return 0;
	}


void subClose(KOMMISSARREXX *mInstance,int f) {
  
  switch(mInstance->openFiles[f].type) {
    case FILE_COM:
//          CloseUART((BYTE)(DWORD)mInstance->openFiles[i].handle);
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
      close((SOCKET)(int)mInstance->openFiles[f].handle);
      // se ACCEPTED, gestire...
      break;
    case FILE_UDP:
      close((SOCKET)(int)mInstance->openFiles[f].handle);
      break;
#endif
    case FILE_DISK:
      SuperFileClose(mInstance->openFiles[f].handle);
      free(mInstance->openFiles[f].handle);
      break;
    }
  }
void cleanup(KOMMISSARREXX *mInstance,uint8_t alsoprogram) {
  int i;
  int ii;
  int size;
  STACK_QUEUE *q,*q1;

  if(mInstance->stack) {
    q=mInstance->stack;
    while(q) {
      q1=q;
      q=q1->next;
      free(q1);
      }
    }
  mInstance->stack=NULL;

	for(i=0; i<mInstance->nfiles; i++) {
    subClose(mInstance,i);
    }
  mInstance->nfiles=0;
  
  for(i=0; i<mInstance->nvariables; i++) {
		if(mInstance->variables[i].sval)
    	free(mInstance->variables[i].sval);
    mInstance->variables[i].sval=0;
		}
  if(mInstance->variables)
	 	free(mInstance->variables);
  mInstance->variables = NULL;
  mInstance->nvariables = 0;

  for(i=0; i<mInstance->ndimVariables; i++) {
    if(mInstance->dimVariables[i].d.str) {
      size = 1;
      for(ii=0; ii<mInstance->dimVariables[i].ndims; ii++)
        size *= mInstance->dimVariables[i].dim[ii];
      for(ii=0; ii<size; ii++) {
        if(mInstance->dimVariables[i].d.str[ii])
          free(mInstance->dimVariables[i].d.str[ii]);
        mInstance->dimVariables[i].d.str[ii]=0;
        }
      free(mInstance->dimVariables[i].d.str);
      mInstance->dimVariables[i].d.str=0;
			}
  	}

  if(mInstance->dimVariables)
		free(mInstance->dimVariables);
 
  mInstance->dimVariables = NULL;
  mInstance->ndimVariables = 0;

	if(alsoprogram) {
	  if(mInstance->lines)
			free(mInstance->lines);
	  mInstance->lines = 0;
  	mInstance->nlines = 0;
		}

  for(i=0; i<mInstance->ngosubs; i++) {
    if(mInstance->gosubStack[i].args)
      free((void*)mInstance->gosubStack[i].args);
    }
      
	mInstance->ngosubs=mInstance->ndos=mInstance->inBlock=0;
  
  mInstance->curline=0;

	mInstance->errorHandler.handler=0;
	mInstance->errorHandler.errorline=0;
	mInstance->errorHandler.errorcode=0;
	}


/*
  error report function.
  for reporting errors in the user's script.
  checks the global errorFlag.
  writes to fperr.
  Params: lineno - the line on which the error occurred
*/
void reportError(KOMMISSARREXX *mInstance,LINE_NUMBER_TYPE lineno) {


  if(mInstance->errorFlag == ERR_CLEAR) {
	  basicAssert(0);
		}
	else if(mInstance->errorFlag >= ERR_SYNTAX && mInstance->errorFlag <= ERR_STOP) {
// tolto a-capo, specie su display piccoli...
    if(lineno != -1) {
      char buf[64];
      sprintf(buf,"%s at line %u",error_msgs[mInstance->errorFlag],lineno +1);
      myTextOut(mInstance,buf);
	    goto acapo;
      }
    else {
      myTextOut(mInstance,error_msgs[mInstance->errorFlag]);
	    goto acapo;
      }
		}
	else {
    if(lineno != -1) {
      char buf[32];
  	  sprintf(buf,"ERROR at line %u", lineno +1);
      myTextOut(mInstance,buf);
      }
    else
      myTextOut(mInstance,"ERROR");
acapo:
    myCR(mInstance);
    myCR(mInstance);
	  }
	}


int16_t /*LINE_NUMBER_TYPE*/ findLine(KOMMISSARREXX *mInstance,LINE_NUMBER_TYPE no) {    // signed per indicare fine...

  if(no<mInstance->nlines)
    return no;
  else
    return -1;
	}

int16_t /*LINE_NUMBER_TYPE*/ findProcedure(KOMMISSARREXX *mInstance,const char *n) {    // signed per indicare errore...
  const char *p;
  int i;

  for(i=0; i<mInstance->nlines; i++) {
		p=skipSpaces(mInstance->lines[i].str);
    if(!strnicmp(p,n,strlen(n))) {
      p+=strlen(n);
      p=skipSpaces(p);
      if(*p == ':')
        return i;
      }
  	}
  return -1;
	}

int16_t findBlockEnd(KOMMISSARREXX *mInstance,uint8_t level) {
  int i;
  const char *p;
  TOKEN_NUM t;
  uint8_t myblock=mInstance->inBlock;

  for(i=mInstance->curline+1; i<mInstance->nlines; i++) {
    do {
      t=getToken(mInstance->string);
      match(mInstance,t);
      switch(t) {
        case DO:
          myblock++;
          break;
        case END:
          if(myblock == level) {
            return i;
            }
          myblock--;
          break;
        }
      } while(t!=EOL && t!=EOS);
  	}
  return -1;
	}
  
int8_t gotoBlockEnd(KOMMISSARREXX *mInstance,uint8_t level) {
  int i;
  const char *p;
  TOKEN_NUM t;
  uint8_t myblock=mInstance->inBlock;

  match(mInstance,mInstance->token);
  mInstance->curline++;
  while(mInstance->curline < mInstance->nlines) {
    mInstance->string=mInstance->lines[mInstance->curline].str;
    do {
      t=getToken(mInstance->string);
      match(mInstance,t);
      switch(t) {
        case DO:
          myblock++;
          break;
        case END:
          if(myblock == level) {
//            match(mInstance,mInstance->token);    // mangio EOL...
//            mInstance->curline++;
            return 1;
            }
          myblock--;
          break;
        }
      } while(t!=EOL && t!=EOS);
    mInstance->curline++;
  	}
  return 0;
	}
  
LINE_NUMBER_TYPE line(KOMMISSARREXX *mInstance) {
  LINE_NUMBER_TYPE answer;
  const char *str;
  TOKEN_NUM t;
  int8_t skipDo=0;

rifo:
	answer = 0;
  switch(mInstance->token) {
    case SAY:
		  doSay(mInstance);
		  break;
    case ARG:
		  doArg(mInstance);
		  break;
		case PARSE:
		  doParse(mInstance);
		  break;
		case DROP:
		  doDrop(mInstance);
		  break;
		case IF:
      {
      const char *oldPos;
			if(doIf(mInstance,&skipDo)) {  // potrei anche chiamare line() ricorsiva, ma...
        if(!skipDo)
          goto rifo;
        else
          answer=mInstance->curline;
        }
      else {
        do {
          oldPos=mInstance->string;
          t=getToken(mInstance->string);
          match(mInstance,t);  
          } while(t != ELSE && t != COMMA && t != EOS && t != EOL); // v.rem
        if(t == ELSE)
          goto rifo;
        else {
          if(getToken(mInstance->string) == ELSE) {
            if(t == EOL)
              mInstance->curline++;   // ma occhio a COMMA... in caso
            match(mInstance,ELSE);
            if(mInstance->token == DO) {
              match(mInstance,DO);
              mInstance->ndos++;
              mInstance->inBlock++;
              match(mInstance,mInstance->token);    // dovrebbe essere EOS o EOL o al limite comma...
              mInstance->curline=getNextStatement(mInstance,mInstance->string); // questo POTREBBE non servire... se da line() restituisco 0
              }
            goto rifo;
            }
          else {
            mInstance->string=oldPos;
        		mInstance->token = getToken(mInstance->string);
            }
          }
        }
      }
		  break;
		case ELSE:			// (defaults to here, i.e. when a IF statement is TRUE)
      {
do_else:
      match(mInstance,ELSE);
      if(mInstance->token == DO) {
        match(mInstance,DO);
        mInstance->ndos++;
        mInstance->inBlock++;
        gotoBlockEnd(mInstance,mInstance->inBlock);
        mInstance->ndos--;
        mInstance->inBlock--;
        }
		  return 0;
/*      do {
        t=getToken(mInstance->string);
        match(mInstance,t);  
        } while(t != COMMA && t != EOS && t != EOL); // v.rem*/
      }
      break;

		case END:
			answer = doEnd(mInstance);
#if 0
      if(mInstance->token == ELSE) {
        match(mInstance,ELSE);
        if(mInstance->token == DO) {
          match(mInstance,DO);
          mInstance->ndos++;
          mInstance->inBlock++;
          gotoBlockEnd(mInstance,mInstance->inBlock);
          mInstance->ndos--;
          mInstance->inBlock--;
          }
        else {
          return 0;   // v. else e rem
#endif
#if 0
          do {
            t=getToken(mInstance->string);
            match(mInstance,t);   // v.rem
            } while(/*t != COMMA &&*/  t != EOS && t != EOL);
#endif
//          }
//        }
		  break;
		case ADDRESS:
			doAddress(mInstance);
		  break;
		case TRACE:
			doTrace(mInstance);
		  break;
		case INTERPRET:
			doInterpret(mInstance);
		  break;
      
		case CALL:
			answer = doCall(mInstance);
			break;
		case SIGNAL:
			answer = doSignal(mInstance);
			break;
		case RETURN:
			answer = doReturn(mInstance);
			break;
		case SELECT:
		  doSelect(mInstance);
		  break;
		case WHEN:
      {
      const char *oldPos;
			if(doWhen(mInstance,&skipDo)) {
        if(!skipDo)
          goto rifo;
        else
          answer=mInstance->curline;
        }
      else {
        do {
          oldPos=mInstance->string;
          t=getToken(mInstance->string);
          match(mInstance,t);  
          } while(t != COMMA && t != EOS && t != EOL);
        }

      mInstance->string=oldPos;
      mInstance->token = getToken(mInstance->string);
      }
		  break;
		case OTHERWISE:
      {
      const char *oldPos;
			if(doOtherwise(mInstance,&skipDo)) {
        if(!skipDo)
          goto rifo;
        else
          answer=mInstance->curline;
        }
      else {
        do {
          oldPos=mInstance->string;
          t=getToken(mInstance->string);
          match(mInstance,t);  
          } while(t != COMMA && t != EOS && t != EOL);
        }
      mInstance->string=oldPos;
      mInstance->token = getToken(mInstance->string);
      }
		  break;
		case EXIT:
      doExit(mInstance);
		  answer = -1;    // per interrompere esecuzione :)
		  break;
		case NOP:
      doNop(mInstance);
		  break;
		case QUEUE:
      doQueue(mInstance);
		  break;

		case PUSH:
		  doPush(mInstance);
		  break;
		case PULL:
		  doPull(mInstance);
		  break;
      
		case REM:
		  doRem(mInstance);
		  goto rifo;
		  break;
		case COMMA:
      match(mInstance,COMMA);   // in teoria c'è già sotto...
      goto rifo;
		case DO:
		  doDo(mInstance);
		  break;
		case ITERATE:
		  answer = doIterate(mInstance);
		  break;
		case LEAVE:
		  answer = doLeave(mInstance);
		  break;
		case NUMERIC:
		  doNumeric(mInstance);
		  break;
		case OPTIONS:
		  doOptions(mInstance);
		  break;
      
#ifdef REGINA_REXX
		case BEEP:
		  doBeep(mInstance);
		  break;
		case CHDIR:
		  doChdir(mInstance);
		  break;
		case DIRECTORY_TOK:
		  doDirectory(mInstance);
		  break;
		case SLEEP:
		  doSleep(mInstance);
		  break;
		case SEEK:
		  doSeek(mInstance);
		  break;
#endif

		default:
			doLet(mInstance);
		  break;
	  }

	if(!answer) {
	  if(mInstance->token == ELSE)    // forse non capita...
			goto do_else;
    
    if(skipDo) {
      gotoBlockEnd(mInstance,mInstance->inBlock);
      mInstance->ndos--;
      mInstance->inBlock--;
			}


	  if(mInstance->token != EOS) {
			str = mInstance->string;
			while(isspace(*str)) {
			  if(*str == '\n' || *str == ',')
			    break;
			  str++;
				}
	
			if(*str == ',') {
				match(mInstance,COMMA);   // in teoria c'è già sopra...
				if(!mInstance->errorFlag)			// specie per STOP
			  	goto rifo;
				else
					goto fine;
				}
			if(*str != '\n')
			  setError(mInstance,ERR_SYNTAX);
	  	}
		}

fine:
  return answer;
	}


void doSay(KOMMISSARREXX *mInstance) {
  char *str;
  LNUM_TYPE x;
	void *ftemp=NULL;
  enum _FILE_TYPES filetype=-1;

  match(mInstance,SAY);		// 

//  mInstance->string=(char *)skipSpaces(mInstance->string);
  
  if(mInstance->token != EOS && mInstance->token != EOL) /*if(mInstance->token == VALUE || mInstance->token == FLTID) */{
    str=expr(mInstance);
    if(!str)
      goto print_cr;
    
    
//    str="123456789012";

    switch(datatype(str)) {
      case STRID:
  // if errorFlag NON dovrebbe stampare...
        switch(filetype) {
          case FILE_COM:
          {
            char *p=str;
            while(*p)
              WriteSerial(*p++);
          }
            break;
    #ifdef USA_USB_SLAVE_CDC
          case FILE_CDC:
            printf("%s",str);
            break;
    #endif
    #ifdef USA_WIFI
          case FILE_TCP:
            strcat(rxBuffer,str);
            break;
          case FILE_UDP:
            strcat(rxBuffer,str);
            break;
    #endif
          case FILE_DISK:
            if(SuperFileWrite(ftemp,str,strlen(str)) != strlen(str)) {     // 
              setError(mInstance,ERR_FILE);
              }
            break;
          default:
            myTextOut(mInstance,str);
            break;
          }

        break;
        
      case INTID:
        {
        int16_t i;
        char buf[digitSize+1];

        i=to_int(str);
        to_string_i(i,buf);
        switch(filetype) {
          case FILE_COM:
            {
              char *p=buf;
              while(*p)
                WriteSerial(*p++);
            }
            break;
    #ifdef USA_USB_SLAVE_CDC
          case FILE_CDC:
            printf("%s",buf);
            break;
    #endif
    #ifdef USA_WIFI
          case FILE_TCP:
            strcat(rxBuffer,buf);
            break;
          case FILE_UDP:
            strcat(rxBuffer,buf);
            break;
    #endif
          case FILE_DISK:
            if(SuperFileWrite(ftemp,buf,strlen(buf)) != strlen(buf)) {     // 
              setError(mInstance,ERR_FILE);
              }
            break;
          default:
            myTextOut(mInstance,buf);
            break;
          }
        }
        break;
        
      default:
        {
        long long n;
        char buf[digitSize+1];

        x = to_num(str);
    // if errorFlag NON dovrebbe stampare...

        // TUTTA STA ROBA potrebbe andare in to_num qua!!
        if(x != (long long)x)	{	// un trucchetto per stampare interi o float
          unsigned char i;

          // metto lo spazio prima dei numeri... se non negativi...
          /* USARE DIGITS! digitSizeDecimal */
//          sprintf(buf,(STRINGFARPTR)"%lf", x); 
          ldtoa(x,buf,digitSizeDecimal);
          for(i=strlen(buf)-1; i; i--) {
            if(buf[i] == '0')
              buf[i]=0;
            else
              break;
            }
          }
#if 0
        n=(long long) (fabsl(x - (long double)(long long)x ) * 1000000.0L /* USARE DIGITS / digitSizeDecimal!*/);
        if(n)	{	// un trucchetto per stampare interi o float
          unsigned char i;

          // metto lo spazio prima dei numeri... se non negativi...
          /* USARE DIGITS! digitSizeDecimal */
          if(x>=0)
            sprintf(buf,(STRINGFARPTR)" %lld.%06llu", (long long) x, (long long) n); 
          else
            sprintf(buf,(STRINGFARPTR)"%lld.%06llu", (long long) x, (long long) n); 
          for(i=strlen(buf)-1; i; i--) {
            if(buf[i] == '0')
              buf[i]=0;
            else
              break;
            }
          }
#endif
        else {
          if(x>=0)
            sprintf(buf," %lld", (long long)x); 	// metto lo spazio prima dei numeri... se non negativi...
          else
            sprintf(buf,"%lld", (long long)x); 
          }
        switch(filetype) {
          case FILE_COM:
            {
              char *p=buf;
              while(*p)
                WriteSerial(*p++);
            }
            break;
    #ifdef USA_USB_SLAVE_CDC
          case FILE_CDC:
            printf("%s",buf);
            break;
    #endif
    #ifdef USA_WIFI
          case FILE_TCP:
            strcat(rxBuffer,buf);
            break;
          case FILE_UDP:
            strcat(rxBuffer,buf);
            break;
    #endif
          case FILE_DISK:
            if(SuperFileWrite(ftemp,buf,strlen(buf)) != strlen(buf)) {     // 
              setError(mInstance,ERR_FILE);
              }
            break;
          default:
            myTextOut(mInstance,buf);
            break;
          }
        }
        break;
      }
    free(str);
    }

  	
// naturalmente, se rxBuffer sfora, errore?!  diverso tra tcp e udp..
// ev. cmq anche SuperFile e 
print_cr:  
  switch(filetype) {
    case FILE_COM:
      WriteSerial('\r'); WriteSerial('\n');
      break;
#ifdef USA_USB_SLAVE_CDC
    case FILE_CDC:
      putchar('\r'); putchar('\n');
      break;
#endif
#ifdef USA_WIFI
    case FILE_TCP:
      strcat(rxBuffer,"\r\n");
      send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
      break;
    case FILE_UDP:
      strcat(rxBuffer,"\r\n");
      send((SOCKET)(int)ftemp, rxBuffer, strlen(rxBuffer), 0);
      break;
#endif
    case FILE_DISK:
      if(SuperFileWrite(ftemp,"\r\n",2) != 2) {     // 
        setError(mInstance,ERR_FILE);
        }
      break;
    default:
#ifndef USING_SIMULATOR
      putchar('\n');
      mInstance->Cursor.y++; mInstance->Cursor.x=0;
      if(mInstance->Cursor.y >= ScreenText.cy) {
        mInstance->Cursor.y--;
        }
      SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
#endif
      break;
    }

  //ScrollWindow()
	}


void doArg(KOMMISSARREXX *mInstance) {
  char buf[128];
  BYTE argnum=1;
  char id[IDLENGTH];
  IDENT_LEN len;
  LLVALUE lv;
  LVARIABLE v;
  char *args;
 
  match(mInstance,ARG);   // ARG funzia solo dentro proc, PARSE ARG ovunque!  NO...

  if(mInstance->ngosubs>0) {
    if(mInstance->gosubStack[mInstance->ngosubs-1].args) {
      args=(char *)mInstance->gosubStack[mInstance->ngosubs-1].args;
      }
    else
      args=(char *)EmptyString;
    }
  else
    args=scriptArgs;
          
rifo:
  switch(mInstance->token) {
    case DOT:
      match(mInstance,DOT);
      argnum++;
      goto rifo;
      break;
    case STRID:
      lvalue(mInstance,&lv);
      get_args(args,argnum,buf);
      strupr(buf);    // qua SEMPRE
      
//      if(*buf) {
        switch(v.type=datatype(buf)) {
          case FLTID:
            v.d.dval=to_num(buf);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case INTID:
            v.d.ival=to_int(buf);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case STRID:
            v.d.sval=(char *)buf;
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          }
//        }
//      else {
//        deleteVariable(mInstance,id,NULL);    // direi... o lasciarla vuota??
//        }

      argnum++;
      goto rifo;
      break;
    default:
      break;
    }
  
	}

void doParse(KOMMISSARREXX *mInstance) {
  const char *str;
  BYTE upp=FALSE;
	TOKEN_NUM t;
  char buf2[128];
  char buf[64];
  BYTE argnum=1;
  char id[IDLENGTH];
  IDENT_LEN len;
  LLVALUE lv;
  LVARIABLE v;
  VARIABLESTRING *v1;
  char *args=NULL;

  match(mInstance,PARSE);   // ARG funzia solo dentro proc, PARSE ARG ovunque!
rifo:
  t=getToken(mInstance->string);
  switch(t) {
    case UPPER:
      // in IBM c'è anche LOWER e CASELESS...
      match(mInstance,UPPER);
      upp=TRUE;
      goto rifo;
      break;
    case ARG:
      match(mInstance,ARG);
      if(mInstance->ngosubs>0) {
        if(mInstance->gosubStack[mInstance->ngosubs-1].args) {
          args=(char *)mInstance->gosubStack[mInstance->ngosubs-1].args;
          }
        else
          args=(char *)EmptyString;
        }
      else
        args=scriptArgs;
      break;
    case LINEIN:
      match(mInstance,LINEIN);
      inputString(buf2,127,"?",0);
      args=buf2;
      break;
    case PULL:
      match(mInstance,PULL);
      doPull(mInstance);
      inputString(buf2,127,"?",0);
      // finire...
      args=buf2;
      break;
    case SOURCE:
      match(mInstance,SOURCE);
      strcpy(buf,strchr(_PC_PIC_CPU_C,' ')+1);
      *strchr(buf,' ')=0;   // tolgo spazio, sicuro :)
      sprintf(buf2,"%s %s %s",buf,"<invoked>",scriptArgs /*manca il nome script...*/);
      args=buf2;
      break;
    case VERSION:
      match(mInstance,VERSION);
      strcpy(buf,KOMMISSARREXX_COPYRIGHT_STRING);
      *strchr(buf,' ')=0;   // tolgo spazio, sicuro :)
      sprintf(buf2,"\'%s\' \'%s\'",buf,__DATE__);
      args=buf2;
      break;
    case VAR:
      match(mInstance,VAR);
			getId(mInstance,mInstance->string, id, &len);
			match(mInstance,STRID);
      v1=findVariable(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
      if(v1) {
        strncpy(buf2,v1->sval,sizeof(buf2)-1);
        buf2[sizeof(buf2)-1]=0;
        }
      else {
        setError(mInstance,ERR_NOSUCHVARIABLE);
        return;
        }
      args=buf2;
      break;
    case VALUETOK:
      match(mInstance,VALUETOK);
      str = expr(mInstance);
      if(str) {
        strncpy(buf2,str,sizeof(buf2)-1);
        buf2[sizeof(buf2)-1]=0;
        free((void*)str);
        }
      
      match(mInstance,WITH);
      
      args=buf2;
      break;
    default:
      setError(mInstance,ERR_SYNTAX);
      return;
      break;
    }
  
rifo2:
  switch(mInstance->token) {
    case DOT:
      match(mInstance,DOT);
      argnum++;
      goto rifo2;
      break;
    case STRID:
      lvalue(mInstance,&lv);
      get_args(args,argnum,buf);
      if(upp)
        strupr(buf);
//      if(*buf) {
        switch(v.type=datatype(buf)) {
          case FLTID:
            v.d.dval=to_num(buf);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case INTID:
            v.d.ival=to_int(buf);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case STRID:
            v.d.sval=(char *)buf;
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          }
  //      }
  //    else {
  //      deleteVariable(mInstance,id,NULL);    // direi... o lasciarla vuota??
  //      }
      argnum++;
      goto rifo2;
      break;
    default:
      break;
    }
  }
  

void doSelect(KOMMISSARREXX *mInstance) {

	match(mInstance,SELECT);

  if(mInstance->ndos > MAXFORS-1) {
		setError(mInstance,ERR_TOOMANYFORS);
	  }
  else {
    sprintf(mInstance->doStack[mInstance->ndos].id,"#SEL%%%u",mInstance->curline);
  	mInstance->doStack[mInstance->ndos].step = mInstance->doStack[mInstance->ndos].toval = 0;
		mInstance->doStack[mInstance->ndos].nextline = getNextStatement(mInstance,mInstance->string);
    mInstance->doStack[mInstance->ndos].expr=NULL;
		mInstance->doStack[mInstance->ndos].parentBlock = mInstance->ndos;
    mInstance->ndos++;
    mInstance->inBlock++;
    }

	}

int8_t doWhen(KOMMISSARREXX *mInstance,uint8_t *isDo) {
  int8_t condition;

  match(mInstance,WHEN);
  condition = boolExpr(mInstance);
 	if(mInstance->ndos==0) {
    setError(mInstance,ERR_NOFOR);    // cambiare!!
    return 0;
    }

	if(condition)
    mInstance->doStack[mInstance->ndos-1].step = 1;   // marker per WHEN
  match(mInstance,THEN);
  if(mInstance->token != DO) {
    return condition;
    }
  else {
    match(mInstance,DO);
    mInstance->ndos++;
    mInstance->inBlock++;
    
    if(condition) {
      *isDo=1;
      match(mInstance,mInstance->token);    // dovrebbe essere EOS o EOL o al limite comma...
      mInstance->curline=getNextStatement(mInstance,mInstance->string); // questo POTREBBE non servire... se da line() restituisco 0
      // PROVARE
      }
    else {
      *isDo=0;
      gotoBlockEnd(mInstance,mInstance->inBlock);
      mInstance->ndos--;
      mInstance->inBlock--;
      }
    
    return condition;
    }

	}

int8_t doOtherwise(KOMMISSARREXX *mInstance,uint8_t *isDo) {
  int8_t condition;

  match(mInstance,OTHERWISE);
 	if(mInstance->ndos==0) {
    setError(mInstance,ERR_NOFOR);    // cambiare!!
    return 0;
    }
  condition=!mInstance->doStack[mInstance->ndos-1].step;
	mInstance->doStack[mInstance->ndos-1].toval = 1;   // marker per OTHERWISE
  if(mInstance->token != DO) {
    return condition;
    }
  else {
    match(mInstance,DO);
    mInstance->ndos++;
    mInstance->inBlock++;
  
    if(condition) {
      *isDo=1;
      match(mInstance,mInstance->token);    // dovrebbe essere EOS o EOL o al limite comma...
      mInstance->curline=getNextStatement(mInstance,mInstance->string); // questo POTREBBE non servire... se da line() restituisco 0
      // PROVARE
      }
    else {
      *isDo=0;
      gotoBlockEnd(mInstance,mInstance->inBlock);
      mInstance->ndos--;
      mInstance->inBlock--;
      }
    
    return condition;
    }
  }


LINE_NUMBER_TYPE doSignal(KOMMISSARREXX *mInstance) {
	TOKEN_NUM t;
  LINE_NUMBER_TYPE toline;
  const char *str=NULL;
  LINE_NUMBER_TYPE retVal=0;
    
	match(mInstance,SIGNAL);
	switch(mInstance->token) {
    case ON:
      match(mInstance,ON);
      switch(mInstance->token) {
        case ERRORTOK:
          match(mInstance,ERRORTOK);
          match(mInstance,NAME);
          str=expr(mInstance);
          mInstance->errorHandler.handler = findProcedure(mInstance,str);
          // che differenza c'è con CALL, qua?
          retVal=0;
          break;
        case FAILURE:
        case HALT:
        case NOTREADY:
        case NOVALUE:
        case SYNTAX:
          break;
        default:
          setError(mInstance,ERR_BADVALUE);
          retVal=-1;
          break;
        }
      break;
    case OFF:
      match(mInstance,OFF);
      if(mInstance->token==ERRORTOK) {
        match(mInstance,ERRORTOK);
        getToken(mInstance->string);
        mInstance->errorHandler.handler = 0;
        retVal=0;
        }
      else {
        setError(mInstance,ERR_BADVALUE);
        retVal=-1;
        }
    default:
      str=expr(mInstance);
      toline = findProcedure(mInstance,str);
      if((int16_t)toline < 0) {
        setError(mInstance,ERR_NOSUCHVARIABLE);   // migliorare...
        retVal=-1;
        }
      retVal=toline;
      break;
    }
  
  free((void*)str);
  return retVal;
	}


 
void doInterpret(KOMMISSARREXX *mInstance) {
  char *str;
  KOMMISSARREXX myInstance;
  int i;

  memset(&myInstance,0,sizeof(KOMMISSARREXX));
  myInstance.hWnd=mInstance->hWnd;
  myInstance.threadID=mInstance->threadID;
  myInstance.Color=0xff0000; myInstance.ColorBK=0xffffff;   // per sfizio :)
  myInstance.ColorPalette=RED; myInstance.ColorPaletteBK=WHITE;
  myInstance.Cursor=mInstance->Cursor;
  myInstance.errorFlag=0;
  myInstance.ngosubs=myInstance.ndos=0;
          
  match(mInstance,INTERPRET);
  str=expr(mInstance);
  if(str) {
// mmm no    kommissarrexx(mInstance,str);
    myInstance.string = str;
    myInstance.token = getToken(myInstance.string);
    myInstance.errorFlag = 0;
    i = line(&myInstance);
    if(myInstance.errorFlag)
      ;
    
//    mInstance->Cursor=myInstance->Cursor;    // volendo... non so voglio!

    cleanup(&myInstance,0);
    free(str);
    }
  
	}

void doAddress(KOMMISSARREXX *mInstance) {
  const char *str;
  VARIABLESTRING *var;
  LVARIABLE v;
  const char *theRC="RC";
  
  match(mInstance,ADDRESS);
  str=expr(mInstance);
  if(str) {
    
    execCmd(str,NULL,NULL /* mettere nome script rexx? */);
    
    var = findVariable(mInstance,theRC,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
            mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
		if(!var)
			var = addVar(mInstance,theRC,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
    if(!var) {
      setError(mInstance,ERR_OUTOFMEMORY);
      return;
      }
    v.d.ival=errorLevel;
    v.type=INTID;
    subAssignVar(mInstance,&var->sval,&v);
    free((void*)str);
    }
	}


void doTrace(KOMMISSARREXX *mInstance) {
  const char *str;
  
  match(mInstance,TRACE);
  str=expr(mInstance);
  if(str) {
    if(isdigit(*str))
      ;   // gestire, v.
    else
      mInstance->traceLevel=toupper(*str);    // accttare solo valori giusti!
    free((void*)str);
    }
  else
    mInstance->traceLevel='O';
    
/* A ?All
C ?Commands
E ?Errors
F ?Failure
I ?Intermediates
L ?Labels
N ?Normal
O ?Off
R ?Results
? ?Toggles interactive tracing on or off; can be followed by any letter in this list only.
A positive whole number?If in interactive trace, skips the number of pauses specified
A negative whole number?Inhibits tracing for the number of clauses specified
 * */
	}
  
void doList(KOMMISSARREXX *mInstance) {
  int i;
  char buf[256],j;
  
  i=0;
 	while(i < mInstance->nlines) {			//Loop through Script Running each Line
    for(j=0; j<255; j++) {
      buf[j]=mInstance->lines[i].str[j];
      if(buf[j]=='\n')
        break;
      }
    buf[j]=0;
		myTextOut(mInstance,buf);
    myCR(mInstance);
   	i++;
    }
  
    myCR(mInstance);

  // fare...
	// ovvero :)
/*	if(theScript)
    puts(theScript);
 * */
	}
  

int subLoad(const char *s,char *p) {
  SUPERFILE f;
  int n=0;
  char *filename;
  
  *p=0;
  getDrive(s,&f,&filename);
  if(SuperFileOpen(&f,filename,'r')) { 
    while(1) {
      if(SuperFileRead(&f,p,1) == 1) {  // o FSfgets??
        if(*p != 13 && *p != 9)    // cmq tolgo CR e TAB
          p++;
        n++;
        }
      else
        break;
      }
    *p=0;
    SuperFileClose(&f);
    return n;
    }
  else {
    return -1;
    }
  }
  

void subAssignVar(KOMMISSARREXX *mInstance, char **v, LVARIABLE *d) {
  char buf[digitSize+1],*p;
  
  switch(d->type) {
    case STRID:
      p=d->d.sval;
      break;
    case INTID:
      p=to_string_i(d->d.ival,buf);
      break;
    case FLTID:
      p=to_string(d->d.dval,buf);
      break;
    }
  if(*v)
    free(*v);
  *v=mystrdup(mInstance,p);
  }
void doLet(KOMMISSARREXX *mInstance) {
	LLVALUE lv;
  LVARIABLE v;
	const char *str;
  char id[IDLENGTH];
  IDENT_LEN len;
  TOKEN_NUM t;
  const char *p;
 
  mInstance->string=skipSpaces(mInstance->string);
	getId(mInstance, mInstance->string, id, &len);   // devo andare a vedere cosa c'è dopo la stringa/var/label...
  p=skipSpaces(mInstance->string+len);

  switch(*p) {
    case ':':
      match(mInstance,STRID);
      match(mInstance,COLON);
      if(mInstance->ngosubs>0) {
        mInstance->gosubStack[mInstance->ngosubs-1].line=mInstance->curline;
        mInstance->gosubStack[mInstance->ngosubs-1].privateVars=0;
        }
      else    // io ci metterei un errore... anche se il doc non lo dice (prog. principale che "cade" in una subroutine)
        ;
rifo:
      switch(mInstance->token) {
        case PROCEDURE:
          match(mInstance,PROCEDURE);
          if(mInstance->ngosubs>0)
            mInstance->gosubStack[mInstance->ngosubs-1].privateVars=1;
          else
            ;
          goto rifo;
          break;
        case EXPOSE:  // senza PROCEDURE tutte globali, con PROC quelle create qua son locali ma non se han lo stesso nome...
          // quindi boh verificare EXPOSE
          match(mInstance,EXPOSE);
rifo2:
          switch(mInstance->token) {
            case OPAREN:
              match(mInstance,OPAREN);      // vedere cosa fanno le parentesi qua...
              goto rifo2;
              break;
            case STRID:


              goto rifo2;
              break;
            default:
              break;
            }
          break;
        default:
          break;
        }
/*      while(mInstance->token != EOS && mInstance->token != EOL) {
        getToken(mInstance->string);
        }*/
      break;
    default:
    //eseguire procedure? o comando esterno?? solo se tra apici??
      if(*mInstance->string=='\'' || *mInstance->string=='"') {
        char myBuf[128];
        
        strncpy(myBuf,mInstance->string+1,127);
        if(p=strchr(myBuf,*mInstance->string))
          *(char *)p=0;
        else if(p=strchr(myBuf,'\n'))   // safety
          *(char *)p=0;
        else
          myBuf[127]=0;
        execCmd(myBuf,NULL,NULL /* mettere nome script rexx? */);
        mInstance->Cursor.x=LOWORD(GetXY());    // per aggiornare pos cursore dopo CLS ecc!
        mInstance->Cursor.y=HIWORD(GetXY());
        do {
          t=getToken(mInstance->string);
          match(mInstance,t);   // ha senso fare così??
          } while(t != COMMA && t != EOS && t != EOL);
        }
      else {
        if(mInstance->token != COMMA && mInstance->token != EOS && mInstance->token != EOL)
          setError(mInstance,ERR_SYNTAX);
        }
      break;
    case '=':
      lvalue(mInstance,&lv);
      match(mInstance,EQUALS);

      str=expr(mInstance);
      if(str) {
        switch(v.type=datatype(str)) {
          case FLTID:
            v.d.dval=to_num(str);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case INTID:
            v.d.ival=to_int(str);
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          case STRID:
            v.d.sval=(char *)str;
            subAssignVar(mInstance,lv.d.sval,&v);
            break;
          }

        free((void *)str);
        break;
      }
    }
	}

BYTE deleteVariable(KOMMISSARREXX *mInstance,const char *id,void *parent) {
  int i,j;
  
  for(i=0; i<mInstance->nvariables; i++) {
		if(mInstance->variables[i].parent == parent && !stricmp(mInstance->variables[i].id, id)) {
      goto doDelete;
      }
    }
  for(i=0; i<mInstance->nvariables; i++) {
		if(!stricmp(mInstance->variables[i].id, id)) {
doDelete:
      if(mInstance->variables[i].sval)
        free(mInstance->variables[i].sval);
      for(j=i+1; j<mInstance->nvariables; j++,i++) {
        mInstance->variables[i] = mInstance->variables[j];
        }

      switch(mInstance->traceLevel) {
        case 'A':
          err_printf("%svariable deleted %s\r\n",parent ? "-" : "",id);    // finire..
          break;
        case '?':
          inputKey();
          break;
        default:
          break;
        }

      mInstance->nvariables--;
      return 1;
      }
    }
  return 0;
  }
void doDrop(KOMMISSARREXX *mInstance) {
  char id[IDLENGTH];
  IDENT_LEN len;
  
  match(mInstance,DROP);
rifo:
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,STRID);
  deleteVariable(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
  if(mInstance->token == STRID)
    goto rifo;
	}

void doDim(KOMMISSARREXX *mInstance) {
  BYTE ndims=0;
  DIM_SIZE dims[MAXDIMS+1];
  char name[IDLENGTH];
  IDENT_LEN len;
  LDIMVARSTRING *dimvar;
  unsigned char i;
  int size = 1;

  match(mInstance,DOT);

  switch(mInstance->token) {
		case DIMSTRID:
      getId(mInstance,mInstance->string, name, &len);
			match(mInstance,mInstance->token);
			dims[ndims++] = to_int(expr(mInstance));
			while(mInstance->token == COMMA) {
				match(mInstance,COMMA);
				dims[ndims++] = to_int(expr(mInstance));
				if(ndims > MAXDIMS)	{
					setError(mInstance,ERR_TOOMANYDIMS);
					return;
					}
				} 

		  match(mInstance,CPAREN);
	  
			for(i=0; i<ndims; i++) {
				if(dims[i] < 0) {
					setError(mInstance,ERR_BADSUBSCRIPT);
					return;
				}
			}
	  switch(ndims) {
	    case 1:
				dimvar = dimension(mInstance,name, 1, dims[0]);
				break;
			case 2:
				dimvar = dimension(mInstance,name, 2, dims[0], dims[1]);
				break;
			case 3:
				dimvar = dimension(mInstance,name, 3, dims[0], dims[1], dims[2]);
				break;
			case 4:
				dimvar = dimension(mInstance,name, 4, dims[0], dims[1], dims[2], dims[3]);
				break;
			case 5:
				dimvar = dimension(mInstance,name, 5, dims[0], dims[1], dims[2], dims[3], dims[4]);
				break;
			}
			break;
		default:
	    setError(mInstance,ERR_SYNTAX);
	    return;
	  }
  if(dimvar == NULL) {
	/* out of memory */
		setError(mInstance,ERR_OUTOFMEMORY);
		return;
		}


  for(i=0; i<dimvar->ndims; i++)
		size *= dimvar->dim[i];
  
  if(mInstance->token == EQUALS) {
    match(mInstance,EQUALS);

    i=0;
    if(dimvar->d.str[i])
      free(dimvar->d.str[i]);
    dimvar->d.str[i++] = expr(mInstance);

    while(mInstance->token == COMMA && i < size)	{
      match(mInstance,COMMA);
      if(dimvar->d.str[i])
        free(dimvar->d.str[i]);
      dimvar->d.str[i++] = expr(mInstance);
      if(mInstance->errorFlag)
        break;
      }
		
		if(mInstance->token == COMMA)
			setError(mInstance,ERR_TOOMANYINITS);
		}
  
  else {    // il baciapile non inizializzava a 0... ;) !
    i=0;
    while(i < size)
      dimvar->d.str[i++]=NULL;
    }

	}


int8_t doIf(KOMMISSARREXX *mInstance,uint8_t *isDo) {
  int8_t condition;

  match(mInstance,IF);
  condition = boolExpr(mInstance);
  match(mInstance,THEN);
  if(mInstance->token != DO) {
    return condition;
    }
  else {
    match(mInstance,DO);
    mInstance->ndos++;
    mInstance->inBlock++;

    if(condition) {
      *isDo=1;
      match(mInstance,mInstance->token);    // dovrebbe essere EOS o EOL o al limite comma...
      mInstance->curline=getNextStatement(mInstance,mInstance->string); // questo POTREBBE non servire... se da line() restituisco 0
      // PROVARE
      }
    else {
      *isDo=0;
      gotoBlockEnd(mInstance,mInstance->inBlock);
      mInstance->ndos--;
      mInstance->inBlock--;
      }
    
    return condition;
    }

	}

LINE_NUMBER_TYPE doIterate(KOMMISSARREXX *mInstance) {
  
  match(mInstance,ITERATE);
	if(mInstance->ndos>0) {
    return mInstance->doStack[mInstance->ndos-1].nextline;
    }
  else
    setError(mInstance,ERR_NOFOR);
	}
  
LINE_NUMBER_TYPE doLeave(KOMMISSARREXX *mInstance) {
  LINE_NUMBER_TYPE n;
  
  match(mInstance,LEAVE);
	if(mInstance->ndos>0) {
    
    n=gotoBlockEnd(mInstance,mInstance->inBlock);
  	mInstance->ndos--;
    mInstance->inBlock--;
    
    return n;
    }
  else
    setError(mInstance,ERR_NOFOR);
	}

LINE_NUMBER_TYPE doCall(KOMMISSARREXX *mInstance) {
  LINE_NUMBER_TYPE toline;
  TOKEN_NUM t;
  char *str=NULL,buf[128],*p;
  LINE_NUMBER_TYPE retVal;

  match(mInstance,CALL);
	switch(mInstance->token) {
    case ON:
      match(mInstance,ON);
      switch(mInstance->token) {
        case ERRORTOK:
          match(mInstance,ERRORTOK);
          match(mInstance,NAME);
          str=expr(mInstance);
          mInstance->errorHandler.handler = findProcedure(mInstance,str);
          // che differenza c'è con SIGNAL, qua?
          retVal=0;
          break;
        case FAILURE:
        case HALT:
        case NOTREADY:
          break;
        default:
          setError(mInstance,ERR_BADVALUE);
          retVal=-1;
          break;
        }
      break;
    case OFF:
      match(mInstance,OFF);
      if(mInstance->token==ERRORTOK) {
        match(mInstance,ERRORTOK);
        getToken(mInstance->string);
        mInstance->errorHandler.handler = 0;
        retVal=0;
        }
      else {
        setError(mInstance,ERR_BADVALUE);
        retVal=-1;
        }
    default:
      str=expr(mInstance);
      toline = findProcedure(mInstance,str);
      // può anche essere una funzione interna, a meno che non sia tra apici, dice
      if((int16_t)toline < 0) {
        setError(mInstance,ERR_NOSUCHVARIABLE);   // migliorare...
        retVal=-1;
        }
      if(mInstance->ngosubs >= MAXGOSUB) {
        setError(mInstance,ERR_TOOMANYGOSUB);
        retVal=-1;
        }
      strncpy(buf, skipSpaces(mInstance->string),sizeof(buf)-1);
      buf[sizeof(buf)-1]=0;
      if(p=strchr(buf,'\n'))
        *p=0;
      mInstance->gosubStack[mInstance->ngosubs].args = mystrdup(mInstance,buf);
      mInstance->gosubStack[mInstance->ngosubs].returnline = getNextStatement(mInstance,mInstance->string);
      mInstance->ngosubs++;
      retVal=toline;
      break;
    }
  
  free(str);
  return retVal;
	}
  

LINE_NUMBER_TYPE doReturn(KOMMISSARREXX *mInstance) {
  TOKEN_NUM t;
  const char *str,*theReturn="RETURN";
  int i,j;

  match(mInstance,RETURN);
  if(mInstance->ngosubs <= 0) {
	  setError(mInstance,ERR_NORETURN);
	  return -1;
		}
  t=getToken(mInstance->string);
  if(t == VALUE) {
    VARIABLESTRING *var;
    LVARIABLE v;
    
    var = findVariable(mInstance,theReturn,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
            mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
		if(!var)
			var = addVar(mInstance,theReturn,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
    if(!var) {
      setError(mInstance,ERR_OUTOFMEMORY);
      return;
      }
    str=expr(mInstance);
    if(str) {
      switch(v.type=datatype(str)) {
        case FLTID:
          v.d.dval=to_num(str);
          subAssignVar(mInstance,&var->sval,&v);
          break;
        case INTID:
          v.d.ival=to_int(str);
          subAssignVar(mInstance,&var->sval,&v);
          break;
        case STRID:
          v.d.sval=(char *)str;
          subAssignVar(mInstance,&var->sval,&v);
          break;
        }
      free((void*)str);
      }
    }
  else {
    deleteVariable(mInstance,theReturn,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
    }
  
  mInstance->ngosubs--;

  if(mInstance->gosubStack[mInstance->ngosubs].args)
    free((void *)mInstance->gosubStack[mInstance->ngosubs].args);
  
  if(mInstance->gosubStack[mInstance->ngosubs].privateVars) {
rifo:
    for(i=0; i<mInstance->nvariables; i++) {
      if(mInstance->variables[i].parent == (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs].line) {
        if(mInstance->variables[i].sval)
          free(mInstance->variables[i].sval);
        for(j=i+1; j<mInstance->nvariables; j++,i++) {
          mInstance->variables[i] = mInstance->variables[j];
          }
        mInstance->nvariables--;
        goto rifo;
        }
      }
    }

  return mInstance->gosubStack[mInstance->ngosubs].returnline;
	}


void doDo(KOMMISSARREXX *mInstance) {
  LLVALUE lv;
  char id[IDLENGTH],buf[8];
  const char *exprPos=NULL,*str,*str2;
  IDENT_LEN len;
  LNUM_TYPE initval, toval, stepval;
  LVARIABLE v1;
  VARIABLESTRING *v;
  BYTE conditionsFound=0;

#warning fare FREE su tutte le expr!
  match(mInstance,DO);
  switch(mInstance->token) {
    case FLTID:
//    case STRID:
#warning SOLO FLTID
      getId(mInstance,mInstance->string, id, &len);

      lvalue(mInstance,&lv);
      if(lv.type != FLTID) {
        setError(mInstance,ERR_TYPEMISMATCH);
        return;
        }
      match(mInstance,EQUALS);
      str=expr(mInstance);
      if(str) {
        initval = to_int(str);
        free((void*)str);
        }
      stepval = 1;
rifo:
      switch(mInstance->token) {
        case BY:
          match(mInstance,BY);
          str=expr(mInstance);
          if(str) {
            stepval = to_int(str);
            free((void*)str);
            }
          goto rifo;
          break;
        case TO:
          match(mInstance,TO);
          str=expr(mInstance);
          if(str) {
            toval = to_int(str);
            free((void*)str);
            }
          conditionsFound=1;
          goto rifo;
          break;
        case FOR:
          match(mInstance,FOR);
          str=expr(mInstance);
          if(str) {
            toval = to_int(str);
            free((void*)str);
            }
          goto rifo;
          break;
        default:
          break;
        }

      if(conditionsFound) {
        v1.type=INTID;
        v1.d.ival=initval;
        subAssignVar(mInstance,lv.d.sval,&v1);
set_var:
        strcpy(mInstance->doStack[mInstance->ndos].id, id);
        }
      else {
    		setError(mInstance,ERR_SYNTAX);
        return;
        }
      break;
    case VALUE:
      str=expr(mInstance);
      if(str) {
        toval = to_int(str);
        free((void*)str);
        }
      initval = 1;
      stepval = 1;
dummy_var:
      sprintf(id,"#DO%%%u",mInstance->curline);
      v=findVariable(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
      if(!v)
        v = addVar(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
      v->sval=mystrdupint(mInstance,initval);
      goto set_var;
      break;
    case WHILE:
      match(mInstance,WHILE);
      exprPos=(const char *)mInstance->string;
      if(boolExpr(mInstance)) {   // qua, PRIMA!
        toval = 0;
        stepval = 0;    // marker di WHILE
        *mInstance->doStack[mInstance->ndos].id=0;
        goto dummy_var;
        }
      else {
        gotoBlockEnd(mInstance,mInstance->inBlock);
        }
      break;
    case UNTIL:
      match(mInstance,UNTIL);
      exprPos=(const char *)mInstance->string;
      toval = 0;
      stepval = 1;    // marker di UNTIL
      *mInstance->doStack[mInstance->ndos].id=0;
      goto dummy_var;
      break;
    case FOREVER:
      match(mInstance,FOREVER);
      toval = 1;
      stepval = 0;
      goto dummy_var;
  		break;
    default:
/*  		stepval = 1;
    	toval = 1;
      goto dummy_var;*/
//      toval = initval = stepval = 0;
//      *mInstance->doStack[mInstance->ndos].id=0;
  		setError(mInstance,ERR_SYNTAX);
      return;
      break;
		}

  if(mInstance->ndos > MAXFORS - 1) {
		setError(mInstance,ERR_TOOMANYFORS);
	  }
  else {
  	mInstance->doStack[mInstance->ndos].step = stepval;
    mInstance->doStack[mInstance->ndos].toval = toval;
		mInstance->doStack[mInstance->ndos].nextline = getNextStatement(mInstance,mInstance->string);
    mInstance->doStack[mInstance->ndos].expr=exprPos;
		mInstance->doStack[mInstance->ndos].parentBlock = mInstance->ndos;
    mInstance->ndos++;
    mInstance->inBlock++;
	  }
      
	}


LINE_NUMBER_TYPE doEnd(KOMMISSARREXX *mInstance) {
  int t;
  VARIABLESTRING *var;


  match(mInstance,END);

	if(mInstance->ndos>0) {
    mInstance->ndos--;
    if(mInstance->doStack[mInstance->ndos].expr) {    // while, until
      const char *oldPos=mInstance->string;
      t=boolExpr(mInstance);
      mInstance->string=oldPos;
      mInstance->token = getToken(mInstance->string);
      if(mInstance->doStack[mInstance->ndos].step)   // marker
        t=!t;
      if(!t) {
        mInstance->inBlock--;
        return 0;
        }
      else {
        return mInstance->doStack[mInstance->ndos++].nextline;
        }
      }
    else if(*mInstance->doStack[mInstance->ndos].id) { // for..to, forever
      if(!strnicmp(mInstance->doStack[mInstance->ndos].id,"#SEL%",5)) {
        if(!mInstance->doStack[mInstance->ndos].toval /*marker per OTHERWISE*/)   // così dice
          setError(mInstance,ERR_SYNTAX);
//          mInstance->ndos--;
        else {
          if(mInstance->inBlock>0) {
            mInstance->inBlock--;
            }
          }
        return 0;
        }
      else {
        var = findVariable(mInstance,mInstance->doStack[mInstance->ndos].id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
                mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
    /*			if(!var) {		// IMPOSSIBILE!
        setError(mInstance,ERR_OUTOFMEMORY);
        return 0;
        }*/
        t = to_int(var->sval);
        t += mInstance->doStack[mInstance->ndos].step;
        free(var->sval);
        var->sval=mystrdupint(mInstance,t);
        if( (mInstance->doStack[mInstance->ndos].step < 0 && t < mInstance->doStack[mInstance->ndos].toval) ||
          (mInstance->doStack[mInstance->ndos].step > 0 && t > mInstance->doStack[mInstance->ndos].toval)) {

          // CANCELLARE la variabile se era una dummy? v. sopra

          mInstance->inBlock--;
          return 0;
          }
        else {
          return mInstance->doStack[mInstance->ndos++].nextline;
          }
        }
      }
    else {    // do..end generico, oppure SELECT
      if(mInstance->inBlock>0) {
        mInstance->inBlock--;
    
        mInstance->doStack[mInstance->ndos].parentBlock ;
        
//  match(mInstance,mInstance->token);    // intanto mangio EOL o EOS...
//  mInstance->curline++;
        
        return 0;
    
    		}
      else    // NON deve succedere
        ;
  		}
    
    }
  else {
    
    //ANCHE per CALL/RETURN :)  eh??
    
    setError(mInstance,ERR_NOFOR);
		return -1;
  	}
	}


void doPull(KOMMISSARREXX *mInstance) {
  LLVALUE lv;
  char buff[256];
  LVARIABLE v;
  STACK_QUEUE *q,*q1;

  match(mInstance,PULL);
  lvalue(mInstance,&lv);
  
  v.type=STRID;
  if(mInstance->stack) {
    v.d.sval=mystrdup(mInstance,mInstance->stack->string);
    free((void*)mInstance->stack->string);
    q1=mInstance->stack->next;
    free(mInstance->stack);
    mInstance->stack=q1;
    }
  else {
    *buff=0;
    if(inputString(buff,255,"?",0) > 0)
      ;
    putchar('\n'); putchar('\n');
    v.d.sval=mystrdup(mInstance,buff);
    }
  subAssignVar(mInstance,lv.d.sval,&v);
  
	}

void doPush(KOMMISSARREXX *mInstance) {
  STACK_QUEUE *q,*q1;
  const char *str;
  
  match(mInstance,PUSH);
  str=expr(mInstance);
  if(str) {
    q=(STACK_QUEUE *)malloc(sizeof(STACK_QUEUE));
    q->string=mystrdup(mInstance,str); q->next=mInstance->stack;
    mInstance->stack=q;
    free((void*)str);
    }
  else
    setError(mInstance,ERR_BADVALUE);
  }

void doQueue(KOMMISSARREXX *mInstance) {
  STACK_QUEUE *q,*q1;
  const char *str;
  
  match(mInstance,QUEUE);
  str=expr(mInstance);
  if(str) {
    if(mInstance->stack) {
      q=mInstance->stack;
      while(q) {
        q1=q;
        q=q->next;
        }
      q=(STACK_QUEUE *)malloc(sizeof(STACK_QUEUE));
      q1->next=q;
      }
    else {
      q=(STACK_QUEUE *)malloc(sizeof(STACK_QUEUE));
      mInstance->stack=q;
      }
    q->string=mystrdup(mInstance,str); q->next=NULL;
    free((void*)str);
    }
  // se non c'è?? non è chiaro
  }


void doExit(KOMMISSARREXX *mInstance) {
  const char *str;
  
  errorLevel=0;
  match(mInstance,EXIT);
  if(mInstance->token != EOL && mInstance->token != EOS) {
    str=expr(mInstance);
    if(str) {
      errorLevel=to_int(str);
      free((void*)str);
      }
    }
  }

void doNop(KOMMISSARREXX *mInstance) {

  match(mInstance,NOP);
  }
  
void doNumeric(KOMMISSARREXX *mInstance) {
  TOKEN_NUM t;
  const char *str=NULL;
  
  match(mInstance,NUMERIC);
  
  switch(mInstance->token) {
    case DIGITS:
      match(mInstance,DIGITS);
      str=expr(mInstance);
      if(str) {
        digitSizeDecimal=to_int(str);
        }
      break;
    case FORM:
      match(mInstance,FORM);
      str=expr(mInstance);
      if(!stricmp(str,"SCIENTIFIC")) {
        digitForm=0;
        }
      else if(!stricmp(str,"ENGINEERING")) {
        digitForm=1;
        }
      else
        setError(mInstance,ERR_BADVALUE);
      break;
    case FUZZ:
      str=expr(mInstance);
      match(mInstance,FUZZ);
      digitFuzz=to_int(str);
      break;
    }
  free((void*)str);
  }
  
void doOptions(KOMMISSARREXX *mInstance) {

  match(mInstance,OPTIONS);
  }
  
void doRem(KOMMISSARREXX *mInstance) {
  TOKEN_NUM t;

  match(mInstance,REM);
  
  do {
    t=getToken(mInstance->string);
    match(mInstance,t);   // ha senso fare così??
    } while(t != REM2 && t != EOS && t != EOL);
    
	}

#ifdef REGINA_REXX
void doBeep(KOMMISSARREXX *mInstance) {
  const char *str=NULL;
  
  match(mInstance,BEEP);
  str=expr(mInstance);
  if(str) {
    StdBeep(to_int(str));
    free((void*)str);
    }
  }
  
void doChdir(KOMMISSARREXX *mInstance) {
  const char *str=NULL;
  
  match(mInstance,CHDIR);
  str=expr(mInstance);
  if(str) {
    free((void*)str);
    }
  }

void doDirectory(KOMMISSARREXX *mInstance) {
  const char *str=NULL;
  
  match(mInstance,DIRECTORY_TOK);
  str=expr(mInstance);
  if(str) {
    free((void*)str);
    }
  }

uint16_t doBitchg(KOMMISSARREXX *mInstance) {
  const char *str=NULL,*str2=NULL;
  const char *answer;
  uint16_t t,t2;
  
  match(mInstance,BITCHG);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    t=to_int(str);
    free((void*)str);
    }
  match(mInstance,COMMA);
  str2=expr(mInstance);
  if(str2) {
    t2=to_int(str2);
    free((void*)str2);
    }
  match(mInstance,CPAREN);
  return t ^ (1 << t2);
  }

uint16_t doBitset(KOMMISSARREXX *mInstance) {
  const char *str=NULL,*str2=NULL;
  const char *answer;
  uint16_t t,t2;
  
  match(mInstance,BITCHG);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    t=to_int(str);
    free((void*)str);
    }
  match(mInstance,COMMA);
  str2=expr(mInstance);
  if(str2) {
    t2=to_int(str2);
    free((void*)str2);
    }
  match(mInstance,CPAREN);
  return t | (1 << t2);
  }

uint16_t doBitclr(KOMMISSARREXX *mInstance) {
  const char *str=NULL,*str2=NULL;
  const char *answer;
  uint16_t t,t2;
  
  match(mInstance,BITCLR);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    t=to_int(str);
    free((void*)str);
    }
  match(mInstance,COMMA);
  str2=expr(mInstance);
  if(str2) {
    t2=to_int(str2);
    free((void*)str2);
    }
  match(mInstance,CPAREN);
  return t & ~(1 << t2);
  }

uint16_t doBittst(KOMMISSARREXX *mInstance) {
  const char *str=NULL,*str2=NULL;
  const char *answer;
  uint16_t t,t2;
  
  match(mInstance,BITCHG);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    t=to_int(str);
    free((void*)str);
    }
  match(mInstance,COMMA);
  str2=expr(mInstance);
  if(str2) {
    t2=to_int(str2);
    free((void*)str2);
    }
  match(mInstance,CPAREN);
  return t & (1 << t2) ? 1 : 0;
  }

int16_t doBitcomp(KOMMISSARREXX *mInstance) {
  const char *str=NULL,*str2=NULL;
  const char *answer;
  uint16_t t,t2;
  BYTE i,j;
  
  match(mInstance,BITCHG);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    t=to_int(str);
    free((void*)str);
    }
  match(mInstance,COMMA);
  str2=expr(mInstance);
  if(str2) {
    t2=to_int(str2);
    free((void*)str2);
    }
  match(mInstance,CPAREN);
  for(i=1,j=0; i; i<<=1, j++) {
    if((t ^ t2) & i)
      return j;
    }
  return -1;
  }


void doSleep(KOMMISSARREXX *mInstance) {
  const char *str=NULL;
  
  match(mInstance,SLEEP);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  if(str) {
    __delay_ms(to_int(str));
    free((void*)str);
    }
  match(mInstance,CPAREN);
  }

void doSeek(KOMMISSARREXX *mInstance) {
  const char *str=NULL;
  
  match(mInstance,SEEK);
  str=expr(mInstance);
  if(str) {
    free((void*)str);
    }

  }
#endif



/*
  Get an lvalue from the environment
  Params: lv - structure to fill.
  Notes: missing variables (but not out of range subscripts)
         are added to the variable list.
*/
void lvalue(KOMMISSARREXX *mInstance,LLVALUE *lv) {
  char name[IDLENGTH];
  IDENT_LEN len;
  VARIABLESTRING *var;
  LDIMVARSTRING *dimvar;
  DIM_SIZE index[MAXDIMS];
  void *valptr = NULL;
  char type;
  
  lv->type = B_ERROR;
  lv->d.dval = 0;		// clears them all

  switch(mInstance->token) {
    case STRID:
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,STRID);
			var = findVariable(mInstance,name,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
			if(!var)
				var = addVar(mInstance,name,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
  		if(!var) {
				setError(mInstance,ERR_OUTOFMEMORY);
				return;
				}
			lv->type = STRID;
			lv->d.sval = &var->sval;
			break;
		case DIMSTRID:
			type = STRID;
			getId(mInstance,mInstance->string, name, &len);
			match(mInstance,mInstance->token);
			dimvar = findDimVar(mInstance,name,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
			if(dimvar) {
				switch(dimvar->ndims)	{
					case 1:
						index[0] = to_int(expr(mInstance));
						if(!mInstance->errorFlag)
              valptr = getDimVar(mInstance,dimvar, index[0]);
						break;
					case 2:
						index[0] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[1] = to_int(expr(mInstance));
						if(!mInstance->errorFlag)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1]);
						break;
					case 3:
						index[0] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[1] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[2] = to_int(expr(mInstance));
						if(!mInstance->errorFlag)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
						break;
				  case 4:
						index[0] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[1] = to_int(expr(mInstance));
						match(mInstance,COMMA);
            index[2] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[3] = to_int(expr(mInstance));
						if(!mInstance->errorFlag)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
						break;
					case 5:
						index[0] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[1] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[2] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[3] = to_int(expr(mInstance));
						match(mInstance,COMMA);
						index[4] = to_int(expr(mInstance));
						if(!mInstance->errorFlag)
							valptr = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
						break;
					}
				match(mInstance,CPAREN);
				}
			else {
				setError(mInstance,ERR_NOSUCHVARIABLE);
				return;
				}
      if(valptr) {
        lv->type = type;
        lv->d.sval = valptr;
        }
		  break;
		default:
			setError(mInstance,ERR_SYNTAX);
		  break;
		}
	}


signed char boolExpr(KOMMISSARREXX *mInstance) {
  signed char left,right;
  
  left = boolFactor(mInstance);

  while(1) {
    switch(mInstance->token) {
			case AND:
				match(mInstance,AND);
				right = boolExpr(mInstance);
				return (left && right) ? -1 : 0;
			case OR:
				match(mInstance,OR);
				right = boolExpr(mInstance);
				return (left || right) ? -1 : 0;
			default:
				return left;
			}
		}
	}


/*
  boolean factor, consists of expression relOp expression
    or string relOp string, or ( boolExpr())
*/
signed char boolFactor(KOMMISSARREXX *mInstance) {
  signed char answer;
  char *left,*right;
  enum REL_OPS op;

  switch(mInstance->token) {
    case OPAREN:
			match(mInstance,OPAREN);
			answer = boolExpr(mInstance);
			match(mInstance,CPAREN);
			break;
		default:
  		left = expr(mInstance);
			op = relOp(mInstance);
			right = expr(mInstance);
			if(!left || !right)	{
				if(left)
					free(left);
				if(right)
					free(right);
				return 0;
				}
      if(datatype(left) == STRID || datatype(right) == STRID) {
				answer= strRop(op,left,right);
				free(left);
				free(right);
			  }
		  else {
        if(datatype(left) == INTID && datatype(right) == INTID)
    			answer= rop(op,to_int(left),to_int(right));
        else
    			answer= rop(op,to_num(left),to_num(right));
//		    left = expr(mInstance);
//v. minibasic...				answer=left;
				}
			break;
	  }

  return answer;
	}


signed char rop(signed char op, LNUM_TYPE left, LNUM_TYPE right) {
	signed char answer;

	switch(op) {
		case ROP_EQ:
		case ROP_EQ_S:
			answer = (left == right) ? -1 : 0;
			break;
		case ROP_NEQ:
		case ROP_NEQ_S:
			answer = (left != right) ? -1 : 0;
			break;
		case ROP_LT:
		case ROP_LT_S:
			answer = (left < right) ? -1 : 0;
			break;
		case ROP_LTE:
		case ROP_LTE_S:
			answer = (left <= right) ? -1 : 0;
			break;
		case ROP_GT:
		case ROP_GT_S:
			answer = (left > right) ? -1 : 0;
			break;
		case ROP_GTE:
		case ROP_GTE_S:
			answer = (left >= right) ? -1 : 0;
			break;
		default:    // NON deve succedere!
		  return 0;
			break;
		}

	return answer;
	}


signed char strRop(signed char op, const char *left, const char *right) {
	signed char answer;

	if(op >= ROP_EQ_S) {
		answer = strcmp(left, right);
		}
	else {
		answer = stricmp(left, right);
	//ecc...
		}
	switch(op) {
		case ROP_EQ:
			answer = answer == 0 ? -1 : 0;
			break;
		case ROP_NEQ:
			answer = answer == 0 ? 0 : -1;
			break;
		case ROP_LT:
			answer = answer < 0 ? -1 : 0;
			break;
		case ROP_LTE:
			answer = answer <= 0 ? -1 : 0;
			break;
		case ROP_GT:
			answer = answer > 0 ? -1 : 0;
			break;
		case ROP_GTE:
			answer = answer >= 0 ? -1 : 0;
			break;
		case ROP_EQ_S:
			answer = answer == 0 ? -1 : 0;
			break;
		case ROP_NEQ_S:
			answer = answer == 0 ? 0 : -1;
			break;
		case ROP_LT_S:
			answer = answer < 0 ? -1 : 0;
			break;
		case ROP_LTE_S:
			answer = answer <= 0 ? -1 : 0;
			break;
		case ROP_GT_S:
			answer = answer > 0 ? -1 : 0;
			break;
		case ROP_GTE_S:
			answer = answer >= 0 ? -1 : 0;
			break;
		default:    // NON deve succedere!
			answer = 0;
			break;
		}

	return answer;
	}


enum REL_OPS relOp(KOMMISSARREXX *mInstance) {

  switch(mInstance->token) {
    case EQUALS:
		  match(mInstance,EQUALS);
			return ROP_EQ;
			break;
    case GREATER:
			match(mInstance,GREATER);
			if(mInstance->token == EQUALS) {
        match(mInstance,EQUALS);
				return ROP_GTE;
				}
			return ROP_GT; 
			break;
		case LESS:
      match(mInstance,LESS);
			if(mInstance->token == EQUALS) {
				match(mInstance,EQUALS);
				return ROP_LTE;
				}
			else if(mInstance->token == GREATER) {
				match(mInstance,GREATER);
				return ROP_NEQ;
				}
			return ROP_LT;
			break;
    case EQUALS_STRICT:
		  match(mInstance,EQUALS_STRICT);
			return ROP_EQ_S;
			break;
    case GREATER_STRICT:
			match(mInstance,GREATER_STRICT);
			if(mInstance->token == EQUALS) {
        match(mInstance,EQUALS);
				return ROP_GTE_S;
				}
			return ROP_GT_S;
			break;
		case LESS_STRICT:
      match(mInstance,LESS_STRICT);
			if(mInstance->token == EQUALS) {
				match(mInstance,EQUALS);
				return ROP_LTE_S;
				}
			else if(mInstance->token == GREATER_STRICT) {
				match(mInstance,GREATER_STRICT);
				return ROP_NEQ_S;
				}
			return ROP_LT_S;
			break;
		default:
			setError(mInstance,ERR_SYNTAX);// TOLTO per la storia dello statement da solo... v. b_error

/* non c'è speranza...
  
     {  int i;
    	match(mInstance,mInstance->token);

    i= getToken(mInstance->string);
      
			if(mInstance->token == EOS)
        return EOS;
			else if(mInstance->token == EOL)
        return EOL;
			else if(mInstance->token == COLON)
        return COLON;
      else*/
        return B_ERROR;
//    }
			break;
		}
	}


char *expr(KOMMISSARREXX *mInstance) {
  char *left,*right;
  uint8_t op;


  left = term(mInstance);
  if(!left)
    return NULL;

  while(1) {
    switch(mInstance->token)	{
			case STRID:   // le stringhe si autoconcatenano qua...
   			right = term(mInstance);
        goto concat_string;
			case PLUS:
				match(mInstance,PLUS);
   			right = term(mInstance);
concat_string:
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            left=realloc(left,strlen(left)+strlen(right)+1);
  //myStrConcat()
            strcat(left,right);
            }
          else {
            LNUM_TYPE n;
            int16_t n1;
            char buf[digitSize+1];
            if(datatype(left) == INTID || datatype(right) == INTID) {
              n1=to_int(left)+to_int(right);
              to_string_i(n1,buf);
              }
            else {
              n=to_num(left)+to_num(right);
              to_string(n,buf);
              }
            free(left);
            left=mystrdup(mInstance,buf);   // lascio... per conversione giusta in to_string
            }
          free(right);
          }
        else    // non dovrebbe succedere cmq..
          return left;
				break;
			case MINUS:
				match(mInstance,MINUS);
				right = term(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            // che fare??
            }
          else {
            LNUM_TYPE n;
            char buf[digitSize+1];
            n=to_num(left)+to_num(right);
            free(left);
            to_string(n,buf);
            left=mystrdup(mInstance,buf);   // lascio, v.sopra
            }
          free(right);
          }
        else    // non dovrebbe succedere cmq..
          return left;
				break;
			default:
				op=relOp(mInstance);			//Find relationship operator  eg = > < >= <=
				if(op != B_ERROR) {		//If operator found OK
   				mInstance->errorFlag=0;
					match(mInstance,op);			//Check Operator
					right = term(mInstance);		//Get Value Data
          if(right) {
            signed char n;
            char buf[digitSize+1];
            if(datatype(left) == STRID || datatype(right) == STRID) {
    					n=strRop(op,left,right);
              }
            else {
              n=rop(op,to_num(left),to_num(right));
              }
            free(left);
            to_string(n,buf);
            left=mystrdup(mInstance,buf);   // idem v.sopra
            free(right);
            }
          else    // non dovrebbe succedere cmq..
            return left;
					}
				else {
   				mInstance->errorFlag=0;
					return left;
					}
				break;
			}
		}
	}


char *term(KOMMISSARREXX *mInstance) {
  char *left,*right;
  LNUM_TYPE n;
  int16_t n1;
  char buf[digitSize+1];

  left = factor(mInstance);
  
  while(1) {
    switch(mInstance->token) {
			case MULT:
				match(mInstance,MULT);
				right = factor(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            }
          else {
            n=to_num(left)*to_num(right);
fine_num:          
            free(left);
            to_string(n,buf);
            left=mystrdup(mInstance,buf);
            }
          free(right);
          }
				break;
			case DIV:
				match(mInstance,DIV);
				right = factor(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            }
          else {
            if(to_num(right) != 0.0) {
              n=to_num(left) / to_num(right);
              goto fine_num;
              }
            else
              setError(mInstance,ERR_DIVIDEBYZERO);
            }
          free(right);
          }
				break;
			case DIVINT:
				match(mInstance,DIVINT);
				right = factor(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            }
          else {
            if(to_num(right) != 0) {
              n1=to_int(left) / to_int(right);
fine_int:              
              free(left);
              left=mystrdupint(mInstance,n1);
              }
            else
              setError(mInstance,ERR_DIVIDEBYZERO);
            }
          free(right);
          }
				break;
      case POW:
        match(mInstance,POW);
				right = factor(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            }
          else {
            if(to_num(right) != 0.0) {
              n=pow(to_num(left),to_num(right));
              goto fine_num;
              }
            else
              setError(mInstance,ERR_DIVIDEBYZERO);
            }
          free(right);
          }
        break;
			case MOD:
				match(mInstance,MOD);
				right = factor(mInstance);
        if(right) {
          if(datatype(left) == STRID || datatype(right) == STRID) {
            }
          else {
            if(to_num(right) != 0.0) {
              n=fmod(to_num(left),to_num(right));
              goto fine_num;
              }
            else
              setError(mInstance,ERR_DIVIDEBYZERO);
            }
          free(right);
          }
				break;
			default:
				return left;
			}
	  }

	}


char *factor(KOMMISSARREXX *mInstance) {
	char *answer=NULL;
  char *temp,*temp2,*temp3;
  LNUM_TYPE n;
  int16_t n1;
  char buf[digitSize+1];
  char *end;		// NO const
	int t,t1;
  IDENT_LEN len;

  switch(mInstance->token) {
    case OPAREN:
			match(mInstance,OPAREN);
			answer = expr(mInstance);
			match(mInstance,CPAREN);
			break;
		case VALUE:
			n = getValue(mInstance->string, &len);
			match(mInstance,VALUE);
      
//            free(left);
fine_num:
      if(n < SHRT_MAX && n > SHRT_MIN && ((int16_t)n) == n)   // v. anche datatype(
        to_string_i(n,buf);
      else
        to_string(n,buf);
      answer=mystrdup(mInstance,buf);
			break;
//		case NOT:
//			match(mInstance,NOT);
//			answer = (LNUM_TYPE) (~( integer(mInstance, factor(mInstance))));
//			break;
		case MINUS:
			match(mInstance,MINUS);
			temp = factor(mInstance);
      if(temp) {
        if(datatype(temp) != STRID) {
          n=-to_num(temp);
          goto fine_num;
          }
        free(temp);
        }
			break;
		case FLTID:
			n = variable(mInstance);
      goto fine_num;
			break;
		case ABS:
			temp = expr(mInstance);
    	match(mInstance,CPAREN);
      if(temp) {
        if(datatype(temp) != STRID) {
          n = fabs(to_num(temp));
          goto fine_num;
          }
        free(temp);
        }
			break;
    case BITOR:
      match(mInstance,BITOR);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
  			match(mInstance,COMMA);
    		temp2 = expr(mInstance);
      	match(mInstance,CPAREN);
        if(temp2) {
          if(datatype(temp) != STRID && datatype(temp2) != STRID) {
            n = to_int(temp) | to_int(temp2);
            free(temp2);
            goto fine_num;
            }
          free(temp2);
          }
        free(temp);
        }
      break;
    case BITAND:
      match(mInstance,BITAND);
			temp = expr(mInstance);
      if(temp) {
  			match(mInstance,COMMA);
    		temp2 = expr(mInstance);
      	match(mInstance,CPAREN);
        if(temp2) {
          if(datatype(temp) != STRID && datatype(temp2) != STRID) {
            n = to_int(temp) & to_int(temp2);
            free(temp2);
            goto fine_num;
            }
          free(temp2);
          }
        free(temp);
        }
      break;
    case BITXOR:
      match(mInstance,BITXOR);
			temp = expr(mInstance);
      if(temp) {
  			match(mInstance,COMMA);
    		temp2 = expr(mInstance);
      	match(mInstance,CPAREN);
        if(temp2) {
          if(datatype(temp) != STRID && datatype(temp2) != STRID) {
            n = to_int(temp) ^ to_int(temp2);
            free(temp2);
            goto fine_num;
            }
          free(temp2);
          }
        free(temp);
        }
      break;
		case MIN:
			match(mInstance,MIN);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
  			match(mInstance,COMMA);
    		temp2 = expr(mInstance);
      	match(mInstance,CPAREN);
        if(temp2) {
          if(datatype(temp) != STRID && datatype(temp2) != STRID) {
            n = to_num(temp) < to_num(temp2) ? to_num(temp) : to_num(temp2);
            free(temp2);
            goto fine_num;
            }
          free(temp2);
          }
        free(temp);
        }
			break;
		case MAX:
			match(mInstance,MAX);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
  			match(mInstance,COMMA);
    		temp2 = expr(mInstance);
      	match(mInstance,CPAREN);
        if(temp2) {
          if(datatype(temp) != STRID && datatype(temp2) != STRID) {
            n = to_num(temp) > to_num(temp2) ? to_num(temp) : to_num(temp2);
            free(temp2);
            goto fine_num;
            }
          free(temp2);
          }
        free(temp);
        }
			break;
		case SIGN:
			match(mInstance,SIGN);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
      	match(mInstance,CPAREN);
        n = to_num(temp) >= 0 ? 1 : 0;
        goto fine_num;
        }
			break;
		case TRUNC:
			match(mInstance,TRUNC);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
      	long long d;
        t=0;
        if(mInstance->token==COMMA) {
          match(mInstance,COMMA);
          temp2 = expr(mInstance);
          if(temp2) {
            t=to_int(temp2);
            free(temp2);
            }
          }
      	match(mInstance,CPAREN);
        t1=1;
        while(t--)
          t1 *= 10;
        d=to_num(temp)*t1;
        n=((LNUM_TYPE)d)/t1;
        goto fine_num;
        }
			break;

#if REGINA_REXX
		case BITCHG:
		  n = doBitchg(mInstance);
      goto fine_num;
		  break;
		case BITCLR:
		  n = doBitclr(mInstance);
      goto fine_num;
		  break;
		case BITSET:
		  n = doBitset(mInstance);
      goto fine_num;
		  break;
		case BITTST:
		  n = doBittst(mInstance);
      goto fine_num;
		  break;
		case BITCOMP:
		  n = doBitcomp(mInstance);
      goto fine_num;
		  break;
#endif
#if 0
		case STATUSTOK:
			{
			signed char c;
			match(mInstance,STATUSTOK);
			match(mInstance,OPAREN);
		  t = integer(mInstance, expr(mInstance));
			match(mInstance,CPAREN);
			switch(t) {
				case 0:		// Reset
//					answer = RCON;
					answer = RCON;    // bah :)
          RCON=0;
					break;
				case 1:		// Wakeup
					answer = (RCONbits.SLEEP ? 1 : 0) | (RCONbits.WDTO ? 2 : 0) | ((powerState & 1) ? 4 : 0);
					break;
				case 2:
					answer = t;   //:)
					break;
				case 3:
//					c=U1RXREG;
					c=HIBYTE(LOWORD(GetStatus(SOUTH_BRIDGE,NULL))); // meglio, qua :)
					answer = c;
					break;
				case 4:
					c=U1RXREG;
					answer = c;
					break;
				case 5:
					c=LOBYTE(HIWORD(GetStatus(SOUTH_BRIDGE,NULL))); // parallela
					answer = c;
					break;
				case 6:
#ifdef USA_WIFI
          m2m_periph_gpio_get_val(M2M_PERIPH_GPIO18,&c);    // leggo il led!
          c=!c;   // [beh...] v. merda non va
#else
          c=-1;
#endif
					answer = c;
					break;
				case 7:
#ifdef USA_ETHERNET
					c=MACIsLinked();
#else
          c=-1;
#endif
					answer = c;
					break;
				case 8:
					c=GetStatus(SOUTH_BRIDGE,NULL);
					answer = c;
					break;
          
				case 15:
					c=BiosArea.bootErrors;    // farne anche una per tutti i biosarea?
					answer = c;
					break;
          
        case 16:
          {
          SUPERFILE f;
          f.drive=currDrive;    // per ora così...
          answer=SuperFileError(&f);
          }
					break;
          
        default:
          answer=-1;
					break;
				}
			}
			break;
#endif
      
		case LENGTH:
			match(mInstance,LENGTH);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
			match(mInstance,CPAREN);
      if(temp) {
        if(datatype(temp) == STRID)
  				n = strlen(temp);
        else
  				n = strlen(to_string(to_num(temp),buf));   // beh :)
				free(temp);
	  		}
			else
				n = 0;
      goto fine_num;
			break;
    case RANDOM:
			match(mInstance,RANDOM);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
        if(mInstance->token==COMMA) {
          match(mInstance,COMMA);
    			temp2 = expr(mInstance);
          if(temp2) {
            if(mInstance->token==COMMA) {
              match(mInstance,COMMA);
        			temp3 = expr(mInstance);
              if(temp3) {
              
                srand( (unsigned) to_int(temp3));
                free(temp3);
                }
              }
            unsigned long long l=to_int(temp2)-to_int(temp);
            l *= rand();
            n = t + (l/RAND_MAX);
            free(temp2);
            }
          else {
            unsigned long long l;
#warning			o usare crypto , GenerateRandomDWORD() da helpers.c
rand_1:
            l=to_int(temp);
            l *= rand();
            n = l/RAND_MAX;
            free(temp);
            }
    			match(mInstance,CPAREN);
          goto fine_num;
          }
        else
          goto rand_1;
        }
			break;
		case POS:
	  	answer = instr(mInstance);
	  	break;
		case LASTPOS:
	  	answer = instr2(mInstance);
	  	break;
		case WORDTOK:
	  	answer = word(mInstance);
	  	break;
		case WORDPOS:
	  	answer = wordpos(mInstance);
	  	break;
		case DELWORD:
	  	answer = delword(mInstance);
	  	break;
    case ARG:
	  	answer = argString(mInstance);
	  	break;
      
    case DIMSTRID:
//			match(mInstance,DIMSTRID);
			answer = mystrdup(mInstance,stringDimVar(mInstance));
			break;
    case STRID:   // 
    {
      const char *oldpos=mInstance->string;   // devo andare a vedere cosa c'è dopo la stringa/var/label...
      char id[IDLENGTH];
      LINE_NUMBER_TYPE toline;
      
      getId(mInstance, mInstance->string, id, &len);
      match(mInstance,STRID);
      if(mInstance->token == OPAREN) {
        
        match(mInstance,OPAREN);
                
                printf("function STRID: %s()\r\n",id);
        
        toline=findProcedure(mInstance,id);
        
        if((int16_t)toline < 0) {
          setError(mInstance,ERR_NOSUCHVARIABLE);   // migliorare...
          answer=NULL;
          }
        else {
          char buf2[128],*p;
          
          printf("found function STRID: %s\r\n",id);
      // secondo il doc, se c'è una funzione chiamata "tra apici" allora va invocata SEMPRE la funzione interna...
      // v. function 
          
          strncpy(buf2, skipSpaces(mInstance->string),sizeof(buf2)-1);
          buf2[sizeof(buf2)-1]=0;
          if(p=strchr(buf2,'\n'))
            *p=0;
          mInstance->gosubStack[mInstance->ngosubs].args = mystrdup(mInstance,buf2);
          mInstance->gosubStack[mInstance->ngosubs].returnline = getNextStatement(mInstance,mInstance->string);
          mInstance->ngosubs++;
          answer=mystrdupint(mInstance,toline);

          //finire ANSWER!
          
          }
                
        match(mInstance,CPAREN);
        }

      else {      
        mInstance->string=oldpos;
        mInstance->token = getToken(mInstance->string);
        answer = mystrdup(mInstance,stringVar(mInstance));
//        match(mInstance,STRID);
        }
      }

		  break;
		case QUOTE:
			answer = stringLiteral(mInstance);
      
      // secondo il doc, se c'è una funzione chiamata "tra apici" allora va invocata SEMPRE la funzione interna...
      // v. function 
			break;
    case COMPARE:
			answer = compareStrings(mInstance);
			break;
		case LEFT:
   		answer = leftString(mInstance);
     	break;
		case RIGHT:
			answer = rightString(mInstance);
			break;
		case SUBSTR:
			answer = subString(mInstance);
			break;
		case STRIP:
			answer = stripString(mInstance);
			break;
		case TRANSLATE:
			answer = translateString(mInstance);
			break;
		case REVERSE:
			answer = reverseString(mInstance);
			break;
		case DELSTR:
			answer = delString(mInstance);
			break;
		case WORDS:
			answer = words(mInstance);
			break;
		case XRANGE:
			answer = xrange(mInstance);
			break;
//		case INKEYSTRING:
//			answer = inkeyString(mInstance);
//			break;
#ifdef USA_BREAKTHROUGH
		case MENUKEYSTRING:
			answer = menukeyString(mInstance);
			break;
#endif
		case CHARIN:
			answer = charin(mInstance);
			break;
		case CHAROUT:
			answer = charout(mInstance);
			break;
		case DIGITS:
			answer = digits(mInstance);
			break;
		case FORM:
			answer = form(mInstance);
			break;
		case FUZZ:
			answer = fuzz(mInstance);
			break;
		case B2X:
			answer = b2x(mInstance);
			break;
		case C2D:
			answer = c2d(mInstance);
			break;
		case C2X:
			answer = c2x(mInstance);
			break;
		case D2C:
			answer = d2c(mInstance);
			break;
		case D2X:
			answer = d2x(mInstance);
			break;
		case X2B:
			answer = x2b(mInstance);
			break;
		case X2C:
			answer = x2c(mInstance);
			break;
		case X2D:
			answer = x2d(mInstance);
			break;
      
#ifdef REGINA_REXX
		case C2B:
			answer = c2b(mInstance);
			break;
		case EXISTS:
			answer = exists(mInstance);
			break;
		case FREESPACE:
			answer = freespace(mInstance);
			break;
		case GETENV:
			answer = mygetenv(mInstance);
			break;
		case GETPID:
			answer = getpid(mInstance);
			break;
		case GETSPACE:
			answer = getspace(mInstance);
			break;
		case IMPORT:
			answer = import(mInstance);
			break;
		case JUSTIFY:
			answer = justify(mInstance);
			break;
		case UNAME:
			answer = uname(mInstance);
			break;
		case WRITECH:
			answer = writech(mInstance);
			break;
		case WRITELN:
			answer = writeln(mInstance);
			break;
#endif
      
		case ERRORTEXT:
			answer = errorString(mInstance);
			break;
    case ADDRESS:
			answer = addressString(mInstance);
	  	break;
    case VERSION:
			answer = verString(mInstance);
	  	break;
    case LINEIN:
			answer = lineinString(mInstance);
	  	break;
    case LINEOUT:
			answer = lineoutString(mInstance);
	  	break;
    case ABBREV:
			answer = abbrevString(mInstance);
	  	break;
		case COPIES:
			answer = stringString(mInstance);
			break;
		case CHANGESTR:
			answer = changeString(mInstance);
			break;
		case DATE:
			answer = dateString(mInstance);
			break;
		case TIME:
			answer = timeString(mInstance);
			break;
    case QUEUED:
      {
      STACK_QUEUE *q=mInstance->stack;
			match(mInstance,QUEUED);
			match(mInstance,OPAREN);
			match(mInstance,CPAREN);
      t=0;
      while(q) {
        t++;
        q=q->next;
        }
      answer=mystrdupint(mInstance,t);
      }
      break;
    case SYMBOL:
			match(mInstance,SYMBOL);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
			match(mInstance,CPAREN);
			if(temp) {
        t=1;
        temp2=temp;
        if(!isvalidrexx(*temp2))
          t=0;
        else {
          while(*temp2) {
            if(!isvalidrexx2(*temp2)) {
              t=0;
              break;
              }
            temp2++;
            }
          }
        if(!t)    //  @, _, !, ., ?, and $
          answer=mystrdup(mInstance,"BAD");
        else
          answer=findVariable(mInstance,temp,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
                  mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0)
                  ? mystrdup(mInstance,"VAR") : mystrdup(mInstance,"LIT");
				free(temp);
	  		}
			else
				setError(mInstance,ERR_SYNTAX);
			break;
    case DATATYPE:
      answer=datatypeString(mInstance);
			break;
//    case DIRSTRING:   // v. anche doDir
//      answer = dirString(mInstance);
//      break;
      
#if 0
    switch(mInstance->token) {
      case PLUS:
        match(mInstance,PLUS);
        right = stringExpr(mInstance);
        if(right) {
          temp = myStrConcat(answer, right);
          free(right);
          if(temp) {
            free(answer);
            answer = temp;
            }
          else
            setError(mInstance,ERR_OUTOFMEMORY);
          }
        else
          setError(mInstance,ERR_OUTOFMEMORY);
        break;
      default:
        return answer;
        break;
      }
#endif
    
		case SOURCELINE:
			match(mInstance,SOURCELINE);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      if(temp) {
        t=to_int(temp);
        if(t) {
          if(t<=mInstance->nlines)
            answer=mystrdup(mInstance,mInstance->lines[to_int(temp)].str);
          else
            answer=NULL;
          }
        else {
          answer=mystrdupint(mInstance,mInstance->nlines);
          }
        }
			match(mInstance,CPAREN);
      free(temp);
			break;
		case TRACE:
			match(mInstance,TRACE);
			match(mInstance,OPAREN);
			temp = expr(mInstance);
      buf[0]=mInstance->traceLevel;
      buf[1]=0;
      if(temp) {
        mInstance->traceLevel=toupper(*temp);    // accttare solo valori giusti!
        }
			match(mInstance,CPAREN);
      answer=mystrdup(mInstance,buf);
      free(temp);
			break;
      
		default:
      setError(mInstance,ERR_SYNTAX);
	  	break;
  	}

  return answer;
	}


char *instr(KOMMISSARREXX *mInstance) {
  char *str,*str2;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

  match(mInstance,POS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    if(offset >= 0 && offset < strlen(str)) {
      end = strstr(str + offset, substr);
      if(end) {
        itoa(buf,end - str + 1,10);
        }
      else
        itoa(buf,0,10);
      }
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}

char *instr2(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

  match(mInstance,LASTPOS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}


char *words(KOMMISSARREXX *mInstance) {
  const char *str,*p;
  int n;
  char *answer;
  char buf[8]; 

  match(mInstance,WORDS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  
  if(str) {
    p=skipSpaces(str);
    n=*p ? 1 : 0;
    while(p=strchr(p,' ')) {
      p=skipSpaces(p);
      if(*p)
        n++;
      }
    
    answer = mystrdupint(mInstance,n);

    free((void*)str);
    return answer;
    }
  else
    return NULL;
	}

char *word(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;
//FARE FINIRE
  match(mInstance,WORDTOK);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}

char *wordpos(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

//FARE FINIRE
  match(mInstance,WORDPOS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        // qua forse bisognerebbe ignorare gli spazi tra le parole... FINIRE
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}

char *wordindex(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

//FARE FINIRE
  match(mInstance,WORDINDEX);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        // qua forse bisognerebbe ignorare gli spazi tra le parole... FINIRE
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}

char *wordlength(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

//FARE FINIRE
  match(mInstance,WORDLENGTH);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        // qua forse bisognerebbe ignorare gli spazi tra le parole... FINIRE
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}

char *delword(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char *substr;
  char *end;
  char *answer =NULL;
  char buf[8];
  int offset=0;

//FARE FINIRE
  match(mInstance,WORDPOS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    substr = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        offset = to_int(str2);
        offset--;
        free((void*)str2);
        }
      }
    match(mInstance,CPAREN);

    p=str;
    *buf=0;
    if(offset >= 0 && offset < strlen(str)) {
      while(*p) {   // in effetti ci si può fermare quando mancano str2 car al fondo..
        // qua forse bisognerebbe ignorare gli spazi tra le parole... FINIRE
        end = strstr(p + offset, substr);
        if(end)
          itoa(buf,end - str + 1,10);
        p++;
        }
      // OTTIMIZZARE cazzo :D :D
      }
    if(!*buf)
      itoa(buf,0,10);
    answer = mystrdup(mInstance,buf);

    free(str);
    free(substr);
    }
  
  return answer;
	}


const char *getVariable(KOMMISSARREXX *mInstance) {
  VARIABLESTRING *var;
  char id[IDLENGTH];
  IDENT_LEN len;
  char *temp;
  
  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,STRID);// son tutte così qua! fa lo stesso... spero
  var = findVariable(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
          mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
  if(var) {
    temp=mystrdup(mInstance,var->sval);
    return temp;
    }
  else {
    temp=strupr(mystrdup(mInstance,id));
    return temp;
	  }
  }

LNUM_TYPE variable(KOMMISSARREXX *mInstance) {
  const char *s;
  LNUM_TYPE t;

  s=getVariable(mInstance);
  t=strtold(s,NULL);
// non va  sscanf(s,"%lg",&t);
  free((void*)s);
  return t;
	}

int16_t ivariable(KOMMISSARREXX *mInstance) {
  const char *s;
  int16_t t;

  s=getVariable(mInstance);
  t=to_int(s);
  free((void*)s);
  return t;
	}

const char *stringVar(KOMMISSARREXX *mInstance) {
  const char *s;
  
  s=getVariable(mInstance);
  return s;
	}


/*
  get value of a dimensioned variable from string.
  matches DIMFLTID
*/
LNUM_TYPE dimVariable(KOMMISSARREXX *mInstance) {
  LDIMVARSTRING *dimvar;
  char id[IDLENGTH];
  IDENT_LEN len;
  DIM_SIZE index[MAXDIMS];
  LNUM_TYPE *answer;

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DOT);
  dimvar = findDimVar(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
  if(!dimvar) {
    setError(mInstance,ERR_NOSUCHVARIABLE);
		return 0.0;
  	}

  if(dimvar) {
    switch(dimvar->ndims) {
		  case 1:
		    index[0] = to_int(expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0]);
				break;
      case 2:
				index[0] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[1] = to_int(expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1]);
				break;
		  case 3:
				index[0] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[1] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[2] = to_int(expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
				break;
		  case 4:
				index[0] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[1] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[2] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[3] = to_int(expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
				break;
		  case 5:
				index[0] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[1] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[2] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[3] = to_int(expr(mInstance));
				match(mInstance,COMMA);
				index[4] = to_int(expr(mInstance));
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3], index[4]);
				break;
			}

		match(mInstance,CPAREN);
  	}

  if(answer)
		return *answer;

  return 0.0;

	}


VARIABLESTRING *findVariable(KOMMISSARREXX *mInstance,const char *id,void *parent,BYTE privateVars) {
  int i;

  if(parent) {
    for(i=0; i<mInstance->nvariables; i++) {
      if(mInstance->variables[i].parent == parent && !stricmp(mInstance->variables[i].id, id))
        return &mInstance->variables[i];
      }
    }
  if(privateVars)
    return NULL;
  for(i=0; i<mInstance->nvariables; i++) {
    if(!stricmp(mInstance->variables[i].id, id))
      return &mInstance->variables[i];
    }
  return NULL;
	}


LDIMVARSTRING *findDimVar(KOMMISSARREXX *mInstance,const char *id,void *parent,BYTE privateVars) {
  int i;

  if(parent) {
    for(i=0; i<mInstance->ndimVariables; i++) {
      if(mInstance->dimVariables[i].parent==parent && !stricmp(mInstance->dimVariables[i].id, id))
        return &mInstance->dimVariables[i];
      }
    }
  if(privateVars)
    return NULL;
  for(i=0; i<mInstance->ndimVariables; i++) {
		if(!stricmp(mInstance->dimVariables[i].id, id))
			return &mInstance->dimVariables[i];
    }
  return 0;
	}


/*
  dimension an array.
  Params: id - the id of the array (include leading ()
          ndims - number of dimension (1-5)
		  ... - integers giving dimension size, 
*/
LDIMVARSTRING *dimension(KOMMISSARREXX *mInstance,const char *id, BYTE ndims, ...) {
  LDIMVARSTRING *dv;
  va_list vargs;
  int size = 1;
  int oldsize = 1;
  unsigned char i;
  DIM_SIZE dimensions[MAXDIMS];
  LNUM_TYPE *dtemp;
  char **stemp;
  int16_t *itemp;

  basicAssert(ndims <= MAXDIMS);
  if(ndims > MAXDIMS)
		return 0;

  dv = findDimVar(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
  if(!dv)
		dv = addDimVar(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL);
  if(!dv) {
    setError(mInstance,ERR_OUTOFMEMORY);
		return 0;
  	}

  if(dv->ndims) {
    for(i=0; i<dv->ndims; i++)
			oldsize *= dv->dim[i];
  	}
  else
		oldsize = 0;

  va_start(vargs, ndims);
  for(i=0; i<ndims; i++) {
		dimensions[i] = va_arg(vargs, int);
    size *= dimensions[i];
  	}
  va_end(vargs);

  if(dv->str) {
    for(i=size; i<oldsize; i++)
      if(dv->str[i]) {
        free(dv->str[i]);
        dv->str[i] = 0;
        }
    }
  stemp = (char **)realloc(dv->str, size * sizeof(char *));
  if(stemp) {
    dv->str = stemp;
    for(i=oldsize; i<size; i++)
      dv->str[i] = 0;
    }
  else {
    for(i=0; i<oldsize; i++)
      if(dv->str[i]) {
        free(dv->str[i]);
        dv->str[i] = 0;
        }
    setError(mInstance,ERR_OUTOFMEMORY);
    return 0;
    }

  for(i=0; i<MAXDIMS; i++)
		dv->dim[i] = dimensions[i];
  dv->ndims = ndims;

  return dv;
	}


/*
  get the address of a dimensioned array element.
  works for both string and real arrays.
  Params: dv - the array's entry in variable list
          ... - integers telling which array element to get
  Returns: the address of that element, 0 on fail
*/ 
void *getDimVar(KOMMISSARREXX *mInstance, LDIMVARSTRING *dv, ...) {
  va_list vargs;
  DIM_SIZE index[MAXDIMS];
  int i;
  void *answer = 0;

  va_start(vargs, dv);
  for(i=0; i<dv->ndims; i++) {
		index[i] = va_arg(vargs, int);
		}
  va_end(vargs);

  for(i=0; i<dv->ndims; i++) {
    if(index[i] >= dv->dim[i] || index[i] < 0) {
			setError(mInstance,ERR_BADSUBSCRIPT);
			return 0;
  		}
		}
//  for(i=0; i<dv->ndims; i++)  OPTION BASE diciamo, lui partiva da 1, io NO!
//    index[i]--;

  switch(dv->ndims)	{
    case 1:
      answer = &dv->d.str[ index[0] ]; 
      break;
    case 2:
      answer = &dv->d.str[ index[1] * dv->dim[0] 
        + index[0] ];
      break;
    case 3:
      answer = &dv->d.str[ index[2] * (dv->dim[0] * dv->dim[1]) 
      + index[1] * dv->dim[0] 
      + index[0] ];
      break;
    case 4:
      answer = &dv->d.str[ index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2]) 
        + index[2] * (dv->dim[0] * dv->dim[1]) 
        + index[1] * dv->dim[0] 
        + index[0] ];
      // MANCAva BREAK???
      break;
    case 5:
      answer = &dv->d.str[ index[4] * (dv->dim[0] + dv->dim[1] + dv->dim[2] + dv->dim[3])
        + index[3] * (dv->dim[0] + dv->dim[1] + dv->dim[2])
        + index[2] * (dv->dim[0] + dv->dim[1])
        + index[1] * dv->dim[0]
        + index[0] ];
      break;
    }

  return answer;
	}


/*
  ------------------
*/
VARIABLESTRING *addNewVar(KOMMISSARREXX *mInstance,const char *id,void *parent) {
  VARIABLESTRING *vars;

  vars = (VARIABLESTRING *)realloc(mInstance->variables, (mInstance->nvariables + 1) * 
          sizeof(VARIABLESTRING));
  if(vars) {
		mInstance->variables = vars;
    strncpy(mInstance->variables[mInstance->nvariables].id, id, IDLENGTH-1);
		mInstance->variables[mInstance->nvariables].sval = NULL;		// clears 
		mInstance->variables[mInstance->nvariables].parent = parent;    // metterci proc/function se locali
		mInstance->nvariables++;
		return &mInstance->variables[mInstance->nvariables-1];
	  }
  else {
		setError(mInstance,ERR_OUTOFMEMORY);
		return 0;
		}
	}

VARIABLESTRING *addVar(KOMMISSARREXX *mInstance,const char *id,void *parent) {
  VARIABLESTRING *v = addNewVar(mInstance,id,parent);

  switch(mInstance->traceLevel) {
    case 'A':
      err_printf("%svariable created %s\r\n",parent ? "-" : "",id);    // finire..
      break;
    case '?':
      inputKey();
      break;
    default:
      break;
    }

  return v; 
	}


LDIMVARSTRING *addDimVar(KOMMISSARREXX *mInstance,const char *id,void *parent) {
  LDIMVARSTRING *vars;

  vars = (LDIMVARSTRING *)realloc(mInstance->dimVariables, (mInstance->ndimVariables + 1) * sizeof(LDIMVAR));
  if(vars) {
    mInstance->dimVariables = vars;
		strcpy(mInstance->dimVariables[mInstance->ndimVariables].id, id);
		mInstance->dimVariables[mInstance->ndimVariables].str = NULL;
		mInstance->dimVariables[mInstance->ndimVariables].ndims = 0;
		mInstance->dimVariables[mInstance->ndimVariables].parent = parent;
		mInstance->ndimVariables++;
		return &mInstance->dimVariables[mInstance->ndimVariables-1];
	  }
  else
		setError(mInstance,ERR_OUTOFMEMORY);
 
  return 0;
	}

#ifdef REGINA_REXX
char *trimString(KOMMISSARREXX *mInstance) {
  char *str;
  char *answer;

  match(mInstance,TRIM);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(!str)
		return NULL;
  match(mInstance,CPAREN);

  while(isspace(*str))
		str++;
  answer = mystrdup(mInstance,str);
  free(str);
  return answer;
	}
#endif

char *stripString(KOMMISSARREXX *mInstance) {
  char *str,*str2=NULL,*str3=NULL;
  char *p1,*p2;
  char *answer=NULL;

  match(mInstance,STRIP);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    if(mInstance->token==COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        if(mInstance->token==COMMA) {
          match(mInstance,COMMA);
          str3 = expr(mInstance);
          }
        }
      }
    match(mInstance,CPAREN);
    
    if(!str3)
      str3=" ";
    
    p1=str;
    if(!str2 || *str2=='B' || *str2=='L') {
      while(*p1 == *str3)
        p1++;
      }
    p2=p1;
    while(*p2)
      p2++;
    p2--;
    if(!str2 || *str2=='B' || *str2=='T') {
      while(*p2 == *str3)
        p2--;
      }
    *(p2+1) = 0;
    answer = mystrdup(mInstance,p1);
    free((void*)str);
    free((void*)str2);
    free((void*)str3);
    if(!answer)
      setError(mInstance,ERR_OUTOFMEMORY);
    }
  
  return answer;
	}

char *abbrevString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3;
  char *answer=NULL;
  int len;

  match(mInstance,ABBREV);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          len = atoi(str3);
          free((void*)str3);
          }
        else
          len=strlen(str2);
        }
      match(mInstance,CPAREN);

      answer = mystrdupint(mInstance,!strncmp(str,str2,len));
      free((void*)str2);
      }
    free((void*)str);
    }
  return answer;
  }

char *timeString(KOMMISSARREXX *mInstance) {
  DWORD ti;
  char *answer,buf[32],*options=(char*)EmptyString,*time_in,*options_in;
  PIC32_DATE date;
  PIC32_TIME time;
  signed char i;

  ti=now;
  match(mInstance,TIME);
  match(mInstance,OPAREN);
  if(mInstance->token != CPAREN) {
    options = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      time_in = expr(mInstance);
      ti=to_time(time_in);
      if(mInstance->token == COMMA) {
        match(mInstance,COMMA);
        options_in = expr(mInstance);
        }
      }
    match(mInstance,CPAREN);
    }

  SetTimeFromNow(ti,&date,&time);
  switch(*options) {
    case 'C':
      sprintf(buf,"%02u:%02u%cm",
        time.hour % 12 ? time.hour % 12 : 12,time.min,time.hour >= 12 ? 'p' : 'a');
      break;
    case 'E':
      sprintf(buf,"%09u.%u", /*%06u*/ now,TMR2);    // boh, fare, shiftare now e sommare timer?
      break;
    case 'R':
      sprintf(buf,"%09u.%u", /*%06u*/ now,TMR2);    // boh, fare e verificare
      break;
    case 'H':
      sprintf(buf,"%02u", time.hour);
      break;
    case 'L':
      sprintf(buf,"%02u:%02u:%02u.%u", time.hour,time.min,time.sec,TMR2); // idem
      break;
    case 'M':
      sprintf(buf,"%02u", time.min);
      break;
    case 'S':
      sprintf(buf,"%02u", time.sec);
      break;
    case 'N':
      sprintf(buf,"%02u:%02u:%02u", time.hour,time.min,time.sec);
      break;
    default:
      i=findTimezone();
      sprintf(buf,"%02u:%02u:%02u %c%d",    // timezone idea mia, non c'era :)
        time.hour,time.min,time.sec,
        i>=0 ? '+' : '-',abs(i));
      break;
    }
/* C (Civil)?Returns hh:mmxx civil-format time. xx is am or pm .
 E (Elapsed)?Returns elapsed time since the clock was started or reset, in the format
sssssssss.uuuuuu
 H (Hours)?Returns the number of completed hours since midnight in the format hh. Values
range from 0 to 23.
 L (Long)?Returns the time in long format: hh:mm:ss.uuuuuu
 M (Minutes)?Returns the number of completed minutes since midnight in the format mmmm
 N (Normal)?Returns the time in the default format (hh:mm:ss)
 R (Reset)?Returns elapsed time since the clock was started or reset in the format sssssssss.uuuuuu
 S (Seconds)?Returns the number of complete seconds since midnight*/
          
  answer = mystrdup(mInstance,buf);
  return answer;
	}

char *dateString(KOMMISSARREXX *mInstance) {
  DWORD ti;
  char *answer,buf[32],*options=(char*)EmptyString,*time_in,*options_in;
  PIC32_DATE date;
  PIC32_TIME time;
  signed char i;
  int t;

  ti=now;
  match(mInstance,DATE);
  match(mInstance,OPAREN);
  if(mInstance->token != CPAREN) {
    options = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      time_in = expr(mInstance);
      ti=to_time(time_in);
      if(mInstance->token == COMMA) {
        match(mInstance,COMMA);
        options_in = expr(mInstance);
        }
      }
    match(mInstance,CPAREN);
    }

  SetTimeFromNow(ti,&date,&time);
  switch(*options) {
    case 'B':
      itoa(buf,now / 86400,10);
      break;
    case 'D':
      {
      i=date.mon;
      t=0;
      while(--i)
        t+=days_month[isleap(date.year)][date.mon];
      t+=date.mday;
      itoa(buf,t,10);
      }
      break;
    case 'E':
      sprintf(buf,"%02u/%02u/%02u",date.mday,date.mon,date.year % 100);
      break;
    case 'M':
      strcpy(buf,months[date.mon]);
      break;
    case 'O':
      sprintf(buf,"%02u:%02u:%02u.%6u", time.hour,time.min,time.sec,0); // idem
      break;
    case 'S':
      sprintf(buf,"%04u%02u%02u",date.year,date.mon,date.mday);
      break;
    case 'U':
      sprintf(buf,"%02u/%02u/%02u",date.mon,date.mday,date.year % 100);
      break;
    case 'W':
      strcpy(buf,wdays[date.weekday]);
      break;
    case 'N':
    default:
      sprintf(buf,"%2u %s %04u",date.mday,months[date.mon],date.year);
      break;
    }
/*B (Base)?Returns the number of complete days since the base date of January 1, 0001.
D (Days)?Returns the number of days so far in the year (includes the current day)
E (European)?Returns the date in EU format, dd/mm/yy
M (Month)?Returns the full English name of the current month, for example: June
N (Normal)?Returns the date in the default format (see above)
O (Ordered)?Returns the date in a sort-friendly format yy/mm/dd
S (Standard)?Returns the date in the sort-friendly format yyyymmdd
U (USA)?Returns the date in American format, mm/dd/yy
W (Weekday)?Returns the English name for the day of the week, for example: Monday*/
          
  answer = mystrdup(mInstance,buf);
  return answer;
	}

char *datatypeString(KOMMISSARREXX *mInstance) {
  const char *temp,*temp2,*temp3;
  char *answer;
  int t;
  
  match(mInstance,DATATYPE);
  match(mInstance,OPAREN);
  temp = expr(mInstance);
  t=0;
  if(mInstance->token == COMMA) {
    match(mInstance,COMMA);
    temp2 = expr(mInstance);
    if(temp2) {
      t=*temp2;
      free((void*)temp2);
      }
    }

  match(mInstance,CPAREN);
  if(temp) {
    switch(t) {
      case 'A':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!isalnum(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'B':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(*temp3 != '0' && *temp3 != '1') {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'L':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!isupper(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'M':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!isalpha(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'N':
        answer=datatype(temp) == STRID ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
        break;
      default:
        answer=datatype(temp) == STRID ? mystrdup(mInstance,"CHAR") : mystrdup(mInstance,"NUM");
        break;
      case 'S':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!isalnum(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'U':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!islower(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      case 'W':
        answer=datatype(temp) == FLTID ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
        break;
      case 'X':
      {
        temp3=temp;
        t=1;
        while(*temp3) {
          if(!isxdigit(*temp3)) {
            t=0;
            break;
            }
          temp3++;
          }
        answer=datatype(temp) == t ? mystrdupint(mInstance,1) : mystrdupint(mInstance,0);
      }
        break;
      }
    if(!answer)
      setError(mInstance,ERR_OUTOFMEMORY);
    free((void*)temp);
    }
  else
    setError(mInstance,ERR_SYNTAX);
  }


BYTE get_args(const char *args,BYTE w,char *ret) {
  BYTE n;
  BYTE inQuote=0;
  char *p1,delimiter;

  if(ret)
    *ret=0;
  args=skipSpaces(args);
  n=*args ? 1 : 0;
  if(!n && !w) {
    return 0;
//    if(ret)
//      strcpy(ret,args);
    }
  p1=ret;
  while(*args) {
    switch(*args) {
      case '"':
        if(inQuote) {
          if(delimiter=='"') {
            inQuote=0;
            delimiter=0;
            }
          else
            goto add_char;
          }
        else {
          inQuote=1;
          delimiter='"';
          }
        break;
      case '\'':
        if(inQuote) {
          if(delimiter=='\'') {
            inQuote=0;
            delimiter=0;
            }
          else
            goto add_char;
          }
        else {
          inQuote=1;
          delimiter='\'';
          }
        break;
      case ' ':
        if(!inQuote) {
          if(p1)
            *p1=0;
          if(n>=w)
            goto fine;
          p1=ret;
          n++;
          }
        else
          goto add_char;
        break;
      default:
add_char:
        if(p1)
          *p1++=*args;
        break;
      }
    args++;
    }
	if(w>n)		// non esiste l'arg richiesto, ergo pulisco il buffer
    p1=ret;
  if(p1)
    *p1=0;
  
#if 0
  while(args=strchr(args,' ')) {
#warning GESTIRE QUOTES in args!
    args=skipSpaces(args);
    if(*args)
      n++;
    if(n==w) {
      if(ret)
        strcpy(ret,args);
      }
    p++;
    }

  if(ret) {
    while(*ret && *ret != ' ')
      ret++;
    // ev. altro carattere separatore...
    *ret=0;
    }
#endif
    
fine:
  return n;
	}

char *argString(KOMMISSARREXX *mInstance) {
  char *answer,buf[32],*str=NULL,*str2=NULL,*args;
  BYTE argnum,i;

  match(mInstance,ARG);
  match(mInstance,OPAREN);
  if(mInstance->token != CPAREN) {
    str = expr(mInstance);
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      }
    match(mInstance,CPAREN);
    }

  if(mInstance->ngosubs>0) {
    if(mInstance->gosubStack[mInstance->ngosubs-1].args) {
      args=(char *)mInstance->gosubStack[mInstance->ngosubs-1].args;
      }
    else
      args=(char *)EmptyString;
    }
  else
    args=scriptArgs;
  
  if(!str && !str2) {
q_arg:
    answer = mystrdupint(mInstance,get_args(args,255,NULL));
    }
  else {
    argnum=to_int(str);
    if(!argnum)
      goto q_arg;     // non c'è ma direi di sì!
    if(!str2) {
      *buf=0;
      get_args(args,argnum,buf);
      answer = mystrdup(mInstance,buf);
      }
    else {
      get_args(args,argnum,buf);
      if(toupper(*str2) == 'E')
        i=*buf ? 1 : 0;
      else
        i=*buf ? 0 : 1;
      answer = mystrdupint(mInstance,i);
      }
    }
          
  return answer;
	}


#if 0
char *inkeyString(KOMMISSARREXX *mInstance) {
  char buf[2];
	int i;
  char *answer;

  match(mInstance,INKEYSTRING);
//  match(mInstance,OPAREN);
//  match(mInstance,CPAREN);

	i=mInstance->incomingChar[0];
	if(i>0) {
		buf[0]=i;
		buf[1]=0;
		}
	else
		buf[0]=0;
  answer = mystrdup(mInstance,buf);

  return answer;
	}
#endif

#ifdef USA_BREAKTHROUGH
char *menukeyString(KOMMISSARREXX *mInstance) {
  char buf[2];
	int i;
  char *answer;

  match(mInstance,MENUKEYSTRING);
  match(mInstance,OPAREN);
  match(mInstance,CPAREN);

//	i=menucommand();
  //fare
		buf[0]=0;
    
  answer = mystrdup(mInstance,buf);

  return answer;
	}
#endif

char *errorString(KOMMISSARREXX *mInstance) {
  char *answer;
	BYTE x;
  const char *str;

  match(mInstance,ERRORTEXT);
  match(mInstance,OPAREN);
  str=expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    x = to_int(str);
  #warning CONTROLLARE EXPR e pulire retvalue, TUTTE!
    if(x >= sizeof(error_msgs)/sizeof(char *)) {
      setError(mInstance, ERR_BADVALUE);
      }

    answer = mystrdup(mInstance,(STRINGFARPTR)error_msgs[x]);

    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}

BSTATIC char *digits(KOMMISSARREXX *mInstance) {

  match(mInstance,DIGITS);
  match(mInstance,OPAREN);
  match(mInstance,CPAREN);
  return mystrdupint(mInstance,digitSizeDecimal);
	}
BSTATIC char *form(KOMMISSARREXX *mInstance) {

  match(mInstance,FORM);
  match(mInstance,OPAREN);
  match(mInstance,CPAREN);
    
  return mystrdup(mInstance,digitForm ? "ENGINEERING" : "SCIENTIFIC");
	}
BSTATIC char *fuzz(KOMMISSARREXX *mInstance) {

  match(mInstance,FUZZ);
  match(mInstance,OPAREN);
  match(mInstance,CPAREN);
  
  return mystrdupint(mInstance,digitFuzz);
	}
BSTATIC char *b2x(KOMMISSARREXX *mInstance) {
  char *answer;
  const char *str;

  match(mInstance,B2X);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    answer = mystrdup(mInstance,myitohex(mybtoi(str)));
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *c2d(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[65];
  const char *str,*str2,*p;
  BYTE len=0;
  unsigned long long n;

  match(mInstance,C2D);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        len = min(16,to_int(str2));
        free((void *)str2);
        }
      }
    match(mInstance,CPAREN);

    n=0;
    p=str;
    if(!len)
      len=strlen(str);
    while(len--) {
      n <<= 8;
      n |= *p++;
      }
    sprintf(buf,"%llu",n); 
    answer = mystrdup(mInstance,buf);
    
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *c2x(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[257],buf2[3];
  const char *str,*p;

  match(mInstance,C2X);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    *buf=0;
    p=str;
    while(*p) {
      sprintf(buf2,"%02X",*p++);
      strcat(buf,buf2);
      }
    answer = mystrdup(mInstance,buf);
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *d2c(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[2];
  const char *str;

  match(mInstance,D2C);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    buf[0]=to_int(str);
    buf[1]=0;
    answer = mystrdup(mInstance,buf);
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *d2x(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[17];
  const char *str;

  match(mInstance,D2X);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    sprintf(buf,"%X",to_int(str));
    answer = mystrdup(mInstance,buf);

    free((void *)str);
    return answer;
    }
  else
    return NULL;

	}
BSTATIC char *x2b(KOMMISSARREXX *mInstance) {
  char *answer;
  const char *str;

  match(mInstance,X2B);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    answer=mystrdup(mInstance,myitob(myhextoi(str)));
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *x2c(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[17];
  const char *str,*p;
  BYTE n,c;

  match(mInstance,X2C);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  match(mInstance,CPAREN);
  if(str) {
    n=0;
    p=str;
    while(*p && n<16) {
      c=myhextob(*p++);
      if(*p)
        break;
      c <<= 4;
      c |= myhextob(*p++);
      buf[n]=c;
      }
    answer = mystrdup(mInstance,buf);
    free((void *)str);
    return answer;
    }
  else
    return NULL;
	}
BSTATIC char *x2d(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[17];
  BYTE len;
  const char *str,*str2;

  match(mInstance,X2D);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        len = min(16,to_int(str2));
        free((void *)str2);
        }
      }
    match(mInstance,CPAREN);

    if(len)
      sprintf(buf,"%d",myhextoi(str));    // boh v. esempio/doc
    else
      sprintf(buf,"%u",myhextoi(str));

    answer = mystrdup(mInstance,buf);
    free((void *)str);
    return answer;
    }
  else
    return NULL;

	}

#ifdef REGINA_REXX
char *c2b(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *exists(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *freespace(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *mygetenv(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *getpid(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *getspace(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *import(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *justify(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *trim(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *uname(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *writech(KOMMISSARREXX *mInstance) {
  char *answer;
	}
char *writeln(KOMMISSARREXX *mInstance) {
  char *answer;
	}
#endif


char *addressString(KOMMISSARREXX *mInstance) {
  char *answer;
  char option=0;
  char *str,buf[32];

  match(mInstance,ADDRESS);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    option=toupper(*str);
    free((void*)str);
    }
  match(mInstance,CPAREN);
  
  switch(option) {
    default:
    case 'N':
      strncpy(buf,_PC_PIC_COMMAND_C /*_PC_PIC_CPU_C*/,sizeof(buf)-1);
      buf[sizeof(buf)-1]=0;
      break;
    case 'I':
      //bah controllare m_stdin
      // usare deviceNames[]
      sprintf(buf,"INPUT NORMAL");
      break;
    case 'O':
      //bah controllare m_stdout
      sprintf(buf,"REPLACE NORMAL");
      break;
    case 'E':
      //bah controllare m_stderr
      sprintf(buf,"REPLACE NORMAL");
      break;
    }
  
  answer = mystrdup(mInstance,buf);
  return answer;
	}

char *verString(KOMMISSARREXX *mInstance) {
  char *answer;
  char buf[7];

  match(mInstance,VERSION);
  buf[0]='v';   // se no rompe le palle con il NUMERIC...
  buf[1]=KOMMISSARREXX_COPYRIGHT_STRING[27];
  buf[2]=KOMMISSARREXX_COPYRIGHT_STRING[27+1];
  buf[3]=KOMMISSARREXX_COPYRIGHT_STRING[27+2];
  buf[4]=KOMMISSARREXX_COPYRIGHT_STRING[27+3];
  buf[5]=KOMMISSARREXX_COPYRIGHT_STRING[27+4];
  buf[6] = 0;
  answer = mystrdup(mInstance,buf);

  return answer;
	}

char *lineinString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*answer;
  SUPERFILE f;
  char buf[256];
  char *filename;
  
  *buf=0;
  match(mInstance,LINEIN);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    if(mInstance->token == COMMA) {
      match(mInstance,COMMA);
      str2 = expr(mInstance);
      if(str2) {
        
        free((void *)str2);
        }
      }
    getDrive(str,&f,&filename);
    if(SuperFileOpen(&f,filename,'r')) {
      SuperFileGets(&f,buf,255);
      SuperFileClose(&f);
      }
    free((void*)str);
    }
  else {
    inputString(buf,255,"?",0);
    putchar('\n'); putchar('\n');
    }
  match(mInstance,CPAREN);
    
  answer=mystrdup(mInstance,buf);
    
  return answer;
	}

char *lineoutString(KOMMISSARREXX *mInstance) {
  char *str=NULL,*str2=NULL,*str3;
  SUPERFILE f;
  int newpos=0;
  int retval=-1;
  char *filename;
  
  match(mInstance,LINEOUT);
  match(mInstance,OPAREN);
  if(mInstance->token != COMMA) {
    str = expr(mInstance);
    }
  match(mInstance,COMMA);
  if(mInstance->token != COMMA) {
    str2 = expr(mInstance);
    }
  if(mInstance->token==COMMA) {
    match(mInstance,COMMA);
    str3 = expr(mInstance);
    if(str3) {
      newpos = to_int(str3);
      free((void*)str3);
      }
    }
  if(str) {
    getDrive(str,&f,&filename);
    if(SuperFileOpen(&f,filename,'a')) {    // GESTIRE NEWPOS!
      if(str2) {
        retval=!SuperFileWrite(&f,str2,strlen(str2));
        SuperFileWrite(&f,"\r\n",2);
        }
      else
        ; //retVal= //fare seek to end
      SuperFileClose(&f);
      }
    free((void*)str);
    }
  else {
    if(str2) {
      puts(str2);
      retval=0;
      }
    }
  match(mInstance,CPAREN);
    
  if(str)
    free((void *)str);
  if(str)
    free((void *)str2);
  
  return mystrdupint(mInstance,retval);
	}

enum DATATYPE datatype(const char *s) {
  enum DATATYPE t=STRID;
  BYTE cnt;
  
  s=skipSpaces(s);
  if(isdigit(*s) || *s == '-' || *s == '+') {
    t=INTID;
    s++;
    cnt=1;
    while(*s) {
      if(!isdigit(*s) && *s != '.')
        return STRID;
      // magari anche E esponenziale..
      if(*s == '.')   // ma le matrici qua??
        t=FLTID;
      s++;
      cnt++;
      }
    }
  if(t==INTID && cnt>4)   // boh per ora così!
//      if(n < SHRT_MAX && n > SHRT_MIN && ((int16_t)n) == n) v. sopra
    t=FLTID;
  return t;
  }

char *leftString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3;
  int x;
  char *answer=NULL;
  char pad=' ';

  match(mInstance,LEFT);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      x = to_int(str2);
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          pad = *str3;
          free((void*)str3);
          }
        }
      free(str2);
      match(mInstance,CPAREN);

      if(x > strlen(str))
        return str;
      if(x < 0) {
        setError(mInstance,ERR_ILLEGALOFFSET);
        return str;
        }
      str[x] = 0;
      answer = mystrdup(mInstance,str);
      }
    free((void*)str);
    }
  return answer;
  }

char *delString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3;
  int x;
  char *answer=NULL;
  char pad=' ';

  match(mInstance,LEFT);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str = expr(mInstance);
    if(str2) {
      x = to_int(str2);
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          pad = *str3;
          free((void*)str3);
          }
        }
      free(str2);
      match(mInstance,CPAREN);

      if(x > strlen(str))
        return str;
      if(x < 0) {
        setError(mInstance,ERR_ILLEGALOFFSET);
        return str;
        }
      str[x] = 0;
      answer = mystrdup(mInstance,str);
      }
    free((void*)str);
    }
  return answer;
  }

char *countString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  char buf[8];
  int n;
  char *answer=NULL;

  match(mInstance,COUNTSTR);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str = expr(mInstance);
    if(str2) {
      match(mInstance,CPAREN);

      n=0;
      p=str2;
      while(p=strstr(p,str)) {
        p+=strlen(str);
        n++;
        }
      answer = mystrdupint(mInstance,n);
      free(str2);
      }
    free((void*)str);
    }
  return answer;
  }

char *rightString(KOMMISSARREXX *mInstance) {
  int x;
  char *str,*str2;
  char *answer;

  match(mInstance,RIGHT);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    match(mInstance,CPAREN);
    if(str2) {
      x = to_int(str2);

      if(x > strlen(str))
        return str;

      if(x < 0) {
        setError(mInstance,ERR_ILLEGALOFFSET);
        return str;
        }

      answer = mystrdup(mInstance, &str[strlen(str) - x]);
      free(str2);
      }
    free(str);
    }
  
  return answer;
	}

char *reverseString(KOMMISSARREXX *mInstance) {
  char *str,c;
  char buf[8];
  int i,n,n2;
  char *answer=NULL;

  match(mInstance,REVERSE);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    n=strlen(str);
    n2=n/2;
    for(i=0; i<n2; i++) {
      c=str[n-1-i];
      str[n-1-i]=str[i];
      str[i]=c;
      }
    answer = mystrdup(mInstance,str);   // in questo caso si potrebbe lasciare l'originale??
    free((void*)str);
    }
  match(mInstance,CPAREN);
  return answer;
  }

char *subString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3,*str4;
  int x;
  int len,newlen;
  char *answer=NULL;
  char *temp;
  char pad=' ';

  match(mInstance,SUBSTR);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      x = to_int(str2);
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          newlen = to_int(str3);
          if(mInstance->token==COMMA) {
            match(mInstance,COMMA);
            str4 = expr(mInstance);
            if(str4) {
              pad = *str4;
              free((void*)str4);
              }
            }
          free((void*)str3);
          }
        else
          newlen=-1;
        }
      }
    match(mInstance,CPAREN);

    len = strlen(str) - x + 1;
    if(newlen == -1)
      newlen=len;

    if(x > strlen(str) || len < 1) {
      free(str);
      answer = mystrdup(mInstance,EmptyString);
      return answer;
      }

    if(x < 1) {
      setError(mInstance,ERR_ILLEGALOFFSET);
      return str;
      }

    temp = &str[x-1];

    answer = (char *)malloc(newlen + 1);
    if(!answer) {
      setError(mInstance,ERR_OUTOFMEMORY);
      return str;
      }
    strncpy(answer, temp, len);
    while(len<newlen)
      answer[len++] = pad;
    answer[len] = 0;
    free((void*)str);
    }
  return answer;
	}

char *compareStrings(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3;
  int i,len1,len2;
  char *answer=NULL;
  char *temp;
  char pad=' ',ch1,ch2;
  char buf[8];

  match(mInstance,COMPARE);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          pad = *str3;
          free((void*)str3);
          }
        }
      match(mInstance,CPAREN);

      len1 = strlen(str);
      len2 = strlen(str2);
      for(i=0; i<max(len1,len2); i++) {
        if(i<len1)
          ch1=str[i];
        else
          ch1=pad;
        if(i<len2)
          ch2=str2[i];
        else
          ch2=pad;
        if(ch1 != ch2)
          break;
        }

      itoa(buf,i == max(len1,len2) ? 0 : i,10);
      answer=mystrdup(mInstance,buf);

      free((void*)str2);
      }
    free((void*)str);
    }
  return answer;
	}

char *translateString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3,*str4;
  int x;
  int len,newlen;
  char *answer=NULL;
  char *temp;
  char pad=' ';

  match(mInstance,TRANSLATE);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      x = to_int(str2);
      if(mInstance->token==COMMA) {
        match(mInstance,COMMA);
        str3 = expr(mInstance);
        if(str3) {
          newlen = to_int(str3);
          if(mInstance->token==COMMA) {
            match(mInstance,COMMA);
            str4 = expr(mInstance);
            if(str4) {
              pad = *str4;
              free((void*)str4);
              }
            }
          free((void*)str3);
          }
        else
          newlen=-1;
        }
      }
    match(mInstance,CPAREN);

    len = strlen(str) - x + 1;
    if(newlen == -1)
      newlen=len;

    if(x > strlen(str) || len < 1) {
      free(str);
      answer = mystrdup(mInstance,EmptyString);
      return answer;
      }

    if(x < 1) {
      setError(mInstance,ERR_ILLEGALOFFSET);
      return str;
      }

    temp = &str[x-1];

    answer = (char *)malloc(newlen + 1);
    if(!answer) {
      setError(mInstance,ERR_OUTOFMEMORY);
      return str;
      }
    strncpy(answer, temp, len);
    while(len<newlen)
      answer[len++] = pad;
    answer[len] = 0;
    free((void*)str);
    }
  return answer;
	}


char *xrange(KOMMISSARREXX *mInstance) {
  char *str,*str2,*p;
  int i,n1=1,n2=255;
  char *answer;

  match(mInstance,XRANGE);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    n1 = to_int(str);
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      n2 = to_int(str2);
      free((void*)str2);
      }
    free((void*)str);
    }
  match(mInstance,CPAREN);
  
  answer=(char *)malloc(n2-n1+1);
  if(!answer) {
    setError(mInstance,ERR_OUTOFMEMORY);
    return NULL;
    }
  p=answer;
  for(i=n1; i<=n2; i++)    // ovviamente lo 0 iniziale non va molto bene :) v. sopra
    *p++=i;
  *p++=0;
    
  return answer;
	}


int16_t to_int(const char *s) {
  
  return atoi(s);
  }

LNUM_TYPE to_num(const char *s) {
  LNUM_TYPE t;
  
  return strtold(s,NULL);
//  non va cmq sscanf(s,"%lg",&t);
  // provare L maiuscola!
//  return t;
  }

char *to_string(LNUM_TYPE n,char *s) {
  
//  sprintf(s,"%Lf",(long double)n);  //Lf funzia ma mette solo 6 digit significativi...
  ldtoa(n,s,digitSizeDecimal);
  return s;
  }

char *to_string_i(int16_t n,char *s) {
  
  if(n>=0)
//    sprintf(s," %d",n);	// metto lo spazio prima dei numeri... se non negativi...
    sprintf(s,"%d",n);
  else
    sprintf(s,"%d",n);
  return s;
  }

DWORD to_time(const char *s) {
  
  //FARE!!
  return 1;
  }


char *stringString(KOMMISSARREXX *mInstance) {
  const char *str,*str2;
  char *answer=NULL;
  int len;
  int N;
  int i;

  match(mInstance,COPIES);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      N = to_int(str2);
      free((void*)str2);
      }
    match(mInstance,CPAREN);

    if(N < 1) {
      answer = mystrdup(mInstance,EmptyString);
      }
    else {
      len = strlen(str);
      answer = (char *)malloc(N * len + 1);
      if(!answer) {
        setError(mInstance,ERR_OUTOFMEMORY);
        return NULL;
        }
      for(i=0; i < N; i++)
        strcpy(answer + len*i, str);
      }
    free((void *)str);
    }

  return answer;
	}

char *charin(KOMMISSARREXX *mInstance) {
  char *str=NULL,*str2=NULL,*str3=NULL,*answer;
  SUPERFILE f;
  char buf[256];
  char *filename;
  BYTE len=1;
  int newpos=-1;
  
  *buf=0;
  match(mInstance,CHARIN);
  match(mInstance,OPAREN);
  if(mInstance->token != COMMA) {
    str = expr(mInstance);
    }
  if(mInstance->token != COMMA) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    len = to_int(str2);
    free((void*)str2);
    }
  if(mInstance->token==COMMA) {
    match(mInstance,COMMA);
    str3 = expr(mInstance);
    if(str3) {
      newpos = to_int(str3);
      free((void*)str3);
      }
    }
  match(mInstance,CPAREN);
  len=min(sizeof(buf),len);
  if(str) {
    getDrive(str,&f,&filename);
    if(SuperFileOpen(&f,filename,'r')) {
      SuperFileGets(&f,buf,len);
      SuperFileClose(&f);
      }
    free((void*)str);
    }
  else {
    inputString(buf,len,"?",0);
    putchar('\n'); putchar('\n');
    }
    
  buf[len]=0;
  answer=mystrdup(mInstance,buf);
    
  return answer;
	}

char *charout(KOMMISSARREXX *mInstance) {
  char *str=NULL,*str2=NULL,*str3;
  int newpos=0;
  int retval=-1;
  SUPERFILE f;
  char *filename;

  match(mInstance,CHAROUT);
  match(mInstance,OPAREN);
  if(mInstance->token != COMMA) {
    str = expr(mInstance);
    }
  match(mInstance,COMMA);
  if(mInstance->token != COMMA) {
    str2 = expr(mInstance);
    }
  if(mInstance->token==COMMA) {
    match(mInstance,COMMA);
    str3 = expr(mInstance);
    if(str3) {
      newpos = to_int(str3);
      free((void*)str3);
      }
    }
  match(mInstance,CPAREN);

  if(str) {
    getDrive(str,&f,&filename);
    if(SuperFileOpen(&f,filename,'a')) {  // GESTIRE newpos
      if(str2) {
        retval=!SuperFileWrite(&f,str2,strlen(str2));
        }
      else
        ; //newpos //retVal= //fare seek to end
      SuperFileClose(&f);
      }
    }
  else {
    if(str2) {
      print(str2);
      retval=0;
      }
    }
  
  free((void*)str2);
  free((void*)str);
  
  return mystrdupint(mInstance,retval);
	}

char *changeString(KOMMISSARREXX *mInstance) {
  char *str,*str2,*str3;
  char *answer=NULL;
  char *p;

  match(mInstance,CHANGESTR);
  match(mInstance,OPAREN);
  str = expr(mInstance);
  if(str) {
    match(mInstance,COMMA);
    str2 = expr(mInstance);
    if(str2) {
      match(mInstance,COMMA);
      str3 = expr(mInstance);
      if(str3) {
        
        p=str2;
        while(p=strstr(p,str)) {
          memcpy(p,str3,strlen(str));
          p+=strlen(str);
          }
        answer=mystrdup(mInstance,str2);
    //FINIRE!! gestire se str e str3 han dim diverse... il doc non dice nulla
        
        free((void*)str2);
        }
      free((void*)str3);
      }
    match(mInstance,CPAREN);
    
    free((void*)str);
    }
  return answer;
	}


/*
  read a dimensioned string variable from input.
  Returns: pointer to string (not malloced) 
*/
char *stringDimVar(KOMMISSARREXX *mInstance) {
  char id[IDLENGTH];
  IDENT_LEN len;
  LDIMVARSTRING *dimvar;
  char **answer;
  DIM_SIZE index[MAXDIMS];

  getId(mInstance,mInstance->string, id, &len);
  match(mInstance,DIMSTRID);
  dimvar = findDimVar(mInstance,id,mInstance->ngosubs>0 ? (void*)(uint32_t)mInstance->gosubStack[mInstance->ngosubs-1].line : NULL,
              mInstance->ngosubs>0 ? mInstance->gosubStack[mInstance->ngosubs-1].privateVars : 0);
#if 0
  if(dimvar) {
    switch(dimvar->ndims) {
	  	case 1:
	    	index[0] = expr(mInstance);
				answer = getDimVar(mInstance,dimvar, index[0]);
				break;
      case 2:
				index[0] = expr(mInstance);
				match(mInstance,COMMA);
				index[1] = expr(mInstance);
				answer = getDimVar(mInstance,dimvar, index[0], index[1]);
				break;
		  case 3:
				index[0] = expr(mInstance);
				match(mInstance,COMMA);
				index[1] = expr(mInstance);
				match(mInstance,COMMA);
				index[2] = expr(mInstance);
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2]);
				break;
		  case 4:
				index[0] = expr(mInstance);
				match(mInstance,COMMA);
				index[1] = expr(mInstance);
				match(mInstance,COMMA);
				index[2] = expr(mInstance);
				match(mInstance,COMMA);
				index[3] = expr(mInstance);
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3]);
				break;
		  case 5:
				index[0] = expr(mInstance);
				match(mInstance,COMMA);
				index[1] = expr(mInstance);
				match(mInstance,COMMA);
				index[2] = expr(mInstance);
				match(mInstance,COMMA);
				index[3] = expr(mInstance);
				match(mInstance,COMMA);
				index[4] = expr(mInstance);
				answer = getDimVar(mInstance,dimvar, index[0], index[1], index[2], index[3], index[4]);
				break;
			}

		match(mInstance,CPAREN);
  	}
  else
		setError(mInstance,ERR_NOSUCHVARIABLE);
#endif
  if(!mInstance->errorFlag)
		if(*answer)
	     return *answer;
	 
  return (char *)EmptyString;
	}


char *stringLiteral(KOMMISSARREXX *mInstance) {
  int len = 1;
  char *answer = 0;
  char *temp;
  char *substr;
  const char *end;
  char delimiter;
  char buf[17];

  while(mInstance->token == QUOTE) {
    mInstance->string=(char *)skipSpaces(mInstance->string);
    delimiter=*mInstance->string;

    end = mystrend((const char *)mInstance->string, delimiter);
    if(end) {
      len = end - mInstance->string;
      substr = (char *)malloc(len);
	  	if(!substr) {
	    	setError(mInstance,ERR_OUTOFMEMORY);
	    	return answer;
	  		}
	  	mystrgrablit(substr, mInstance->string);
	  	if(answer) {
				temp = myStrConcat(answer, substr);
	    	free(substr);
				free(answer);
				answer = temp;
				if(!answer) {
	      	setError(mInstance,ERR_OUTOFMEMORY);
		  		return answer;
					}
	  		}
	  	else
	    	answer = substr;
		  mInstance->string = (char *)end;
			}
		else {
		  setError(mInstance,ERR_SYNTAX);
		  return answer;
			}

		match(mInstance,QUOTE);
  	}

  if(toupper(*mInstance->string) == 'B') {
    mInstance->string++;
    len=mybtoi(answer);
    free(answer);
    answer=mystrdupint(mInstance,len);
    }
  else if(toupper(*mInstance->string) == 'X') {
    mInstance->string++;
    len=myhextoi(answer);
    free(answer);
    answer=mystrdupint(mInstance,len);
    }
  return answer;
	}


void match(KOMMISSARREXX *mInstance,TOKEN_NUM tok) {

  if(mInstance->token != tok) {
		setError(mInstance,ERR_SYNTAX);
		return;
	  }

  mInstance->string=(char *)skipSpaces(mInstance->string);

  mInstance->string += tokenLen(mInstance, mInstance->string, mInstance->token);
  mInstance->token = getToken(mInstance->string);
  if(mInstance->token == B_ERROR)
		setError(mInstance,ERR_SYNTAX);

	}


void setError(KOMMISSARREXX *mInstance,enum INTERPRETER_ERRORS errorcode) {

  if(!mInstance->errorFlag || !errorcode)
		mInstance->errorFlag = errorcode;
	}


/*
  get the next line number
  Params: str - pointer to parse string
  Returns: line no of next line, 0 if end
  Notes: goes to newline, then finds
         first line starting with a digit.
*/
LINE_NUMBER_TYPE getNextStatement(KOMMISSARREXX *mInstance,const char *str) {

  // FINIRE gestendo , multistatement..
  return mInstance->curline+1;
	}



TOKEN_NUM getToken(const char *str) {
	unsigned char i,j;
  
  str=skipSpaces(str);

  if(isdigit(*str))
    return VALUE;
 
  switch(*str) {
    case 0:
	  	return EOS;
    case '\n':
	  	return EOL;
		case '/': 
      switch(*(str+1)) {
        case '/':
  				return DIVINT;
        case '*':
  				return REM;
        default:
    		  return DIV;
        }
		case '*':
      switch(*(str+1)) {
        case '/':
  				return REM2;
        case '*':
  				return POW;
        default:
    		  return MULT;
        }
		case '%':
		  return MOD;
		case '(':
		  return OPAREN;
		case ')':
		  return CPAREN;
		case '+':
		  return PLUS;
		case '-':
		  return MINUS;
		case '&':
		  if(*(str+1)=='&')
        return XOR;
      else
        return AND;
		case '|':
		  return OR;
		case ',':
		  return COMMA;
		case '.':
		  return DOT;
		case ';':
		  return SEMICOLON;
		case ':':
		  return COLON;
		case '"':
		case '\'':
		  if(*(str+1)=='\'')    // mangiarlo...
        ;
		  return QUOTE;
		case '\\': 
		  return NOT;
		case '=':
		  return EQUALS;
		case '<':
		  if(*(str+1)=='<')
  		  return LESS_STRICT;
      else
  		  return LESS;
		case '>':
		  if(*(str+1)=='>')
  		  return GREATER_STRICT;
      else
        return GREATER;
		default:

			for(i=0; i<sizeof(tl)/sizeof(TOKEN_LIST); i++) {
// bit of wasted code: ? and ' are searched here too, but they are not found (not strings) and handled above...
        j=tl[i].length;
        if(!strnicmp((char *)str, (char *)tl[i].tokenname, j) &&
          !isvalidrexx2(str[j]))
          return i+ADDRESS;
        }
	
		  if(isvalidrexx(*str)) {   // @, _, !, ., ?, and $
        // il punto è a parte, direi!
				while(isvalidrexx2(*str))
				  str++;
				switch(*str) {
				  case '.':
						return DIMSTRID;
            break;
          default:
            return STRID;
            break;
					}
			  }
		
			return B_ERROR;
			break;
	  } 		// switch
	}


unsigned char tokenLen(KOMMISSARREXX *mInstance, const char *str, TOKEN_NUM token) {
  IDENT_LEN len=0;
  char buff[20];

  switch(token) {
    case EOS:
	  	return 0;
    case EOL:
	  	return 1;
    case VALUE:
	  	getValue(str, &len);
	  	return len;
		case DIMSTRID:
		case STRID:
			// fallthrough
//		  getId(mInstance,str, buff, &len);
//	  	return len;
		case INTID:
			// fallthrough
//	  	getId(mInstance,str, buff, &len);
//	  	return len;
		case FLTID:
	  	getId(mInstance, str, buff, &len);
	  	return len;

    case DIV:
    case MULT:
		case OPAREN:
    case CPAREN:
    case PLUS:
    case MINUS:
    case MOD:
    case COMMA:
    case DOT:
		case QUOTE:
		case EQUALS:
		case LESS:
		case GREATER:
		case SEMICOLON:
		case COLON:
		case AND:
		case OR:
		case NOT:
			return 1;

		case REM:
		case REM2:
		case DIVINT:
		case POW:
		case XOR:
		case EQUALS_STRICT:
		case GREATER_STRICT:
		case LESS_STRICT:
			return 2;
      
		case LESSEQUALS_STRICT:
		case GREATEREQUALS_STRICT:
			return 3;
      
    case B_ERROR:
	  	return 0;

		default:
			if(token>=ADDRESS && token<(ADDRESS+sizeof(tl)/sizeof(TOKEN_LIST)))
				return tl[token-ADDRESS].length;
			else
		  	basicAssert(0);
		  return 0;
  	}
	}


signed char isString(TOKEN_NUM token) {

  if(token == QUOTE)
    return 1;
  if(token == STRID || token == DIMSTRID )
		return 1;
  return 0;
	}


LNUM_TYPE getValue(const char *str, IDENT_LEN *len) {
  LNUM_TYPE answer;
  char *end;		// no CONST
	
  answer = strtold(str, &end);
//non va  sscanf(str,"%lg",&answer);

  basicAssert(end != str);
  *len = end - str;
  return answer;
	}


void getId(KOMMISSARREXX *mInstance,const char *str, char *out, IDENT_LEN *len) {
  IDENT_LEN nread = 0;

  str=skipSpaces(str);
  basicAssert(isalpha(*str));
  if(isvalidrexx(*str)) {   // forse qua non serve la differenziazione perché arrivo già "sicuro" .. o forse no!
  	out[nread++] = *str++;
    }
  while(isvalidrexx2(*str)) {
		if(nread < IDLENGTH-1)
			out[nread++] = *str++;
		else {
			setError(mInstance,ERR_IDTOOLONG);
			break;
			}
		}
  if(*str == '.') {
		if(nread < IDLENGTH-1)
		  out[nread++] = *str++;
		else
		  setError(mInstance,ERR_IDTOOLONG);
	  }
  out[nread] = 0;
  *len = nread;
	}



static void mystrgrablit(char *dest, const char *src) {
  char delimiter=*src;
  
	basicAssert(*src == '\"' || *src =='\'');
  src++;
  
  while(*src) {
		if(*src == delimiter)	{
	  	if(src[1] == delimiter) {
				*dest++ = *src;
	    	src++;
	    	src++;
	  		}
	  	else
				break;
			}
		else
    	*dest++ = *src++;
  	}

  *dest = 0;
	}



/*
  find where a source string literal ends
  Params: src - string to check (must point to quote)
          quote - character to use for quotation
  Returns: pointer to quote which ends string
  Notes: quotes escape quotes
*/
static const char *mystrend(const char *str, char quote) {
  
	basicAssert(*str == quote);
  str++;

  while(*str) {
    while(*str != quote) {
	  	if(*str == '\n' || *str == 0)
				return 0;
	  	str++;
			}
    if(str[1] == quote)
	  	str += 2;
		else
	  	break;
  	}

  return (char *) (*str ? str : 0);
	}


unsigned int myStrCount(const char *str, char ch) {
  unsigned int answer=0;

  while(*str) {
    if(*str++ == ch)
		  answer++;
  	}

  return answer;
	}

char *myStrConcat(const char *str, const char *cat) {
  int len;
  char *answer;

  len = strlen(str) + strlen(cat);
  answer = (char *)malloc(len + 1);
  if(answer) {
    strcpy(answer, str);
    strcat(answer, cat);
  	}
  return answer;
	}



/******************** PIC32 ?? Specific Functions **********************/

static const char *skipSpaces(const char *str) {

	while(/* isspace*/ *str == ' ' || *str == 9)
		str++;
	return str;
	}


// ----------------------------------------------------
int myTextOut(KOMMISSARREXX *mInstance,const char *s) {

#ifdef USA_BREAKTHROUGH
  HDC myDC,*hDC;
  
    hDC=GetDC(mInstance->hWnd,&myDC);
  SetTextColor(hDC,Color24To565(mInstance->Color));
  SetBkColor(hDC,Color24To565(mInstance->ColorBK));

  TextOut(hDC,mInstance->Cursor.x*hDC->font.size*6,
          mInstance->Cursor.y*hDC->font.size*8,s);
  ReleaseDC(mInstance->hWnd,hDC);
#else
#ifndef USING_SIMULATOR
  SetColors(Color24To565(mInstance->Color),Color24To565(mInstance->ColorBK));
  SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
  
  while(*s) {
    switch(*s) {
      case 8:   // filtro via un po' di cose qua!
      case 10:
      case 13:
        break;
      case 9:   // TAB dovrebbe essere gestito sopra da PRINT
        break;
      default:
        putchar(*s);
        mInstance->Cursor.x++;
        if(mInstance->Cursor.x >= ScreenText.cx) {
          mInstance->Cursor.y++;
          if(mInstance->Cursor.y >= ScreenText.cy) {
// ??    while(isCtrlS());
            mInstance->Cursor.y--;
            }
        break;
        }
      }
    s++;
    }
#endif
#endif
  
  return 1;
  }

int myCR(KOMMISSARREXX *mInstance) {
  
  mInstance->Cursor.x=0;  mInstance->Cursor.y++;
  if(mInstance->Cursor.y >= ScreenText.cy) {
    while(isCtrlS());
    mInstance->Cursor.y--;
    putchar('\n');    // forzo scroll!
    }
#ifndef USING_SIMULATOR
  SetXYText(mInstance->Cursor.x,mInstance->Cursor.y);
#endif
  return 1;
  }

int inkey(void) {
  BYTE keypress[2];
//  extern struct KEYPRESS keypress;

  ReadPMPs(SOUTH_BRIDGE,BIOS_KEYBOARD_READ,keypress,2);
	//qua faccio così... verificare...
  //e i keypressModif
  return keypress[0] /* | keypress[1]*/;
  
//v.      inputKey();
//    i=inputKey();
//    ch=tolower(LOBYTE(i));

  }

char *mystrdup(KOMMISSARREXX *mInstance,const char *s) {
  char *p;
  
  p=strdup(s);
  if(!p)
		setError(mInstance,ERR_OUTOFMEMORY);
    
  return p;
  }

char *mystrdupint(KOMMISSARREXX *mInstance,int16_t t) {
  char *p;
  char buf[8];
  
  itoa(buf,t,10);
  p=strdup(buf);
  if(!p)
		setError(mInstance,ERR_OUTOFMEMORY);
    
  return p;
  }

char *mystrdupnum(KOMMISSARREXX *mInstance,LNUM_TYPE n) {
  char *p;
  char buf[digitSize+1];
  
//  sprintf(buf,"%lf",n);
  ldtoa(n,buf,digitSizeDecimal);
  p=strdup(buf);
  if(!p)
		setError(mInstance,ERR_OUTOFMEMORY);
    
  return p;
  }

BYTE isvalidrexx(char c) {
  
  return isalpha(c) || c=='@' || c=='_' || c=='!' || c=='?' || c=='$';
    // @, _, !, ., ?, and $
  }

BYTE isvalidrexx2(char c) {
  
  return isalnum(c) || c=='@' || c=='_' || c=='!' || c=='?' || c=='$';
    // @, _, !, ., ?, and $
  }


// https://chromium.googlesource.com/external/github.com/kripken/emscripten/+/refs/tags/1.2.9/system/lib/libc/stdlib/strtod.c
// io l'ho espansa a long double...
static int maxExponent = 511;	/* Largest possible base 10 exponent.  Any
				 * exponent larger than this will already
				 * produce underflow or overflow, so there's
				 * no need to worry about additional digits.
				 */
static long double powersOf10[] = {	/* Table giving binary powers of 10.  Entry */
    10.,			/* is 10^2^i.  Used to convert decimal */
    100.,			/* exponents into floating-point numbers. */
    1.0e4,
    1.0e8,
    1.0e16,
    1.0e32,
    1.0e64L,
    1.0e128L,
    1.0e256L,
// no..    1.0e512L
  };

long double strtold(string, endPtr)
    const char *string;		/* A decimal ASCII floating-point number,
				 * optionally preceded by white space.
				 * Must have form "-I.FE-X", where I is the
				 * integer part of the mantissa, F is the
				 * fractional part of the mantissa, and X
				 * is the exponent.  Either of the signs
				 * may be "+", "-", or omitted.  Either I
				 * or F may be omitted, or both.  The decimal
				 * point isn't necessary unless F is present.
				 * The "E" may actually be an "e".  E and X
				 * may both be omitted (but not just one).
				 */
    char **endPtr;		/* If non-NULL, store terminating character's address here. */
{
  int8_t sign, expSign = FALSE;
  long double fraction, dblExp, *d;
  register const char *p;
  register int c;
  int8_t exp = 0;		/* Exponent read from "EX" field. */
  int8_t fracExp = 0;		/* Exponent that derives from the fractional
       * part.  Under normal circumstatnces, it is
       * the negative of the number of digits in F.
       * However, if I is very long, the last digits
       * of I get dropped (otherwise a long I with a
       * large negative exponent could cause an
       * unnecessary overflow on I alone).  In this
       * case, fracExp is incremented one for each
       * dropped digit. */
  int8_t mantSize;		/* Number of digits in mantissa. */
  int8_t decPt;			/* Number of mantissa digits BEFORE decimal point. */
  const char *pExp;		/* Temporarily holds location of exponent in string. */

  /*
   * Strip off leading blanks and check for a sign.
   */
  p = string;
  while(isspace(*p)) 
    p++;
  if(*p == '-') {
    sign = TRUE;
    p++;
    } 
  else {
    if(*p == '+')
      p++;

    sign = FALSE;
    }
  /*
   * Count the number of digits in the mantissa (including the decimal
   * point), and also locate the decimal point.
   */
  decPt = -1;
  for(mantSize=0; ; mantSize ++)    {
    c = *p;
    if(!isdigit(c)) {
      if((c != '.') || (decPt >= 0)) {
        break;
        }
      decPt = mantSize;
      }
    p++;
    }
  /*
   * Now suck up the digits in the mantissa.  Use two integers to
   * collect 9 digits each (this is faster than using floating-point).
   * If the mantissa has more than 18 digits, ignore the extras, since
   * they can't affect the value anyway.
   * PASSO A 32/16! 27/7/22
   */
  pExp  = p;
  p -= mantSize;
  if(decPt < 0) {
    decPt = mantSize;
    }
  else {
    mantSize -= 1;			/* One of the digits was the point. */
    }
  if(mantSize > 32) {
    fracExp = decPt - 32;
    mantSize = 32;
    }
  else {
    fracExp = decPt - mantSize;
    }
  if(mantSize == 0) {
    fraction = 0.0;
    p = string;
    goto done;
    }
  else {
    long long frac1, frac2;
    frac1 = 0;
    for( ; mantSize > 16; mantSize --)	{
      c = *p++;
      if(c == '.')
        c = *p++;
      frac1 = 10*frac1 + (c - '0');
      }
    frac2 = 0;
    for( ; mantSize > 0; mantSize --) {
      c = *p++;
      if(c == '.')
        c = *p++;
      frac2 = 10*frac2 + (c - '0');
      }
    fraction = (1.0e16L * frac1) + frac2;
    }
  /*
   * Skim off the exponent.
   */
  p = pExp;
  if(toupper(*p) == 'E') {
    p++;
    if(*p == '-') {
      expSign = TRUE;
      p++;
      }
    else {
      if(*p == '+')
        p++;
      expSign = FALSE;
      }
    while(isdigit(*p)) {
      exp = (exp*10) + (*p - '0');
      p++;
      }
    }
  if(expSign)
    exp = fracExp - exp;
  else
    exp = fracExp + exp;
  /*
   * Generate a floating-point number that represents the exponent.
   * Do this by processing the exponent one bit at a time to combine
   * many powers of 2 of 10. Then combine the exponent with the
   * fraction.
   */
  if(exp < 0) {
    expSign = TRUE;
    exp = -exp;
    } 
  else {
    expSign = FALSE;
    }
  if(exp > maxExponent) {
    exp = maxExponent;
// non c'è! che fare?    errno = ERANGE;
    }
  dblExp = 1.0L;
  for(d = powersOf10; exp != 0; exp >>= 1, d += 1) {
    if(exp & 1)
      dblExp *= *d;
    }
  if(expSign)
    fraction /= dblExp;
  else
    fraction *= dblExp;
  
done:
  if(endPtr)
    *endPtr = (char *)p;
  if(sign)
    return -fraction;
  else
    return fraction;
  }


// Reverses a string 'str' of length 'len'
void reverse(char* str, BYTE len) {
  BYTE i=0, j = len - 1;
  char temp;
  
  while(i < j) {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
    }
  }
 
// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
BYTE intToStr(unsigned long long x, char str[], int d) {
  BYTE i = 0;
  
  while (x) {
    str[i++] = (x % 10) + '0';
    x = x / 10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
  while(i < d)
    str[i++] = '0';
 
  reverse(str, i);
  str[i] = '\0';
  return i;
  }
 
// Converts a floating-point/double number to a string.
void ldtoa(long double n, char* res, BYTE afterpoint) {
  // Extract integer part
  unsigned long long ipart = (unsigned long long)n;

  // Extract floating part
  long double fpart = n - (long double )ipart;

  // convert integer part to string
  BYTE i = intToStr(ipart, res, 0);

  // check for display option after point
  if(afterpoint != 0) {
    res[i] = '.'; // add dot

    // Get the value of fraction part upto given no.
    // of points after dot. The third parameter
    // is needed to handle cases like 233.007
    fpart = fpart * powl(10, afterpoint);

    intToStr((unsigned long long)fpart, res + i + 1, afterpoint);
    }
  }

