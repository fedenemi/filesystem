#define FUSE_USE_VERSION 30
#define MAX_DATA_SIZE 4096

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "structures.h"


#define DEFAULT_FILE_DISK "persistence_file.fisopfs"

char *filedisk = DEFAULT_FILE_DISK;

struct superblock superblock;       // Superbloque.
int ibitmap[TOTAL_INODES];          // Bitmap de inodos.
struct inode inodes[TOTAL_INODES];  // Arreglo de inodos.

int
get_inode(const char *path)
{
	for (int i = 0; i < TOTAL_INODES; i++) {
		if (ibitmap[i] == FREE) {
			continue;
		}
		if (strcmp(inodes[i].name, path) == 0) {
			return i;
		}
	}
	return -1;
}


int
get_free_inode()
{
	for (int i = 0; i < TOTAL_INODES; i++) {
		if (ibitmap[i] == FREE) {
			return i;
		}
	}
	return -1;
}


int
inode_initialize(const char *path, int type, mode_t mode)
{
	int inode_idx = get_free_inode();
	if ((inode_idx == -1) || (strlen(path) > NAME_SIZE)) {
		return -1;
	}
	ibitmap[inode_idx] = OCCUPIED;
	struct inode *current_inode = &inodes[inode_idx];
	time_t right_now = time(NULL);
	nlink_t tot_links = 0;
	if (type == DIRECTORY) {
		tot_links = 2;
	} else {
		tot_links = 1;
	}

	current_inode->inum = inode_idx;
	current_inode->type = type;
	strcpy(current_inode->name, path);
	current_inode->mode = mode;
	current_inode->owner = getuid();
	current_inode->group = getgid();
	current_inode->size = 0;
	current_inode->ctime = right_now;
	current_inode->mtime = right_now;
	current_inode->atime = right_now;
	current_inode->links_count = tot_links;
	memset(current_inode->data, 0, BLOCK_SIZE);
	return 0;
}

void
inode_delete(int i_pos)
{
	ibitmap[i_pos] = FREE;
	memset(&inodes[i_pos], 0, sizeof(struct inode));
}

void
fisopfs_persistence()
{
	FILE *file;
	file = fopen(filedisk, "w");
	fwrite(&superblock, sizeof(struct superblock), 1, file);
	fwrite(ibitmap, sizeof(int), TOTAL_INODES, file);
	fwrite(inodes, sizeof(struct inode), TOTAL_INODES, file);
	fclose(file);
}

void
fisopfs_initialize()
{
	superblock.inodes = TOTAL_INODES;
	superblock.ibitmap = ibitmap;
	fisopfs_persistence();
}

void
read_persistence(FILE *file_name)
{
	int result_superblock =
	        fread(&superblock, sizeof(struct superblock), 1, file_name);
	int result_ibitmap =
	        fread(&ibitmap, sizeof(int), TOTAL_INODES, file_name);
	int result_inodes =
	        fread(&inodes, sizeof(struct inode), TOTAL_INODES, file_name);
	if (result_superblock == -1 || result_ibitmap == -1 ||
	    result_inodes == -1) {
		printf("Ocurrio un error al cargar el archivo de persistencia");
	}
	superblock.ibitmap = ibitmap;
}

void *
fisopfs_init(struct fuse_conn_info *conn_info)
{
	printf("[debug] fisopfs_init\n");
	FILE *file = fopen(filedisk, "r");
	if (!file) {
		fisopfs_initialize();
	} else {
		read_persistence(file);
		fclose(file);
	}
	return NULL;
}

static void
fisopfs_destroy()
{
	fisopfs_persistence();
}


static int
fisopfs_getattr(const char *path, struct stat *st)
{
	printf("[debug] fisopfs_getattr - path: %s\n", path);

	if (strcmp(path, ROOT) == 0) {
		st->st_uid = 1717;
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
	}
	int inode_idx = get_inode(path);
	if (inode_idx == -1) {
		return -ENOENT;
	}
	struct inode inode = inodes[inode_idx];
	st->st_dev = 0;
	st->st_ino = inode_idx;
	st->st_uid = inode.owner;
	st->st_mode = inode.mode;
	st->st_atime = inode.atime;
	st->st_mtime = inode.mtime;
	st->st_ctime = inode.ctime;
	st->st_size = inode.size;
	st->st_gid = inode.group;
	st->st_blksize = BLOCK_SIZE;
	st->st_nlink = inode.links_count;

	if (inode.type == TYPE_FILE) {
		st->st_mode = __S_IFREG | 0644;
	} else {
		st->st_mode = __S_IFDIR | 0755;
	}
	return 0;
}


int
get_dentries(const char *path)
{
	if (!path)
		return 0;
	int tot_dentries = 0;
	int length = strlen(path);
	for (int i = 0; i < length; i++) {
		if (path[i] == '/') {
			tot_dentries++;
		}
	}
	return tot_dentries;
}

int
dir_in_path(const char *dir_path, char *inode_path)
{
	if (!(strlen(inode_path) > strlen(dir_path) && strlen(dir_path) > 0)) {
		return 0;
	}
	if (strncmp(dir_path, inode_path, strlen(dir_path)) == 0) {
		return 1;
	}

	return 0;
}

int
get_name_offset(const char *path)
{
	int name_start = -1;
	for (int i = 0; i < strlen(path); i++) {
		if (path[i] == '/') {
			name_start = i + 1;
		}
	}
	return name_start;
}

static int
fisopfs_readdir(const char *path,
                void *buffer,
                fuse_fill_dir_t filler,
                off_t offset,
                struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_readdir - path: %s\n", path);

	// Los directorios '.' y '..'
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);


	if (strcmp(path, ROOT) == 0) {
		for (int i = 0; i < TOTAL_INODES; i++) {
			if (ibitmap[i] == FREE) {
				continue;
			}
			if (get_dentries((inodes[i].name)) == 1) {
				filler(buffer, inodes[i].name + 1, NULL, 0);
			}
		}
		return 0;
	}
	int position = get_inode(path);
	if (position == -1) {
		printf("Error en path: inodo no encontrado\n");
		return -ENOENT;
	}
	struct inode directory = inodes[position];
	if (directory.type != DIRECTORY) {
		printf("El path pasado no era de tipo directorio\n");
		return -ENOTDIR;
	}
	directory.atime = time(NULL);

	for (int i = 0; i < TOTAL_INODES; i++) {
		if (ibitmap[i] == FREE ||
		    !dir_in_path(directory.name, inodes[i].name)) {
			continue;
		}
		int name_position = get_name_offset(inodes[i].name);
		filler(buffer, inodes[i].name + name_position, NULL, 0);
	}
	return 0;
}
/*#define MAX_CONTENIDO 100
static char fisop_file_contenidos[MAX_CONTENIDO] = "hola fisopfs!\n";*/

static int
fisopfs_mkdir(const char *path, mode_t mode)
{
	printf("[debug] fisopfs_mkdir - path: %s\n", path);
	return inode_initialize(path, DIRECTORY, mode);
}

static int
fisopfs_unlink_file(const char *path)
{
	printf("[debug] fisopfs_unlink - path: %s\n", path);
	int position = get_inode(path);
	if (position == -1) {
		return -ENOENT;
	}
	if (inodes[position].type == DIRECTORY) {
		return -EINVAL;  // Podria ser EISDIR quiza
	}
	inodes[position].links_count--;
	if (inodes[position].links_count == 0) {
		inode_delete(position);
	}
	return 0;
}

static int
fisopfs_rmdir(const char *path)
{
	printf("[debug] fisopfs_rmdir - path: %s\n", path);
	int inode_pos = get_inode(path);
	if (strcmp(path, ROOT) == 0) {
		return -EPERM;
	}
	if (inode_pos == -1) {
		return -ENOENT;
	}
	if (inodes[inode_pos].type != DIRECTORY) {
		return -ENOTDIR;
	}
	for (int i = 0; i < TOTAL_INODES; i++) {
		if (ibitmap[i] == 0 || !dir_in_path(path, inodes[i].name)) {
			continue;
		}
		fisopfs_unlink_file(
		        inodes[i].name);  // Esto seria suponiendo que hay un unico nivel de recursion de directorios
	}
	inode_delete(inode_pos);
	return 0;
}

static int
fisopfs_read(const char *path,
             char *buffer,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_read - path: %s, offset: %lu, size: %lu\n",
	       path,
	       offset,
	       size);
	int i_pos = get_inode(path);
	if (i_pos == -1) {
		return -ENOENT;
	}
	if (inodes[i_pos].type == DIRECTORY) {
		return -EISDIR;
	}
	if (offset + size > inodes[i_pos].size) {
		size = inodes[i_pos].size - offset;
	}
	if (offset < 0 || size <= 0) {
		return -EINVAL;
	}
	inodes[i_pos].atime = time(NULL);
	memcpy(buffer, inodes[i_pos].data + offset, size);
	return size;
}

static int
fisopfs_write(const char *path,
              const char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_write - path: %s, size: %lu, offset: %lu\n",
	       path,
	       size,
	       offset);

	if (size + offset > MAX_DATA_SIZE) {
		printf("[debug] fisopfs_write - size + offset > MAX_DATA_SIZE: "
		       "%lu\n",
		       (unsigned long) MAX_DATA_SIZE);
		return -EFBIG;
	}

	int i_pos = get_inode(path);


	if (i_pos == -1) {
		int f = inode_initialize(path, TYPE_FILE, 0);

		if (f != 0) {
			return f;
		}

		int pos = get_inode(path);

		if (pos == -1) {
			printf("[debug] fisopfs_write - inode not found\n");
			return -ENOENT;
		}
	}

	if (inodes[i_pos].type != TYPE_FILE) {
		printf("[debug] fisopfs_write - inode is not a file\n");
		return -EISDIR;
	}


	strncpy(inodes[i_pos].data + offset, buf, size);


	inodes[i_pos].size += size;
	inodes[i_pos].data[inodes[i_pos].size] = '\0';
	inodes[i_pos].atime = time(NULL);
	inodes[i_pos].mtime = time(NULL);

	return size;
}

static int
fisopfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	printf("[debug] fisopfs_create - path: %s\n", path);

	if (get_inode(path) != -1) {
		printf("[debug] fisopfs_create - file already exists - path: "
		       "%s\n",
		       path);
		return -EEXIST;
	}

	int inode_idx = get_free_inode();

	if (inode_idx == -1) {
		printf("[debug] fisopfs_create - no free inodes\n");
		return -ENOSPC;
	}

	if (inode_initialize(path, TYPE_FILE, mode) != 0) {
		printf("[debug] fisopfs_create - inode creation failed\n");
		return -ENOENT;
	}

	return 0;
}

static int
fisopfs_truncate(const char *path, off_t size)
{
	printf("[debug] fisopfs_truncate - path: %s, size: %lu\n", path, size);

	if (size > MAX_DATA_SIZE) {
		printf("[debug] fisopfs_truncate - size > MAX_DATA_SIZE: %lu\n",
		       (unsigned long) MAX_DATA_SIZE);
		return -EFBIG;
	}

	int inode_idx = get_free_inode();

	if (inode_idx == -1) {
		printf("[debug] fisopfs_truncate - inode not found\n");
		return -ENOENT;
	}

	if (inodes[inode_idx].type != TYPE_FILE) {
		printf("[debug] fisopfs_truncate - inode is not a file\n");
		return -EISDIR;
	}

	if (size < inodes[inode_idx].size) {
		memset(inodes[inode_idx].data + size,
		       0,
		       inodes[inode_idx].size - size);
	} else if (size > inodes[inode_idx].size) {
		memset(inodes[inode_idx].data + inodes[inode_idx].size,
		       0,
		       size - inodes[inode_idx].size);
	}

	inodes[inode_idx].size = size;
	inodes[inode_idx].mtime = time(NULL);
	inodes[inode_idx].atime = time(NULL);

	return 0;
}

static struct fuse_operations operations = {
	.init = fisopfs_init,
	.getattr = fisopfs_getattr,
	.readdir = fisopfs_readdir,
	.read = fisopfs_read,
	.mkdir = fisopfs_mkdir,
	.unlink = fisopfs_unlink_file,
	.rmdir = fisopfs_rmdir,
	.destroy = fisopfs_destroy,
	.write = fisopfs_write,
	.create = fisopfs_create,

};

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc - 1; i++) {
		if (strcmp(argv[i], "--filedisk") == 0) {
			filedisk = argv[i + 1];

			// We remove the argument so that fuse doesn't use our
			// argument or name as folder.
			// Equivalent to a pop.
			for (int j = i; j < argc - 1; j++) {
				argv[j] = argv[j + 2];
			}

			argc = argc - 2;
			break;
		}
	}

	return fuse_main(argc, argv, &operations, NULL);
}
