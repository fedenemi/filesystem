# fisop-fs

# Estructuras
 - 1 Superbloque.
 - 1 Bitmap de inodos.
 - 80 Inodos.

# Superbloque
El superbloque contiene metadata de todo el filesystem: cantidad total de inodos presentes en el mismo y un puntero al comienzo del bitmap de inodos.

# Bitmap de inodos
El bitmap es una estructura implementada con un arreglo que consta de unos y ceros que indican el estado de los inodos, Es decir, en caso de que el inodo i este ocupado, la ubicacion i del arreglo del bitmap contendra un uno, mientras que de estar libre, habra un cero.

# Inodos
Los inodos son estructuras que guardan la data y la metadata de un archivo. Para la implementacion llevada a cabo se evito la creacion de datablock al hacer que los propios inodos sean los que guarden la data en su estructura.
Los campos que componen al inodo son los siguientes:
- inum          -> numero de inodo.
- tipo          -> Tipo de inodo, si es un archivo o directorio.
- nombre        -> Nombre del inodo, incluyendo ruta completa.
- modo          -> Si el archivo puede ser leido/ escrito/ ejecutado.
- dueño         -> User id del creador del archivo.
- grupo         -> Group id.
- tamaño        -> Tamaño del archivo, cuanto esta en uso realmente.
- ctime         -> Hora en la que fue creado.
- mtime         -> Hora en la que fue modificado.
- atime         -> Hora en la que se accedio por ultima vez.
- links_count   -> Cantidad de hard links, para la implementacion siempre sera uno para los archivos y dos para los directorios debido a que no se implementaron hardlinks ni directorios anillados de mas de un nivel de recursion.
- data          -> Data del inodo.

Ademas se cuenta con un arreglo de inodos en el cual se guardan ordenados segun su numero de inodo(realmente el numero de inodo viene dado por la posicion que ocupan en este arreglo).

## Cómo el sistema de archivos encuentra un archivo específico dado un path.

Para encontrar un archivo mediante un path se utiliza la funcion auxiliar:

```c
int get_inode(const char *path)
```
La cual, inicialmente recorre el bitmap de inodos donde compara el nombre del path buscado con el nombre de los inodos que se encuentren ocupados segun el bitmap.

## Serializacion en disco.
Para la persistencia se escriben los bytes del estado actual de las estructuras dentro de un archivo, el cual por defecto es "persistence_file.fisopfs" pero puede cambiarse si se envia un nuevo nombre de archivo por parametro al inicializar el filesystem.
Cuando se reinicia el filesystem se lee el archivo y se setean todas las estructuras en funcion a lo que se lea, de no existir el archivo, lo crea para iniciar la serializacion.

# Comentario
Como se nota en las imagenes de los tests realizados, hay un problema a la hora del uso de ls . y de touch el cual no logramos resolver.