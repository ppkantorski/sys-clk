#!/bin/bash
set -e
CURRENT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DEST="$CURRENT_DIR/../data/logo_rgba.bin"
FONT="$CURRENT_DIR/../../manager/resources/fira/FiraSans-Medium-rnx.ttf"
FONT_SIZE="30.5"
TEXT="sys-clk"
WIDTH=110
HEIGHT=39

function render() {
	magick -size ${WIDTH}x${HEIGHT} xc:transparent \
	       -background transparent -depth 8 \
	       -fill white -font "$1" -pointsize "$2" \
	       -gravity center -annotate +0+0 "$3" \
	       "$4"
}

render "$FONT" "$FONT_SIZE" "$TEXT" info:
render "$FONT" "$FONT_SIZE" "$TEXT" "RGBA:$DEST"

# Verify the output size
echo "Generated binary size: $(stat -f%z "$DEST" 2>/dev/null || stat -c%s "$DEST" 2>/dev/null) bytes"
echo "Expected size: $((WIDTH * HEIGHT * 4)) bytes (${WIDTH}x${HEIGHT} RGBA)"
