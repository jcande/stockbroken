#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <jemalloc/jemalloc.h>

#include "runtime_library.h"
#include "profile.h"
#include "symbol.h"
#include "utility.h"

#define MAGIC_DIR   "/tmp/stocks/"

extern int InvokeTests(void);



void
PrintWelcomeBanner(
    void
    )
{
    printf("Welcome to the best stock ticker capitalism can produce!\n");
    printf("You are currently running a TRIAL version.\n");
    printf("Upgrade to our premium diamond gold silver platinum cobalt star\n");
    printf("alliance version to enjoy features such as:\n");
    printf("\t- Market manipulation (Marnip TM) assistant\n");
    printf("\t- Pump and dump coordination\n");
    printf("\t- Chop stock hunter\n");
    printf("\t- SEC subpoena scrambler\n");
    printf("\t- Grindr for insider trading\n");
    printf("\t- And much more!\n");
    printf("\n");
}

unsigned
LookupSymbolPrice(
    char *Name
    )
{
    unsigned price, defaultPrice;
    char request[1024], *response, *prev_token, *token;
    int response_success;

    char *requestTemplate = "GET /1.0/tops/last?symbols=%s HTTP/1.1\r\n"
                            "Host: api.iextrading.com\r\n"
                            "Connection: close\r\n\r\n";
    (void) sprintf(&request[0], requestTemplate, Name);
    response = MakeGetRequest("api.iextrading.com", 80, request);

    price = defaultPrice = RtlGetRandom(0, 313370000);

    //
    // N.B., This is a super hacky way of doing things that will break if
    // Yahoo! ever changes their API.
    //
    // We check for a "200" and then we assume the very last line contains the
    // result of our query. Since we are only querying 1 symbol, it will be,
    // plainly, the value of that symbol.
    //

    response_success = 0;
    prev_token = NULL;
    token = strtok(response, "\r\n");
    do {
        if (!response_success)
        {
            response_success = (strcmp(token, "HTTP/1.1 200 OK") == 0);
        }

        prev_token = token;
        token = strtok(NULL, "\r\n");
    } while (token != NULL);

    if (response_success && strstr(prev_token, "N/A") == NULL)
    {
        //
        // We have a successful response. This means that prev_token contains
        // the symbol's price in dollars. We must now convert it to cents.
        //

        char *decimal = strchr(prev_token, '.');

        if (decimal != NULL)
        {
            size_t len = strlen(decimal) - 1;
            memmove(decimal, &decimal[1], len);
            decimal[len] = '\0';
        }

        price = strtoul(prev_token, NULL, 10);
        if (price == ULONG_MAX)
        {
            price = defaultPrice;
        }
    }

    je_free(response);

    return price;
}

int
AddSymbol(
    Profile *Settings
    )
{
    PStockSymbolEntry newSymbol;
    char choiceBuf[MAX_SYMBOL_SIZE];
    unsigned sharePrice;
    int status = STATUS_SUCCESS;

    CHECK(status = RtlPromptInput(
        "Enter a symbol",
        &choiceBuf[0],
        sizeof(choiceBuf)));

    SanitizeSymbol(&choiceBuf[0], sizeof(choiceBuf));
    if (strlen(&choiceBuf[0]) < 1)
    {
        BAIL(status = -EINVAL);
    }

    if (RtlListFind(&Settings->StockSymbolList,
            SymbolListEntryCompareName,
            &choiceBuf[0]) != NULL)
    {
        BAIL(status = -EEXIST);
    }

    //
    // Lookup the current share price for the symbol.
    //

    if (Settings->LookupCount < MAX_LOOKUPS)
    {
        sharePrice = LookupSymbolPrice(&choiceBuf[0]);
        //
        // Set a lookup limit just so that we're not making a new connection
        // constantly (this drastically speeds up an exploit).
        //
        Settings->LookupCount++;
    }
    else
    {
        sharePrice = RtlGetRandom(0, 313370666);
    }

    CHECK(status = SymbolAlloc(&newSymbol));
    CHECK(status = SymbolInitialize(newSymbol,
                        &Settings->StockSymbolList,
                        &choiceBuf[0],
                        sharePrice,
                        NULL));

    printf(": Added\n");

Cleanup:
    return status;
}

int
AddComment(
    Profile *Settings
    )
{
    PStockSymbolEntry symbol;
    PRtlListEntry entry;
    char choiceBuf[MAX_SYMBOL_SIZE];
    int size, status = STATUS_SUCCESS;

    CHECK(status = RtlPromptInput(
        "Enter symbol name",
        &choiceBuf[0],
        sizeof(choiceBuf)));

    SanitizeSymbol(&choiceBuf[0], sizeof(choiceBuf));
    if (strlen(&choiceBuf[0]) < 1)
    {
        BAIL(status = -EINVAL);
    }

    entry = RtlListFind(&Settings->StockSymbolList,
                SymbolListEntryCompareName,
                &choiceBuf[0]);
    if (entry == NULL)
    {
        BAIL(status = -ENOENT);
    }
    printf(" : Ok\n");

    symbol = UNEMBED(StockSymbolEntry, ListEntry, entry);

    CHECK(status = RtlPromptInput(
        "Enter comment length",
        &choiceBuf[0],
        sizeof(choiceBuf)));
    size = strtoul(&choiceBuf[0], NULL, 10);
    if (size == ULONG_MAX)
    {
        BAIL(status = -errno);
    }
    else if (size > MAX_COMMENT_SIZE)
    {
        BAIL(status = -EINVAL);
    }

    je_free(symbol->Comment);
    symbol->Comment = je_malloc(size);
    if (symbol->Comment == NULL)
    {
        BAIL(status = -ENOMEM);
    }
    printf(" : Ok\n");

    CHECK(status = RtlPromptInput(
        "",
        symbol->Comment,
        size));
    // XXX this might make it too hard
    //SanitizeText(symbol->Comment, size);

    printf(": Posted\n");

Cleanup:
    return status;
}

int
RemoveSymbol(
    Profile *Settings
    )
{
    PRtlListEntry entry;
    char choiceBuf[MAX_SYMBOL_SIZE];
    int status = STATUS_SUCCESS;

    CHECK(status = RtlPromptInput(
        "Enter symbol name",
        &choiceBuf[0],
        sizeof(choiceBuf)));

    SanitizeSymbol(&choiceBuf[0], sizeof(choiceBuf));
    if (strlen(&choiceBuf[0]) < 1)
    {
        BAIL(status = -EINVAL);
    }

    entry = RtlListFind(&Settings->StockSymbolList,
                SymbolListEntryCompareName,
                &choiceBuf[0]);
    if (entry == NULL)
    {
        BAIL(status = -ENOENT);
    }

    RtlListPopEntry(&Settings->StockSymbolList, entry);
    SymbolListEntryTeardown(entry);

    printf(": Gone\n");

Cleanup:
    return status;
}

int
ViewSymbol(
    Profile *Settings
    )
{
    PRtlListEntry entry;
    char choiceBuf[MAX_SYMBOL_SIZE];
    int status = STATUS_SUCCESS;

    CHECK(status = RtlPromptInput(
        "Enter a symbol",
        &choiceBuf[0],
        sizeof(choiceBuf)));

    SanitizeSymbol(&choiceBuf[0], sizeof(choiceBuf));
    if (strlen(&choiceBuf[0]) < 1)
    {
        BAIL(status = -EINVAL);
    }

    entry = RtlListFind(&Settings->StockSymbolList,
                SymbolListEntryCompareName,
                &choiceBuf[0]);
    if (entry == NULL)
    {
        BAIL(status = -ENOENT);
    }

    (void) SymbolListEntryPrintDetailed(NULL, entry);
    printf(": Done\n");

Cleanup:
    return status;
}

int
SerializeProfile(
    Profile *Settings
    )
{
    int status = STATUS_SUCCESS;
    SerializeContext tuple;
    char *filename;
    mode_t mode;
    int fd;

    fd = -1;
    filename = NULL;

    //
    // First, calculate the length of the saved state.
    //

    CHECK(status = ProfileSerializeContextInitialize(&tuple, Settings));

    //
    // Serialize the saved state.
    //

    CHECK(status = ProfileSerialize(&tuple, Settings));

    //
    // Write the saved state to a file.
    // N.B., we use tempname(3) because we need the filename in order to
    // unlink(2).
    //

    filename = tempnam(MAGIC_DIR, "sucker_");
    if (filename == NULL)
    {
        BAIL(status = -errno);
    }

    mode = S_IRUSR | S_IWUSR;
    fd = open(filename, O_CREAT | O_RDWR, mode);
    if (fd < 0)
    {
        BAIL(status = -errno);
    }
    (void) unlink(filename);

    if (write(fd, tuple.State, strlen(tuple.State)) < 0)
    {
        BAIL(status = -errno);
    }

    printf(": Saved as fd %d.\n", fd);

Cleanup:
    ProfileSerializeContextTeardown(&tuple);
    free(filename);

    //
    // N.B., we "leak" the fd on successful runs intentionally. This is because
    // we don't want to remove the file until the program has closed. We could
    // add it to some type of fd list and cleanup properly upon termination but
    // FUCK THAT SHIT.
    //
    if (FAILURE(status) && fd >= 0)
    {
        close(fd);
    }

    return status;
}

int
UnserializeProfile(
    PProfile *Settings
    )
{
    char choiceBuf[16];
    off_t length;
    int fd, status;
    char *state;
    Profile *newSettings;

    status = STATUS_SUCCESS;
    state = NULL;
    newSettings = NULL;

    CHECK(status = RtlPromptInput(
        "Enter fd",
        &choiceBuf[0],
        sizeof(choiceBuf)));
    fd = strtoul(&choiceBuf[0], NULL, 10);
    if (fd == ULONG_MAX)
    {
        BAIL(status = -errno);
    }

    if (fd < 3)
    {
        //
        // Don't let the user specify 0, 1, 2, or negative fds.
        //
        BAIL(status = -EBADFD);
    }

    length = lseek(fd, 0, SEEK_END);
    if (length < 0)
    {
        BAIL(status = -errno);
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
    {
        BAIL(status = -errno);
    }

    state = je_malloc(length + 1);
    if (state == NULL)
    {
        BAIL(status = -ENOMEM);
    }

    if (read(fd, state, length) != length)
    {
        BAIL(status = -errno);
    }
    state[length] = '\0';

    CHECK(status = ProfileAlloc(&newSettings));
    CHECK(status = ProfileInitialize(newSettings));

    CHECK(status = ProfileUnserialize(newSettings, state, length));

    ProfileTeardown(*Settings);
    *Settings = newSettings;
    newSettings = NULL;

    printf(": Loaded\n");

Cleanup:
    je_free(state);

    if (newSettings)
    {
        ProfileTeardown(newSettings);
    }

    return status;
}

// TODO Make this function take in ALL of the input so that we can just pass
// the parameters to the functions above. This would make testing them a lot
// simpler.
int
Loop(
    void
    )
{
    int status = STATUS_SUCCESS;
    Profile *settings;

    CHECK(status = ProfileAlloc(&settings));
    CHECK(status = ProfileInitialize(settings));

    for (;;)
    {
        char choiceBuf[64];

        char *menu = "\n"
            ": (A)dd symbol\n"
            ": (C)omment on a symbol\n"
            ": (R)emove symbol\n"
            ": (V)iew symbol\n"
            ": (S)ave profile\n"
            ": (L)oad profile\n"
            ": (B)ail\n";

        CHECK(status = RtlPromptInput(
            menu,
            &choiceBuf[0],
            sizeof(choiceBuf)));

        switch (toupper(choiceBuf[0])) {
        case 'A':
            //
            // Add a symbol
            //
            status = AddSymbol(settings);
            if (status == -EEXIST)
            {
                printf("! You already added this symbol\n");
            }
            else if (status == -EINVAL)
            {
                printf("! That symbol is awful\n");
            }
            else
            {
                CHECK(status);
            }

            break;
        case 'C':
            //
            // Add a comment
            //
            status = AddComment(settings);
            if (status == -ENOENT)
            {
                printf("! You need to add that symbol before you can comment on it\n");
            }
            else if (status == -EINVAL)
            {
                printf("! LIFE Alert dispatched. Remain calm\n");
            }
            else
            {
                CHECK(status);
            }

            break;
        case 'R':
            //
            // Remove a symbol
            //
            status = RemoveSymbol(settings);
            if (status == -ENOENT)
            {
                printf(": Gone (already)\n");
            }
            else if (status == -EINVAL)
            {
                printf("! Make sure you're logged into the CEO's account\n");
            }
            else
            {
                CHECK(status);
            }

            break;
        case 'V':
            //
            // View symbol's value and comment (if available)
            //

            printf(": Current symbols:\n");
            (void) RtlListFold(&settings->StockSymbolList, SymbolListEntryPrint, 0);

            status = ViewSymbol(settings);
            if (status == -ENOENT)
            {
                printf("! You never added that symbol\n");
            }
            else if (status == -EINVAL)
            {
                printf("! Make sure you're selecting a non-shitty symbol\n");
            }
            else
            {
                CHECK(status);
            }

            break;
        case 'S':
            //
            // Save current profile
            //
            CHECK(status = SerializeProfile(settings));

            break;
        case 'L':
            //
            // Load a profile
            //

            status = UnserializeProfile(&settings);
            if (status == -EBADF ||
                status == -EILSEQ ||
                status == -ENODATA)
            {
                printf("! Alert: hack-attack detected\n");
            }
            else
            {
                CHECK(status);
            }

            break;
        case 'P':
            //
            // When serializing, preserve (or don't) share price.
            //
            settings->PreserveShareprice = (choiceBuf[0] == 'P');

            break;
        case 'B':
            //
            // Quit
            //

            status = STATUS_SUCCESS;
            goto Cleanup;

            break;
        default:
            printf("! Upgrade today to enjoy this feature!\n");

            break;
        }
    }

Cleanup:
    if (settings != NULL)
    {
        ProfileTeardown(settings);
    }

    if (FAILURE(status))
    {
        printf("! %d: The stock market is crashing. May god have mercy on our souls\n", status);
    }

    return status;
}

int
Setup(
    void
    )
{
#ifdef TEST
    if (FAILURE(InvokeTests()))
    {
        printf("! not all test passed\n");
        return 1;
    }
#endif

    srand(time(0));
    DisableBuffering();
    PrintWelcomeBanner();

    return STATUS_SUCCESS;
}

int
main(
    void
    )
{
    int status;

    CHECK(status = Setup());

    CHECK(status = Loop());

Cleanup:
    return FAILURE(status);
}
