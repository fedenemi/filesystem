#!/bin/bash

make clean

rmdir prueba

#persistencia, comentar lo de abajo
#rm persistence_file.fisopfs

make

mkdir prueba

./fisopfs -f prueba/