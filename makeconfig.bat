@echo off
rem Quick script to create config.h
IF EXIST "%PROGRAMFILES%\TortoiseSVN\bin\SubWCRev.exe" (
    "%PROGRAMFILES%\TortoiseSVN\bin\SubWCRev.exe" . NifCommon\config.h.in NifCommon\config.h > nul 2<&1
) else (
    copy NifCommon\config.h.in NifCommon\config.h
)
