@echo off

SET DOC_ROOT=Documentation
SET LOG_FILE=%DOC_ROOT%\Doxygen_log.txt 

REM run Doxygen
Doxygen > %LOG_FILE% 2>&1

REM display the log file
explorer.exe %LOG_FILE%

REM display the documentation
explorer.exe "%DOC_ROOT%\html\index.html"
