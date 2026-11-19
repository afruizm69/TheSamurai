; Instalador de Windows para THE SAMURAI (Inno Setup 6)
; Se compila automáticamente en GitHub Actions; también puedes abrirlo con Inno Setup y pulsar "Compile".

#define AppName      "THE SAMURAI"
#define AppVersion   "1.0.0"
#define AppPublisher "Samurai Audio"
#define BuildDir     "..\build\TheSamurai_artefacts\Release"

[Setup]
AppId={{8C3B2E1A-5D4F-4C2B-9A7E-1F0D3C6B5A92}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=..\build\installer
OutputBaseFilename=THE SAMURAI Setup {#AppVersion}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#AppName}
UninstallDisplayIcon={app}\{#AppName}.exe

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";   Description: "Instalación completa"
Name: "custom"; Description: "Instalación personalizada"; Flags: iscustom

[Components]
Name: "vst3";       Description: "Plugin VST3 (para tu DAW)";          Types: full custom
Name: "standalone"; Description: "Aplicación independiente (.exe)";    Types: full custom

[Tasks]
Name: "desktopicon"; Description: "Crear un icono en el escritorio"; Components: standalone

[Files]
Source: "{#BuildDir}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildDir}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}";             Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\Desinstalar {#AppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#AppName}";       Filename: "{app}\{#AppName}.exe"; Components: standalone; Tasks: desktopicon

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"

[Run]
Filename: "{app}\{#AppName}.exe"; Description: "Abrir {#AppName}"; Components: standalone; Flags: nowait postinstall skipifsilent
