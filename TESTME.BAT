@echo off
set QUERY_STRING="DSN=IODBC+SAMPLE&SQL=SELECT+*+FROM+person"
set REQUEST_METHOD=GET
perl \http\iodbc\sql.pl | more
