#include <string.h>

#include <errno.h>

#include <jemalloc/jemalloc.h>

#include "runtime_library.h"
#include "profile.h"
#include "symbol.h"

void
ProfileTeardown(
    PProfile Settings
    )
{
    RtlListFlush(&Settings->StockSymbolList, SymbolListEntryTeardown);
    je_free(Settings);
}

int
ProfileInitialize(
    PProfile Settings
    )
{
    RtlListInitialize(&Settings->StockSymbolList);

    Settings->LookupCount = 0;
    Settings->PreserveShareprice = 0;

    return STATUS_SUCCESS;
}

int
ProfileAlloc(
    PProfile *Settings
    )
{
    Profile *newProfile = *Settings = NULL;
    int status = STATUS_SUCCESS;

    newProfile = je_malloc(sizeof(*newProfile));
    if (newProfile == NULL)
    {
        BAIL(status = -ENOMEM);
    }

    memset(Settings, 0, sizeof(*Settings));

    *Settings = newProfile;
    status = STATUS_SUCCESS;

Cleanup:

    if (FAILURE(status))
    {
        je_free(newProfile);
    }

    return status;
}

//
// Serialize
//

static
unsigned
CalculateDigits(
    unsigned N
    )
{
    unsigned digits = 0;
    while (N != 0)
    {
        digits += 1;
        N /= 10;
    }

    return digits;
}

void *
ProfilepSerializeStockSymbolListCalculateLengthHelper(
    void *Context,
    PRtlListEntry Entry
    )
{
    PSerializeContext tuple = (PSerializeContext) Context;
    PStockSymbolEntry symbol = UNEMBED(StockSymbolEntry, ListEntry, Entry);
    size_t nameLength;

    if (!tuple->FirstElement)
    {
        //
        // Since there are elements in front of us, add in ,.
        //
        tuple->Length += 1;
    }

    //
    // We stuff all of the symbol's data into an array INCLUDING the length of
    // the symbol name. The format is: [<length>,"<name>"]
    //
    nameLength = strlen(&symbol->Name[0]);
    tuple->Length += 1 + CalculateDigits(nameLength) + 1 + 1 + nameLength + 1 + 1;

    if (tuple->PreserveShareprice)
    {
        //
        // We need to add in the ,.
        //
        tuple->Length += 1 + CalculateDigits(symbol->SharePrice);
    }

    tuple->FirstElement = 0;

    return tuple;
}

//
// FOLD
//
int
ProfilepSerializeCalculateLength(
    PSerializeContext Context,
    PProfile Settings
    )
{
    int status = STATUS_SUCCESS;

    //
    // Add on the constants: {"symbols":[ ... ]}
    //
    Context->Length += 1 + (1+7+1) + 1 + (1+1) + 1;

    Context->FirstElement = 1;
    (void) RtlListFold(&Settings->StockSymbolList,
            ProfilepSerializeStockSymbolListCalculateLengthHelper,
            Context);

    //
    // Invoke helpers for any other member we are to serialize.
    //

    return status;
}

void
ProfileSerializeContextTeardown(
    PSerializeContext Context
    )
{
    je_free(Context->State);
    Context->State = NULL;

    if (Context->String)
    {
        Context->String->MaxSize = Context->String->Length = 0;
        je_free(Context->String->Buffer);
        Context->String->Buffer = NULL;
    }
}

int
ProfileSerializeContextInitialize(
    PSerializeContext Context,
    PProfile Settings
    )
{
    int status = STATUS_SUCCESS;

    memset(Context, 0, sizeof(*Context));

    Context->PreserveShareprice = Settings->PreserveShareprice;

    CHECK(status = ProfilepSerializeCalculateLength(Context, Settings));

    Context->State = je_malloc(Context->Length);
    if (Context->State == NULL)
    {
        BAIL(status = -ENOMEM);
    }

Cleanup:
    return status;
}

//
// FOLD
//
void *
ProfilepSerializeListEntry(
    void *Context,
    PRtlListEntry Entry
    )
{
    PSerializeContext tuple = (PSerializeContext) Context;
    PStockSymbolEntry symbol = UNEMBED(StockSymbolEntry, ListEntry, Entry);

    char number[RTL_MAX_UINT64_DIGITS];
    size_t nameLength;
    int status = STATUS_SUCCESS;

    if (!tuple->FirstElement)
    {
        CHECK(status = RtlStringAppend(tuple->String, ","));
    }

    CHECK(status = RtlStringAppend(tuple->String, "["));

    nameLength = strlen(&symbol->Name[0]);
    CHECK(status = RtlConvertUintToCString(nameLength, &number[0]));
    CHECK(status = RtlStringAppend(tuple->String, &number[0]));
    CHECK(status = RtlStringAppend(tuple->String, ","));

    CHECK(status = RtlStringAppend(tuple->String, "\""));
    CHECK(status = RtlStringAppend(tuple->String, &symbol->Name[0]));
    CHECK(status = RtlStringAppend(tuple->String, "\""));

    if (tuple->PreserveShareprice)
    {
        CHECK(status = RtlStringAppend(tuple->String, ","));

        CHECK(status = RtlConvertUintToCString(symbol->SharePrice, &number[0]));
        CHECK(status = RtlStringAppend(tuple->String, &number[0]));
    }

    CHECK(status = RtlStringAppend(tuple->String, "]"));

Cleanup:

    tuple->Status = status;
    tuple->FirstElement = 0;

    return tuple;
}

int
ProfileSerialize(
    PSerializeContext Context,
    PProfile Settings
    )
{
    int status = STATUS_SUCCESS;
    RtlString save = {
        .Length = 0,
        .MaxSize = 1024,
        .Buffer = NULL
        };

    save.Buffer = je_malloc(save.MaxSize);
    if (save.Buffer == NULL)
    {
        BAIL(status = -ENOMEM);
    }
    Context->String = &save;


    CHECK(status = RtlStringAppend(&save, "{"));

    CHECK(status = RtlStringAppend(&save, "\"symbols\":["));

    Context->FirstElement = 1;
    (void) RtlListFold(&Settings->StockSymbolList, ProfilepSerializeListEntry, Context);
    CHECK(status = Context->Status);

    CHECK(status = RtlStringAppend(&save, "]"));

    //
    // Invoke helpers for any other member we are to serialize.
    //

    CHECK(status = RtlStringAppend(&save, "}"));


    strncpy(Context->State, (char *) save.Buffer, Context->Length);

Cleanup:

    Context->String = NULL;
    je_free(save.Buffer);

    return status;
}

//__attribute__ ((noinline))
char *
my_strncpy(
    char *dest,
    const char *src,
    size_t n
    )
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; ++i)
    {
        dest[i] = src[i];
    }
    for ( ; i < n; ++i)
    {
        dest[i] = '\0';
    }
    return dest;
}

int
ProfileUnserialize(
    PProfile Settings,
    char *State,
    size_t Length
    )
{
#define ADD(Index, Addend) ({       \
    if ((Index) < Length)           \
    {                               \
        /*                          \
        // N.B., we only verify we  \
        // don't overflow only      \
        // AFTER we perform the     \
        // write.                   \
        */                          \
        (Index) += (Addend);        \
    }                               \
    else                            \
    {                               \
        BAIL(status = -EOVERFLOW);  \
    }                               \
})

    size_t i;
    int status;
    char *next;

    status = STATUS_SUCCESS;
    i = 0;

    //
    // This function is shit because we have trusted data :)
    //

    char *prefix = "{\"symbols\":[";
    if (Length < strlen(prefix))
    {
        BAIL(status = -ENODATA);
    }
    else if (strncmp(State, prefix, strlen(prefix)) != 0)
    {
        BAIL(status = -EILSEQ);
    }
    ADD(i, strlen(prefix));

    //
    // Our input now looks like: [1,"d"],[1,"c"],[1,"b"],[1,"a"]]}
    // We bail if it doesn't.
    //
    while (i < Length)
    {
        PStockSymbolEntry newSymbol;
        uint32_t symbolLength;

        if (State[i] == ',')
        {
            ADD(i, 1);
        }
        else if (State[i] == ']' && State[i + 1] == '}')
        {
            //
            // We're done!
            //
            break;
        }
        else if (State[i] != '[')
        {
            BAIL(status = -EILSEQ);
        }
        ADD(i, 1);

        next = strchr(&State[i], ',');
        symbolLength = strtoul(&State[i], NULL, 10);
        ADD(i, next - &State[i] + 1);

        if (State[i] != '"')
        {
            BAIL(status = -EILSEQ);
        }
        ADD(i, 1);

        CHECK(status = SymbolAlloc(&newSymbol));
        my_strncpy(&newSymbol->Name[0], &State[i], symbolLength);
        RtlListPush(&Settings->StockSymbolList, &newSymbol->ListEntry);
        ADD(i, symbolLength);

        if (State[i] != '"')
        {
            BAIL(status = -EILSEQ);
        }
        ADD(i, 1);

        if (State[i] != ']')
        {
            BAIL(status = -EILSEQ);
        }
        ADD(i, 1);
    }

    (void) RtlListFold(&Settings->StockSymbolList, SymbolListEntryPrintDetailed, 0);

Cleanup:
    return status;
}
