#! /bin/bash
# Join images vertically, to create a sprite sheat.
# Dependencies: Imagemagick (convert)

convert white_bright_normal.png \
        white_bright_hover.png \
        white_bright_on_red.png \
        black_normal.png \
        black_hover.png \
        black_on_red.png \
        -append bright_sprite.png

# convert white_dark_normal.png \
#         white_dark_hover.png \
#         white_dark_on_red.png \
#         black_normal.png \
#         black_hover.png \
#         black_on_red.png \
#         -append dark_sprite.png
