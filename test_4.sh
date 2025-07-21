echo "En este test probamos que siga existiendo lo del test_3 y lo eliminamos"
ls
echo "Entramos al directorio prueba2"
cd prueba2
echo "Leemos el archivo"
cat archivo.txt
echo "Eliminamos el archivo"
unlink archivo.txt
echo "Eliminamos el directorio prueba2, y luego prueba1"
cd ..
rmdir prueba1
rmdir prueba2
ls
