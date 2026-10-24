; Inno Setup script - builds the Windows installer for Tension Builder (VST3)
#define MyAppName "Tension Builder"
#define MyAppVersion "1.0.0"

[Setup]
AppId={{B7F0E8A1-5C3D-4E92-9A61-7D2F4B8C1E55}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=TechHouseTools
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
UninstallFilesDir={commonappdata}\TechHouseTools\TensionBuilder
OutputDir=..\..\dist
OutputBaseFilename=TensionBuilder-Setup-Windows
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern

[Files]
Source: "..\..\build\TensionBuilder_artefacts\Release\VST3\Tension Builder.vst3\*"; \
    DestDir: "{commoncf64}\VST3\Tension Builder.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Tension Builder.vst3"

[Messages]
FinishedLabel=Tension Builder was installed to your VST3 folder.%n%nIn Ableton Live: Preferences > Plug-Ins > make sure "Use VST3 Plug-In System Folders" is on, then click Rescan. Find it under Plug-Ins > Tension Builder.
