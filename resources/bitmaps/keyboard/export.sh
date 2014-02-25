# TODO: split flattable from non-flattable PNGs

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

for ID in $IDs; do
  inkscape keyboard.svg --export-id=$ID --export-png=$ID.png
done
