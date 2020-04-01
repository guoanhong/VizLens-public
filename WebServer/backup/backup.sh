mkdir -p $1/
chmod -R 777 $1/
cp ../db/VizLensDynamic.db $1/VizLensDynamic.db
cp -vr ../images_original $1/
cp -vr ../images_reference $1/
cp -vr ../images_video $1/
