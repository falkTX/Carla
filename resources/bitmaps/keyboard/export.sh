#! /bin/bash
# Export several areas in keyboard.svg to optimized PNG.
# Dependencies: Inkscape, pngcrush.

# IDs of target area rectangles in keyboard.svg:
IDs="
white_bright_normal
white_bright_hover
white_bright_on_red
white_bright_on_orange
white_bright_on_green
white_bright_on_blue
white_dark_normal
white_dark_hover
white_dark_on_red
white_dark_on_orange
white_dark_on_green
white_dark_on_blue
black_normal
black_hover
black_on_red
black_on_orange
black_on_green
black_on_blue
"

# Export images:
for ID in $IDs; do
  inkscape keyboard.svg --export-id=$ID --export-png=$ID.png
done

# Optimize opaque images and remove their alpha channels.
# pngcrush will not overwrite input files, so use a temp dir
# and move/overwrite files afterwards.
# pngcrush creates the dir given for -d, if necessary.
pngcrush -c 2 -reduce -d crushed white_*.png

# Optimze transparent images, keeping their alpha channels:
pngcrush -c 6 -reduce -d crushed black_*.png

# Cleanup:
mv crushed/*.png .
rmdir crushed
