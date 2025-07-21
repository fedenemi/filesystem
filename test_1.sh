echo "creamos directorio"
mkdir directorio_test
ls
cd directorio_test
echo "creamos un archivo dentro del nuevo directorio"
touch archivo.txt
ls
echo "borramos el archivo"
unlink archivo.txt
ls
cd ..
echo "volvemos a root y eliminamos el directorio"
rmdir directorio_test
ls