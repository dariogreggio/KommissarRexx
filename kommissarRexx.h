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


typedef long double LNUM_TYPE;
typedef union __attribute((packed)) {
  LNUM_TYPE dval;			  /* its value if a real */
  char    *sval;				/* its value if a string (malloc'ed) */
  int16_t  ival;	        /* its value if an int */
	} LVARIABLEDATA;

typedef struct __attribute((packed)) _LVARIABLE {
  LVARIABLEDATA d;               /* data in variable */
  char id[IDLENGTH];				/* id of variable */
//#warning fatto IDLEN a 31 e type uint8 !
  enum DATATYPE  /*ALIGNMENT in alloc dinamica! */   type;		    /* its type, STRID or FLTID or INTID */
	// unsigned int * fixed_address??
	} LVARIABLE;
STATIC_ASSERT(!(sizeof(LVARIABLE) % 4),0);

typedef struct __attribute((packed)) _VARIABLESTRING {
  char *sval;                   /* its value if a string (malloc'ed) */
  void *parent;             // ci metto il numero di linea della proc padre...
  char id[IDLENGTH];				/* id of variable */
  uint8_t filler;           // alignment..
	} VARIABLESTRING;
    
typedef union __attribute((packed)) {
  char        **str;	        /* pointer to string data */
  LNUM_TYPE     *dval;	        /* pointer to real data */
  int16_t      *ival;	        /* pointer to int data */
	} LDIMVARPTR;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of dimensioned variable */
  enum DATATYPE type;					/* its type, STRID or FLTID (or INTID)*/
  uint32_t /*uint8_t  ALIGNMENT in alloc dinamica! */  ndims;			/* number of dimensions */
  DIM_SIZE dim[MAXDIMS];			/* dimensions in x y order */
  LDIMVARPTR     d;              /* pointers to string/real data */
	} LDIMVAR;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of dimensioned variable */
  uint8_t  ndims;			/* number of dimensions */
  void *parent;             // ci metto il numero di linea della proc padre...
  DIM_SIZE dim[MAXDIMS];			/* dimensions in x y order */
  LDIMVARPTR     d;              /* pointers to string/real data */
  char **str;                   /* its value if a string (malloc'ed) */
	} LDIMVARSTRING;

typedef union __attribute((packed)) {
  char        **sval;			/* pointer to string data */
  LNUM_TYPE     *dval;		    /* pointer to real data */
  int16_t      *ival;	        /* pointer to int data */
	} LLVALUEDATA;

typedef struct __attribute((packed)) {
  LLVALUEDATA    d;              /* data pointed to by LVALUE */
  uint8_t type;			/* type of variable (STRID or FLTID or INTID or B_ERROR) */   
	} LLVALUE;

typedef struct __attribute((packed)) {
  char id[IDLENGTH];			/* id of control variable */
  uint8_t parentBlock;
  LINE_NUMBER_TYPE nextline;	/* line below DO to which control passes */
  LNUM_TYPE toval;			/* terminal value */
  LNUM_TYPE step;			/* step size */
  const char *expr;
	} BLOCK_DESCRIPTOR;

typedef struct __attribute((packed)) {
  const char *args;
  LINE_NUMBER_TYPE line;	/* line usato come identificatore di subroutine/funzione */
  LINE_NUMBER_TYPE returnline;	/* line after CALL/() */
  uint8_t privateVars;          // 1 se PROCEDURE (e poi serve lista di EXPOSEd...)
	} PROC_DESCRIPTOR;

typedef struct __attribute((packed)) _STACK_QUEUE {
  const char *string;
  struct _STACK_QUEUE *next;
  } STACK_QUEUE;

typedef struct __attribute((packed)) _KOMMISSARREXX {
  //OCCHIO ALLINEAMENTI!
  PROC_DESCRIPTOR gosubStack[MAXGOSUB];		// GOSUB stack
  BLOCK_DESCRIPTOR doStack[MAXFORS];		// DO stack - qua usiamo FOR :)
  uint8_t ngosubs;
  uint8_t ndos;
  uint8_t inBlock;

  FILE_DESCR openFiles[MAXFILES];		// open files descriptors

  uint8_t filler;

  VARIABLESTRING *variables;			// the script's variables
  LDIMVARSTRING *dimVariables;		// dimensioned arrays
  LINE *lines;					// list of line starts

  uint16_t nvariables;				// number of variables
  uint16_t ndimVariables;			// number of dimensioned arrays
  uint16_t nlines;					// number of BASIC lines in program
  int16_t curline;      // usa -1 come marker di fine...

  
  const char *string;        // string we are parsing
  STACK_QUEUE *stack;

  HWND hWnd;
  THREAD *threadID;

  enum INTERPRETER_ERRORS errorFlag;           // set when error in input encountered
  uint8_t nfiles;
  uint8_t incomingChar[2];
  
  struct __attribute((packed)) EVENT_HANDLER errorHandler;
  uint8_t traceLevel;
  
  COLORREF Color,ColorBK;
  POINT Cursor;
  GFX_COLOR ColorPalette,ColorPaletteBK;

  TOKEN_NUM token;          // current token (lookahead)
  
  uint8_t filler3;
  
  } KOMMISSARREXX;
STATIC_ASSERT(!(sizeof(struct _KOMMISSARREXX) % 4),0);

int rexx(KOMMISSARREXX *instance,const char *script,const char *args,BYTE where);

#endif


