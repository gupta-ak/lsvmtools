/*
**==============================================================================
**
** LSVMTools 
** 
** MIT License
** 
** Copyright (c) Microsoft Corporation. All rights reserved.
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in 
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE
**
**==============================================================================
*/
#include "memblkdev.h"
#include "alloc.h"
#include "strings.h"
#include "print.h"
#include "chksum.h"

typedef struct _BlkdevImpl BlkdevImpl;

struct _BlkdevImpl
{
    Blkdev base;
    void* data;
    UINTN size;
};

static int _Close(
    Blkdev* dev)
{
    return 0;
}

static int _GetN(
    Blkdev* dev,
    UINTN blkno,
    UINTN nblocks,
    void* data)
{
    int rc = -1;
    BlkdevImpl* impl = (BlkdevImpl*)dev;
    UINTN offset = blkno * BLKDEV_BLKSIZE;
    UINTN toRead = nblocks * BLKDEV_BLKSIZE;

    if (!dev || !data)
        goto done;

    if (offset + toRead >= impl->size)
        goto done;

    Memcpy(data, (const UINT8*)impl->data + offset, toRead);
    rc = 0;

done:
    return rc;
}


static int _Get(
    Blkdev* dev,
    UINTN blkno,
    void* data)
{
    return _GetN(dev, blkno, 1, data);
}

static int _PutN(
    Blkdev* dev,
    UINTN blkno,
    UINTN nblocks,
    const void* data)
{
    int rc = -1;
    BlkdevImpl* impl = (BlkdevImpl*)dev;
    UINTN offset = blkno * BLKDEV_BLKSIZE;
    UINTN toWrite = nblocks * BLKDEV_BLKSIZE;

    if (!dev || !data)
        goto done;

    if (offset + toWrite >= impl->size)
        goto done;

    Memcpy((UINT8*)impl->data + offset, data, toWrite);
    rc = 0;

done:
    return rc;
}

static int _Put(
    Blkdev* dev,
    UINTN blkno,
    const void* data)
{
    return _PutN(dev, blkno, 1, data);
}

static int _SetFlags(
    Blkdev* dev,
    UINT32 flags)
{
    /* No flags supported */
    return -1;
}

Blkdev* BlkdevFromMemory(
    void* data,
    UINTN size)
{
    BlkdevImpl* impl = NULL;

    if (!data || !size)
        goto done;

    if (!(impl = Calloc(1, sizeof(BlkdevImpl))))
        goto done;

    impl->base.Close = _Close;
    impl->base.Get = _Get;
    impl->base.GetN = _GetN;
    impl->base.Put = _Put;
    impl->base.PutN = _PutN;
    impl->base.SetFlags = _SetFlags;
    impl->data = data;
    impl->size = size;

done:
    return &impl->base;
}
