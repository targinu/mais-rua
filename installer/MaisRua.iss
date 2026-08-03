; Instalador do Mais Rua (Inno Setup 6). Compilar com:
;   ISCC.exe installer\MaisRua.iss
; Espera que o build Release já tenha sido feito (cmake --build build --config Release)
; antes de compilar o instalador.

#define MyAppName "Mais Rua"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Rua Audio"
#define MyBuildDir "..\build\MaisRua_artefacts\Release"

[Setup]
AppId={{9C2A9C7C-2E4B-4C7B-9C2B-3E7F1B6D2A11}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\installer-output
OutputBaseFilename=MaisRua-{#MyAppVersion}-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
UninstallDisplayIcon={app}\Mais Rua.exe
WizardStyle=modern

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Completo (VST3 + Standalone)"
Name: "vst3only"; Description: "Só o plugin VST3 (pra usar dentro da DAW)"
Name: "standalonealone"; Description: "Só o app Standalone (sem DAW)"

[Components]
Name: "vst3"; Description: "Plugin VST3"; Types: full vst3only
Name: "standalone"; Description: "App Standalone"; Types: full standalonealone

[Files]
Source: "{#MyBuildDir}\VST3\Mais Rua.vst3\*"; DestDir: "{commoncf64}\VST3\Mais Rua.vst3"; Flags: recursesubdirs ignoreversion; Components: vst3
Source: "{#MyBuildDir}\Standalone\Mais Rua.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone

[Icons]
Name: "{group}\Mais Rua"; Filename: "{app}\Mais Rua.exe"; Components: standalone
Name: "{group}\Desinstalar Mais Rua"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Mais Rua.vst3"
