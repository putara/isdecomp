#ifndef ISDECOMP_HEADER
#define ISDECOMP_HEADER

#ifdef _MSC_VER
#include <specstrings.h>
#else
#define ISDECOMP_DUMMY_SAL_DEFINED
#define __in
#define __out
#define __inout
#define __in_opt
#define __out_opt
#define __inout_opt
#define __in_bcount(x)
#define __out_ecount(x)
#define __out_bcount_part(x, y)
#define __deref_out
#endif


#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#ifndef ICOMP_API
#define ICOMP_API(type)         type
#endif

#define ICOMP_S_OK              0
#define ICOMP_S_ENDOFLIST       2
#define ICOMP_E_READ            -1
#define ICOMP_E_WRITE           -2
#define ICOMP_E_INVALIDARG      -3
#define ICOMP_E_OUTOFMEM        -4
#define ICOMP_E_BADFORMAT       -5
#define ICOMP_E_NOTAVAIL        -6
#define ICOMP_E_ABORT           -7
#define ICOMP_E_UNEXPECTED      -33
#define ICOMP_E_UNIMPL          -42

#define ICOMP_SEEK_SET          0
#define ICOMP_SEEK_CUR          1
#define ICOMP_SEEK_END          2

#define ICOMP_CCHMAX_NAME       256

#define ICOMP_ALL               0

// ICOMP_FILE::flags
#define ICOMP_FILE_AVAIL        0x00
#define ICOMP_FILE_NOTAVAIL     0x01
#define ICOMP_FILE_PARTIAL      0x02


typedef struct tagICOMP_HANDLE *HICOMP;

typedef struct tagICOMP_TIME
{
    unsigned int    year;   // 1980-2107
    unsigned int    month;  // 1-12
    unsigned int    day;    // 1-31
    unsigned int    hour;   // 0-23
    unsigned int    minute; // 0-59
    unsigned int    second; // 0-59
} ICOMP_TIME;

typedef struct tagICOMP_INFO
{
    unsigned int    directories;
    unsigned int    files;
    uint32_t        archiveSize;
    uint32_t        totalSize;
    ICOMP_TIME      time;
} ICOMP_INFO;

typedef struct tagICOMP_DIR
{
    int32_t             id;
    unsigned int        files;
    char                name[ICOMP_CCHMAX_NAME];
} ICOMP_DIR;

typedef struct tagICOMP_FILE
{
    int32_t             id;
    int32_t             dirId;
    uint8_t             attributes;
    uint32_t            size;
    uint32_t            compSize;
    unsigned int        flags;
    ICOMP_TIME          time;
    char                name[ICOMP_CCHMAX_NAME];
} ICOMP_FILE;

typedef struct tagICOMP_READER
{
    void* (*allocFn)(unsigned int cb);
    void (*freeFn)(__inout_opt void* p);
    unsigned int (*readFn)(void* context, __out_bcount_part(cb, return) void* buf, unsigned int cb);
    int32_t (*seekFn)(void* context, int32_t off, int type);
} ICOMP_READER;

typedef struct tagICOMP_WRITER
{
    unsigned int (*writeFn)(void* user, __in_bcount(cb) const void* buf, unsigned int cb);
    HICOMP (*nextFn)(void* user, unsigned int seq);
} ICOMP_WRITER;


ICOMP_API(HICOMP) IcompOpen(__in_opt void* context, __in const ICOMP_READER* fns);
ICOMP_API(int) IcompClose(__inout_opt HICOMP hicomp);
ICOMP_API(int) IcompGetInfo(__inout HICOMP hicomp, __out ICOMP_INFO* info);

ICOMP_API(int) IcompFirstDir(__inout HICOMP hicomp);
ICOMP_API(int) IcompNextDir(__inout HICOMP hicomp);
ICOMP_API(int) IcompGetCurDirInfo(__inout HICOMP hicomp, __out ICOMP_DIR* dir);

ICOMP_API(int) IcompFirstFile(__inout HICOMP hicomp);
ICOMP_API(int) IcompNextFile(__inout HICOMP hicomp);
ICOMP_API(int) IcompGetCurFileInfo(__inout HICOMP hicomp, __out ICOMP_FILE* file, __out_opt ICOMP_DIR* dir);

ICOMP_API(int) IcompCopyCurFile(__inout HICOMP hicomp, __in_opt void* user, __in const ICOMP_WRITER* fns);

#endif  // ISDECOMP_HEADER


#ifdef ISDECOMP_BUILD_LIB

#if 0

// * tested with archive files created with i3comp.exe.
//   - InstallShield File Compressor
//   - Version 3.00.062 for Microsoft Windows 95
//   - Copyright(c) 1990-1995 Stirling Technologies, Inc. All Rights Reserved.
//
// * i3comp.exe can be found in Universal Extractor.
// * i3comp has some problems:
//   - stops with a blank message box if it sees >= 16000 files.
//   - crashes if a full path is too long (>= 239 characters including the terminating null)


struct header
{
    uint8_t     signature[8];       // +00: 13 5D 65 8C 3A 01 02 00
    uint16_t    unknown1;           // +08: ??? always zero?
    uint16_t    flags;              // +0a: 01 = split files
    uint16_t    files;              // +0c: number of files
    uint32_t    datetime;           // +0e: ms-dos date/time
    uint32_t    archiveSize;        // +12: size of all archive files
    uint32_t    totalSize;          // +16: total size of files
    uint32_t    unknown2;           // +1a: ???
    uint8_t     splitFiles;         // +1e: number of split files
    uint8_t     curSeq;             // +1f: current sequence #
    uint8_t     unknown3;           // +20: ??? some flags?
    uint32_t    unknown4;           // +21: ???
    uint32_t    unknown5;           // +25: ???
    uint32_t    dirOffset;          // +29: offset to the first directory entry
    uint32_t    unknown6;           // +2d: ???
    uint16_t    directories;        // +31: number of directories
    uint32_t    fileOffset;         // +33: offset to the first file entry
    uint32_t    fileEntSize;        // +37: size of file entries
    uint32_t    curArcSize;         // +3b: size of current archive (for split files)
};

struct directory_entry
{
    uint16_t    files;              // +00: number of files
    uint16_t    hdrSize;            // +02: size of this entry
    uint8_t     nameLength;         // +04: length of the name
    uint8_t     unknown1;           // +05: ??? always zero?
    char        name[nameLength];   // +06: name
};

struct file_entry
{
    uint8_t     seqLast;            // +00: last seq# of split file
    uint16_t    directory;          // +01: zero-based directory index
    uint32_t    size;               // +03: file size
    uint32_t    compSize;           // +07: compressed size
    uint32_t    compOffset;         // +0b: compressed offset
    uint32_t    datetime;           // +0f: ms-dos date/time
    uint8_t     attributes;         // +13: ms-dos file attributes
    uint8_t     unknown1[3];        // +14: ??? always zero?
    uint16_t    hdrSize;            // +17: size of this entry
    uint8_t     unknown2;           // +19: ??? always zero?
    uint8_t     partial;            // +1a: 01 = file crosses archive files
    uint8_t     unknown3;           // +1b: ??? always zero?
    uint8_t     seqFirst;           // +1c: first seq# of split file
    uint8_t     nameLength;         // +1d: length of the name
    char        name[nameLength];   // +1e: name
};

#endif


typedef struct tagICOMP_CUR_DIR
{
    uint32_t        index;
    uint32_t        numFiles;
    uint32_t        offNext;
    unsigned int    cchName;
    uint32_t        offName;
} ICOMP_CUR_DIR;

typedef struct tagICOMP_CUR_FILE
{
    uint32_t        index;
    unsigned int    indexDir;
    uint32_t        offNext;
    uint32_t        offComp;
    uint8_t         attributes;
    uint32_t        size;
    uint32_t        compSize;
    uint32_t        datetime;
    unsigned int    cchName;
    uint32_t        offName;
    int             partial;
    unsigned int    seqFirst;
    unsigned int    seqLast;
} ICOMP_CUR_FILE;

typedef struct tagICOMP_HANDLE
{
    ICOMP_READER    fns;
    void*           context;
    unsigned int    numFiles;
    uint32_t        datetime;
    uint32_t        archiveSize;
    uint32_t        totalSize;
    uint32_t        offDirHdr;
    unsigned int    numDirs;
    uint32_t        offFileHdr;
    int             split;
    unsigned int    numSplit;
    unsigned int    curSeq;
    ICOMP_CUR_DIR   curDir;
    ICOMP_CUR_FILE  curFile;
} ICOMP_HANDLE;

typedef struct tagICOMP_BLAST
{
    ICOMP_WRITER    fns;
    void*           user;
    HICOMP          hicomp;
    int             partial;
    int             abort;
    unsigned int    remains;
    unsigned int    curRemains;
    unsigned char   buf[1024];
} ICOMP_BLAST;

#include "blast.h"


#ifdef _MSC_VER
#define ICOMP_INLINE                __inline
#else
#define ICOMP_INLINE                inline
#endif

#define ICOMP_CB_HEADER             0xff
#define ICOMP_CBMIN_DIRENTRY        6
#define ICOMP_CBMIN_FILEENTRY       0x1e

#define ICOMP_MIN(x, y)             ((x) < (y) ? (x) : (y))
#define ICOMP_MAX(x, y)             ((x) > (y) ? (x) : (y))

#define ICOMP_ALLOC(hi, cb)         (((hi)->fns.allocFn)((cb)))
#define ICOMP_FREE(hi, p)           (((hi)->fns.freeFn)((p)))
#define ICOMP_READN(hi, buf, cb)    (((hi)->fns.readFn)((hi)->context, (buf), (cb)))
#define ICOMP_READ(hi, buf, cb)     (ICOMP_READN((hi), (buf), (cb)) == (cb) ? ICOMP_S_OK : ICOMP_E_READ)
#define ICOMP_SEEK(hi, off, type)   (((hi)->fns.seekFn)((hi)->context, (off), (type)))

#define ICOMP_GET8(src, off)        ((src)[(off)])
#define ICOMP_GET16(src, off)       (ICOMP_GET8((src), (off)) | ICOMP_GET8((src), (off) + 1) << 8)
#define ICOMP_GET32(src, off)       (ICOMP_GET16((src), (off)) | (ICOMP_GET16((src), (off) + 2) << 16))

static int IcomppDosTimeToTime(uint32_t datetime, __out ICOMP_TIME* tm)
{
    // TODO: strict validation
    tm->year = 1980 + ((datetime >> 9) & 0x7f);
    tm->month = (datetime >> 5) & 0xf;
    tm->day = datetime & 0x1f;
    tm->hour = (datetime >> 27);
    tm->minute = (datetime >> 21) & 0x3f;
    tm->second = ((datetime >> 16) & 0x1f) * 2;
    return ICOMP_S_OK;
}

static int IcomppSeek(__inout HICOMP hicomp, uint32_t offset)
{
    if (offset > INT32_MAX) {
        return ICOMP_E_READ;
    }
    if (ICOMP_SEEK(hicomp, (int32_t)(offset), ICOMP_SEEK_SET) != (int32_t)(offset)) {
        return ICOMP_E_READ;
    }
    return ICOMP_S_OK;
}

static int IcomppReadHeader(HICOMP hicomp)
{
    static const uint8_t c_signature[] = { 0x13, 0x5D, 0x65, 0x8C, 0x3A, 0x01, 0x02, 0x00 };
    uint8_t buf[ICOMP_CB_HEADER];  // header is 255 bytes including padding

    int ret = IcomppSeek(hicomp, 0);
    if (ret == ICOMP_S_OK) {
        ret = ICOMP_READ(hicomp, buf, sizeof(buf));
    }
    if (ret == ICOMP_S_OK) {
        ret = memcmp(buf, c_signature, sizeof(c_signature)) == 0 ? ICOMP_S_OK : ICOMP_E_BADFORMAT;
    }
    if (ret == ICOMP_S_OK) {
        hicomp->numFiles            = ICOMP_GET16(buf, 0x0c);
        hicomp->datetime            = ICOMP_GET32(buf, 0x0e);
        hicomp->archiveSize         = ICOMP_GET32(buf, 0x12);
        hicomp->totalSize           = ICOMP_GET32(buf, 0x16);
        hicomp->offDirHdr           = ICOMP_GET32(buf, 0x29);
        hicomp->numDirs             = ICOMP_GET16(buf, 0x31);
        hicomp->offFileHdr          = ICOMP_GET32(buf, 0x33);
        // limit to 2GB. I don't think ICOMP can handle more than that.
        ret = ((hicomp->archiveSize | hicomp->totalSize) <= INT32_MAX
            && (hicomp->offDirHdr < hicomp->archiveSize)
            && (hicomp->offFileHdr < hicomp->archiveSize))
            ? ICOMP_S_OK : ICOMP_E_BADFORMAT;
    }
    if (ret == ICOMP_S_OK) {
        unsigned int flags = ICOMP_GET16(buf, 0x0a);
        if (flags & 1) {
            // ex) split file
            // name     num     seq
            // data.1     3       1
            // data.2     0       2
            // data.3     0       3
            hicomp->split           = 1;
            hicomp->numSplit        = ICOMP_GET8(buf, 0x1e);
            hicomp->curSeq          = ICOMP_GET8(buf, 0x1f);
            ret = (hicomp->curSeq > 0
                && (hicomp->numSplit == 0 || hicomp->curSeq <= hicomp->numSplit))
                ? ICOMP_S_OK : ICOMP_E_BADFORMAT;
        } else {
            hicomp->split           = 0;
            hicomp->numSplit        = 0;
            hicomp->curSeq          = 0;
        }
    }
    return ret;
}

static HICOMP IcomppNew(__in_opt void* context, __in const ICOMP_READER* fns)
{
    HICOMP hicomp = (HICOMP)(fns->allocFn(sizeof(ICOMP_HANDLE)));
    if (hicomp == NULL) {
        return NULL;
    }
    memset(hicomp, 0, sizeof(ICOMP_HANDLE));
    hicomp->fns             = *fns;
    hicomp->context         = context;
    hicomp->curDir.index    = UINT32_MAX;
    hicomp->curFile.index   = UINT32_MAX;
    if (IcomppReadHeader(hicomp) == ICOMP_S_OK) {
        return hicomp;
    }
    fns->freeFn(hicomp);
    return NULL;
}

ICOMP_API(HICOMP) IcompOpen(__in_opt void* context, __in const ICOMP_READER* fns)
{
    if (fns == NULL || fns->allocFn == NULL || fns->freeFn == NULL || fns->seekFn == NULL || fns->readFn == NULL) {
        return NULL;
    }
    return IcomppNew(context, fns);
}

ICOMP_API(int) IcompClose(__inout_opt HICOMP hicomp)
{
    if (hicomp == NULL) {
        return ICOMP_E_INVALIDARG;
    }
#ifdef _DEBUG
    memset(hicomp, 0xdd, sizeof(ICOMP_HANDLE));
#endif
    ICOMP_FREE(hicomp, hicomp);
    return ICOMP_S_OK;
}

ICOMP_API(int) IcompGetInfo(__inout HICOMP hicomp, __out ICOMP_INFO* info)
{
    if (hicomp == NULL || info == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    info->directories   = hicomp->numDirs;
    info->files         = hicomp->numFiles;
    info->archiveSize   = hicomp->archiveSize;
    info->totalSize     = hicomp->totalSize;
    return IcomppDosTimeToTime(hicomp->datetime, &info->time);
}

static int IcomppReadString(__inout HICOMP hicomp, uint32_t offset, __out_ecount(cch + 1) char* buf, unsigned int cch)
{
    int ret = (offset > 0) ? ICOMP_S_OK : ICOMP_E_READ;
    if (ret == ICOMP_S_OK) {
        if (cch > 0) {
            ret = IcomppSeek(hicomp, offset);
            if (ret == ICOMP_S_OK) {
                ret = ICOMP_READ(hicomp, buf, cch);
            }
            if (ret != ICOMP_S_OK) {
                cch = 0;
            }
        }
    }
    buf[cch] = 0;
    return ret;
}

static int IcomppLoadDir(__inout HICOMP hicomp, unsigned int index, uint32_t offset)
{
    uint8_t buf[ICOMP_CBMIN_DIRENTRY];
    unsigned int numFiles, cbEntry;
    int ret;

    if (index == hicomp->curDir.index) {
        // already loaded
        return ICOMP_S_OK;
    }

    memset(&hicomp->curDir, 0, sizeof(hicomp->curDir));
    hicomp->curDir.index = UINT32_MAX;

    ret = (index < hicomp->numDirs) ? ICOMP_S_OK : ICOMP_S_ENDOFLIST;
    if (ret == ICOMP_S_OK) {
        ret = IcomppSeek(hicomp, offset);
    }
    if (ret == ICOMP_S_OK) {
        ret = ICOMP_READ(hicomp, buf, sizeof(buf));
    }
    if (ret == ICOMP_S_OK) {
        numFiles = ICOMP_GET16(buf, 0);
        cbEntry = ICOMP_GET16(buf, 2);
        // TODO: strict validation
        ret = (numFiles <= hicomp->numFiles
            && cbEntry >= ICOMP_CBMIN_DIRENTRY
            && offset + cbEntry <= hicomp->archiveSize)
            ? ICOMP_S_OK : ICOMP_E_BADFORMAT;
        if (ret == ICOMP_S_OK) {
            hicomp->curDir.index    = index;
            hicomp->curDir.numFiles = numFiles;
            hicomp->curDir.offNext  = offset + cbEntry;
            hicomp->curDir.cchName  = ICOMP_GET8(buf, 4);
            hicomp->curDir.offName  = offset + ICOMP_CBMIN_DIRENTRY;
        }
    }
    return ret;
}

ICOMP_API(int) IcompFirstDir(__inout HICOMP hicomp)
{
    if (hicomp == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    return IcomppLoadDir(hicomp, 0, hicomp->offDirHdr);
}

ICOMP_API(int) IcompNextDir(__inout HICOMP hicomp)
{
    if (hicomp == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    if (hicomp->curDir.offNext == 0) {
        return ICOMP_E_READ;
    }
    if (hicomp->curDir.index >= hicomp->numDirs) {
        return ICOMP_S_ENDOFLIST;
    }
    return IcomppLoadDir(hicomp, hicomp->curDir.index + 1, hicomp->curDir.offNext);
}

static int IcomppSeekDir(__inout HICOMP hicomp, unsigned int index)
{
    int ret;
    if (index == hicomp->curDir.index) {
        return ICOMP_S_OK;
    }
    ret = IcompFirstDir(hicomp);
    while (ret == ICOMP_S_OK && index != hicomp->curDir.index) {
        ret = IcompNextDir(hicomp);
    }
    return ret;
}

ICOMP_API(int) IcompGetCurDirInfo(__inout HICOMP hicomp, __out ICOMP_DIR* dir)
{
    if (hicomp == NULL || dir == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    if (hicomp->curDir.offNext == 0) {
        return ICOMP_E_READ;
    }
    dir->id = hicomp->curDir.index + 1; // +1 for ICOMP_ALL
    dir->files = hicomp->curDir.numFiles;
    return IcomppReadString(hicomp, hicomp->curDir.offName, dir->name, hicomp->curDir.cchName);
}

static int IcomppLoadFile(__inout HICOMP hicomp, unsigned int index, uint32_t offset)
{
    uint8_t buf[ICOMP_CBMIN_FILEENTRY];
    unsigned int indexDir, cbEntry, offComp;
    uint32_t cbUncomp, cbComp;
    int ret;

    if (index == hicomp->curFile.index) {
        // already loaded
        return ICOMP_S_OK;
    }

    memset(&hicomp->curFile, 0, sizeof(hicomp->curFile));
    hicomp->curFile.index = UINT32_MAX;
    hicomp->curFile.indexDir = UINT_MAX;

    ret = (index < hicomp->numFiles) ? ICOMP_S_OK : ICOMP_S_ENDOFLIST;
    if (ret == ICOMP_S_OK) {
        ret = IcomppSeek(hicomp, offset);
    }
    if (ret == ICOMP_S_OK) {
        ret = ICOMP_READ(hicomp, buf, sizeof(buf));
    }
    if (ret == ICOMP_S_OK) {
        indexDir    = ICOMP_GET16(buf, 1);
        cbUncomp    = ICOMP_GET32(buf, 3);
        cbComp      = ICOMP_GET32(buf, 7);
        offComp     = ICOMP_GET32(buf, 0xb);
        cbEntry     = ICOMP_GET16(buf, 0x17);
        // TODO: strict validation
        ret = (indexDir < hicomp->numDirs
                // && cbUncomp <= hicomp->totalSize
                && cbComp < hicomp->archiveSize
                && offComp < hicomp->archiveSize
                && offComp + cbComp <= hicomp->archiveSize
                && cbEntry >= ICOMP_CBMIN_FILEENTRY
                && offset + cbEntry <= hicomp->archiveSize)
                ? ICOMP_S_OK : ICOMP_E_BADFORMAT;
        if (ret == ICOMP_S_OK) {
            hicomp->curFile.index       = index;
            hicomp->curFile.indexDir    = indexDir;
            hicomp->curFile.offNext     = offset + cbEntry;
            hicomp->curFile.offComp     = offComp;
            hicomp->curFile.attributes  = ICOMP_GET8(buf, 0x13);
            hicomp->curFile.size        = cbUncomp;
            hicomp->curFile.compSize    = cbComp;
            hicomp->curFile.datetime    = ICOMP_GET32(buf, 0xf);
            hicomp->curFile.cchName     = ICOMP_GET8(buf, 0x1d);
            hicomp->curFile.offName     = offset + ICOMP_CBMIN_FILEENTRY;
            hicomp->curFile.partial     = ICOMP_GET8(buf, 0x1a) & 1;
            hicomp->curFile.seqFirst    = ICOMP_GET8(buf, 0x1c);
            hicomp->curFile.seqLast     = ICOMP_GET8(buf, 0);
        }
    }
    return ret;
}

ICOMP_API(int) IcompFirstFile(__inout HICOMP hicomp)
{
    if (hicomp == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    return IcomppLoadFile(hicomp, 0, hicomp->offFileHdr);
}

ICOMP_API(int) IcompNextFile(__inout HICOMP hicomp)
{
    if (hicomp == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    if (hicomp->curFile.offNext == 0) {
        return ICOMP_E_READ;
    }
    if (hicomp->curFile.index >= hicomp->numFiles) {
        return ICOMP_S_ENDOFLIST;
    }
    return IcomppLoadFile(hicomp, hicomp->curFile.index + 1, hicomp->curFile.offNext);
}

ICOMP_API(int) IcompGetCurFileInfo(__inout HICOMP hicomp, __out ICOMP_FILE* file, __out_opt ICOMP_DIR* dir)
{
    int ret;
    if (hicomp == NULL || file == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    if (hicomp->curFile.offNext == 0) {
        return ICOMP_E_READ;
    }

    file->id            = hicomp->curFile.index + 1;
    file->dirId         = hicomp->curFile.indexDir + 1;
    file->attributes    = hicomp->curFile.attributes;
    file->size          = hicomp->curFile.size;
    file->compSize      = hicomp->curFile.compSize;
    file->attributes    = hicomp->curFile.attributes;
    file->flags         = ICOMP_FILE_AVAIL;

    if (hicomp->split) {
        if (hicomp->curFile.seqFirst != hicomp->curSeq) {
            file->flags = ICOMP_FILE_NOTAVAIL;
        } else if (hicomp->curFile.seqLast != hicomp->curSeq) {
            file->flags = ICOMP_FILE_PARTIAL;
        }
    }

    ret = IcomppDosTimeToTime(hicomp->curFile.datetime, &file->time);
    if (ret == ICOMP_S_OK) {
        if (dir != NULL) {
            // back up curDir to prevent IcomppSeekDir from thrashing it
            ICOMP_CUR_DIR cd;
            memcpy(&cd, &hicomp->curDir, sizeof(cd));
            ret = IcomppSeekDir(hicomp, hicomp->curFile.indexDir);
            if (ret == ICOMP_S_OK) {
                ret = IcompGetCurDirInfo(hicomp, dir);
            }
            memcpy(&hicomp->curDir, &cd, sizeof(cd));
        }
    }
    if (ret == ICOMP_S_OK) {
        ret = IcomppReadString(hicomp, hicomp->curFile.offName, file->name, hicomp->curFile.cchName);
    }
    return ret;
}

static int IcomppFeedNextFile(ICOMP_BLAST* bl)
{
    const unsigned int offComp = ICOMP_CB_HEADER;
    HICOMP cur = bl->hicomp, next;
    if (bl->fns.nextFn != NULL) {
        next = bl->fns.nextFn(bl->user, cur->curSeq + 1);
        if (next != NULL
            && next->numFiles == cur->numFiles
            && next->datetime == cur->datetime
            && next->archiveSize == cur->archiveSize
            && next->totalSize == cur->totalSize
            && next->split
            && next->numSplit == 0
            && next->curSeq == cur->curSeq + 1
            && IcomppSeek(next, offComp) == ICOMP_S_OK) {
            bl->curRemains = ICOMP_MIN(bl->remains, next->offDirHdr - offComp);
            bl->hicomp = next;
            return 1;
        }
        IcompClose(next);
    }
    return 0;
}

static unsigned IcomppBlastRead(void* how, __deref_out unsigned char** buf)
{
    ICOMP_BLAST* bl = (ICOMP_BLAST*)(how);
    unsigned int cb, cbRead;
    if (bl->curRemains == 0 && bl->partial && IcomppFeedNextFile(bl) == 0) {
        bl->abort = 1;
        return 0;
    }
    cb = ICOMP_MIN(bl->curRemains, sizeof(bl->buf));
    cbRead = ICOMP_READN(bl->hicomp, bl->buf, cb);
    // should call "assert(cb >= cbRead)" here ?
    if (cb < cbRead) {
        cbRead = 0;
    }
    bl->curRemains -= cbRead;
    bl->remains -= cbRead;
    *buf = bl->buf;
    return cbRead;
}

static int IcomppBlastWrite(void* how, __in_bcount(cb) unsigned char* buf, unsigned cb)
{
    ICOMP_BLAST* bl = (ICOMP_BLAST*)(how);
    return bl->fns.writeFn(bl->user, buf, cb) == cb ? 0 : 1;
}

static int IcomppBlastToKnownResult(int blret)
{
    if (blret == 0) {
        return ICOMP_S_OK;
    }
    if (blret < 0) {
        return ICOMP_E_BADFORMAT;
    }
    switch (blret) {
    case 1:
        return ICOMP_E_WRITE;
    case 2:
        return ICOMP_E_READ;
    }
    // shouldn't be here
    return ICOMP_E_UNEXPECTED;
}

ICOMP_API(int) IcompCopyCurFile(__inout HICOMP hicomp, __in_opt void* user, __in const ICOMP_WRITER* fns)
{
    ICOMP_BLAST bl;
    int ret, blret;

    if (hicomp == NULL || fns == NULL || fns->writeFn == NULL) {
        return ICOMP_E_INVALIDARG;
    }
    if (hicomp->curFile.offNext == 0) {
        return ICOMP_E_READ;
    }
    if (hicomp->split && hicomp->curFile.seqFirst != hicomp->curSeq) {
        return ICOMP_E_NOTAVAIL;
    }

    ret = IcomppSeek(hicomp, hicomp->curFile.offComp);
    if (ret == ICOMP_S_OK) {
        bl.fns          = *fns;
        bl.user         = user;
        bl.hicomp       = hicomp;
        bl.partial      = hicomp->curFile.partial;
        bl.abort        = 0;
        bl.remains      = hicomp->curFile.compSize;
        if (hicomp->split) {
            bl.curRemains = ICOMP_MIN(bl.remains, hicomp->offDirHdr - hicomp->curFile.offComp);
        } else {
            bl.curRemains = bl.remains;
        }
        blret = blast(IcomppBlastRead, &bl, IcomppBlastWrite, &bl, NULL, NULL);
        if (bl.hicomp != hicomp) {
            IcompClose(bl.hicomp);
        }
        ret = bl.abort ? ICOMP_E_ABORT : IcomppBlastToKnownResult(blret);
    }
    return ret;
}

#undef ICOMP_INLINE
#undef ICOMP_CB_HEADER
#undef ICOMP_CBMIN_DIRENTRY
#undef ICOMP_CBMIN_FILEENTRY
#undef ICOMP_MIN
#undef ICOMP_MAX
#undef ICOMP_ALLOC
#undef ICOMP_FREE
#undef ICOMP_READN
#undef ICOMP_READ
#undef ICOMP_SEEK
#undef ICOMP_GET8
#undef ICOMP_GET16
#undef ICOMP_GET32

#endif  // ISDECOMP_BUILD_LIB


#ifdef ISDECOMP_DUMMY_SAL_DEFINED
#undef ISDECOMP_DUMMY_SAL_DEFINED
#undef __in
#undef __out
#undef __inout
#undef __in_opt
#undef __out_opt
#undef __inout_opt
#undef __in_bcount
#undef __out_ecount
#undef __out_bcount_part
#undef __deref_out
#endif  // ISDECOMP_DUMMY_SAL_DEFINED
