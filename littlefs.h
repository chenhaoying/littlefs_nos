#ifndef __LFS_IF_H__
#define __LFS_IF_H__
#include "lfs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct lfs_file_fun_s
{
    lfs_file_t *(*open)(const char *path, int flags);
    int (*write)(void *pbuf, int size, lfs_file_t *file);
    int (*read)(void *pbuf, int size, lfs_file_t *file);
    int (*close)(lfs_file_t *file);
    int (*seek)(lfs_file_t *file, int offset, int whence);
    int (*tell)(lfs_file_t *file);
    int (*rewind)(lfs_file_t *file);
    int (*truncate)(int size, lfs_file_t *file);
} lfs_file_fun_t;

typedef struct lfs_dir_fun_s
{
    int (*mkdir)(const char *path);
    lfs_dir_t *(*open)(const char *path);
    int (*close)(lfs_dir_t *dir);
    int (*read)(lfs_dir_t *dir, struct lfs_info *info);
    int (*rewind)(lfs_dir_t *dir);
    int (*tell)(lfs_dir_t *dir);
    int (*seek)(lfs_dir_t *dir, int offset);
} lfs_dir_fun_t;

typedef struct littlefs_s 
{
    lfs_t *lfs;

    lfs_file_fun_t *file;
    lfs_dir_fun_t *dir;
    
    int (*format)(lfs_t *_lfs, const struct lfs_config *_cfg);
    int (*mount)(lfs_t *_lfs, const struct lfs_config *_cfg);
    int (*unmount)(lfs_t *_lfs);
    int (*remove)(const char *path);
    int (*rename)(const char *oldpath, const char *newpath);
    int (*free)(void);
    int (*tree)(const char *dir_path, char *pbuf, int buf_size);
} littlefs_t;

littlefs_t *littlefs;
int littlefs_init(littlefs_t *lfs_obj, struct lfs_config *config, struct lfs_file_config *file_config);

typedef struct rec_property_s 
{
    char *data_dir;
    void *invaild_data; // default 0
    int rec_file_limit;
    int bytes_of_data;
} rec_property_t;

typedef struct data_rec_s
{
    rec_property_t *property;
    int (*write)(void *pbuf, int offset, int date);
    int (*read)(void *pbuf, int offset, int date);
    int (*nblocks_read)(void *pbuf, int offset, int date, int nblocks);
    int (*tree)(char *pbuf, int buf_size);
    int (*clear)(void);
} data_rec_t;

extern data_rec_t data_rec;
int filesystem_init(void);
#ifdef __cplusplus
}
#endif
#endif
