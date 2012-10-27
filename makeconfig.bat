@echo off
REM Quick script to create config.h
setlocal

set WCREV=0
set WCDATE=
set WCURL=

for /f "usebackq delims=|" %%i in (`git log -1 "--pretty=format:%%h"`) do set WCREV=%%i
for /f "usebackq delims=|" %%i in (`git log -1 "--pretty=format:%%cd"`) do set WCDATE=%%i
for /f "usebackq delims=|" %%i in (`git config --get remote.origin.url`) do set WCURL=%%i
sed "s/[$]WCREV[$]/%WCREV%/g" < NifCommon/config.h.in | sed "s/[$]WCDATE[$]/%WCDATE%/g" | sed "s![$]WCURL[$]!%WCURL%!g" > NifCommon/config.h

endlocal