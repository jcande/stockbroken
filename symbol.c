#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <errno.h>

#include <jemalloc/jemalloc.h>

#include "runtime_library.h"
#include "profile.h"
#include "symbol.h"

void
SymbolTeardown(
    PStockSymbolEntry Symbol
    )
{
    assert(Symbol != NULL);

    je_free(Symbol->Comment);
    je_free(Symbol);
}

void
SymbolListEntryTeardown(
    PRtlListEntry Entry
    )
{
    PStockSymbolEntry sym;

    sym = UNEMBED(StockSymbolEntry, ListEntry, Entry);
    SymbolTeardown(sym);
}

int
SymbolInitialize(
    PStockSymbolEntry Symbol,
    PRtlListHead List,
    char *Name,
    unsigned SharePrice,
    char *Comment
    )
{
    strncpy(&Symbol->Name[0], Name, sizeof(Symbol->Name));
    Symbol->SharePrice = SharePrice;
    Symbol->Comment = Comment;

    RtlListPush(List, &Symbol->ListEntry);

    return STATUS_SUCCESS;
}

int
SymbolAlloc(
    PStockSymbolEntry *Symbol
    )
{
    PStockSymbolEntry newSymbol = *Symbol = NULL;
    int status = STATUS_SUCCESS;

    newSymbol = je_malloc(sizeof(*newSymbol));
    if (newSymbol == NULL)
    {
        BAIL(status = -ENOMEM);
    }

    memset(newSymbol, 0, sizeof(*newSymbol));

    *Symbol = newSymbol;
    status = STATUS_SUCCESS;

Cleanup:

    if (FAILURE(status))
    {
        je_free(newSymbol);
    }

    return status;
}

//
// Used in folds
//

int
SymbolListEntryCompareName(
    void *SymbolName,
    PRtlListEntry Entry
    )
{
    PStockSymbolEntry subtrahend = UNEMBED(StockSymbolEntry, ListEntry, Entry);

    return strcasecmp(SymbolName, &subtrahend->Name[0]);
}

void *
SymbolListEntryPrint(
    void *Context,
    PRtlListEntry Entry
    )
{
    PStockSymbolEntry symbol = UNEMBED(StockSymbolEntry, ListEntry, Entry);
    printf(": . %s\n", &symbol->Name[0]);
    return NULL;
}

void *
SymbolListEntryPrintDetailed(
    void *Context,
    PRtlListEntry Entry
    )
{
    PStockSymbolEntry symbol = UNEMBED(StockSymbolEntry, ListEntry, Entry);
    printf(": %s: %u\n", &symbol->Name[0], symbol->SharePrice);
    if (symbol->Comment)
    {
        printf(":: %s\n", symbol->Comment);
    }
    return NULL;
}
