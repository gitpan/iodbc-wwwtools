
//  iodbc   Copyright  1995 - FFE Software, Inc.
//  Version 0.11 (Beta 2)

#include <windows.h>

#include <sql.h>
#include <sqlext.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

typedef struct DESCRIBE_ // ODBC column information
{
  void *pData ;       // pointer to allocated data storage, passed to ODBC
  SWORD sqlType ;     // ODBC SQL datatype for column
  int width ;         // display width of column
  BOOL leftJustify ;  // TRUE if column is left justified else right justified
  SDWORD cbValue ;    // column length/status value from ODBC
} DESCRIBE ;

typedef struct ODBC_  // ODBC control block
{
   UCHAR *pUID ;   // /U command line argument - user id
   UCHAR *pPWD ;   // /P command line argument - user password
   UCHAR *pDB ;    // /d command line argument - USE database
   UCHAR *pDSN ;   // /S command line argument - data source name (servername)
   UCHAR *pWSID ;  // /H command line argument - workstation name
   UCHAR colSeparator[2] ; // column separator string for formatted report (/s)
   long colWidth ;     // screen width for formatted report (/w)
   long connTimeout ;  // connect timeout (/l)
   long queryTimeout ; // statement timeout (/t)
   BOOL setQueryTimeout ; // SQLSetStmtOption(SQL_QUERY_TIMEOUT) supported?
   DESCRIBE *pCol ;    // column descriptions
   SWORD colMax ;      // # of columns
   UCHAR *pBuffer ;    // buffer for data print (size = colWidth + 2)
   HENV hEnv ;         // odbc environment handle
   HDBC hDbc ;         // odbc connection handle
   BOOL firstConnect ; // no previous connections on this DSN
   BOOL driverConnect ; // SQLDriverConnect supported?
   BOOL moreResults ;  // SQLMoreResults supported?
   BOOL setConnectOption ; // SQLSetConnectOption supported?
   BOOL setStmtOption ; // SQLSetStmtOption supported?
   long errLevel ;     // error level to print (/m)
   int exitCode ;      // process exit code
} ODBC ;

// values for ODBC::exitCode
// isql returns
#define EXIT_PRIORERROR -100  // Error occurred prior to exit/quit command

typedef struct IO_  // text input/output block
{
   FILE *pFileIn ;    // input (command) file
   FILE *pFileOut ;   // output file
   UCHAR *pLine ;     // line buffer (size is maxLine)
   UCHAR *pCurrent ;  // current buffer scan pointer
   UCHAR *pBegin ;    // ODBC save pointer in buffer
   UCHAR *pODBC ;     // saved text for ODBC
   UCHAR *pDisplay ;  // saved text for display
   int maxLine ;      // max line size ;
   BOOL skipText ;    // skipping over comments
   BOOL saveLines ;   // saving SQL lines for echo
   BOOL echoLines ;   // echo SQL lines
   BOOL promptLines ; // print prompt
   int lineNo ;       // command line #
} IO ;

typedef struct TOGGLE_  // toggle information
{
   UCHAR *pQuery ;    // query from /Q or /q
   BOOL exitQuery ;   // query and exit (/Q)
} TOGGLE ;

typedef struct TOKEN_  // token parsing block
{
   UCHAR *pText ;     // pointer to beginning of token
   SWORD lenText ;    // length of token
   UWORD type ;       // token type
   BOOL beginLine ;   // token first in line ?
} TOKEN ;

// values for TOKEN::type
#define TOKENSYMBOL 1  // ODBC symbol
#define TOKENINTEGER 2 // integer value
#define TOKENQUOTED 3  // quoted string
#define TOKENQUOTER 4  // illegal quoted string
#define TOKENDELIM 5   // delimiter

typedef struct LOOKUP_  // look-up table entry
{
   UCHAR *pText ;  // match string for token
   int type ;      // string id or type
   UCHAR *pHelp ;  // help display string
} LOOKUP ;

typedef struct ODBCERROR_ // ODBC Error Block
{
   RETCODE ret ;
   UCHAR state[SQL_SQLSTATE_SIZE + 1] ;
   UCHAR msg[SQL_MAX_MESSAGE_LENGTH] ;
   SDWORD nativeEr ;
   SWORD msgSz ;
} ODBCERROR ;

// extend allocated string with characters + separator string
BOOL StringAttach(UCHAR **, UCHAR *, SWORD, UCHAR *) ;

// scan over spaces (whitespace)
UCHAR *SkipSpaces(UCHAR *) ;

// output fatal error - allocation
void AllocError(FILE *) ;

// start/end skip text for ODBC
BOOL ODBCSkip(IO *, UCHAR *) ;
// save non-comment text for ODBC
BOOL ODBCSave(IO *, UCHAR *) ;
// read command line
BOOL InputLine(IO *) ;

// test token for value
BOOL TokenTest(TOKEN *, UCHAR *) ;
// look-up token in table
LOOKUP *TokenLookup(TOKEN *, LOOKUP *) ;
int TokenFind(TOKEN *, LOOKUP *) ;

// parse input line for token (also skip over comments)
BOOL InputToken(IO *, TOKEN *) ;

// free saved areas
void FreeSQL(IO *) ;
// execute SQL command
void ExecSQL(ODBC *, IO *) ;
// process input commands
int ProcessInput(ODBC *, IO *) ;
// find specific toggle
UCHAR **ToggleFind(UCHAR **, UCHAR) ;
// dump toggle error
void ToggleError(FILE *, UCHAR *) ;
// collect toggle argument
UCHAR *ToggleArg(UCHAR ***) ;
// collect toggle argument - long
BOOL ToggleLong(UCHAR ***, long *) ;
// process toggles
BOOL ProcessToggles(ODBC *, IO *, TOGGLE *, UCHAR **) ;

// print query results
SDWORD PrintQuery(FILE *, ODBC *, HSTMT, SWORD) ;
// format and print data for ODBC fetch
void PrintData(FILE *, ODBC *) ;

// set ODBC connect or stmt option
BOOL SetODBCOption(FILE *, ODBC *, HSTMT, UWORD, UDWORD, BOOL *) ;
// call SQLGetFunctions to test for existence of a function
BOOL TestODBCFunction(ODBC *, UWORD) ;
// login to ODBC data source
BOOL ODBCConnect(FILE *, ODBC *, UCHAR *) ;
// startup ODBC
BOOL ODBCStartup(FILE *, ODBC *) ;
// shutdown ODBC
BOOL ODBCShutdown(FILE *, ODBC *) ;
// dump all ODBC errors
BOOL ODBCError(FILE *, ODBC *, HSTMT, RETCODE) ;
// fetch ODBC error
BOOL ODBCErrorFetch(FILE *, ODBC *, HSTMT, RETCODE, ODBCERROR *) ;
// dump ODBC error
void ODBCErrorPrint(FILE *, ODBC *, ODBCERROR *) ;

void ProcessLines(ODBC *, IO *) ;

int MainProcess(FILE *, FILE *, UCHAR **) ;
