@echo off
setlocal
pushd "%~dp0"

IF EXIST "setbuildenv.bat" call setbuildenv.bat
call "%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" amd64

start devenv.exe

popd
endlocal
