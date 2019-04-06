#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#ifdef _MSC_VER
#define ICOMP_API(type) __declspec(noinline) type
#endif

#include "isdecomp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT    0x600
#include <windows.h>

#include <direct.h>
#include <io.h>
#include <share.h>

#define MKDIR(x)        _mkdir((x))
#define OPEN(x,y,...)   _open((x), (y), __VA_ARGS__)
#define READ(x,y,z)     _read((x), (y), (z))
#define WRITE(x,y,z)    _write((x), (y), (z))
#define LSEEK(x,y,z)    _lseek((x), (y), (z))
#define COMMIT(x)       _commit((x))
#define CLOSE(x)        _close((x))

#else
#include <unistd.h>
#include <utime.h>

#define MKDIR(x)        mkdir((x), 0775)
#define OPEN(x,y,...)   open((x), (y), __VA_ARGS__)
#define READ(x,y,z)     read((x), (y), (z))
#define WRITE(x,y,z)    write((x), (y), (z))
#define LSEEK(x,y,z)    lseek((x), (y), (z))
#define COMMIT(x)       /* no equivalent?? */
#define CLOSE(x)        close((x))
#define O_BINARY        0   /* ignore */
#define UNREFERENCED_PARAMETER(x) ((x) = (x))
#define __in
#define __inout
#define __in_opt

#endif


int wildmatch_worker(__in const char* str, __in const char* pat)
{
    for (; *pat; pat++) {
        switch (*pat) {
        case '?':
            str++;
            break;

        case '*':
            for (pat++; *str; str++) {
                if (wildmatch_worker(str, pat)) {
                    return 1;
                }
            }
            return *pat == '\0';

        default:
            if (tolower(*str) != tolower(*pat)) {
                return 0;
            }
            str++;
            break;
        }
    }
    return *str == '\0';
}

// compare with wildcard chars ('*' and '?')
int wildmatch(__in const char* str, __in const char* pat)
{
    // handle special cases
    // FIXME: treat "folder/*.*" as "folder/*"
    if (!*pat || strcmp(pat, "*") == 0 || strcmp(pat, "*.*") == 0) {
        return 1;
    }
    return wildmatch_worker(str, pat);
}

// convert '\' to '/'
// remove ".." to prevent directory traversal
void pathsanitise(__inout char* path)
{
    int squash = 1;
    char* dst = path;
    const char* src = path;
    for (; *src == '\\' || *src == '/'; src++);
    for (; *src; ) {
        if (squash) {
            const char* fetch = src;
            do {
                for (; *fetch == '.'; fetch++);
                if (!*fetch) {
                    break;          // "\.."
                }
                if (*fetch == '\\' || *fetch == '/') {
                    for (fetch++; *fetch == '\\' || *fetch == '/'; fetch++);
                    src = fetch;    // "..\.."
                } else {
                    squash = 0;     // "..a"
                    break;
                }
            } while (*fetch == '.');
            if (squash) {
                src = fetch;
                squash = 0;
            }
        } else if (*src == '\\' || *src == '/') {
            for (; *src == '\\' || *src == '/'; src++);
            *dst++ = '/';   // always use slash
            squash = 1;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = 0;
}

void* f_alloc(unsigned int cb)
{
    return malloc(cb);
}

void f_free(void* p)
{
    free(p);
}

int32_t f_seek(void* ctx, int32_t off, int type)
{
    return LSEEK((int)(intptr_t)ctx, off, type);
}

unsigned int f_read(void* ctx, void* p, unsigned int cb)
{
    return READ((int)(intptr_t)ctx, p, cb);
}

typedef struct WRITE_CONTEXT
{
    int fh;
    char file[256];
} WRITE_CONTEXT;

unsigned int f_write(void* user, const void* buf, unsigned int cb)
{
    WRITE_CONTEXT* ctx = (WRITE_CONTEXT*)user;
    if (ctx->fh == -1) {
        return (int)cb;
    }
    return WRITE(ctx->fh, buf, cb);
}

HICOMP f_next(void* user, unsigned int seq)
{
    int fh;
    ICOMP_READER fns;
    WRITE_CONTEXT* ctx = (WRITE_CONTEXT*)user;
    char* p;
    printf("(>>#%u)", seq);
    if (*ctx->file == 0) {
        return NULL;
    }
    p = strrchr(ctx->file, '.');
    if (p == NULL) {
        return NULL;
    }
    sprintf(p, ".%u", seq);

    fh = OPEN(ctx->file, O_BINARY | O_RDONLY, S_IREAD);
    if (fh == -1) {
        return NULL;
    }

    fns.allocFn = f_alloc;
    fns.freeFn = f_free;
    fns.readFn = f_read;
    fns.seekFn = f_seek;

    return IcompOpen((void*)(intptr_t)fh, &fns);
}

void FAIL(__in const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    exit(1);
}

#define TIMEFMT     "%04u-%02u-%02u %02u:%02u:%02u"
#define TIME(t)     (t).year, (t).month, (t).day, (t).hour, (t).minute, (t).second

enum MODE
{
    M_LIST,
    M_EXTRACT,
    M_TEST
};

int wildmatches(const char* str, __in_opt char** pats)
{
    int i;
    if (pats) {
        for (i = 0; pats[i] && !wildmatch(str, pats[i]); i++);
        return !!pats[i];
    }
    return 1;
}

char* strcpyx(char* dst, const char* src)
{
    for (; *src; *dst++ = *src++);
    *dst = 0;
    return dst;
}

void makedirs(__inout char* path)
{
    const char* dir = path;
    for (; *path; path++) {
        if (*path == '/') {
            *path = 0;
            MKDIR(dir);
            *path = '/';
        }
    }
}

void pathcombine(char* path, const char* dir, const char* file)
{
    char* p = path;
    if (*dir) {
        p = strcpyx(path, dir);
        pathsanitise(path);
        if (p > path && p[-1] != '/') {
            *p++ = '/';
        }
    }
    strcpyx(p, file);
    pathsanitise(p);
}

void list(HICOMP hicomp, __in const char* fn, __in_opt const char* outdir, __in_opt char** pats)
{
    ICOMP_INFO info;
    ICOMP_FILE file;
    ICOMP_DIR dir;
    char name[ICOMP_CCHMAX_NAME * 2];
    int ret;

    ret = IcompGetInfo(hicomp, &info);
    if (ret == ICOMP_S_OK) {
        printf(
            "   Date      Time    Attr         Size   Compressed  Name\n"
            "------------------- ----- ------------ ------------  ------------------------\n");

        ret = IcompFirstDir(hicomp);
        if (ret == ICOMP_S_OK) {
            do {
                ICOMP_DIR dir;
                ret = IcompGetCurDirInfo(hicomp, &dir);
                if (ret == ICOMP_S_OK) {
                    strcpy(name, dir.name);
                    pathsanitise(name);
                    printf(TIMEFMT " D....            0            0  %s\n", TIME(info.time), *name ? name : ".");
                } else {
                    FAIL("cannot get dir info: %d\n", ret);
                }
            } while ((ret = IcompNextDir(hicomp)) == ICOMP_S_OK);
            if (ret != ICOMP_S_ENDOFLIST) {
                FAIL("cannot move to next dir: %d\n", ret);
            }
        } else {
            FAIL("cannot move to first dir: %d\n", ret);
        }

        ret = IcompFirstFile(hicomp);
        if (ret == ICOMP_S_OK) {
            do {
                ret = IcompGetCurFileInfo(hicomp, &file, &dir);
                if (ret == ICOMP_S_OK) {
                    if (!wildmatches(file.name, pats)) {
                        continue;
                    }
                    pathcombine(name, dir.name, file.name);
#define ATTR(b,t)       ((file.attributes & (b)) ? (t) : '.')
                    printf(TIMEFMT " .%c%c%c%c %12u %12u  %s\n",
                        TIME(file.time),
                        ATTR(1, 'R'),
                        ATTR(2, 'H'),
                        ATTR(4, 'S'),
                        ATTR(0x20, 'A'),
                        file.size, file.compSize,
                        name);
#undef ATTR
                } else {
                    FAIL("cannot get file info: %d\n", ret);
                }
            } while ((ret = IcompNextFile(hicomp)) == ICOMP_S_OK);
            if (ret != ICOMP_S_ENDOFLIST) {
                FAIL("cannot move to next file: %d\n", ret);
            }
        } else {
            FAIL("cannot move to first file: %d\n", ret);
        }

        printf(
            "------------------- ----- ------------ ------------  ------------------------\n");
        printf(TIMEFMT "       %12u %12u  %u files, %u folders\n",
            TIME(info.time),
            info.totalSize, info.archiveSize,
            info.files, info.directories);
    } else {
        FAIL("cannot get info: %d\n", ret);
    }
    UNREFERENCED_PARAMETER(fn);
    UNREFERENCED_PARAMETER(outdir);
}

void setfileinfo(const char* filename, const ICOMP_TIME* time, unsigned int attributes)
{
#ifdef _WIN32
    SYSTEMTIME loc, utc;
    FILETIME ft;
    HANDLE hfile;
    memset(&loc, 0, sizeof(loc));
    memset(&ft, 0, sizeof(ft));
    loc.wYear   = (WORD)time->year;
    loc.wMonth  = (WORD)time->month;
    loc.wDay    = (WORD)time->day;
    loc.wHour   = (WORD)time->hour;
    loc.wMinute = (WORD)time->minute;
    loc.wSecond = (WORD)time->second;
    if (TzSpecificLocalTimeToSystemTime(NULL, &loc, &utc)) {
        if (SystemTimeToFileTime(&utc, &ft)) {
            hfile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hfile != INVALID_HANDLE_VALUE) {
                SetFileTime(hfile, NULL, NULL, &ft);
                CloseHandle(hfile);
            }
        }
    }
    SetFileAttributesA(filename, attributes & 0x27);
#else
    struct utimbuf utmb;
    struct tm tm;
    time_t t;
    tm.tm_year  = time->year - 1900;
    tm.tm_mon   = time->month - 1;
    tm.tm_mday  = time->day;
    tm.tm_hour  = time->hour;
    tm.tm_min   = time->minute;
    tm.tm_sec   = time->second;
    tm.tm_isdst = -1;
    t = mktime(&tm);
    if (t != -1) {
        utmb.actime = utmb.modtime = t;
        utime(filename, &utmb);
    }
    chmod(filename, (attributes & 1) ? 0444 : 0664);
#endif
}

void extratest(int extract, HICOMP hicomp, __in const char* fn, __in_opt const char* outdir, __in_opt char** pats)
{
    ICOMP_WRITER fns;
    WRITE_CONTEXT ctx;
    unsigned int success = 0, fail = 0;
    char* path = (char*)malloc(strlen(outdir) + 2 + ICOMP_CCHMAX_NAME * 2);
    int ret = path != NULL ? ICOMP_S_OK : ICOMP_E_OUTOFMEM;
    if (ret == ICOMP_S_OK) {
        ret = IcompFirstFile(hicomp);
    }
    fns.writeFn = f_write;
    fns.nextFn = f_next;
    if (ret == ICOMP_S_OK) {
        do {
            ICOMP_FILE file;
            ICOMP_DIR dir;
            ret = IcompGetCurFileInfo(hicomp, &file, &dir);
            if (ret == ICOMP_S_OK) {
                char name[ICOMP_CCHMAX_NAME * 2];
                int fh = -1;
                if (file.flags & ICOMP_FILE_NOTAVAIL) {
                    continue;
                }
                if (!wildmatches(file.name, pats)) {
                    continue;
                }
                pathcombine(name, dir.name, file.name);
                if (extract) {
                    pathcombine(path, outdir, name);
                    makedirs(path);
                    fh = OPEN(path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE);
                    if (fh == -1) {
                        FAIL("cannot create file: \"%s\"\n", path);
                    }
                }
                printf("%sing: \"%s\" ", extract ? "Extract" : "Test", name);
                ctx.fh = fh;
                if (strlen(fn) < sizeof(ctx.file) - 16) {
                    strcpy(ctx.file, fn);
                } else {
                    *ctx.file = 0;
                }
                ret = IcompCopyCurFile(hicomp, &ctx, &fns);
                if (fh != -1) {
                    COMMIT(fh);
                    CLOSE(fh);
                    setfileinfo(path, &file.time, file.attributes);
                }
                if (ret == ICOMP_S_OK) {
                    printf("[OK]\n");
                    success++;
                } else {
                    printf("[FAIL: %d]\n", ret);
                    fail++;
                }
            } else {
                FAIL("cannot get file info: %d\n", ret);
            }
        } while ((ret = IcompNextFile(hicomp)) == ICOMP_S_OK);
        if (ret != ICOMP_S_ENDOFLIST) {
            FAIL("cannot move to next file: %d\n", ret);
        }
    } else {
        FAIL("cannot move to first file: %d\n", ret);
    }
    free(path);
    printf("%u files %s, %u succeeded, %u failed.\n", success + fail, extract ? "extracted" : "tested", success, fail);
}

void extract(HICOMP hicomp, __in const char* fn, __in_opt const char* outdir, __in_opt char** pats)
{
    extratest(1, hicomp, fn, outdir, pats);
}

void test(HICOMP hicomp, __in const char* fn, __in_opt const char* outdir, __in_opt char** pats)
{
    extratest(0, hicomp, fn, outdir, pats);
}

#undef CDECL

#ifdef _MSC_VER
#define CDECL           __cdecl
#else
#define CDECL
#endif

int CDECL main(int argc, char** argv)
{
    typedef void (*CMD)(HICOMP hicomp, __in const char* file, __in_opt const char* outdir, __in_opt char** pats);
    static const CMD cmds[] = { list, extract, test };
    enum MODE mode = M_LIST;

    int i, fh;
    ICOMP_READER fns;
    HICOMP hicomp;
    const char* dir = "";

    printf("InstallShield 3 Archive Extractor Version 1.01\n");

    for (i = 1; i < argc && argv[i][0] == '-'; i++) {
        char ch = argv[i][1];
        if (ch == '-') {
            i++;
            break;
        }
        switch (ch | 0x20) {
        case 'l': mode = M_LIST; break;
        case 'x': mode = M_EXTRACT; break;
        case 't': mode = M_TEST; break;
        case 'd':
            dir = argv[i] + 2;
            if (*dir == 0 && i < argc) {
                i++;
                dir = argv[i];
            }
            break;

        default: FAIL("unknown option: %s\n", argv[i]);
        }
    }

    if (i >= argc) {
        printf("%s [-l|-x|-t] [-d DIR] data.z [files...]\n", argv[0]);
        return 1;
    }

    fh = OPEN(argv[i], O_BINARY | O_RDONLY, S_IREAD);
    if (fh == -1) {
        FAIL("cannot open file: %s\n", argv[i]);
    }

    fns.allocFn = f_alloc;
    fns.freeFn = f_free;
    fns.readFn = f_read;
    fns.seekFn = f_seek;

    hicomp = IcompOpen((void*)(intptr_t)fh, &fns);
    if (hicomp == NULL) {
        FAIL("invalid file format or out of memory\n");
    }

    cmds[mode](hicomp, argv[i], dir, i + 1 < argc ? argv + i + 1 : NULL);

    IcompClose(hicomp);
    CLOSE(fh);

    return 0;
}
