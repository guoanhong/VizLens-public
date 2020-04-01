# Read which interface to clean
interface="$1"

rm -r ./samples/"$interface"/images/
rm -r ./samples/"$interface"/log/

mkdir ./samples/"$interface"/images/
mkdir ./samples/"$interface"/log/

cp ../WebServer/backup/VizLensDynamic.db ../WebServer/db/VizLensDynamic.db
rm -r ../WebServer/images_original/testsession/