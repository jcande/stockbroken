#include <stdio.h>
#include <string.h>

#include <errno.h>

#include <jemalloc/jemalloc.h>

#include "runtime_library.h"
#include "profile.h"
#include "symbol.h"

void *
PrintElements(
    void *Context,
    struct qelem *Entry
    )
{
    PStockSymbolEntry element = UNEMBED(StockSymbolEntry, ListEntry, Entry);
    printf("%p: %s %u %s\n", element, element->Name, element->SharePrice, element->Comment);
    return NULL;
}

void *
SumList(
    void *Context,
    struct qelem *Entry
    )
{
    int accum = (int) (uint64_t) Context;

    PStockSymbolEntry element = UNEMBED(StockSymbolEntry, ListEntry, Entry);

    return (void *) (uint64_t) (accum + element->Name[0]);
}

int
TestFind(
    void *Minuend,
    PRtlListEntry Entry
    )
{
    PStockSymbolEntry subtrahend = UNEMBED(StockSymbolEntry, ListEntry, Entry);

    return memcmp(Minuend, subtrahend->Name, 2);
}

int
ListTests(
    void
    )
{
    RtlListHead list;
    PRtlListEntry entry;
    PStockSymbolEntry sym1, sym2;
    int i;
    int status = STATUS_SUCCESS;

    memset(&list, 0, sizeof(list));

    //
    // Push and Pop are inverses.
    //
    sym1 = je_malloc(sizeof(*entry));
    sym1->Name[0] = '$';
    RtlListPush(&list, &sym1->ListEntry);

    entry = RtlListPop(&list);
    sym2 = UNEMBED(StockSymbolEntry, ListEntry, entry);
    if (sym1 != sym2)
    {
        printf("push/pop aren't inverses\n");
        CHECK(status = -EBADE);
    }

    je_free(sym1);
    sym1 = sym2 = NULL;
    entry = NULL;


    //
    // Pop on an empty list does nothing.
    //
    entry = RtlListPop(&list);
    if (entry != NULL)
    {
        printf("pop on empty list is fucked up\n");
        CHECK(status = -EBADE);
    }

    //
    // Ensure we're adding multiple entries properly.
    //

    int sum = 0;
    for (i = 0; i < 10; ++i)
    {
        int value;
        sym1 = je_malloc(sizeof(*sym1));

        memset(sym1, 0, sizeof(*sym1));
        value = 'A' + i;
        sym1->Name[0] = value;
        sum += value;

        RtlListPush(&list, &sym1->ListEntry);
    }

    if (sum != (int) (uint64_t) RtlListFold(&list, SumList, 0))
    {
        printf("push or fold are fucked up\n");
        CHECK(status = -EBADE);
    }

    //
    // Ensure we can find elements.
    //

    entry = (PRtlListEntry) list.Flink;
    sym1 = UNEMBED(StockSymbolEntry, ListEntry, entry);
    entry = RtlListFind(&list, TestFind, sym1->Name);
    if (entry != NULL)
    {
        sym2 = UNEMBED(StockSymbolEntry, ListEntry, entry);
        if (sym1 != sym2)
        {
            printf("find is fucked up: %p vs %p\n", sym1, sym2);
            CHECK(status = -EBADE);
        }
    }
    else
    {
        printf("find returned null...\n");
        CHECK(status = -EBADE);
    }

    //
    // Cleanup.
    //

    RtlListFlush(&list, SymbolListEntryTeardown);
    if (list.Flink != NULL && list.Blink != NULL)
    {
        printf("flush is fucked up\n");
        CHECK(status = -EBADE);
    }

Cleanup:
    
    return status;
}

int
InvokeTests(
    void
    )
{
    int status = STATUS_SUCCESS;

    CHECK(status = ListTests());

Cleanup:
    return status;
}
