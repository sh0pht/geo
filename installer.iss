#define MyAppName "GeoCAD"
#define MyAppVersion "0.2"
[Setup]
AppId={{9C18A8E3-AC0B-47EF-B7A9-F48BC1C6AD7E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\GeoCAD
DefaultGroupName=GeoCAD
OutputDir=dist
OutputBaseFilename=GeoCAD_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\GeoCAD.exe
[Files]
Source: "package\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
[Icons]
Name: "{group}\GeoCAD"; Filename: "{app}\GeoCAD.exe"
Name: "{autodesktop}\GeoCAD"; Filename: "{app}\GeoCAD.exe"; Tasks: desktopicon
[Tasks]
Name: desktopicon; Description: "Создать ярлык на рабочем столе"; GroupDescription: "Дополнительно:"
[Run]
Filename: "{app}\GeoCAD.exe"; Description: "Запустить GeoCAD"; Flags: nowait postinstall skipifsilent
