mkdir prueba
cd prueba

echo "Creamos archivo"
touch archivo_touch.txt

echo "Verificamos que se haya creado y vemos sus stats"
ls
stat archivo_touch.txt

echo "Escribimos en el archivo"
echo "Hola Mundo" > archivo_touch.txt

echo "Leemos el archivo"
cat archivo_touch.txt

echo "Volvemos a ver las stats"
stat archivo_touch.txt

echo "Eliminamos el archivo"
unlink archivo_touch.txt

ls

echo "Creamos archivo mediante write y leemos su contenido"
echo "Archivo desde write" > archivo_write.txt

ls

echo "Vemos las stats del archivo"
stat archivo_write.txt

echo "Leemos el archivo"
cat archivo_write.txt

echo "Borramos archivo creado mediante write"
unlink archivo_write.txt
ls
cd ..
rmdir prueba
