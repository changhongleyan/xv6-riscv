// open file
#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_RDFIFO  0x004
#define O_WRFIFO  0x008
#define O_CREATE  0x200
#define O_TRUNC   0x400
// memory map
#define PROT_NONE   0x0
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

#define MAP_SHARED  0x1
#define MAP_PRIVATE 0x2
#define MAP_ANON    0x4

#define IPC_CREATE  0x100