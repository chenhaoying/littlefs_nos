#include "littlefs.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

lfs_file_t *data_file;
char file_name[64];
int file_size;
lfs_dir_t *data_dir;
struct lfs_info data_dir_info;
#if 0
#define DEBUG(format,...) printf(""format"", ##__VA_ARGS__) 
#else
#define DEBUG(...)
#endif
int data_record_write(void *pbuf, int offset, int date);
int data_record_read(void *pbuf, int offset, int date);
int data_record_nblocks_read(void *pbuf, int offset, int date, int nblocks);
int data_record_clear(void);
int data_record_tree(char *pbuf, int buf_size);
data_rec_t data_rec = 
{
    .write = data_record_write,
    .read = data_record_read,
    .nblocks_read = data_record_nblocks_read,
    .tree = data_record_tree,
    .clear = data_record_clear,
};

int isnum(char *str)
{
    int i;
    for(i=0;i<strlen(str);i++)
    {
        if(isdigit(str[i]) == 0) return -1;
    }
    return 0;
}

int remove_not_date_file(void)
{
    data_dir = littlefs->dir->open(data_rec.property->data_dir);
    littlefs->dir->rewind(data_dir);
    while(littlefs->dir->read(data_dir, &data_dir_info))
    {
        if(strcmp(data_dir_info.name, ".") == 0) continue;
        if(strcmp(data_dir_info.name, "..") == 0) continue;
        if(isnum(data_dir_info.name))
        {
            littlefs->dir->close(data_dir);
            memset(file_name, 0, sizeof(file_name));
            sprintf(file_name, "%s/%.30s", data_rec.property->data_dir, data_dir_info.name);
            littlefs->remove(file_name);
            return -1;
        }
    }
    littlefs->dir->close(data_dir);
    return 0;
}

int remove_older_file(void)
{
    int older_date, temp_date, file_num;
    older_date = 0;
    file_num = 0;
    data_dir = littlefs->dir->open(data_rec.property->data_dir);
    littlefs->dir->rewind(data_dir);
    while(littlefs->dir->read(data_dir, &data_dir_info))
    {
        if(strcmp(data_dir_info.name, ".") == 0) continue;
        if(strcmp(data_dir_info.name, "..") == 0) continue;
        sscanf(data_dir_info.name, "%d", &temp_date);
        if((file_num == 0)||(older_date > temp_date)) older_date = temp_date;
        file_num ++;    // 计算文件总数
    }
    littlefs->dir->close(data_dir);
    DEBUG("remove_older_file: file_num = %d\n", file_num);
    if((file_num<=data_rec.property->rec_file_limit)||(data_rec.property->rec_file_limit==0)) return 0;  // 文件数少于记录文件数限制
    file_num --;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%06d", data_rec.property->data_dir, older_date);
    DEBUG("remove_older_file: remove file: %s\n", file_name);
    littlefs->remove(file_name); // 删除一个最旧的记录
    return -1;
}

int remove_all_data_file(void)
{
    data_dir = littlefs->dir->open(data_rec.property->data_dir);
    if(data_dir == NULL) return 0;
    littlefs->dir->rewind(data_dir);
    while(littlefs->dir->read(data_dir, &data_dir_info))
    {
        if(strcmp(data_dir_info.name, ".") == 0) continue;
        if(strcmp(data_dir_info.name, "..") == 0) continue;
        break;
    }
    littlefs->dir->close(data_dir);
    if(!strlen(data_dir_info.name)) return 0;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%.30s", data_rec.property->data_dir, data_dir_info.name);
    DEBUG("remove_all_data_file: remove file: %s\n", file_name);
    littlefs->remove(file_name);
    return -1;
}

int data_dir_check(void)
{
    int ret;
    data_dir = littlefs->dir->open(data_rec.property->data_dir);
    if(data_dir == NULL)
    {
        ret = littlefs->dir->mkdir(data_rec.property->data_dir);
        if(ret) return -1;
    }
    littlefs->dir->close(data_dir);
    return 0;
}

int data_record_write(void *pbuf, int offset, int date)
{
    int write_pos, write_size;
    int dummy_data = 0;

    if(data_dir_check()) return -1;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%06d", data_rec.property->data_dir, date);
    DEBUG("data_record_write: date = %d offset = %d\n", date, offset);
    DEBUG("data_record_write: write file name: %s\n", file_name);
    data_file = littlefs->file->open(file_name, LFS_O_RDWR);
    if(data_file == NULL)
    {
        data_file = littlefs->file->open(file_name, LFS_O_RDWR|LFS_O_CREAT);
        littlefs->file->close(data_file);
        while(remove_not_date_file());  // 删除所有非日期命名yymmdd的文件
        while(remove_older_file()); // 删除旧记录，直到少于或等于设定的文件记录限制数量
        sprintf(file_name, "%s/%06d", data_rec.property->data_dir, date);
        data_file = littlefs->file->open(file_name, LFS_O_RDWR);
    }
    littlefs->file->seek(data_file, 0, LFS_SEEK_END);
    file_size = littlefs->file->tell(data_file);
    write_pos = offset * data_rec.property->bytes_of_data;
    DEBUG("data_record_write: file size = %d\n", file_size);
    if(file_size != write_pos)
    {
        if(file_size < write_pos) {
            while(file_size < write_pos)
            {
                if(data_rec.property->invaild_data != NULL)
                {
                    write_size = data_rec.property->bytes_of_data;
                    if(littlefs->file->write(data_rec.property->invaild_data, write_size, data_file) != write_size)
                    {
                        littlefs->file->close(data_file);
                        return -1;
                    }
                }
                else 
                {
                    if((write_pos - file_size) > sizeof(dummy_data))
                        write_size = sizeof(dummy_data);
                    else 
                        write_size = write_pos - file_size;
                    if(littlefs->file->write(&dummy_data, write_size, data_file) != write_size)
                    {
                        littlefs->file->close(data_file);
                        return -1;
                    }
                }
                file_size += write_size;
            }
        }
        else
        {
            if(littlefs->file->truncate(write_pos, data_file))
            {
                littlefs->file->close(data_file);
                return -1;
            }
        }
        littlefs->file->seek(data_file, write_pos, LFS_SEEK_SET);
    }
    if(littlefs->file->write(pbuf, data_rec.property->bytes_of_data, data_file)
        !=data_rec.property->bytes_of_data)
    {
        littlefs->file->close(data_file);
        return -1;
    }
    littlefs->file->close(data_file);
    return 0;
}

int data_record_read(void *pbuf, int offset, int date)
{
    int read_pos, read_size;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%06d", data_rec.property->data_dir, date);
    DEBUG("data_record_read: date = %d offset = %d\n", date, offset);
    DEBUG("data_record_read: read file name: %s\n", file_name);
    data_file = littlefs->file->open(file_name, LFS_O_RDWR);
    if(data_file == NULL)
    {
        DEBUG("data_record_read: %s: No such file or directory\n", file_name);
        return -1;
    }
    littlefs->file->seek(data_file, 0, LFS_SEEK_END);
    file_size = littlefs->file->tell(data_file);
    read_size = data_rec.property->bytes_of_data;
    read_pos = offset * read_size;
    DEBUG("data_record_read: file size = %d\n", file_size);
    if(read_pos+read_size > file_size)
    {
        DEBUG("data_record_read: Read data out of bounds\n");
        littlefs->file->close(data_file);
        if(data_rec.property->invaild_data)
            memcpy(pbuf, data_rec.property->invaild_data, read_size);
        else
            memset(pbuf, 0, read_size);
        return -1;
    }
    littlefs->file->seek(data_file, read_pos, LFS_SEEK_SET);
    if(littlefs->file->read(pbuf, read_size, data_file)!=read_size)
    {
        littlefs->file->close(data_file);
        if(data_rec.property->invaild_data)
            memcpy(pbuf, data_rec.property->invaild_data, read_size);
        else
            memset(pbuf, 0, read_size);
        return -1;
    }
    littlefs->file->close(data_file);
    return 0;
}

int data_record_nblocks_read(void *pbuf, int offset, int date, int nblocks)
{
    int i;
    int read_pos, read_size;
    read_size = data_rec.property->bytes_of_data;
    read_pos = offset * read_size;
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "%s/%06d", data_rec.property->data_dir, date);
    DEBUG("data_record_nblocks_read: date = %d offset = %d nblocks = %d\n", date, offset, nblocks);
    DEBUG("data_record_nblocks_read: read file name: %s\n", file_name);
    data_file = littlefs->file->open(file_name, LFS_O_RDWR);
    if(data_file == NULL)
    {
        DEBUG("data_record_nblocks_read: %s: No such file or directory\n", file_name);
        if(data_rec.property->invaild_data)
        {
            for(i=0;i<nblocks;i++)
            {
                memcpy(pbuf, data_rec.property->invaild_data, read_size);
                pbuf += read_size;
            }
        }
        else memset(pbuf, 0, read_size * nblocks);
        return -1;
    }
    littlefs->file->seek(data_file, 0, LFS_SEEK_END);
    file_size = littlefs->file->tell(data_file);
    DEBUG("data_record_read: file size = %d\n", file_size);
    if(read_pos+read_size > file_size)
    {
        DEBUG("data_record_nblocks_read: Read data out of bounds\n");
        littlefs->file->close(data_file);
        if(data_rec.property->invaild_data)
        {
            for(i=0;i<nblocks;i++)
            {
                memcpy(pbuf, data_rec.property->invaild_data, read_size);
                pbuf += read_size;
            }
        }
        else memset(pbuf, 0, read_size * nblocks);
        return -1;
    }
    littlefs->file->seek(data_file, read_pos, LFS_SEEK_SET);
    for(i=0;i<nblocks;i++)
    {
        if(littlefs->file->read(pbuf, read_size, data_file)!=read_size)
        {
            DEBUG("data_record_nblocks_read: read data fail.\n");
            if(data_rec.property->invaild_data)
                memcpy(pbuf, data_rec.property->invaild_data, read_size);
            else 
                memset(pbuf, 0, read_size);
        }
        pbuf += read_size;
    }
    littlefs->file->close(data_file);
    return 0;
}

int data_record_clear(void)
{
    while(remove_all_data_file());
    return 0;
}

int data_record_tree(char *pbuf, int buf_size)
{
    return littlefs->tree(data_rec.property->data_dir, pbuf, buf_size);
}
