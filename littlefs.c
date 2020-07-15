#include "littlefs.h"
#include <string.h>
#include <stdio.h>

littlefs_t *littlefs;

struct lfs_config *cfg = 0;
struct lfs_file_config *file_cfg = 0;
lfs_t lfs;
lfs_file_t lfs_file;
lfs_dir_t lfs_dir;

lfs_file_t* file_open(const char *path, int flags)
{
    int ret;
    ret = lfs_file_opencfg(&lfs, &lfs_file, path, flags, file_cfg);
    if(ret != LFS_ERR_OK) return 0;
    return &lfs_file;
}

int file_write(void *pbuf, int size, lfs_file_t *file)
{
    return lfs_file_write(&lfs, file, pbuf, size);
}

int file_read(void *pbuf, int size, lfs_file_t *file)
{
    return lfs_file_read(&lfs, file, pbuf, size);
}

int file_close(lfs_file_t *file)
{
    return lfs_file_close(&lfs, file);
}

int file_seek(lfs_file_t *file, int offset, int whence)
{
    return lfs_file_seek(&lfs, file, offset, whence);
}

int file_tell(lfs_file_t *file)
{
    return lfs_file_tell(&lfs, file);
}

int file_rewind(lfs_file_t *file)
{
    return lfs_file_rewind(&lfs, file);
}

int file_truncate(int size, lfs_file_t *file)
{
    return lfs_file_truncate(&lfs, file, size);
}

int remove(const char *path)
{
    return lfs_remove(&lfs, path);
}

int rename(const char *oldpath, const char *newpath)
{
    return lfs_rename(&lfs, oldpath, newpath);
}

int dir_mkdir(const char *path)
{
    return lfs_mkdir(&lfs, path);
}

lfs_dir_t *dir_open(const char *path)
{
    int ret;
    ret = lfs_dir_open(&lfs, &lfs_dir, path);
    if(ret != LFS_ERR_OK) return 0;
    return &lfs_dir;
}

int dir_close(lfs_dir_t *dir)
{
    return lfs_dir_close(&lfs, dir);
}

int dir_read(lfs_dir_t *dir, struct lfs_info *info)
{
    return lfs_dir_read(&lfs, dir, info);
}

int dir_rewind(lfs_dir_t *dir)
{
    return lfs_dir_rewind(&lfs, dir);
}

int dir_tell(lfs_dir_t *dir)
{
    return lfs_dir_tell(&lfs, dir);
}

int dir_seek(lfs_dir_t *dir, int offset)
{
    return lfs_dir_seek(&lfs, dir, offset);
}

int format(lfs_t *_lfs, const struct lfs_config *_cfg)
{
    return lfs_format(_lfs, _cfg);
}

int mount(lfs_t *_lfs, const struct lfs_config *_cfg)
{
    return lfs_mount(_lfs, _cfg);
}

int unmount(lfs_t *_lfs)
{
    return lfs_unmount(_lfs);
}

static int _traverse_df_cb(void *p, lfs_block_t block)
{
    int *nb = p;
    *nb += 1;
    return 0;
}

int disk_free(void)
{
    int err;
    int available;
	int _df_nballocatedblock = 0;
	err = lfs_fs_traverse(&lfs, _traverse_df_cb, &_df_nballocatedblock);
	if(err < 0) return err;
	available = cfg->block_count*cfg->block_size - _df_nballocatedblock*cfg->block_size;
	return available;
}

int disk_tree(const char *dir_path, char *pbuf, int buf_size)
{
    lfs_dir_t *dir;
    struct lfs_info info;
    char file_name[64];
    char dir_name[64];
    int ret, length, dir_off;

    memset(pbuf, 0, buf_size);

    dir = dir_open(dir_path);
    if(!dir) return 0;

    dir_rewind(dir);
    dir_off = dir_tell(dir);
    dir_close(dir);
    length = 0;
    while(1)
    {
        dir = dir_open(dir_path);
        dir_seek(dir, dir_off);
        ret = dir_read(dir, &info);
        if(ret <= 0)
        {
            dir_close(dir);
            break;
        }
        dir_off = dir_tell(dir);
        dir_close(dir);
        if(info.type == LFS_TYPE_REG)
        {
            sprintf(file_name, "%s/%.30s\r\n", dir_path, info.name);
            // printf("disk_tree: file: %s", file_name);
            if(strlen(file_name) > (buf_size - length - 1)) break;
            strcpy(&pbuf[length], file_name);
            // memcpy(&pbuf[length], file_name, strlen(file_name));
            length += strlen(file_name);
        }
        else if(info.type == LFS_TYPE_DIR)
        {
            if(strcmp(info.name, ".") == 0) continue;
            if(strcmp(info.name, "..") == 0) continue;
            if(strcmp(&dir_name[strlen(dir_path)+1], info.name) == 0) continue;
            sprintf(dir_name, "%s/%.30s", dir_path, info.name);
            if(strlen(dir_name) >= (buf_size-length)) break;
            length += disk_tree(dir_name, &pbuf[length], buf_size-length);
        }
    }
    return length;
}

const lfs_file_fun_t file_fun = 
{
    .open = file_open,
    .write = file_write,
    .read = file_read,
    .close = file_close,
    .seek = file_seek,
    .tell = file_tell,
    .rewind = file_rewind,
    .truncate = file_truncate
};

const lfs_dir_fun_t dir_fun = 
{
    .mkdir = dir_mkdir,
    .open = dir_open,
    .close = dir_close,
    .read = dir_read,
    .rewind = dir_rewind,
    .tell = dir_tell,
    .seek = dir_seek
};

int littlefs_init(littlefs_t *lfs_obj, struct lfs_config *config, struct lfs_file_config *file_config)
{
    littlefs = lfs_obj;
    cfg = config;
    file_cfg = file_config;
    littlefs->lfs = &lfs;
    littlefs->file = (lfs_file_fun_t *)&file_fun;
    littlefs->dir = (lfs_dir_fun_t *)&dir_fun;
    littlefs->format = format;
    littlefs->mount = mount;
    littlefs->unmount = unmount;
    littlefs->remove = remove;
    littlefs->rename = rename;
    littlefs->free = disk_free;
    littlefs->tree = disk_tree;
    return 0;
}

void *lfs_malloc(size_t size)
{
    return NULL;
}

void lfs_free(void *p)
{
    ;
}
