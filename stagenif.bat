@echo off
setlocal
pushd "%~dp0"
set SRC=%~dp0
set DEST=%~dp0\output\%1

if "%1" == "" (
  echo Give a version ...
  goto exit
)

rm -r -f "%DEST%"

for /L %%i in (5,1,8) do (
    mkdir "%DEST%\max-%%i-plugins-%1\plugcfg" > nul 2<&1
    mkdir "%DEST%\max-%%i-plugins-%1\plugins" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.txt" "%DEST%\max-%%i-plugins-%1\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.ini" "%DEST%\max-%%i-plugins-%1\plugcfg\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.dl*" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\..\contrib\niflib\NifMopp\NifMopp.dll" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\..\contrib\NifMagic\Win32\NifMagic.dll" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1
    REM pushd "%DEST%\max-%%i-plugins-%1\" && ntzip -9 -r "..\max-%%i-plugins-%1.zip" * && popd
)

mkdir "%DEST%\gmax-1.2-plugins-%1\plugcfg" > nul 2<&1
mkdir "%DEST%\gmax-1.2-plugins-%1\plugins" > nul 2<&1
xcopy /D /Y /I "%SRC%\staging\release - gmax\*.txt" "%DEST%\gmax-1.2-plugins-%1\" > nul 2<&1
xcopy /D /Y /I "%SRC%\staging\release - gmax\*.ini" "%DEST%\gmax-1.2-plugins-%1\plugcfg\" > nul 2<&1
xcopy /D /Y /I "%SRC%\staging\release - gmax\*.dlu" "%DEST%\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
xcopy /D /Y /I "%SRC%\staging\release - gmax\*.dll" "%DEST%\gmax-1.2-plugins-%1\" > nul 2<&1
xcopy /D /Y /I "%SRC%\..\contrib\niflib\NifMopp\NifMopp.dll" "%DEST%\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
xcopy /D /Y /I "%SRC%\..\contrib\NifMagic\Win32\NifMagic.dll" "%DEST%\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
REM pushd "%DEST%\gmax-1.2-plugins-%1\" && ntzip -9 -r "..\gmax-1.2-plugins-%1.zip" * && popd

for %%i in (9 2008 2009 2010 2011 2012) do (
    mkdir "%DEST%\max-%%i-plugins-%1\plugcfg" > nul 2<&1
    mkdir "%DEST%\max-%%i-plugins-%1\plugins" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.txt" "%DEST%\max-%%i-plugins-%1\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.ini" "%DEST%\max-%%i-plugins-%1\plugcfg\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\staging\release - max %%i\*.dl*" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\..\contrib\niflib\NifMopp\NifMopp.dll" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "%SRC%\..\contrib\NifMagic\Win32\NifMagic.dll" "%DEST%\max-%%i-plugins-%1\plugins\" > nul 2<&1

    REM mkdir "%DEST%\max-%%i-plugins-%1-x64\plugcfg" > nul 2<&1
    REM mkdir "%DEST%\max-%%i-plugins-%1-x64\plugins" > nul 2<&1
    REM xcopy /D /Y /I "%SRC%\staging\release - max %%i - x64\*.txt" "%DEST%\max-%%i-plugins-%1-x64\" > nul 2<&1
    REM xcopy /D /Y /I "%SRC%\staging\release - max %%i - x64\*.ini" "%DEST%\max-%%i-plugins-%1-x64\plugcfg\" > nul 2<&1
    REM xcopy /D /Y /I "%SRC%\staging\release - max %%i - x64\*.dl*" "%DEST%\max-%%i-plugins-%1-x64\plugins\" > nul 2<&1
    REM xcopy /D /Y /I "%SRC%\..\contrib\NifMagic\x64\NifMagic.dll" "%DEST%\max-%%i-plugins-%1-x64\plugins\" > nul 2<&1
)

pushd "%SRC%"
set VERSION=%1
set GITVERSION=0
for /f "usebackq delims=|" %%i in (`git log -1 "--pretty=format:%%h"`) do set VERSION=%VERSION%.%%i

IF EXIST "c:\Program Files\7-Zip\7z.exe" (
    pushd "%DEST%"
    "c:\Program Files\7-Zip\7z.exe" a -r niftools-max-plugins-%VERSION%.7z *
    popd
)

IF EXIST "c:\Program Files\Inno Setup 5\ISCC.exe" (
    pushd "%SRC%"
    "c:\Program Files\Inno Setup 5\ISCC.exe" /Q /fniftools-max-plugins-%VERSION% "/o%DEST%\" MaxNifTools.iss
    popd
)
popd

:exit
endlocal
popd

REM cd "%DEST%\"
