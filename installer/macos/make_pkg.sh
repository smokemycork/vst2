#!/bin/bash
# Builds the macOS installer (.pkg) from the compiled plugin bundles.
set -euo pipefail
cd "$(dirname "$0")/../.."

ART="build/TensionBuilder_artefacts/Release"
STAGE="build/pkgroot"
rm -rf "$STAGE"
mkdir -p "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components" dist

cp -R "$ART/VST3/Tension Builder.vst3"       "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "$ART/AU/Tension Builder.component"    "$STAGE/Library/Audio/Plug-Ins/Components/"

# ad-hoc sign (required for Apple Silicon to load the binaries)
codesign --force --deep -s - "$STAGE/Library/Audio/Plug-Ins/VST3/Tension Builder.vst3"
codesign --force --deep -s - "$STAGE/Library/Audio/Plug-Ins/Components/Tension Builder.component"

pkgbuild --root "$STAGE" \
         --identifier com.techhousetools.tensionbuilder.pkg \
         --version 1.0.0 \
         --install-location / \
         dist/TensionBuilder-macOS.pkg
echo "Created dist/TensionBuilder-macOS.pkg"
