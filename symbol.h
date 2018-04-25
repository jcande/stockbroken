#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#define MAX_SYMBOL_SIZE     8
#define MAX_COMMENT_SIZE    256
  
typedef struct _StockSymbolEntry {
    RtlListEntry ListEntry;

    char *Comment;
    char Name[MAX_SYMBOL_SIZE];
    unsigned SharePrice;
} StockSymbolEntry, *PStockSymbolEntry;


void
SymbolTeardown(
    PStockSymbolEntry Symbol
    );

void
SymbolListEntryTeardown(
    PRtlListEntry Entry
    );

int
SymbolInitialize(
    PStockSymbolEntry Symbol,
    PRtlListHead List,
    char *Name,
    unsigned SharePrice,
    char *Comment
    );

int
SymbolAlloc(
    PStockSymbolEntry *Symbol
    );

//
// Used in folds
//

int
SymbolListEntryCompareName(
    void *SymbolName,
    PRtlListEntry Entry
    );

void *
SymbolListEntryPrint(
    void *Context,
    PRtlListEntry Entry
    );

void *
SymbolListEntryPrintDetailed(
    void *Context,
    PRtlListEntry Entry
    );

#endif // _SYMBOL_H_
