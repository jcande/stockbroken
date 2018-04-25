#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

#include <search.h>

#include <jemalloc/jemalloc.h>

#include "runtime_library.h"

int
RtlPromptInput(
    char *Prompt,
    char *Buffer,
    size_t BufferSize
    )
{
    int status = STATUS_SUCCESS;
    char *choice;

    printf("%s> ", Prompt);
    choice = fgets(Buffer, BufferSize, stdin);
    if (choice == NULL)
    {
        BAIL(status = -errno);
    }

Cleanup:
    return status;
}

unsigned
RtlGetRandom(
    unsigned Low,
    unsigned High
    )
{
    unsigned r;

    do {
        r = rand();
    } while (r < Low || r >= High);

    return r;
}

int
RtlConvertUintToCString(
    uint64_t Number,
    char *Output
    )
{
    uint64_t probe = Number;

    do {
        Output++;
        probe /= 10;
    } while (probe != 0);
    *Output-- = '\0';

    do {
        *Output-- = '0' + (Number % 10);
        Number /= 10;
    } while (Number != 0);

    return STATUS_SUCCESS;
}

int
RtlStringAppend(
    PRtlString String,
    char *Append
    )
{
    size_t length;
    uint64_t result;
    int status = STATUS_SUCCESS;

    length = strlen(Append);
    if (__builtin_add_overflow(length, String->Length, &result))
    {
        BAIL(status = -EOVERFLOW);
    }

    if (result > String->MaxSize)
    {
        void *newBuffer;
        uint64_t newSize;

        if (__builtin_mul_overflow(result, 2, &newSize))
        {
            //
            // We attempted to allocate a bit more so that we don't have to
            // keep allocating with each new append but that didn't work out.
            // Such is life.
            //
            newSize = result;
        }

        newBuffer = je_realloc(String->Buffer, newSize);
        if (newBuffer == NULL)
        {
            BAIL(status = -ENOMEM);
        }

        String->Buffer = newBuffer;
        String->MaxSize = newSize;
    }

    memcpy(&String->Buffer[String->Length], Append, length);
    String->Length = result;

Cleanup:
    return status;
}


void
RtlListInitialize(
    PRtlListHead List
    )
{
    List->Flink = List->Blink = NULL;
}

void
RtlListFlush(
    PRtlListHead List,
    RtlListFlushFunc Cleanup
    )
{
    PRtlListEntry cur;

    //
    // Start at the first entry and not the head.
    //
    cur = List->Flink;

    while (cur != NULL)
    {
        PRtlListEntry old;

        old = cur;
        cur = cur->Flink;

        remque(old);
        Cleanup(old);
    }

    RtlListInitialize(List);
}

void
RtlListPush(
    PRtlListHead List,
    PRtlListEntry Entry
    )
{
    insque(Entry, List);
}

PRtlListEntry
RtlListPop(
    PRtlListHead List
    )
{
    PRtlListEntry cur;

    //
    // Start at the first entry and not the head.
    //
    cur = List->Flink;

    if (cur != NULL)
    {
        remque(cur);
    }

    return cur;
}

void
RtlListPopEntry(
    PRtlListHead List,
    PRtlListEntry Entry
    )
{
    remque(Entry);
}

void *
RtlListFold(
    PRtlListHead List,
    RtlListFoldFunc Func,
    void *Default
    )
{
    PRtlListEntry cur;
    void *accumulator;

    //
    // Start at the first entry and not the head.
    //
    cur = List->Flink;

    accumulator = Default;
    while (cur != NULL)
    {
        accumulator = Func(accumulator, cur);

        cur = cur->Flink;
    }

    return accumulator;
}

void *
RtlpListFindHelper(
    void *Context,
    PRtlListEntry Entry
    )
{
    PRtlpListFindTuple tuple = (PRtlpListFindTuple) Context;

    if (tuple->Result != NULL)
    {
        goto Cleanup;
    }
    else if (tuple->Find(tuple->Needle, Entry) == 0)
    {
        tuple->Result = Entry;
    }

Cleanup:

    return tuple;
}

PRtlListEntry
RtlListFind(
    PRtlListHead List,
    RtlListFindFunc Find,
    void *Needle
    )
{
    RtlpListFindTuple tuple = {
        .Needle = Needle,
        .Find = Find,
        .Result = NULL
    };
    (void) RtlListFold(List, RtlpListFindHelper, &tuple);

    return tuple.Result;
}
