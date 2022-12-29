
#ifndef STATUS_H
#define STATUS_H

#define M_ALL_OK 0
#define EIO 1
#define EINVARG 2
#define ENOMEM 3
#define EBADPATH 4
#define EFSNOTUS 5
#define ERDONLY 6
#define EUNIMP 7
#define EISTKN 8
#define EINFORMAT 9

#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)

#endif
