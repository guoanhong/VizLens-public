rm ../db/VizLensDynamic.db
cp $1/VizLensDynamic.db ../db/VizLensDynamic.db
chmod -R 777 ../db/VizLensDynamic.db
rm -rf ../images_original/
rm -rf ../images_reference/
rm -rf ../images_video/
mkdir ../images_original/
chmod -R 777 ../images_original/
mkdir ../images_reference/
chmod -R 777 ../images_reference/
mkdir ../images_video/
chmod -R 777 ../images_video/
mkdir ../images_video/coffee_machine/
chmod -R 777 ../images_video/coffee_machine/
mkdir ../images_video/coffee_machine/log/
chmod -R 777 ../images_video/coffee_machine/log/
cp -vr $1/images_original ../
cp -vr $1/images_reference ../
cp -vr $1/images_video ../
