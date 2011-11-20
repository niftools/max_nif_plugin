; -- MaxNifUtils.iss --
;

[Setup]
AppName=NIF Utilities for 3ds Max
AppVerName=NIF Utilities {code:GetParam|fullver} for 3ds Max
AppPublisher=NIF File Format Library and Tools
AppCopyright=Copyright © 2011, NIF File Format Library and Tools
OutputBaseFilename=niftools-max-plugins
DisableProgramGroupPage=yes
Compression=lzma2/ultra64
SolidCompression=yes
DirExistsWarning=no
EnableDirDoesntExistWarning=yes
UsePreviousAppDir=yes
DefaultDirName={win}{\}Installer\NifTools
UninstallFilesDir={win}{\}Installer\NifTools
Uninstallable=yes
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
SourceDir=.
OutputDir=.\Output
;UninstallDisplayIcon={app}{\}..\Oblivion.exe

[Types]
;Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "gmax12"; Description: "gmax 1.2"; Types: custom;
;Name: "max4"; Description: "3ds Max 4"; Types: custom;
;Name: "max42"; Description: "3ds Max 4.2"; Types: custom;
Name: "max5"; Description: "3ds Max 5"; Types: custom;
Name: "max6"; Description: "3ds Max 6"; Types: custom;
Name: "max7"; Description: "3ds Max 7"; Types: custom;
Name: "max8"; Description: "3ds Max 8"; Types: custom;
Name: "max9"; Description: "3ds Max 9 (Win32)"; Types: custom;
Name: "max9x64"; Description: "3ds Max 9 (x64)"; Types: custom;
Name: "max10"; Description: "3ds Max 2008 (Win32)"; Types: custom;
Name: "max10x64"; Description: "3ds Max 2008 (x64)"; Types: custom;
Name: "max11"; Description: "3ds Max 2009 (Win32)"; Types: custom;
Name: "max11x64"; Description: "3ds Max 2009 (x64)"; Types: custom;
Name: "max12"; Description: "3ds Max 2010 (Win32)"; Types: custom;
Name: "max12x64"; Description: "3ds Max 2010 (x64)"; Types: custom;
Name: "max13"; Description: "3ds Max 2011 (Win32)"; Types: custom;
Name: "max13x64"; Description: "3ds Max 2011 (x64)"; Types: custom;
Name: "max14"; Description: "3ds Max 2012 (Win32)"; Types: custom;
Name: "max14x64"; Description: "3ds Max 2012 (x64)"; Types: custom;
;Name: "src"; Description: "Program Source";

[Files]
Source: "License.txt"; DestDir: "{app}"; Flags: isreadme ignoreversion;
Source: "ChangeLog.txt"; DestDir: "{app}"; Flags: isreadme ignoreversion;
Source: "Readme.txt"; DestDir: "{app}"; Flags: isreadme ignoreversion;

;Source: "Staging\Release - gmax\Readme.txt"; DestName: "NifPlugins_Readme.txt"; DestDir: "{code:InstallPath|gmax12}"; Components: "gmax12"; Flags: isreadme ignoreversion;
Source: "Staging\Release - gmax\winmm.dll"; DestDir: "{code:InstallPath|gmax12}"; Components: "gmax12"; Flags: ignoreversion;
Source: "Staging\Release - gmax\NifPlugins.dlu"; DestDir: "{code:InstallPath|gmax12}{\}plugins"; Components: "gmax12"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|gmax12}{\}plugcfg"; Components: "gmax12"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|gmax12}{\}plugins"; Components: "gmax12"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|gmax12}{\}plugins"; Components: "gmax12"; Flags: ignoreversion;

;Source: "Staging\Release - Max 4\Readme.txt"; DestName: "NifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max4}"; Components: "max4"; Flags: isreadme ignoreversion;
;Source: "Staging\Release - Max 4\NifPlugins.dlu"; DestDir: "{code:InstallPath|max4}{\}plugins"; Components: "max4"; Flags: ignoreversion;
;Source: "Staging\Release - Max 4\MaxNifTools.ini"; DestDir: "{code:InstallPath|max4}{\}plugcfg"; Components: "max4"; Flags: ignoreversion;

;Source: "Staging\Release - Max 4.2\Readme.txt"; DestName: "NifPlugins_Readme.txt"; DestDir: "{code:InstallPath|max42}"; Components: "max42"; Flags: isreadme ignoreversion;
;Source: "Staging\Release - Max 4.2\NifPlugins.dlu"; DestDir: "{code:InstallPath|max42}{\}plugins"; Components: "max42"; Flags: ignoreversion;
;Source: "Staging\Release - Max 4.2\MaxNifTools.ini"; DestDir: "{code:InstallPath|max42}{\}plugcfg"; Components: "max42"; Flags: ignoreversion;
;Source: "..\contrib\niflib\NifMopp\NifMopp.dll" DestDir: "{code:InstallPath|max42}{\}plugins"; Components: "gmax12"; Flags: ignoreversion;

Source: "Staging\Release - Max 5\NifPlugins.dlu"; DestDir: "{code:InstallPath|max5}{\}plugins"; Components: "max5"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max5}{\}plugcfg"; Components: "max5"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max5}{\}plugins"; Components: "max5"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max5}{\}plugins"; Components: "max5"; Flags: ignoreversion;

Source: "Staging\Release - Max 6\NifPlugins.dlu"; DestDir: "{code:InstallPath|max6}{\}plugins"; Components: "max6"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max6}{\}plugcfg"; Components: "max6"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max6}{\}plugins"; Components: "max6"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max6}{\}plugins"; Components: "max6"; Flags: ignoreversion;

Source: "Staging\Release - Max 7\NifPlugins.dlu"; DestDir: "{code:InstallPath|max7}{\}plugins"; Components: "max7"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max7}{\}plugcfg"; Components: "max7"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max7}{\}plugins"; Components: "max7"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max7}{\}plugins"; Components: "max7"; Flags: ignoreversion;

Source: "Staging\Release - Max 8\NifPlugins.dlu"; DestDir: "{code:InstallPath|max8}{\}plugins"; Components: "max8"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max8}{\}plugcfg"; Components: "max8"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max8}{\}plugins"; Components: "max8"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max8}{\}plugins"; Components: "max8"; Flags: ignoreversion;

Source: "Staging\Release - Max 9\NifPlugins.dlu"; DestDir: "{code:InstallPath|max9}{\}plugins"; Components: "max9"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max9}{\}plugcfg"; Components: "max9"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\9 - 32bit\enu\plugcfg"; Components: "max9"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max9}{\}plugins"; Components: "max9"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max9}{\}plugins"; Components: "max9"; Flags: ignoreversion;

Source: "Staging\Release - Max 9 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max9x64}{\}plugins"; Components: "max9x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max9x64}{\}plugcfg"; Components: "max9x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\9 - 64bit\enu\plugcfg"; Components: "max9x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max9x64}{\}plugins"; Components: "max9x64"; Flags: ignoreversion;

Source: "Staging\Release - Max 2008\NifPlugins.dlu"; DestDir: "{code:InstallPath|max10}{\}plugins"; Components: "max10"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max10}{\}plugcfg"; Components: "max10"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\2008 - 32bit\enu\plugcfg"; Components: "max10"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max10}{\}plugins"; Components: "max10"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max10}{\}plugins"; Components: "max10"; Flags: ignoreversion;

Source: "Staging\Release - Max 2008 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max10x64}{\}plugins"; Components: "max10x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max10x64}{\}plugcfg"; Components: "max10x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\10 - 64bit\enu\plugcfg"; Components: "max10x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max10x64}{\}plugins"; Components: "max10x64"; Flags: ignoreversion;

Source: "Staging\Release - Max 2009\NifPlugins.dlu"; DestDir: "{code:InstallPath|max11}{\}plugins"; Components: "max11"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max11}{\}plugcfg"; Components: "max11"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\2009 - 32bit\enu\plugcfg"; Components: "max11"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max11}{\}plugins"; Components: "max11"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max11}{\}plugins"; Components: "max11"; Flags: ignoreversion;

Source: "Staging\Release - Max 2009 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max11x64}{\}plugins"; Components: "max11x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max11x64}{\}plugcfg"; Components: "max11x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\11 - 64bit\enu\plugcfg"; Components: "max11x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max11x64}{\}plugins"; Components: "max11x64"; Flags: ignoreversion;

Source: "Staging\Release - Max 2010\NifPlugins.dlu"; DestDir: "{code:InstallPath|max12}{\}plugins"; Components: "max12"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max12}{\}plugcfg"; Components: "max12"; Flags: ignoreversion
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\2010 - 32bit\enu\plugcfg"; Components: "max12"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max12}{\}plugins"; Components: "max12"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max12}{\}plugins"; Components: "max12"; Flags: ignoreversion;

Source: "Staging\Release - Max 2010 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max12x64}{\}plugins"; Components: "max12x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max12x64}{\}plugcfg"; Components: "max12x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\12 - 64bit\enu\plugcfg"; Components: "max12x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max12x64}{\}plugins"; Components: "max12x64"; Flags: ignoreversion;

Source: "Staging\Release - Max 2011\NifPlugins.dlu"; DestDir: "{code:InstallPath|max13}{\}plugins"; Components: "max13"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max13}{\}plugcfg"; Components: "max13"; Flags: ignoreversion
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\2011 - 32bit\enu\plugcfg"; Components: "max13"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max13}{\}plugins"; Components: "max13"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max13}{\}plugins"; Components: "max13"; Flags: ignoreversion;

Source: "Staging\Release - Max 2011 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max13x64}{\}plugins"; Components: "max13x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max13x64}{\}plugcfg"; Components: "max13x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\13 - 64bit\enu\plugcfg"; Components: "max13x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max13x64}{\}plugins"; Components: "max13x64"; Flags: ignoreversion;

Source: "Staging\Release - Max 2012\NifPlugins.dlu"; DestDir: "{code:InstallPath|max14}{\}plugins"; Components: "max14"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max14}{\}plugcfg"; Components: "max14"; Flags: ignoreversion
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\2012 - 32bit\enu\plugcfg"; Components: "max14"; Flags: ignoreversion;
Source: "..\contrib\niflib\NifMopp\NifMopp.dll"; DestDir: "{code:InstallPath|max14}{\}plugins"; Components: "max14"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\Win32\NifMagic.dll"; DestDir: "{code:InstallPath|max14}{\}plugins"; Components: "max14"; Flags: ignoreversion;

Source: "Staging\Release - Max 2012 - x64\NifPlugins.dlu"; DestDir: "{code:InstallPath|max14x64}{\}plugins"; Components: "max14x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{code:InstallPath|max14x64}{\}plugcfg"; Components: "max14x64"; Flags: ignoreversion;
Source: "MaxNifTools.ini"; DestDir: "{localappdata}{\}Autodesk\3dsmax\14 - 64bit\enu\plugcfg"; Components: "max14x64"; Flags: ignoreversion;
Source: "..\contrib\NifMagic\x64\NifMagic.dll"; DestDir: "{code:InstallPath|max14x64}{\}plugins"; Components: "max14x64"; Flags: ignoreversion;

;Source: "src\*"; DestDir: "{reg:HKLM\SOFTWARE\Bethesda Softworks\Oblivion,Path|{pf}{\}Bethesda Softworks\Oblivion}\Data\niftools";  Components: "src"; Flags: ignoreversion recursesubdirs;

;[Icons]
;Name: "{commondesktop}\nif-gmax"; Filename: "{code:InstallPath|gmax12}\nifgmax.exe"; Components: "gmax12"; Tasks: desktopicon; WorkingDir: "{code:InstallPath|gmax12}"; Flags: createonlyiffileexists;

;[Tasks]
;Name: desktopicon; Description: "Create shortcut for nifgmax on Desktop"; Components: "gmax12";


[Code]
#include "NifCommon\build.isi"

var i: Integer;
    UsagePage: TInputOptionWizardPage;
    OBDataDirPage: TInputDirWizardPage;
    OBSIDataDirPage: TInputDirWizardPage;
    MWDataDirPage: TInputDirWizardPage;
    Civ4DataDirPage: TInputDirWizardPage;
    DAoCDataDirPage: TInputDirWizardPage;
    FFDataDirPage: TInputDirWizardPage;
    FF3RDataDirPage: TInputDirWizardPage;
    BCDataDirPage: TInputDirWizardPage;
    F3DataDirPage: TInputDirWizardPage;

function InitializeSetup(): Boolean;
begin
  Result := True;
end;

function GetOptionString(value: Boolean): String;
begin
  if value then begin
    Result := 'True';
  end else begin
    Result := 'False';
  end;
end;

function GetOptionBool(value: String): Boolean;
begin
  if value = 'True' then begin
    Result := True;
  end else begin
    Result := False;
  end;
end;

function GetPrevDataBool(key: String; value: Boolean): Boolean;
begin
    Result := GetOptionBool(GetPreviousData(key, GetOptionString(value)));
end;

function SetPrevDataString(PreviousDataKey: Integer; key:String; value: Boolean): Boolean;
begin
    Result := SetPreviousData(PreviousDataKey, key, GetOptionString(value));
end;

function DataDirPage_ShouldSkipPage(Page: TWizardPage): Boolean;
begin
    Result := True;
    case Page.ID of
      OBDataDirPage.ID: Result    := not UsagePage.Values[0];
      OBSIDataDirPage.ID: Result  := not UsagePage.Values[1];
      MWDataDirPage.ID: Result    := not UsagePage.Values[2];
      Civ4DataDirPage.ID: Result  := not UsagePage.Values[3];
      DAoCDataDirPage.ID: Result  := not UsagePage.Values[4];
      FFDataDirPage.ID: Result    := not UsagePage.Values[5];
      FF3RDataDirPage.ID: Result  := not UsagePage.Values[6];
      BCDataDirPage.ID: Result    := not UsagePage.Values[7];
      F3DataDirPage.ID: Result    := not UsagePage.Values[8];
    end;
end;

procedure InitializeWizard;
begin
  { Create the pages }

  { Add each supported game to option page }
  UsagePage := CreateInputOptionPage(wpSelectComponents,
    'Custom Directories', 'Select Custom Directories for supported Games',
    'Please specify which games you wish to add custom directories for, then click Next.',
    False, False);
  UsagePage.Add('Oblivion');
  UsagePage.Add('Oblivion: Shivering Isles');
  UsagePage.Add('Morrowind');
  UsagePage.Add('Civilization 4');
  UsagePage.Add('Dark Age of Camelot');
  UsagePage.Add('Freedom Force');
  UsagePage.Add('Freedom Force vs. the 3rd Reich');
  UsagePage.Add('Star Trek: Bridge Commander');
  UsagePage.Add('Fallout 3');

  { Create pages for each Game texture locations }
  OBDataDirPage := CreateInputDirPage(UsagePage.ID,
    'Select Oblivion Data Directory', 'Where are the extracted Oblivion data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  OBDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  OBDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  OBDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  OBSIDataDirPage := CreateInputDirPage(OBDataDirPage.ID,
    'Select Oblivion Shivering Isles Data Directory', 'Where are the extracted Oblivion data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  OBSIDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  OBSIDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  OBSIDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  MWDataDirPage := CreateInputDirPage(OBSIDataDirPage.ID,
    'Select Morrowind Data Directory', 'Where are the extracted Morrowind data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  MWDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  MWDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  MWDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  Civ4DataDirPage := CreateInputDirPage(MWDataDirPage.ID,
    'Select Civilization 4 Data Directory', 'Where are the extracted Civilization 4 data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  Civ4DataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  Civ4DataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  Civ4DataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  DAoCDataDirPage := CreateInputDirPage(Civ4DataDirPage.ID,
    'Select Dark Age of Camelot Data Directory', 'Where are the extracted Dark Age of Camelot data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  DAoCDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  DAoCDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  DAoCDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  FFDataDirPage := CreateInputDirPage(DAoCDataDirPage.ID,
    'Select Freedom Force Data Directory', 'Where are the extracted Freedom Force data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  FFDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  FFDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  FFDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  FF3RDataDirPage := CreateInputDirPage(FFDataDirPage.ID,
    'Select Freedom Force vs. the 3rd Reich Data Directory', 'Where are the extracted Freedom Force vs. the 3rd Reich data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  FF3RDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  FF3RDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  FF3RDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  BCDataDirPage := CreateInputDirPage(FF3RDataDirPage.ID,
    'Select Star Trek: Bridge Commander Data Directory', 'Where are the extracted Star Trek: Bridge Commander data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  BCDataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  BCDataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  BCDataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');

  F3DataDirPage := CreateInputDirPage(BCDataDirPage.ID,
    'Select Fallout 3 Data Directory', 'Where are the extracted Fallout 3 data files located?',
    'Select the folders in which 3ds Max should look for files, then click Next.',
    False, '');
  F3DataDirPage.OnShouldSkipPage := @DataDirPage_ShouldSkipPage;
  F3DataDirPage.Add('Extracted Model Directory (e.g. root directory where NIF files are located)');
  F3DataDirPage.Add('Extracted Textures Directory (e.g. root directory containing the Textures directory)');
  
  { Set default values, using settings that were stored last time if possible }
  UsagePage.Values[0] := GetPrevDataBool('bOB', False);
  UsagePage.Values[1] := GetPrevDataBool('bOBSI', False);
  UsagePage.Values[2] := GetPrevDataBool('bMW', False);
  UsagePage.Values[3] := GetPrevDataBool('bCiv4', False);
  UsagePage.Values[4] := GetPrevDataBool('bDAoC', False);
  UsagePage.Values[5] := GetPrevDataBool('bFF', False);
  UsagePage.Values[6] := GetPrevDataBool('bFF3R', False);
  UsagePage.Values[7] := GetPrevDataBool('bBC', False);
  UsagePage.Values[8] := GetPrevDataBool('bF3', False);

  OBDataDirPage.Values[0] := GetPreviousData('OBModelDir', '');
  OBDataDirPage.Values[1] := GetPreviousData('OBTexDir', '');
  OBSIDataDirPage.Values[0] := GetPreviousData('OBSIModelDir', '');
  OBSIDataDirPage.Values[1] := GetPreviousData('OBSITexDir', '');
  MWDataDirPage.Values[0] := GetPreviousData('MWModelDir', '');
  MWDataDirPage.Values[1] := GetPreviousData('MWTexDir', '');
  Civ4DataDirPage.Values[0] := GetPreviousData('Civ4ModelDir', '');
  Civ4DataDirPage.Values[1] := GetPreviousData('Civ4TexDir', '');
  DAoCDataDirPage.Values[0] := GetPreviousData('DAoCModelDir', '');
  DAoCDataDirPage.Values[1] := GetPreviousData('DAoCTexDir', '');
  FFDataDirPage.Values[0] := GetPreviousData('FFModelDir', '');
  FFDataDirPage.Values[1] := GetPreviousData('FFTexDir', '');
  FF3RDataDirPage.Values[0] := GetPreviousData('FF3RModelDir', '');
  FF3RDataDirPage.Values[1] := GetPreviousData('FF3RTexDir', '');
  BCDataDirPage.Values[0] := GetPreviousData('BCModelDir', '');
  BCDataDirPage.Values[1] := GetPreviousData('BCTexDir', '');
  F3DataDirPage.Values[0] := GetPreviousData('F3ModelDir', '');
  F3DataDirPage.Values[1] := GetPreviousData('F3TexDir', '');

end;


procedure RegisterPreviousData(PreviousDataKey: Integer);
var
  UsageMode: String;
begin
  { Store the settings so we can restore them next time }
  SetPrevDataString(PreviousDataKey, 'bOB',   UsagePage.Values[0]);
  SetPrevDataString(PreviousDataKey, 'bOBSI', UsagePage.Values[1]);
  SetPrevDataString(PreviousDataKey, 'bMW',   UsagePage.Values[2]);
  SetPrevDataString(PreviousDataKey, 'bCiv4', UsagePage.Values[3]);
  SetPrevDataString(PreviousDataKey, 'bDAoC', UsagePage.Values[4]);
  SetPrevDataString(PreviousDataKey, 'bFF',   UsagePage.Values[5]);
  SetPrevDataString(PreviousDataKey, 'bFF3R', UsagePage.Values[6]);
  SetPrevDataString(PreviousDataKey, 'bBC',   UsagePage.Values[7]);
  SetPrevDataString(PreviousDataKey, 'bF3',   UsagePage.Values[8]);

  SetPreviousData(PreviousDataKey, 'OBModelDir', OBDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'OBTexDir', OBDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'OBSIModelDir', OBSIDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'OBSITexDir', OBSIDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'MWModelDir', MWDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'MWTexDir', MWDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'Civ4ModelDir', Civ4DataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'Civ4TexDir', Civ4DataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'DAoCModelDir', DAoCDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'DAoCTexDir', DAoCDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'FFModelDir', FFDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'FFTexDir', FFDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'FF3RModelDir', FF3RDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'FF3RTexDir', FF3RDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'BCModelDir', BCDataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'BCTexDir', BCDataDirPage.Values[1]);
  SetPreviousData(PreviousDataKey, 'F3ModelDir', F3DataDirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'F3TexDir', F3DataDirPage.Values[1]);

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
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\9.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 9}{\}');
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
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 9}{\}');
        end;
    'max10': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 2008}{\}');
        end;
    'max10x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 2008}{\}');
        end;
    'max11': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 2009}{\}');
        end;
    'max11x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 2009}{\}');
        end;
    'max12': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 2010}{\}');
        end;
    'max12x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 2010}{\}');
        end;
    'max13': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 2011}{\}');
        end;
    'max13x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 2011}{\}');
        end;
    'max14': 
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\14.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\14.0\' + Names[I], 'Installdir', Result) then begin
                break;
              end;
            end;
          end;
          if (Length(Result) = 0) then
            Result := ExpandConstant('{pf32}{\}AutoDesk\3ds Max 2012}{\}');
        end;
    'max14x64':
        begin
          if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\14.0', Names) then begin
            for I := 0 to GetArrayLength(Names)-1 do begin
              if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\14.0\' + Names[I], 'InstallDir', Result) then
                break;              
            end;
          end;
          if Length(Result) = 0 then
            Result := ExpandConstant('{pf64}{\}AutoDesk\3ds Max 2012}{\}');
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
    '3ds Max 2008 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2008 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\10.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2009 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2009 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\11.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2010 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2010 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\12.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2011 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2011 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2012 (Win32)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'InstallDir', Result) then
              break;              
          end;
        end;
    '3ds Max 2012 (x64)':
        if RegGetSubkeyNames(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0', Names) then begin
          for I := 0 to GetArrayLength(Names)-1 do begin
            if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\Autodesk\3dsMax\13.0\' + Names[I], 'InstallDir', Result) then
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
    if not IsWin64() and (Param = '3ds Max 9 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 2008 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 2009 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 2010 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 2011 (x64)') then
       Result := False;
    if not IsWin64() and (Param = '3ds Max 2012 (x64)') then
       Result := False;
  end;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if CurPageId = wpSelectComponents then
    for i := 0 to WizardForm.ComponentsList.Items.Count - 1 do
       WizardForm.ComponentsList.Checked[i] := IsInstalled(WizardForm.ComponentsList.ItemCaption[i]);
end;

procedure FixPathInINI(component: String);
  var
    iniFile: String;
begin
  if IsComponentSelected(component) then begin
      iniFile :=  InstallPath(component);
      iniFile := AddBackslash(iniFile) + '\plugcfg\MaxNifTools.ini';
      iniFile := ExpandFileName(iniFile);

      if UsagePage.Values[0] then begin {Oblivion}
        SetIniString('Oblivion', 'MeshRootPath', OBDataDirPage.Values[0], iniFile);
        SetIniString('Oblivion', 'TextureRootPath', OBDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[1] then begin
        SetIniString('Oblivion', 'IslesMeshRootPath', OBSIDataDirPage.Values[0], iniFile);
        SetIniString('Oblivion', 'IslesTextureRootPath', OBSIDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[2] then begin
        SetIniString('Morrowind', 'MeshRootPath', MWDataDirPage.Values[0], iniFile);
        SetIniString('Morrowind', 'TextureRootPath', MWDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[3] then begin
        SetIniString('Civilization 4', 'MeshRootPath', Civ4DataDirPage.Values[0], iniFile);
        SetIniString('Civilization 4', 'TextureRootPath', Civ4DataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[4] then begin
        SetIniString('Dark Age of Camelot', 'MeshRootPath', DAoCDataDirPage.Values[0], iniFile);
        SetIniString('Dark Age of Camelot', 'TextureRootPath', DAoCDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[5] then begin
        SetIniString('Freedom Force', 'MeshRootPath', FFDataDirPage.Values[0], iniFile);
        SetIniString('Freedom Force', 'TextureRootPath', FFDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[6] then begin
        SetIniString('Freedom Force vs. the 3rd Reich', 'MeshRootPath', FF3RDataDirPage.Values[0], iniFile);
        SetIniString('Freedom Force vs. the 3rd Reich', 'TextureRootPath', FF3RDataDirPage.Values[1], iniFile);
      end
      if UsagePage.Values[7] then begin
        SetIniString('Star Trek: Bridge Commander', 'MeshRootPath', BCDataDirPage.Values[0], iniFile);
        SetIniString('Star Trek: Bridge Commander', 'TextureRootPath', BCDataDirPage.Values[1], iniFile);
      end;
      if UsagePage.Values[8] then begin
        SetIniString('Fallout 3', 'MeshRootPath', F3DataDirPage.Values[0], iniFile);
        SetIniString('Fallout 3', 'TextureRootPath', F3DataDirPage.Values[1], iniFile);
      end;
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) then begin
      FixPathInINI('gmax12');
      FixPathInINI('max4');
      FixPathInINI('max42');
      FixPathInINI('max5');
      FixPathInINI('max6');
      FixPathInINI('max7');
      FixPathInINI('max8');
      FixPathInINI('max9');
      FixPathInINI('max9x64');
      FixPathInINI('max10');
      FixPathInINI('max10x64');
      FixPathInINI('max11');
      FixPathInINI('max11x64');
      FixPathInINI('max12');
      FixPathInINI('max12x64');
      FixPathInINI('max13');
      FixPathInINI('max13x64');
      FixPathInINI('max14');
      FixPathInINI('max14x64');
  end;
end;

