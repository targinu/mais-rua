; Instalador do Mais Rua (Inno Setup 6). Compilar com:
;   ISCC.exe installer\MaisRua.iss
; Espera que o build Release já tenha sido feito (cmake --build build --config Release)
; antes de compilar o instalador.

#define MyAppName "Mais Rua"
#define MyAppVersion "1.2.2"
#define MyAppPublisher "FrozenShade"
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
UninstallDisplayIcon={uninstallexe}
WizardStyle=modern

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyBuildDir}\VST3\Mais Rua.vst3\*"; DestDir: "{commoncf64}\VST3\Mais Rua.vst3"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\Desinstalar Mais Rua"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Mais Rua.vst3"
