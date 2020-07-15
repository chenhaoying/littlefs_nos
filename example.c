#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "littlefs.h"

littlefs_t test;
unsigned char test_area[8192];  // 4K空间
int _read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    int address;
    address = c->block_size * block + off;
    memcpy(buffer, &test_area[address], size);
    return 0;
}

int _prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    int address;
    address = c->block_size * block + off;
    memcpy(&test_area[address], buffer, size);
    return 0;
}

int _erase(const struct lfs_config *c, lfs_block_t block)
{
    int address;
    address = c->block_size * block;
    memset(&test_area[address], 0xFF, c->block_size);
    return 0;
}

int _sync(const struct lfs_config *c)
{
    return 0;
}

#define LFS_CACHE_SIZE      256
unsigned char read_buf[LFS_CACHE_SIZE];
unsigned char prog_buf[LFS_CACHE_SIZE];
unsigned char file_cache[LFS_CACHE_SIZE];
unsigned char lookahead_buf[4];

struct lfs_config test_cfg = {
    // block device operations
    .read  = _read,
    .prog  = _prog,
    .erase = _erase,
    .sync  = _sync,

    // block device configuration
    .read_size = LFS_CACHE_SIZE,
    .prog_size = LFS_CACHE_SIZE,
    .block_size = 256,
    .block_count = 32,
    .cache_size = LFS_CACHE_SIZE,
    .lookahead_size = 4,
    .block_cycles = 100,
    .read_buffer = (void*)read_buf,
    .prog_buffer = (void*)prog_buf,
    .lookahead_buffer = (void*)lookahead_buf,
};

struct lfs_file_config test_file_cfg = {
    .buffer = (void*)file_cache,
    .attrs = NULL,
    .attr_count = 0
};

const int test_invaild = -1;    // 无效数据值
const rec_property_t test_property =
{
    .data_dir = "/test_data",   // 数据存储路径
    .invaild_data = (void *)&test_invaild,  // 无效数据值，不设置默认0
    .rec_file_limit = 4,    // 历史记录文件数量
    .bytes_of_data = 4,     // 单个数据长度
};

int startup_test(void);
int data_rec_test(void);
int discrete_test(void);
int end_test(void);
int main(void)
{
    printf("\nlittlefs test example:\n");
    if(startup_test()) return -1;
    if(data_rec_test()) return -1;
    if(discrete_test()) return -1;
    if(end_test()) return -1;
    return 0;
}

int startup_test(void)
{
    int ret;
    int disk_free;
    
    littlefs_init(&test, &test_cfg, &test_file_cfg);
    printf("startup_test: format    ");
    ret = test.format(test.lfs, &test_cfg);
    if(ret) 
    {
        printf("[FAIL]\n");
        printf("Error Code: %d\n", ret);
        return -1;
    }
    printf("[ OK ]\n");
    printf("startup_test: mount     ");
    ret = test.mount(test.lfs, &test_cfg);
    if(ret) 
    {
        printf("[FAIL]\n");
        printf("Error Code: %d\n", ret);
        return -1;
    }
    printf("[ OK ]\n");
    disk_free = test.free();
    printf("startup_test: available size = %d Bytes\n", disk_free);
    return 0;
}

int data_rec_check(int date)
{
    int i;
    int data;
    printf("\n");
    printf("data_rec_check: %d data record write.    ", date);
    for(i=0;i<100;i++)
    {
        if(data_rec.write(&i, i, date))
        {
            printf("[FAIL]\n");
            return -1;
        }
    }
    printf("[ OK ]\n");
    printf("data_rec_check: %d data record read.     ", date);
    for(i=0;i<100;i++)
    {
        if(data_rec.read(&data, i, date))
        {
            printf("[FAIL]\n");
            return -1;
        }
        if(data != i)
        {
            printf("[FAIL]\n");
            printf("data = %d i = %d\n", data, i);
            return -1;
        }
    }
    printf("[ OK ]\n");
    return 0;
}

int data_rec_test(void)
{
    int disk_free;
    char buffer[128];
    int date;
    data_rec.property = (rec_property_t *)&test_property;
    for(date = 200715; date <= 200722; date ++)
    {
        if(data_rec_check(date)) return -1;
        disk_free = test.free();
        printf("data_rec_test: free = %d Bytes\n", disk_free);
        data_rec.tree(buffer, 128);
        printf("data_rec_test: (%d)tree:\n%s", date, buffer);
    }

    if(data_rec_check(200725)) return -1;
    disk_free = test.free();
    printf("data_rec_test: free = %d Bytes\n", disk_free);
    data_rec.tree(buffer, 128);
    printf("data_rec_test: (200725)tree:\n%s", buffer);

    printf("\ndata_rec_test: clear all data file.\n");
    data_rec.clear();
    disk_free = test.free();
    printf("data_rec_test: free = %d Bytes\n", disk_free);
    data_rec.tree(buffer, 128);
    printf("data_rec_test: data tree: ");
    if(strlen(buffer) != 0)
    {
        printf("\n%s", buffer);
    }
    else printf("NULL\n");
    return 0;
}

int discrete_test(void)
{
    int buffer[5];
    int data;
    data_rec.property = (rec_property_t *)&test_property;
    data = 100;
    data_rec.write(&data, 10, 200715);
    data = 120;
    data_rec.write(&data, 11, 200715);
    data = 140;
    data_rec.write(&data, 13, 200715);
    printf("\ndiscrete_test: specil position read\n");
    data = 0;
    data_rec.read(&data, 10, 200715);
    printf("discrete_test: offset = 10 data = %d\n", data);
    data = 0;
    data_rec.read(&data, 11, 200715);
    printf("discrete_test: offset = 11 data = %d\n", data);
    data = 0;
    data_rec.read(&data, 12, 200715);
    printf("discrete_test: offset = 12 data = %d\n", data);
    data = 0;
    data_rec.read(&data, 13, 200715);
    printf("discrete_test: offset = 13 data = %d\n", data);
    data = 0;
    data_rec.read(&data, 14, 200715);
    printf("discrete_test: offset = 14 data = %d\n", data);
    printf("\ndiscrete_test: nblocks_read offset = 12\n");
    data_rec.nblocks_read(buffer, 12, 200715, 5);
    printf("buffer[0] = %d\n", buffer[0]);
    printf("buffer[1] = %d\n", buffer[1]);
    printf("buffer[2] = %d\n", buffer[2]);
    printf("buffer[3] = %d\n", buffer[3]);
    printf("buffer[4] = %d\n", buffer[4]);
    return 0;
}

int end_test(void)
{
    int disk_free;
    test.unmount(test.lfs);
    test.format(test.lfs, &test_cfg);
    test.mount(test.lfs, &test_cfg);
    disk_free = test.free();
    printf("\nend_test: available size = %d Bytes\n", disk_free);
    return 0;
}



