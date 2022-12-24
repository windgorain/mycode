#ifndef _BPF_MAP_H
#define _BPF_MAP_H

#include "bs.h"

typedef struct {
    char filename[256]; /* pinning file name */
    unsigned int type;
    unsigned int key_size;
    unsigned int value_size;
    unsigned int max_elem;
    unsigned int flags;
    unsigned int id;
    unsigned int pinning;
}BPF_MAP_PARAM_S;

int BPF_CreateMap(IN BPF_MAP_PARAM_S *p);
int BPF_NewMap(char *filename, int type, int key_size, int val_size, int max_elements, int flags);
void BPF_DelMap(int fd);
int BPF_PinMap(int fd, char *filename);

int BPF_LookupMapEle(int fd, void *key, void *val);
int BPF_DelMapEle(int fd, void *key);
int BPF_UpdateMapEle(int fd, void *key, void *val, int flags);
int BPF_GetNextMapKey(int fd, const void *key, void *next_key);

#endif
