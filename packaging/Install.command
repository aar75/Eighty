#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"

echo "Installing Eighty (CS-80 x Jupiter-8 synth)..."
echo ""

mkdir -p "$VST3_DEST" "$AU_DEST"

rm -rf "$VST3_DEST/Eighty.vst3" "$AU_DEST/Eighty.component"
cp -R "$DIR/Eighty.vst3" "$VST3_DEST/"
cp -R "$DIR/Eighty.component" "$AU_DEST/"

# Clear the "downloaded from the internet" quarantine flag so Gatekeeper
# doesn't block an ad-hoc-signed (non-notarized) plugin from loading.
xattr -cr "$VST3_DEST/Eighty.vst3" 2>/dev/null || true
xattr -cr "$AU_DEST/Eighty.component" 2>/dev/null || true

echo "Installed:"
echo "  VST3 -> $VST3_DEST/Eighty.vst3"
echo "  AU   -> $AU_DEST/Eighty.component"
echo ""
echo "Done. Restart your DAW (or rescan/reset its plugin list) to see Eighty."
echo ""
read -p "Press Return to close this window..."
