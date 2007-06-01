; -- NWN2Utils.iss --
;

[Setup]
AppName=NIF Utilities for 3ds Max
AppVerName=NIF Utilities {code:CurVer} for 3ds Max
OutputBaseFilename=niftools-max-plugins-0.2.11
DisableProgramGroupPage=yes
Compression=lzma
SolidCompression=yes
DirExistsWarning=no
EnableDirDoesntExistWarning=yes
UsePreviousAppDir=yes
DefaultDirName={win}{\}Installer\NifTools
UninstallFilesDir={win}{\}Installer\NifTools
Uninstallable=yes
DisableDirPage=yes

SourceDir=.
;UninstallDisplayIcon={app}{\}..\Oblivion.exe

[Types]
;Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "gmax12"; Description: "gmax 1.2"; Types: custom;
;Name: "max4"; Description: "3ds Max 4"; Types: custom;
Name: "max42"; Description: "3ds Max 4.2"; Types: custom;
Name: "max5"; Description: "3ds Max 5"; Types: custom;
Name: "max6"; Description: "3ds Max 6"; Types: custom;
Name: "max7"; Description: "3ds Max 7"; Types: custom;
Name: "max8"; Description: "3ds Max 8"; Types: custom;
Name: "max9"; Description: "3ds Max 9 (Win32)"; Types: custom;
Name: "max9x64"; Description: "3ds Max 9 (x64)"; Types: custom;
;Name: "src"; Description: "Program Source";

[Files]
Source: "Staging\Release - gmax\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|gmax12}"; Components: "gmax12"; Flags: isreadme ignoreversion;
Source: "Staging\Release - gmax\nifgmax.exe"; DestDir: "{code:InstallPath|gmax12}"; Components: "gmax12"; Flags: ignoreversion;
Source: "Staging\Release - gmax\NifPlugins.dlu"; DestDir: "{code:InstallPath|gmax12}{\}plugins"; Components: "gmax12"; Flags: ignoreversion;
Source: "Staging\Release - gmax\MaxNifTools.ini"; DestDir: "{code:InstallPath|gmax12}{\}plugcfg"; Components: "gmax12"; Flags: ignoreversion;

;Source: "Staging\Release - Max 4\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max4}"; Components: "max4"; Flags: isreadme ignoreversion;
;Source: "Staging\Release - Max 4\NifPlugins.dlu"; DestDir: "{code:InstallPath|max4}{\}plugins"; Components: "max4"; Flags: ignoreversion;
;Source: "Staging\Release - Max 4\MaxNifTools.ini"; DestDir: "{code:InstallPath|max4}{\}plugcfg"; Components: "max4"; Flags: ignoreversion;

Source: "Staging\Release - Max 4.2\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max42}"; Components: "max42"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 4.2\NifPlugins.dlu"; DestDir: "{code:InstallPath|max42}{\}plugins"; Components: "max42"; Flags: ignoreversion;
Source: "Staging\Release - Max 4.2\MaxNifTools.ini"; DestDir: "{code:InstallPath|max42}{\}plugcfg"; Components: "max42"; Flags: ignoreversion;

Source: "Staging\Release - Max 5\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max5}"; Components: "max5"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 5\NifPlugins.dlu"; DestDir: "{code:InstallPath|max5}{\}plugins"; Components: "max5"; Flags: ignoreversion;
Source: "Staging\Release - Max 5\MaxNifTools.ini"; DestDir: "{code:InstallPath|max5}{\}plugcfg"; Components: "max5"; Flags: ignoreversion;

Source: "Staging\Release - Max 6\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max6}"; Components: "max6"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 6\NifPlugins.dlu"; DestDir: "{code:InstallPath|max6}{\}plugins"; Components: "max6"; Flags: ignoreversion;
Source: "Staging\Release - Max 6\MaxNifTools.ini"; DestDir: "{code:InstallPath|max6}{\}plugcfg"; Components: "max6"; Flags: ignoreversion;

Source: "Staging\Release - Max 7\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max7}"; Components: "max7"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 7\NifPlugins.dlu"; DestDir: "{code:InstallPath|max7}{\}plugins"; Components: "max7"; Flags: ignoreversion;
Source: "Staging\Release - Max 7\MaxNifTools.ini"; DestDir: "{code:InstallPath|max7}{\}plugcfg"; Components: "max7"; Flags: ignoreversion;

Source: "Staging\Release - Max 8\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max8}"; Components: "max8"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 8\NifPlugins.dlu"; DestDir: "{code:InstallPath|max8}{\}plugins"; Components: "max8"; Flags: ignoreversion;
Source: "Staging\Release - Max 8\MaxNifTools.ini"; DestDir: "{code:InstallPath|max8}{\}plugcfg"; Components: "max8"; Flags: ignoreversion;

Source: "Staging\Release - Max 9\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max9}"; Components: "max9"; Flags: isreadme ignoreversion;
Source: "Staging\Release - Max 9\NifPlugins.dlu"; DestDir: "{code:InstallPath|max9}{\}plugins"; Components: "max9"; Flags: ignoreversion;
Source: "Staging\Release - Max 9\MaxNifTools.ini"; DestDir: "{code:InstallPath|max9}{\}plugcfg"; Components: "max9"; Flags: ignoreversion;
Source: "Staging\Release - Max 9\MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\9 - 32bit\enu\plugcfg"; Components: "max9"; Flags: ignoreversion;

Source: "Staging\x64\Release - Max 9\MaxNifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max9x64}"; Components: "max9x64"; Flags: isreadme ignoreversion;
Source: "Staging\x64\Release - Max 9\NifPlugins.dlu"; DestDir: "{code:InstallPath|max9x64}{\}plugins"; Components: "max9x64"; Flags: ignoreversion;
Source: "Staging\x64\Release - Max 9\MaxNifTools.ini"; DestDir: "{code:InstallPath|max9x64}{\}plugcfg"; Components: "max9x64"; Flags: ignoreversion;
Source: "Staging\x64\Release - Max 9\MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\9 - 64bit\enu\plugcfg"; Components: "max9x64"; Flags: ignoreversion;

;Source: "src\*"; DestDir: "{reg:HKLM\SOFTWARE\Bethesda Softworks\Oblivion,Path|{pf}{\}Bethesda Softworks\Oblivion}\Data\niftools";  Components: "src"; Flags: ignoreversion recursesubdirs;

[Code]
var sVersion: String;
    i: Integer;

function InitializeSetup(): Boolean;
begin
  sVersion := '0.2.11';
  Result := True;
end;

function InstallPath(Param: String): String;
  var
    Names: TArrayOfString;
    I: Integer;
begin
  Result := '';
  case Param of
    'gmax12':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\gmax', 'uninstallpath', Result) then
          Result := ExpandConstant('{sd}{\}gmax12');
    'max4':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 4', 'uninstallpath', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax4');
    'max42':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 4', 'uninstallpath', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax42');
    'max5':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 5', 'uninstallpath', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax5');
    'max6':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\6.0', 'InstallDir', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax6');
    'max7':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\7.0', 'InstallDir', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax7');
    'max8':
        if not RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\8.0', 'InstallDir', Result) then
          Result := ExpandConstant('{sd}{\}3dsmax8');
    'max9': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0\' + Names[I], 'InstallDir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf}{\}AutoDesk\3ds Max 9}{\}plugcfg');
        end;
    'max9x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf}{\}AutoDesk\3ds Max 9}{\}plugcfg');
        end;
    else
      Result := '';
  end;
end;

function RegInstallPath(Param: String): String;
  var
    Names: TArrayOfString;
    I: Integer;
begin
  Result := '';
  case Param of
    'gmax 1.2':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\gmax', 'uninstallpath', Result);
    '3ds Max 4':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 4', 'uninstallpath', Result);
    '3ds Max 4.2':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 4', 'uninstallpath', Result);
    '3ds Max 5':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\3ds max 5', 'uninstallpath', Result);
    '3ds Max 6':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\6.0', 'InstallDir', Result);
    '3ds Max 7':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\7.0', 'InstallDir', Result);
    '3ds Max 8':
        RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\8.0', 'InstallDir', Result);
    '3ds Max 9 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 9 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    else
      Result := '';
  end;
end;

function IsInstalled(Param: String): Boolean;
begin
  Result := Length(RegInstallPath(Param)) <> 0;
  if Result then begin
    if IsWin64() and (Param <> '3ds Max 9 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 9 (x64)') then
       Result := False;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageId = wpSelectComponents then
    for i := 0 to WizardForm.ComponentsList.Items.Count - 1 do
       WizardForm.ComponentsList.Checked[i] := IsInstalled(WizardForm.ComponentsList.ItemCaption[i]);
end;

function CurVer(Param: String): String;
begin
  Result := sVersion;
end;
