#ifndef _RUNTIME_LIBRARY_H_
#define _RUNTIME_LIBRARY_H_

#include <stdint.h>
#include <stddef.h>

struct qelem {
    struct qelem *Flink;
    struct qelem *Blink;
};

typedef struct qelem RtlListHead, *PRtlListHead;
typedef struct qelem RtlListEntry, *PRtlListEntry;

#define STATUS_SUCCESS  0
#define SUCCESS(Status) ((Status) == STATUS_SUCCESS)
#define FAILURE(Status) (!SUCCESS(Status))

#define CHECK(Status) ({    \
if (FAILURE(Status))  {     \
    goto Cleanup;           \
}})

#define BAIL(Status) ({     \
    Status;                 \
    goto Cleanup;           \
})

/*++
    Struct *
    EXTRACT(
        Struct,
        Field,
        Entry
    )

Arguments

    Struct - This is the container struct with an embedded RtlListEntry.
    Field - This is the field within the containre struct that is the embedded
            RtlListEntry.
    Entry - The actual address of an embedded RtlListEntry.

Description

    Given an RtlListEntry, this returns the containing structure.

Return Value

    The containing structure for an embedded RtlListEntry.

--*/
#define UNEMBED(Struct, Field, Entry)   \
    ((void *)(((uint8_t *) (Entry)) - offsetof(Struct, Field)))

#define RTL_MAX_UINT64_DIGITS   24

int
RtlPromptInput(
    char *Prompt,
    char *Buffer,
    size_t BufferSize
    );

unsigned
RtlGetRandom(
    unsigned Low,
    unsigned High
    );

int
RtlConvertUintToCString(
    uint64_t Number,
    char *Output
    );

typedef struct _RtlString {
    uint8_t *Buffer;
    uint32_t Length, MaxSize;
} RtlString, *PRtlString;

int
RtlStringAppend(
    PRtlString String,
    char *Append
    );

void
RtlListInitialize(
    PRtlListHead List
    );

typedef void (*RtlListFlushFunc)(PRtlListEntry);

void
RtlListFlush(
    PRtlListHead List,
    RtlListFlushFunc Cleanup
    );

void
RtlListPush(
    PRtlListHead List,
    PRtlListEntry Entry
    );

PRtlListEntry
RtlListPop(
    PRtlListHead List
    );

void
RtlListPopEntry(
    PRtlListHead List,
    PRtlListEntry Entry
    );

typedef void *(*RtlListFoldFunc)(void *, PRtlListEntry);

void *
RtlListFold(
    PRtlListHead List,
    RtlListFoldFunc Func,
    void *Default
    );

typedef int (*RtlListFindFunc)(void *, PRtlListEntry);

typedef struct _RtlpListFindTuple {
    void *Needle;
    RtlListFindFunc Find;
    PRtlListEntry Result;
} RtlpListFindTuple, *PRtlpListFindTuple;

PRtlListEntry
RtlListFind(
    PRtlListHead List,
    RtlListFindFunc Find,
    void *Needle
    );

#endif // _RUNTIME_LIBRARY_H_
