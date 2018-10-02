#!/bin/sh

svgfiles="src/zoom-area.svg
          src/zoom-generic.svg
	  src/zoom-in.svg
	  src/zoom-out.svg
	  src/cut.svg"

filter=Gaussian

for svgfile in ${svgfiles} ; do
  filename=${svgfile/%.svg/}

  # Imagemagick with rsvg support
  convert -background none -density 1200 \
    -define filter:blur=0.75 -filter ${filter} -resize 24x24 \
    "${filename}.svg" "${filename}-black.png"
  convert -background none \
    -resize 384x384 \
    -channel red -negate \
    -channel green -negate \
    -channel blue -negate \
    -define filter:blur=0.75 -filter ${filter} -resize 24x24 \
    "${filename}.svg" "${filename}-white.png"

  # Imagemagick without rsvg support
  #rsvg-convert -w 24 -h 24 "${filename}.svg" "${filename}-black-hd.png"
  #convert \
  #  -channel red -negate \
  #  -channel green -negate \
  #  -channel blue -negate "${filename}-black-hd.png" "${filename}-white-hd.png"
  #convert -filter Sinc -background none -density 1200 -resize 24x24 "${filename}-black-hd.png" "${filename}-black.png"
  #convert -filter Sinc -background none -density 1200 -resize 24x24 "${filename}-white-hd.png" "${filename}-white.png"

  mv -f "${filename}-black.png" "${filename}-white.png" ./
done

