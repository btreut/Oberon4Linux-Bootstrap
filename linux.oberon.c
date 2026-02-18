/*------------------------------------------------------
 * Oberon Boot File Loader RC modernized for Linux with the help of ChatGPT
 * 32-bit Bootfiles, executable heap, tested 16.02.2026 BdT
 * based on RLi version 1.7.02, dated 03.11.99
 *
 * revision history:
 *
 * derived from HP and Windows Boot Loader
 * MAD, 23.05.94
 * PR,  01.02.95  support for sockets added
 * PR,  05.02.95  support for V24 added
 * PR,  23.12.95  migration to shared ELF libraries
 * RLI, 22.08.96  added some math primitives
 * RLI, 27.01.97  included pixmap
 * RLI, 13.10.97  changed name of Fontmap - File
 * RLI, 03.11.99  adaptions for glibc2.1
 *
 * with the help  * of ChatGPT modified Makefile included
 *------------------------------------------------------*/

#define _LINUX_SOURCE
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <unistd.h>   // for _exit()
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>      // for executable Heap
#include <setjmp.h>
#include <math.h>
#include "oberon.xpm"

typedef void (*Proc)();
typedef int LONGINT;

typedef int32_t HEAPADR;   // all Heap-addresses strict 32-Bit

FILE *fd;
char *OBERON, *SHLPATH;
char path[4096];
char *dirs[255];
char fullname[512];
int nofdir;
char defaultpath[] = ".:/usr/local/Oberon:/usr/local/Oberon/.Fonts";
char defaultshlpath[] = "/lib /usr/lib /usr/lib/X11 /usr/lib/X386";
char mod[64] = "Oberon", cmd[64] =  "Loop";
char dispname[64] = "";
char bootname[64] = "LinuxOberon.Boot";
char geometry[64] = "";
char fontname[64] = "Font.Map";	            /* Default map file */
int debugOn = 0;
int32_t heapSize, heapAdr;
int Argc, coption;
char **Argv;


/*----------------------------------------
 * Dynamic Loader Wrappers
 *----------------------------------------*/

int dl_open(char *lib, int mode)
{
    void *handle;
    if ((handle = dlopen(lib, RTLD_NOW)) == NULL) {
        printf("Error! Could not open Library %s in mode %d\n%s\n", lib, mode, dlerror());
        exit(-1);
    }
    return (int)handle;
}

int dl_close(int handle)
{
    dlclose((void *)handle);
    return 0;
}

int dl_sym(int handle, char *symbol, int *adr)
{
    if (strcmp("dlopen", symbol) == 0) *adr = (int)dl_open;
    else if (strcmp("dlclose", symbol) == 0) *adr = (int)dl_close;
    else if (strcmp("mknod", symbol) == 0) *adr = (int)mknod;
    else if (strcmp("stat", symbol) == 0) *adr = (int)stat;
    else if (strcmp("lstat", symbol) == 0) *adr = (int)lstat;
    else if (strcmp("fstat", symbol) == 0) *adr = (int)fstat;
    else if (strcmp("heapAdr", symbol) == 0) *adr = heapAdr;
    else if (strcmp("heapSize", symbol) == 0) *adr = heapSize;
    else if (strcmp("modPtr", symbol) == 0) *adr = (int)mod;
    else if (strcmp("cmdPtr", symbol) == 0) *adr = (int)cmd;
    else if (strcmp("fontnameadr", symbol) == 0) *adr = (int)fontname;
    else if (strcmp("dispnameadr", symbol) == 0) *adr = (int)dispname;
    else if (strcmp("geometryadr", symbol) == 0) *adr = (int)geometry;
    else if (strcmp("debugOn", symbol) == 0) *adr = (int)debugOn;
    else if (strcmp("defaultFont", symbol) == 0) *adr = (int)fontname;
    else if (strcmp("coption", symbol) == 0) *adr = coption;
    else if (strcmp("argc", symbol) == 0) *adr = Argc;
    else if (strcmp("argv", symbol) == 0) *adr = (int)Argv;
    else if (strcmp("errno", symbol) == 0) *adr = (int)&errno;
    else if (strcmp("OBERON", symbol) == 0) *adr = (int)OBERON;
    else if (strcmp("SHLPATH", symbol) == 0) *adr = (int)SHLPATH;
    else if (strcmp("exit", symbol) == 0) *adr = (int)exit;
    else if (strcmp("sin", symbol) == 0) *adr = (int)sin;
    else if (strcmp("cos", symbol) == 0) *adr = (int)cos;
    else if (strcmp("log", symbol) == 0) *adr = (int)log;
    else if (strcmp("atan", symbol) == 0) *adr = (int)atan;
    else if (strcmp("exp", symbol) == 0) *adr = (int)exp;
    else if (strcmp("sqrt", symbol) == 0) *adr = (int)sqrt;
    else if (strcmp("oberonPixmap", symbol) == 0) *adr = (int)oberonPixmap;
    else {
        *adr = (int)dlsym((void *) handle, symbol);
        if (*adr == 0) { printf("symbol %s not found\n", symbol); exit(-1); }
    }
    return 0;
}


/*----------------------------------------
 * File reading primitives
 *----------------------------------------*/

int Rint() 
{
    unsigned char b[4];
    b[0] = fgetc(fd); b[1] = fgetc(fd); b[2] = fgetc(fd); b[3] = fgetc(fd);
    return *((int *) b);
}

int RNum()
{
    int n = 0, shift = 0;
    unsigned char x = fgetc(fd);
    while (x >= 128) {
        n += (x & 0x7f) << shift;
        shift += 7;
        x = fgetc(fd);
    }
    return n + (((x & 0x3f) - ((x >> 6) << 6)) << shift);
}


/*----------------------------------------
 * Relocate
 *----------------------------------------*/

void Relocate(int32_t heapAdr, int32_t shift)
{
    int len = RNum();
    while(len != 0) {
        int32_t adr = RNum() + heapAdr;
        *((int32_t *)adr) += shift;
        len--;
    }
}


/*----------------------------------------
 * Boot
 *----------------------------------------*/

void Boot()
{
    int d = 0, notfound = 1;
    int32_t fileHeapAdr, fileHeapSize, shift1;
    int32_t adr, len, dlsymAdr;
    Proc body;

    // find Bootfile 
    while ((d < nofdir) && notfound) {
        snprintf(fullname, sizeof(fullname), "%s/%s", dirs[d++], bootname);
        fd = fopen(fullname, "r");
        if (fd != NULL) notfound = 0;
    }
    if (notfound) { printf("oberon: boot file %s not found\n", bootname); exit(-1); }

    // Bootfile Header
    fileHeapAdr = Rint();
    fileHeapSize = Rint();
    if (fileHeapSize >= heapSize) { printf("oberon: heap too small\n"); exit(-1); }

    // allocate Heap mmap executable
    heapAdr = (int32_t)mmap(NULL, heapSize,
                            PROT_READ | PROT_WRITE | PROT_EXEC,
                            MAP_ANON | MAP_PRIVATE, -1, 0);
    if (heapAdr == (int32_t)MAP_FAILED) { perror("mmap"); exit(-1); }

    // zero fill Heap 
    memset((void*)heapAdr, 0, fileHeapSize + 32);

    shift1 = heapAdr - fileHeapAdr;

    // load datd & code
    adr = Rint();
    len = Rint();
    while(len != 0) {
        adr += shift1;
        int32_t end = adr + len;
        while(adr != end) {
            *((int32_t *)adr) = Rint();
            adr += 4;
        }
        adr = Rint();
        len = Rint();
    }

    body = (Proc)(adr + shift1);

    // Relocate pointers
    Relocate(heapAdr, shift1);

    dlsymAdr = Rint();
    *((int32_t *)(heapAdr + dlsymAdr)) = (int32_t)dl_sym;

    fclose(fd);

    // transfer to oberon boot code
    (*body)();
}


/*----------------------------------------
 * InitPath
 *----------------------------------------*/

void InitPath()
{
    int pos = 0;
    char ch;

    if ((OBERON = getenv("OBERON")) == NULL) OBERON = defaultpath;
    strcpy(path, OBERON);

    nofdir = 0;
    ch = path[pos++];
    while(ch != '\0') {
        while((ch == ' ') || (ch == ':')) ch = path[pos++];
        dirs[nofdir] = &path[pos-1];
        while((ch > ' ') && (ch != ':')) ch = path[pos++];
        path[pos-1] = '\0';
        nofdir++;
    }

    if ((SHLPATH = getenv("SHLPATH")) == NULL) SHLPATH = defaultshlpath;
}


/*----------------------------------------
 * doexit
 *----------------------------------------*/

void doexit(int ret, void *arg)
{
    _exit(ret);
}


/*----------------------------------------
 * main
 *----------------------------------------*/

int main(int argc, char *argv[])
{
    int c;

    on_exit(doexit, NULL);

    heapSize = 4; Argc = argc; Argv = argv; coption = 0;

    while (--argc > 0 && (*++argv)[0] == '-') {
        c = *++argv[0];
        switch(c) {
            case 'h':
                if (--argc > 0) { sscanf(*++argv, "%d", &heapSize); if(heapSize<1) heapSize=1; }
                break;
            case 'x':
                if((argc -= 2) > 0) { sscanf(*++argv, "%s", mod); sscanf(*++argv, "%s", cmd); }
                break;
            case 'b':
                if(--argc > 0) { sscanf(*++argv, "%s", bootname); }
                break;
            case 'f':
                if(--argc > 0) { sscanf(*++argv, "%s", fontname); }
                break;
            case 'd':
                if(--argc > 0) { sscanf(*++argv, "%s", dispname); }
                break;
            case 'g':
                if(--argc > 0) { sscanf(*++argv, "%s", geometry); }
                break;
            case 'c':
                coption = 1;
                break;
            default:
                printf("oberon: illegal option %c\n", c);
                argc = -1;
                break;
        }
    }

    if(argc != 0) {
        printf("Usage: oberon [-h heapsizeinMB] [-x module command] [-b bootfile]\n");
        printf("              [-f fontmapfile] [-d displayname] [-g geometry] [-c]\n");
        exit(-1);
    }

    // allocate Heap
    heapSize *= 0x100000; // MB → Bytes

    // heapAdr = (int32_t)malloc(heapSize); // nnot useed anymore, 
    // will be allocated in Boot() via mmap 

    InitPath();
    Boot();

    return 0;
}