
//  iodbc   Copyright  1995 - FFE Software, Inc.
//  Version 0.11 (Beta 2)

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, Version 2.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

//#define CONSOLE

#include "iodbc.h"

// command types
#define CMDGO 1
#define CMDEXIT 2
#define CMDRESET 3
#define CMDQUIT 4
#define CMDHELP 5

static LOOKUP cmdLookup[] =
{
   { "go", CMDGO, "go - execute sql command" },
   { "exit", CMDEXIT, "exit - exit iodbc" },
   { "reset", CMDRESET, "reset - clear sql command" },
   { "quit", CMDQUIT, "quit - quit iodbc" },
   { "help", CMDHELP, "help [cmd] - show command help; help ? for conditions" },
   { NULL, 0 }
} ;

#ifdef CONSOLE
int main(int argc, char **argv)
{
   int exitCode ;
   exitCode = MainProcess(stdin, stdout, (UCHAR **) argv + 1) ;
   return exitCode ;
}
#else
// Windows support
int PASCAL WinMain(HANDLE hcurins, HANDLE hprevins, LPSTR cmdline,
                   int cmdshow)
{
  //static UCHAR *arg[] = { "/U", "demo", "/S", "firstsql", "/e", "/n", NULL } ;
  static UCHAR *arg[] = { "/Udemo", "/Sfirstsql", "/e", "/n", "/s|", "/w119",
                          NULL } ;
  int exitCode ;
  FILE *pFileIn, *pFileOut ;
  exitCode = 1 ;
  if (pFileOut = fopen("iodbc.out", "wt"))
  {
     if (pFileIn = fopen("iodbc.inp", "rt"))
     {
        exitCode = MainProcess(pFileIn, pFileOut, arg) ;
        fclose(pFileIn) ;
     }
     else
        fprintf(pFileOut, "unable to open 'iodbc.inp'\n") ;
     fclose(pFileOut) ;
  }
  return exitCode ;
}
#endif

int MainProcess(FILE *pFileIn, FILE *pFileOut, UCHAR **pArgV)
{
   static IO io ;
   static TOGGLE toggle ;
   static UCHAR buf[1024] ;
   static ODBC odbc ;
   io.pFileIn = pFileIn ;
   io.pFileOut = pFileOut ;
   strcpy(odbc.colSeparator, " ") ;
   odbc.colWidth = 80l ;
   odbc.connTimeout = odbc.queryTimeout = 8l ;
   odbc.firstConnect = TRUE ;
   odbc.errLevel = 16 ;
   io.pLine = buf ;
   io.maxLine = 1024 ;
   io.promptLines = TRUE ;
   io.echoLines = FALSE ;
   // got toggle to turn off logon message?
   if (!ToggleFind(pArgV, 'z'))
      fprintf(pFileOut,
       "iodbc  Ver. 0.11 (Beta 2)  Copyright 1995 FFE Software, Inc.\n"
       "This is free software, and you are welcome to redistribute it under certain\n"
       "conditions; type 'help ?' for details.\n") ;
   odbc.exitCode = EXIT_PRIORERROR ;
   if (ProcessToggles(&odbc, &io, &toggle, pArgV) &&
       ODBCStartup(io.pFileOut, &odbc))
   {
      odbc.pBuffer = malloc(odbc.colWidth + 2) ;
      if (odbc.pBuffer)
      {
         // if no query
         {
            io.saveLines = FALSE ;
            io.skipText = TRUE ;
            InputLine(&io) ;
            io.skipText = FALSE ;
         }
         io.saveLines = io.echoLines ;
         odbc.exitCode = 0 ;
         ProcessLines(&odbc, &io) ;
      }
      else
         AllocError(io.pFileOut) ;
      ODBCShutdown(io.pFileOut, &odbc) ;
   }
   return odbc.exitCode ;
}

// login to ODBC data source
BOOL ODBCConnect(FILE *pOut, ODBC *pODBC, UCHAR *pDb)
{
   UCHAR *pConn, connStr[128], *pDSN ;
   SWORD connOut ;
   RETCODE ret ;
   BOOL retFlag ;
   ODBCERROR errODBC ;
   pDSN = pODBC->pDSN ? pODBC->pDSN : "default" ;
   if (pODBC->driverConnect || pODBC->firstConnect)
   {
      pConn = connStr ;
      pConn += sprintf(pConn, "DSN=%s", pDSN) ;
      if (pODBC->pUID)
         pConn += sprintf(pConn, ";UID=%s", pODBC->pUID) ;
      //if (pODBC->pPWD)
         pConn += sprintf(pConn, ";PWD=%s", pODBC->pPWD ? pODBC->pPWD : "") ;
      if (pDb)
         pConn += sprintf(pConn, ";DATABASE=%s", pDb) ;
      if (pODBC->pWSID)
         pConn += sprintf(pConn, ";WSID=%s", pODBC->pWSID) ;
      ret = SQLDriverConnect(pODBC->hDbc, NULL, connStr, SQL_NTS, connStr,
                             128, &connOut, SQL_DRIVER_NOPROMPT) ;
      retFlag = ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ;
      if (retFlag)
      {
         pODBC->firstConnect = FALSE ;
         pODBC->driverConnect = TRUE ;
      }
      else
         while (ODBCErrorFetch(pOut, pODBC, SQL_NULL_HSTMT, ret, &errODBC))
            if (strcmp(errODBC.state, "IM001"))  // SQLDriverConnect support?
               ODBCErrorPrint(pOut, pODBC, &errODBC) ;
            else
               pODBC->driverConnect = FALSE ;
   }
   else
      retFlag = FALSE ;
   if (!retFlag)
   {
      ret = SQLConnect(pODBC->hDbc, pDSN, SQL_NTS, pODBC->pUID,
                       (SWORD) (pODBC->pUID ? SQL_NTS : 0), pODBC->pPWD,
                       (SWORD) (pODBC->pPWD ? SQL_NTS : 0)) ;
      retFlag = ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ;
      if (retFlag)
         pODBC->firstConnect = pODBC->driverConnect = FALSE ;
      else
         ODBCError(pOut, pODBC, SQL_NULL_HSTMT, ret) ;
   }
   if (retFlag)
   {
      pODBC->moreResults = TestODBCFunction(pODBC, SQL_API_SQLMORERESULTS) ;
      pODBC->setConnectOption =
                        TestODBCFunction(pODBC, SQL_API_SQLSETCONNECTOPTION) ;
      pODBC->setStmtOption =
                           TestODBCFunction(pODBC, SQL_API_SQLSETSTMTOPTION) ;
   }
   return retFlag ;
}

// startup ODBC
BOOL ODBCStartup(FILE *pOut, ODBC *pODBC)
{
   BOOL retFlag, dummy ;
   retFlag = FALSE ;
   if (SQLAllocEnv(&pODBC->hEnv) == SQL_SUCCESS)
   {
      if (SQLAllocConnect(pODBC->hEnv, &pODBC->hDbc) == SQL_SUCCESS)
      {
         pODBC->setConnectOption = TRUE ;
         SetODBCOption(pOut, pODBC, SQL_NULL_HSTMT, SQL_LOGIN_TIMEOUT,
                       pODBC->connTimeout, &dummy) ;
         retFlag = ODBCConnect(pOut, pODBC, pODBC->pDB) ;
         if (!retFlag)
            SQLFreeConnect(pODBC->hDbc) ;
      }
      else
         AllocError(pOut) ;
      if (!retFlag)
         SQLFreeEnv(pODBC->hEnv) ;
   }
   else
      AllocError(pOut) ;
   return retFlag ;
}

// shutdown ODBC
BOOL ODBCShutdown(FILE *pOut, ODBC *pODBC)
{
   SQLDisconnect(pODBC->hDbc) ;
   SQLFreeConnect(pODBC->hDbc) ;
   SQLFreeEnv(pODBC->hEnv) ;
   return TRUE ;
}

// set ODBC connect or stmt option
BOOL SetODBCOption(FILE *pOut, ODBC *pODBC, HSTMT hStmt, UWORD fOption,
                   UDWORD vParam, BOOL *pOption)
{
   RETCODE ret ;
   ODBCERROR errODBC ;
   if (hStmt)
   {
      if (!pODBC->setStmtOption)
         return *pOption = FALSE ;
      ret = SQLSetStmtOption(hStmt, fOption, vParam) ;
   }
   else
   {
      if (!pODBC->setConnectOption)
         return *pOption = FALSE ;
      ret = SQLSetConnectOption(pODBC->hDbc, fOption, vParam) ;
   }
   *pOption = TRUE ;
   while (ODBCErrorFetch(pOut, pODBC, hStmt, ret, &errODBC))
      if (strcmp(errODBC.state, "IM001") && strcmp(errODBC.state, "S1092") &&
          strcmp(errODBC.state, "S1C00"))
      {
         if (ret != SQL_SUCCESS_WITH_INFO)
            *pOption = FALSE ;
         ODBCErrorPrint(pOut, pODBC, &errODBC) ;
      }
      else
         *pOption = FALSE ;
   return *pOption ;
}

// call SQLGetFunctions to test for existence of a function
BOOL TestODBCFunction(ODBC *pODBC, UWORD fFunction)
{
   UWORD exists ;
   return SQLGetFunctions(pODBC->hDbc, fFunction, &exists) == SQL_SUCCESS &&
          exists == 1 ;
}

// dump ODBC error
BOOL ODBCError(FILE *pOut, ODBC *pODBC, HSTMT hStmt, RETCODE ret)
{
   BOOL retFlag ;
   ODBCERROR errODBC ;
   retFlag = FALSE ;
   while (ODBCErrorFetch(pOut, pODBC, hStmt, ret, &errODBC))
   {
       retFlag = TRUE ;
       ODBCErrorPrint(pOut, pODBC, &errODBC) ;
   }
   return retFlag ;
}

BOOL ODBCErrorFetch(FILE *pOut, ODBC *pODBC, HSTMT hStmt, RETCODE ret,
                    ODBCERROR *pErr)
{
   BOOL retFlag ;
   pErr->ret = ret ;
   retFlag = FALSE ;
   switch (ret)
   {
      case SQL_SUCCESS :
      case SQL_NO_DATA_FOUND :
         break ;
      case SQL_INVALID_HANDLE :
         strcpy(pErr->state, "******") ;
         strcpy(pErr->msg, "Unknown Error") ;
         pErr->nativeEr = 0 ;
         ODBCErrorPrint(pOut, pODBC, pErr) ;
         break ;
      default :
         ret = SQLError(pODBC->hEnv, pODBC->hDbc, hStmt, pErr->state,
                        &pErr->nativeEr, pErr->msg,
                        SQL_MAX_MESSAGE_LENGTH - 1, &pErr->msgSz) ;
         switch (ret)
         {
            case SQL_SUCCESS :
            case SQL_SUCCESS_WITH_INFO :
               retFlag = TRUE ;
               break ;
            case SQL_ERROR :
               pErr->ret = ret ;
               strcpy(pErr->state, "00000") ;
               strcpy(pErr->msg, "ODBC Failure") ;
               pErr->nativeEr = 0 ;
               ODBCErrorPrint(pOut, pODBC, pErr) ;
               break ;
            case SQL_NO_DATA_FOUND :
               break ;
            default :
               pErr->ret = SQL_ERROR ;
               strcpy(pErr->state, "00000") ;
               strcpy(pErr->msg, "Unknown Error") ;
               pErr->nativeEr = 0 ;
               ODBCErrorPrint(pOut, pODBC, pErr) ;
               break ;
         }
         break ;
   }
   return retFlag ;
}

// dump ODBC error
void ODBCErrorPrint(FILE *pOut, ODBC *pODBC, ODBCERROR *pErr)
{
   long errLevel ;
   switch (pErr->ret)
   {
      case SQL_SUCCESS :
      case SQL_NO_DATA_FOUND :
         errLevel = 0 ;
         break ;
      case SQL_SUCCESS_WITH_INFO :
         errLevel = 1 ;
         break ;
      default :
         pODBC->exitCode = EXIT_PRIORERROR ;
         errLevel = 16 ;
         break ;
   }
   if (errLevel >= pODBC->errLevel)
      fprintf(pOut, "Msg %ld, Level %ld, State %s:\n%s\n",
              pErr->nativeEr, errLevel, pErr->state, pErr->msg) ;
}

void ExecSQL(ODBC *pODBC, IO *pIo)
{
   HSTMT hStmt ;
   BOOL success, dummy ;
   SWORD numCols ;
   SDWORD numRows ;
   RETCODE ret ;
   if (pIo->pODBC && *SkipSpaces(pIo->pODBC))
   {
      success = FALSE ;
      if (SQLAllocStmt(pODBC->hDbc, &hStmt) == SQL_SUCCESS)
      {
         SetODBCOption(pIo->pFileOut, pODBC, hStmt, SQL_QUERY_TIMEOUT,
                       pODBC->queryTimeout, &dummy) ;
         ret = SQLExecDirect(hStmt, pIo->pODBC, SQL_NTS) ;
         if (ret == SQL_SUCCESS)
            success = TRUE ;
         else
            ODBCError(pIo->pFileOut, pODBC, hStmt, ret) ;
      }
      else
      {
         hStmt = SQL_NULL_HSTMT ;
         AllocError(pIo->pFileOut) ;
      }
      if (pIo->echoLines)
         fprintf(pIo->pFileOut, pIo->pDisplay) ;
      while (success)
      {
         // if query, fetch results
         if (SQLNumResultCols(hStmt, &numCols) == SQL_SUCCESS && numCols)
            numRows = PrintQuery(pIo->pFileOut, pODBC, hStmt, numCols) ;
         else // else get number of rows affected
         {
            numRows = 0l ;
            SQLRowCount(hStmt, &numRows) ;
         }
         fprintf(pIo->pFileOut, "\n(%ld rows affected)\n", numRows) ;
         if (pODBC->moreResults)
         {
            ret = SQLMoreResults(hStmt) ;
            success = ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO ;
            ODBCError(pIo->pFileOut, pODBC, hStmt, ret) ;
         }
         else
           success = FALSE ;
      }
      if (hStmt != SQL_NULL_HSTMT)
         SQLFreeStmt(hStmt, SQL_DROP) ;
   }
   else
      fprintf(pIo->pFileOut, "empty sql\n") ;
   FreeSQL(pIo) ;
}

// print query results
SDWORD PrintQuery(FILE *pOut, ODBC *pODBC, HSTMT hStmt, SWORD numCols)
{
   long colDisp ;
   SDWORD numRows ;
   UCHAR *pHeading, *pMarker, colName[32], *pText ;
   UDWORD prec ;
   SWORD i, len, scale, nullable, type ;
   int width, lenText ;
   BOOL leftJustify ;
   RETCODE ret ;
   numRows = 0l ;
   pODBC->colMax = numCols ;
   pODBC->pCol = malloc(numCols * sizeof (DESCRIBE)) ;
   if (pODBC->pCol)
   {
      // build heading & markers
      pHeading = pMarker = NULL ;
      StringAttach(&pHeading, NULL, 0, pODBC->colSeparator) ;
      StringAttach(&pMarker, NULL, 0, pODBC->colSeparator) ;
      colDisp = 1 ;
      for (i = 0 ; i < numCols ; i++)
      {
         pText = NULL ;
         leftJustify = FALSE ;
         ret = SQLDescribeCol(hStmt, (UWORD) (i + 1), colName, 32, &len, 
                              &pODBC->pCol[i].sqlType, &prec, &scale,
                              &nullable) ;
         if (ret == SQL_SUCCESS)
         {
            switch (pODBC->pCol[i].sqlType)
            {
               case SQL_INTEGER :
               case SQL_TINYINT :
               case SQL_SMALLINT :
                  width = prec + 1 ;
                  lenText = sizeof (long) ;
                  type = SQL_C_LONG ;
                  break ;
               case SQL_NUMERIC :
               case SQL_DECIMAL :
                  width = prec + 2 ;
                  lenText = width + 1 ;
                  type = SQL_C_CHAR ;
                  break ;
               case SQL_FLOAT :
               case SQL_REAL :
               case SQL_DOUBLE :
                  width = 20 ;
                  lenText = sizeof (double) ;
                  type = SQL_C_DOUBLE ;
                  break ;
               case SQL_DATE :
                  width = 12 ;
                  lenText = sizeof (DATE_STRUCT) ;
                  type = SQL_C_DATE ;
                  break ;
               case SQL_TIME :
                  width = 8 ;
                  lenText = sizeof (TIME_STRUCT) ;
                  type = SQL_C_TIME ;
                  break ;
               case SQL_TIMESTAMP :
                  width = 20 ;
                  lenText = sizeof (TIMESTAMP_STRUCT) ;
                  type = SQL_C_TIMESTAMP ;
                  break ;
               case SQL_BINARY :
               case SQL_VARBINARY :
               case SQL_LONGVARBINARY :
                  width = prec * 2 ;
                  lenText = width + 1 ;
                  type = SQL_C_CHAR ;
                  break ;
               case SQL_BIT :
                  width = 1 ;
                  lenText = width + 1 ;
                  type = SQL_C_BIT ;
                  break ;
               case SQL_BIGINT :
                  width = prec + 1 ;
                  lenText = width + 1 ;
                  type = SQL_C_CHAR ;
                  leftJustify = TRUE ;
                  break ;
               case SQL_CHAR :
               case SQL_VARCHAR :
               case SQL_LONGVARCHAR :
               default :
                  width = prec ;
                  lenText = width + 1 ;
                  type = SQL_C_CHAR ;
                  leftJustify = TRUE ;
                  break ;
            }
            pText = malloc(lenText) ;
            SQLBindCol(hStmt, (UWORD) (i + 1), type, (PTR) pText,
                       (SWORD) lenText, &pODBC->pCol[i].cbValue) ;
            if (width < (int) strlen(colName))
               width = strlen(colName) ;
            if (width >= pODBC->colWidth)
               width = pODBC->colWidth - 1 ;
            sprintf(pODBC->pBuffer, "%-*.*s", width, width, colName) ;
            if (width + 1 + colDisp > pODBC->colWidth)
            {
               StringAttach(&pHeading, NULL, 0, "\n\t") ;
               StringAttach(&pMarker, NULL, 0, "\n\t") ;
               colDisp = 8 ;
            }
            StringAttach(&pHeading, pODBC->pBuffer, (SWORD) width,
                         pODBC->colSeparator) ;
            memset(pODBC->pBuffer, '-', width) ;
            StringAttach(&pMarker, pODBC->pBuffer, (SWORD) width,
                         pODBC->colSeparator) ;
            colDisp += width + 1 ;
         }
         else
         {
            ODBCError(pOut, pODBC, hStmt, ret) ;
            width = 0 ;
            pODBC->pCol[i].sqlType = 0 ;
         }
         pODBC->pCol[i].pData = pText ;
         pODBC->pCol[i].width = width ;
         pODBC->pCol[i].leftJustify = leftJustify ;
      }
      StringAttach(&pHeading, NULL, 0, "\n") ;
      StringAttach(&pMarker, NULL, 0, "\n") ;
      // fetch and print data
      if (pHeading)
         fprintf(pOut, pHeading) ;
      if (pMarker)
         fprintf(pOut, pMarker) ;
      while ((ret = SQLFetch(hStmt)) == SQL_SUCCESS ||
             ret == SQL_SUCCESS_WITH_INFO)
      {
         ODBCError(pOut, pODBC, hStmt, ret) ;
         PrintData(pOut, pODBC) ;
         ++numRows ;
      }
      if (ret != SQL_NO_DATA_FOUND)
         ODBCError(pOut, pODBC, hStmt, ret) ;
      for (i = 0 ; i < numCols ; i++)
         free(pODBC->pCol[i].pData) ;
      free(pHeading) ;
      free(pMarker) ;
      free(pODBC->pCol) ;
   }
   return numRows ;
}

// format and print data for ODBC fetch
void PrintData(FILE *pOut, ODBC *pODBC)
{
   SWORD i ;
   long colDisp ;
   UCHAR *pText, work[128] ;
   BOOL leftJustify ;
   DATE_STRUCT *pDate ;
   TIME_STRUCT *pTime ;
   TIMESTAMP_STRUCT *pTimestamp ;
   struct tm tm ;
   fprintf(pOut, pODBC->colSeparator) ;
   for (colDisp = 1l, i = 0 ; i < pODBC->colMax ; ++i)
   {
      if (pODBC->pCol[i].width + 1 + colDisp > pODBC->colWidth)
      {
         fprintf(pOut, "\n\t") ;
         colDisp = 8 ;
      }
      if (pODBC->pCol[i].sqlType && pODBC->pCol[i].cbValue == SQL_NULL_DATA)
      {
         leftJustify = TRUE ;
         pText = "NULL" ;
      }
      else
      {
         leftJustify = pODBC->pCol[i].leftJustify ;
         switch (pODBC->pCol[i].sqlType)
         {
            case 0 :
                continue ;
            case SQL_INTEGER :
            case SQL_TINYINT :
            case SQL_SMALLINT :
               sprintf(pText = work, "%ld", *((long *) pODBC->pCol[i].pData)) ;
               break ;
            case SQL_FLOAT :
            case SQL_REAL :
            case SQL_DOUBLE :
               sprintf(pText = work, "%.6f",
                       *((double *) pODBC->pCol[i].pData)) ;
               break ;
            case SQL_DATE :
               pDate = (DATE_STRUCT *) pODBC->pCol[i].pData ;
               tm.tm_year = (pDate->year > 1900) ?
                            pDate->year - 1900 : pDate->year ;
               tm.tm_mon = pDate->month - 1 ;
               tm.tm_mday = pDate->day ;
               strftime(pText = work, 127, "%b %d %Y", &tm) ;
               break ;
            case SQL_TIME :
               pTime = (TIME_STRUCT *) pODBC->pCol[i].pData ;
               tm.tm_hour = pTime->hour ;
               tm.tm_min = pTime->minute ;
               tm.tm_sec = pTime->second ;
               strftime(pText = work, 127, "%I:%M%p", &tm) ;
               break ;
            case SQL_TIMESTAMP :
               pTimestamp = (TIMESTAMP_STRUCT *) pODBC->pCol[i].pData ;
               tm.tm_hour = pTimestamp->hour ;
               tm.tm_min = pTimestamp->minute ;
               tm.tm_sec = pTimestamp->second ;
               tm.tm_year = (pTimestamp->year > 1900) ?
                            pTimestamp->year - 1900 : pTimestamp->year ;
               tm.tm_mon = pTimestamp->month - 1 ;
               tm.tm_mday = pTimestamp->day ;
               strftime(pText = work, 127, "%b %d %Y %I:%M%p", &tm) ;
               break ;
            case SQL_BIT :
               sprintf(pText = work, "%d",
                       *((UCHAR *) pODBC->pCol[i].pData) != 0) ;
               break ;
            case SQL_BINARY :
            case SQL_VARBINARY :
            case SQL_LONGVARBINARY :
            case SQL_BIGINT :
            case SQL_CHAR :
            case SQL_VARCHAR :
            case SQL_LONGVARCHAR :
            case SQL_NUMERIC :
            case SQL_DECIMAL :
            default :
               pText = pODBC->pCol[i].pData ;
               break ;
         }
      }
      fprintf(pOut, leftJustify ? "%-*.*s%s" : "%*.*s%s",
              pODBC->pCol[i].width, pODBC->pCol[i].width, pText,
              pODBC->colSeparator) ;
      colDisp += pODBC->pCol[i].width + 1 ;
   }
   fprintf(pOut, "\n") ;
}

void AllocError(FILE *pOut)
{
   fprintf(pOut, "Msg 0, Level 256, State S1000:\nAllocation Error\n") ;
}

void FreeSQL(IO *pIo)
{
   if (pIo->pODBC)
   {
      free(pIo->pODBC) ;
      pIo->pODBC = NULL ;
   }
   if (pIo->pDisplay)
   {
      free(pIo->pDisplay) ;
      pIo->pDisplay = NULL ;
   }
}

// process toggles
BOOL ProcessToggles(ODBC *pODBC, IO *pIo, TOGGLE *pToggle, UCHAR **pArgV)
{
   BOOL retFlag ;
   UCHAR *pArg, *pValue ;
   long value ;
   retFlag = TRUE ;
   while (retFlag && (pArg = *pArgV++))
      switch (*pArg)
      {
         case '/' :
         case '-' :
            // toggle
            switch (*(pArg + 1))
            {
               case 'd' :  // USE database (special for SQL SERVER)
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     pODBC->pDB = pValue ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'H' :  // workstation name (special for SQL SERVER)
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     pODBC->pWSID = pValue ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'U' :  // user id (UID)
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     pODBC->pUID = pValue ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'P' :  // user password (PWD)
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     pODBC->pPWD = pValue ;
                  break ;
               case 'S' :  // data source name (servername) (DSN)
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     pODBC->pDSN = pValue ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'e' :  // echo SQL input
                  pIo->echoLines = TRUE ;
                  break ;
               case 'n' :  // suppress prompt
                  pIo->promptLines = FALSE ;
                  break ;
               case 's' : // column separator on formatted report
                  pValue = ToggleArg(&pArgV) ;
                  if (pValue)
                     *pODBC->colSeparator = *pValue ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'z' : // turn off log-in message
                  break ; // already processed
               case 'm' : // error level
                  if (ToggleLong(&pArgV, &value))
                     pODBC->errLevel = value ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 't' : // query timeout
                  if (ToggleLong(&pArgV, &value))
                     pODBC->queryTimeout = value ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'l' : // login timeout
                  if (ToggleLong(&pArgV, &value))
                     pODBC->connTimeout = value ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case 'w' : // column width
                  if (ToggleLong(&pArgV, &value) && value > 8)
                     pODBC->colWidth = value ;
                  else
                     ToggleError(pIo->pFileOut, pArg) ;
                  break ;
               case '?' : // dump toggles
                  fprintf(pIo->pFileOut, 
                        "usage: iodbc [-U login id] [-e echo input]\n"
                        "\t[-n remove numbering]\n"
                        "\t[-w columnwidth] [-s colseparator]\n"
                        "\t[-m errorlevel] [-t query timeout] [-l login timeout]\n"
                        "\t[-H hostname] [-P password]\n"
                        "\t[-S data source name] [-d use database name]\n"
                        "\t[-w column width] [-t command timeout]\n"
                        "\t[-z suppress log-on message]\n"
                        "\t[-? show syntax summary (this screen)]\n") ;
                  retFlag = FALSE ;
                  break ;
               // isql toggles not supported
               case 'L' : // later
               case 'p' :
                  fprintf(pIo->pFileOut,
                          "Unsupported command line toggle : %s (ignored)\n",
                          pArg) ;
                  break ;
               case 'q' : // to do
               case 'Q' : // to do
               case 'h' : // later
               case 'r' : // later
               case 'i' : // later
               case 'o' : // later
               case 'a' : // later
               case 'c' :
                  fprintf(pIo->pFileOut,
                          "Unsupported command line toggle : %s (ignored)\n",
                          pArg) ;
                  ToggleArg(&pArgV) ;
                  break ;
               default :
                  fprintf(pIo->pFileOut,
                          "Fatal - Unrecognized command line toggle : %s\n",
                          pArg) ;
                  retFlag = FALSE ;
                  break ;
            }
            break ;
         case '\0' :
            break ;
         default :
            fprintf(pIo->pFileOut,
                    "Fatal - Invalid command line argument : %s\n",
                    pArg) ;
            retFlag = FALSE ;
            break ;
      }
   return retFlag ;
}

// find specific toggle
// return NULL if not found
UCHAR **ToggleFind(UCHAR **pArgV, UCHAR toggleChar)
{
   while (*pArgV &&
          ((**pArgV != '/' && **pArgV != '-') || *(*pArgV + 1) != toggleChar))
      ++pArgV ;
   return *pArgV ? pArgV + 1 : NULL ;
}

// collect toggle argument
// return NULL if none
UCHAR *ToggleArg(UCHAR ***ppArgV)
{
   UCHAR *pArg ;
   // examine toggle arg itself
   if (*(*(*ppArgV - 1) + 2)) // argument adjacent
      pArg = *(*ppArgV - 1) + 2 ;
   else
   {
      pArg = **ppArgV ;
      if (pArg)
      {
         // is argument a toggle?
         if (*pArg == '-' || *pArg == '/')
            pArg = NULL ;
         else
            (*ppArgV)++ ;
      }
   }
   return pArg ;
}

// collect toggle argument - long
BOOL ToggleLong(UCHAR ***ppArgV, long *pValue)
{
   long value ;
   UCHAR *pArg, *pEnd ;
   pArg = ToggleArg(ppArgV) ;
   if (pArg)
   {
      value = strtol((char *) pArg, (char **) &pEnd, 10) ;
      if (errno == ERANGE || *SkipSpaces(pEnd))
         pArg = NULL ;
      else
         *pValue = value ;
   }
   return pArg != NULL ;
}

// dump toggle error
void ToggleError(FILE *pOut, UCHAR *pArg)
{
   fprintf(pOut, "Invalid value for toggle : %s (ignored)\n", pArg) ;
}

// process input commands
void ProcessLines(ODBC *pODBC, IO *pIo)
{
   TOKEN token ;
   BOOL process ;
   int cmdId ;
   LOOKUP *pLook ;
   process = TRUE ;
   while (process && InputToken(pIo, &token))
      if (token.type == TOKENSYMBOL && token.beginLine &&
          (cmdId = TokenFind(&token, cmdLookup)))
      {
         pIo->lineNo = 0 ;
         switch (cmdId)
         {
            case CMDEXIT :
               process = FALSE ;
               break ;
            case CMDQUIT :
               process = FALSE ;
               break ;
            case CMDRESET :
               process = InputLine(pIo) ;
               FreeSQL(pIo) ;
               break ;
            case CMDGO :
               pIo->saveLines = FALSE ;
               ODBCSkip(pIo, pIo->pBegin) ;
               ExecSQL(pODBC, pIo) ;
               process = InputLine(pIo) ;
               ODBCSkip(pIo, NULL) ;
               break ;
            case CMDHELP :
               pIo->saveLines = FALSE ;
               ODBCSkip(pIo, pIo->pBegin) ;
               if (*(pIo->pCurrent = SkipSpaces(pIo->pCurrent)))
               {
                  InputToken(pIo, &token) ;
                  pLook = TokenLookup(&token, cmdLookup) ;
                  if (pLook->type)
                     fprintf(pIo->pFileOut, "%s\n", pLook->pHelp) ;
                  else
                     if (TokenTest(&token, "?"))
                     {
                        fprintf(pIo->pFileOut,
      "This program is free software; you can redistribute it and/or modify\n"
      "it under the terms of the GNU General Public License as published by\n"
      "the Free Software Foundation, Version 2.\n\n"
      "This program is distributed in the hope that it will be useful,\n"
      "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "GNU General Public License for more details.\n\n"
      "You should have received a copy of the GNU General Public License\n"
      "along with this program; if not, write to the Free Software\n"
      "Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n\n"
      "Contact FFE Software at P.O. Box 1519, El Cerrito, CA 94530, USA\n"
      "        Phone: (510) 232-6800 Fax: (510) 237-7433 "
      "Internet: iodbc@firstsql.com\n") ;
                     }
                     else
                        fprintf(pIo->pFileOut, "***unrecognized command\n") ;
               }
               else
               {
                  fprintf(pIo->pFileOut,
"iodbc - Interactive ODBC Application (type SQL commands; then GO to execute)\n"
                          "  help ?   -  redistribution conditions\n"
                          "  help cmd - help on specific command\n"
                          "  commands -") ;
                  for (cmdId = 0 ; cmdLookup[cmdId].pText ; ++cmdId)
                     fprintf(pIo->pFileOut, " %s", cmdLookup[cmdId].pText) ;
                  fprintf(pIo->pFileOut, "\n") ;
               }
               process = InputLine(pIo) ;
               ODBCSkip(pIo, NULL) ;
               break ;
         }
      }
   FreeSQL(pIo) ;
}


// test token for value
BOOL TokenTest(TOKEN *pToken, UCHAR *pTest)
{
   return !memicmp(pTest, pToken->pText, pToken->lenText) &&
          pToken->lenText == (int) strlen(pTest) ;
}

// look-up token in table
LOOKUP *TokenLookup(TOKEN *pToken, LOOKUP *pLook)
{
   while (pLook->pText && !TokenTest(pToken, pLook->pText))
      ++pLook ;
   return pLook ;
}

int TokenFind(TOKEN *pToken, LOOKUP *pLook)
{
   return TokenLookup(pToken, pLook)->type ;
}


// parse input line for token (also skip over comments)
BOOL InputToken(IO *pIo, TOKEN *pToken)
{
   int c ;
   SWORD depth ;
   BOOL retFlag ;
   retFlag = TRUE ;
   pToken->beginLine = pIo->pCurrent == pIo->pLine ;
   // slide over any whitespace
   while (retFlag && !*(pIo->pCurrent = SkipSpaces(pIo->pCurrent)))
   {
      pToken->beginLine = TRUE ;
      retFlag = InputLine(pIo) ;
   }
   if (retFlag)
   {
      pToken->pText = pIo->pCurrent ;
      c = *pIo->pCurrent ;
      if (c == '_' || isalpha(c))
      {
         pToken->type = TOKENSYMBOL ;
         do
            c = *++pIo->pCurrent ;
         while (c == '_' || isalnum(c)) ;
      }
      else
         if (isdigit(c))
         {
            pToken->type = TOKENINTEGER ;
            do
               c = *++pIo->pCurrent ;
            while (isdigit(c)) ;
         }
         else
            switch (c)
            {
               case '\'' :
                  do
                     ++pIo->pCurrent ;
                  while ((pIo->pCurrent = strchr(pIo->pCurrent, '\'')) &&
                         *++pIo->pCurrent == '\'') ;
                  if (pIo->pCurrent)
                     pToken->type = TOKENQUOTED ;
                  else
                  {
                     pToken->type = TOKENQUOTER ;
                     pIo->pCurrent = pIo->pLine + strlen(pIo->pLine) ;
                  }
                  break ;
               case '/' :
                  if (*++pIo->pCurrent == '*')
                  {  // scan over nested comments
                     // save previous and start skipping
                     retFlag = ODBCSkip(pIo, pIo->pCurrent - 1) ;
                     ++pIo->pCurrent ;
                     pToken->beginLine = FALSE ;
                     for (depth = 1 ; retFlag && depth ;)
                        switch (*pIo->pCurrent)
                        {
                           case '/' :
                              if (*++pIo->pCurrent == '*')
                              {
                                 ++depth ;
                                 ++pIo->pCurrent ;
                              }
                              break ;
                           case '*' :
                              if (*++pIo->pCurrent == '/')
                              {
                                 --depth ;
                                 ++pIo->pCurrent ;
                              }
                              break ;
                           case '\0' :
                              retFlag = InputLine(pIo) ;
                              break ;
                           default :
                              ++pIo->pCurrent ;
                              break ;
                        }
                     ODBCSkip(pIo, NULL) ; // turn off text skipping
                     if (retFlag)
                        retFlag = InputToken(pIo, pToken) ;
                  }
                  else
                     pToken->type = TOKENDELIM ;
                  break ;
               default :
                  pToken->type = TOKENDELIM ;
                  ++pIo->pCurrent ;
                  break ;
            }
      if (retFlag)
         pToken->lenText = pIo->pCurrent - pToken->pText ;
   }
   return retFlag ;
}

// read command line
BOOL InputLine(IO *pIo)
{
   BOOL retFlag ;
   SWORD sizeInput ;
   // save previous line if needed
   retFlag = (!pIo->saveLines ||
        StringAttach(&pIo->pDisplay, pIo->pLine,
                     (SWORD) strlen(pIo->pLine), "\n")) &&
         ODBCSave(pIo, pIo->pCurrent) ;
   if (retFlag)
   {
      if (pIo->promptLines)
         fprintf(pIo->pFileOut, "%d> ", ++pIo->lineNo) ;
      retFlag = fgets(pIo->pLine, pIo->maxLine, pIo->pFileIn) != NULL ;
      if (retFlag)
      {
         pIo->saveLines = pIo->echoLines ;
         sizeInput = strlen(pIo->pLine) ;
         pIo->pBegin = pIo->pCurrent = pIo->pLine ;
         // remove terminating \n if any
         if (sizeInput && *(pIo->pLine + sizeInput - 1) == '\n')
            *(pIo->pLine + sizeInput - 1) = '\0' ;
      }
   }
   else
      AllocError(pIo->pFileOut) ;
   return retFlag ;
}

// start/end skip text for ODBC
BOOL ODBCSkip(IO *pIo, UCHAR *pCurrent)
{
   BOOL retFlag ;
   if (pCurrent)
   {  // start skipping, save previous
      retFlag = pCurrent == pIo->pBegin || ODBCSave(pIo, pCurrent) ;
      pIo->skipText = TRUE ;
      if (!retFlag)
         AllocError(pIo->pFileOut) ;
   }
   else
   {
      // end skipping
      retFlag = TRUE ;
      if (pIo->skipText)
      {
         pIo->pBegin = pIo->pCurrent ;
         pIo->skipText = FALSE ;
      }
   }
   return retFlag ;
}

// save non-comment text for ODBC
BOOL ODBCSave(IO *pIo, UCHAR *pCurrent)
{
   return pIo->skipText ||
         StringAttach(&pIo->pODBC, pIo->pBegin,
                      (SWORD) (pCurrent - pIo->pBegin), " ") ;
}

// scan over spaces (whitespace)
UCHAR *SkipSpaces(UCHAR *pScan)
{
   while (isspace(*pScan))
      ++pScan ;
   return pScan ;
}


// extend allocated string with characters + separator string
BOOL StringAttach(UCHAR **ppStr, UCHAR *pStr, SWORD len, UCHAR *pSeparator)
{
   SWORD sizeOld, sizeSeparator ;
   sizeSeparator = strlen(pSeparator) ;
   if (*ppStr)
   {
      sizeOld = strlen(*ppStr) ;
      *ppStr = realloc(*ppStr, sizeOld + len + sizeSeparator + 1) ;
   }
   else
   {
      sizeOld = 0 ;
      *ppStr = malloc(len + sizeSeparator + 1) ;
   }
   if (*ppStr)
   {
      memcpy(*ppStr + sizeOld, pStr, len) ;
      strcpy(*ppStr + sizeOld + len, pSeparator) ;
   }
   return *ppStr != NULL ;
}
