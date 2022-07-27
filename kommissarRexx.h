#ifndef _KOMMISSARREXX_H
#define _KOMMISSARREXX_H

/*
  kommissarRexx header file
  By G.Dar 2022
	(based upon Minibasic by Malcolm Mclean)

*/


#include <stdio.h>
#include "../../generictypedefs.h"
#include "../../Compiler.h"

#include "../pc_pic_cpu.h"
#include "../breakthrough.h"
#include "interpreter_includes.h"


//#define REGINA_REXX 1

//--------------- Version -----------------------------------------
typedef long double LNUM_TYPE;


typedef struct __attribute((packed)) _KOMMISSARREXX {
  //OCCHIO ALLINEAMENTI!
  LINE_NUMBER_TYPE gosubStack[MAXGOSUB];		// GOSUB stack
  FORLOOP doStack[MAXFORS];		// DO stack - qua usiamo FOR :)
  uint8_t ngosubs;
  uint8_t ndos;
  uint8_t inBlock;

  FILE_DESCR openFiles[MAXFILES];		// open files descriptors

  uint8_t filler;

  VARIABLESTRING *variables;			// the script's variables
  DIMVARSTRING *dimVariables;		// dimensioned arrays
  LINE *lines;					// list of line starts

  uint16_t nvariables;				// number of variables
  uint16_t ndimVariables;			// number of dimensioned arrays
  uint16_t nlines;					// number of BASIC lines in program
  int16_t curline;      // usa -1 come marker di fine...

  
  const char *string;        // string we are parsing

  HWND hWnd;
  THREAD *threadID;

  enum INTERPRETER_ERRORS errorFlag;           // set when error in input encountered
  uint8_t nfiles;
  uint8_t incomingChar[2];
  
  struct __attribute((packed)) EVENT_HANDLER errorHandler;
  uint8_t filler2;
  
  COLORREF Color,ColorBK;
  POINT Cursor;
  GFX_COLOR ColorPalette,ColorPaletteBK;

  TOKEN_NUM token;          // current token (lookahead)
  
  uint8_t filler3;
  
  } KOMMISSARREXX;
STATIC_ASSERT(!(sizeof(struct _KOMMISSARREXX) % 4),0);

int rexx(KOMMISSARREXX *instance,const char *script,const char *args,BYTE where);

#endif


