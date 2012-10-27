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

for %%j in (Win32) do (
    for /L %%i in (5,1,8) do (
        mkdir "%DEST%\%%j\max-%%i-plugins-%1\plugcfg" > nul 2<&1
        mkdir "%DEST%\%%j\max-%%i-plugins-%1\plugins" > nul 2<&1
        xcopy /D /Y /I "%SRC%*.txt" "%DEST%\%%j\max-%%i-plugins-%1\" > nul 2<&1
        xcopy /D /Y /I "%SRC%MaxNifTools.ini" "%DEST%\%%j\max-%%i-plugins-%1\plugcfg\" > nul 2<&1
        xcopy /D /Y /I "%SRC%staging\%%j\release - max %%i\NifPlugins\*.dl*" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
        xcopy /D /Y /I "%SRC%..\NifMopp\NifMopp.dll" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
        xcopy /D /Y /I "%SRC%..\NifMagic\%%j\NifMagic.dll" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
        REM pushd "%DEST%\max-%%i-plugins-%1\" && ntzip -9 -r "..\max-%%i-plugins-%1.zip" * && popd
    )
    
    mkdir "%DEST%\%%j\gmax-1.2-plugins-%1\plugcfg" > nul 2<&1
    mkdir "%DEST%\%%j\gmax-1.2-plugins-%1\plugins" > nul 2<&1
    xcopy /D /Y /I "%SRC%README.txt" "%DEST%\%%j\gmax-1.2-plugins-%1\" > nul 2<&1
    xcopy /D /Y /I "%SRC%MaxNifTools.ini" "%DEST%\%%j\gmax-1.2-plugins-%1\plugcfg\" > nul 2<&1
    xcopy /D /Y /I "%SRC%staging\%%j\release - gmax\NifPlugins\*.dl*" "%DEST%\%%j\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "\gmax12\winmm.dll" "%DEST%\%%j\gmax-1.2-plugins-%1\" > nul 2<&1
    xcopy /D /Y /I "%SRC%..\NifMopp\%%j\NifMopp.dll" "%DEST%\%%j\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
    xcopy /D /Y /I "%SRC%..\NifMagic\%%j\NifMagic.dll" "%DEST%\%%j\gmax-1.2-plugins-%1\plugins\" > nul 2<&1
    REM pushd "%DEST%\gmax-1.2-plugins-%1\" && ntzip -9 -r "..\gmax-1.2-plugins-%1.zip" * && popd
)

for %%j in (Win32 x64) do (
    for %%i in (9 2008 2009 2010 2011 2012) do (
        mkdir "%DEST%\%%j\max-%%i-plugins-%1\plugcfg" > nul 2<&1
        mkdir "%DEST%\%%j\max-%%i-plugins-%1\plugins" > nul 2<&1
        xcopy /D /Y /I "%SRC%README.txt" "%DEST%\%%j\max-%%i-plugins-%1\" > nul 2<&1
        xcopy /D /Y /I "%SRC%MaxNifTools.ini" "%DEST%\%%j\max-%%i-plugins-%1\plugcfg\" > nul 2<&1
        xcopy /D /Y /I "%SRC%staging\%%j\release - max %%i\NifPlugins\*.dl*" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
        xcopy /D /Y /I "%SRC%..\NifMopp\%%j\NifMopp.dll" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
        xcopy /D /Y /I "%SRC%..\NifMagic\%%j\NifMagic.dll" "%DEST%\%%j\max-%%i-plugins-%1\plugins\" > nul 2<&1
    )
)
pushd "%DEST%"
rm -r -f NifPlugins_Development_Readme.txt
popd

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
