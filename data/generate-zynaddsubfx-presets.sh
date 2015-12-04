#!/bin/bash

ZYN_URI="http://kxstudio.sf.net/carla/plugins/zynaddsubfx"

rm -rf carla-zyn-presets.lv2
mkdir carla-zyn-presets.lv2
cd carla-zyn-presets.lv2

echo "\
@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .
@prefix pset: <http://lv2plug.in/ns/ext/presets#> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#>.
" > manifest.ttl

ls "/usr/share/zynaddsubfx/banks/" | while read i; do

BANK_NAME=$i
BANK_URI=`echo $BANK_NAME | awk '{ sub(" ","%20"); print }' | awk '{ sub(" ","%20"); print }'`

echo "\
<$ZYN_URI#bank_$BANK_URI>
    a pset:Bank ;
    rdfs:label \"$BANK_NAME\" .
" >> manifest.ttl

echo "\
@prefix lv2:   <http://lv2plug.in/ns/lv2core#> .
@prefix pset:  <http://lv2plug.in/ns/ext/presets#> .
@prefix state: <http://lv2plug.in/ns/ext/state#> .
" > "bank-$BANK_NAME.ttl"

ls "/usr/share/zynaddsubfx/banks/$i" | while read j; do

if [ "$j"x != "README"x ]; then

PROG_NAME=$j
PROG_URI=`echo $PROG_NAME | awk '{ sub(".xiz",""); print }' | awk '{ sub(" ","%20"); print }' | awk '{ sub(" ","%20"); print }' | awk '{ sub(" ","%20"); print }' | awk '{ sub(" ","%20"); print }'`

echo "\
<$ZYN_URI#preset_"$BANK_URI"_"$PROG_URI">
    a pset:Preset ;
    lv2:appliesTo <$ZYN_URI> ;
    rdfs:label \"$BANK_NAME: $PROG_NAME\" ;
    rdfs:seeAlso <bank-$BANK_URI.ttl> .
" >> manifest.ttl

echo "\
<$ZYN_URI#preset_"$BANK_URI"_"$PROG_URI">
    a pset:Preset ;
    lv2:appliesTo <$ZYN_URI> ;
    pset:bank <$ZYN_URI#bank_$BANK_URI> ;
    state:state [
        <http://kxstudio.sf.net/ns/carla/chunk>
\"\"\"" >> "bank-$BANK_NAME.ttl"
echo $j
cat "/usr/share/zynaddsubfx/banks/$i/$j" | gzip -d >> "bank-$BANK_NAME.ttl"
echo "\"\"\"
    ] .
" >> "bank-$BANK_NAME.ttl"

fi

done

done
