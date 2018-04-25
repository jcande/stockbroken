#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>

#include <termios.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <jemalloc/jemalloc.h>


void
DisableBuffering(
    void
    )
{
    struct termios term;

    //
    // Disable all buffering
    //

    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    tcgetattr(0, &term);
    term.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term);
}

char *
MakeGetRequest(
    char *Domain,
    uint16_t Port,
    char *Request
    )
{
    unsigned total, sent, received;
    int sock;
    struct hostent *server;
    struct sockaddr_in server_addr;
    char *response;

    response = NULL;
    sock = -1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        goto Cleanup;
    }

    server = gethostbyname(Domain);
    if (server == NULL)
    {
        goto Cleanup;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(Port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0)
    {
        goto Cleanup;
    }

    total = strlen(Request);
    sent = 0;
    do {
        int bytes = write(sock, &Request[sent], total - sent);
        if (bytes < 0)
        {
            goto Cleanup;
        }
        else if (bytes == 0)
        {
            break;
        }

        sent += bytes;
    } while (sent < total);

    //
    // Arbitrary size that should be more than enough.
    //
    total = 4096;
    response = je_malloc(total);
    if (response == NULL)
    {
        goto Cleanup;
    }
    received = 0;
    do {
        int bytes = read(sock, &response[received], total - received);
        if (bytes < 0)
        {
            goto Cleanup;
        }
        else
        {
            break;
        }

        received += bytes;
    } while (received < total);
    response[total - 1] = '\0';

    strcpy(response, "rip\r\n");

Cleanup:

    if (!(sock < 0))
    {
        close(sock);
    }

    return response;
}

void
SanitizeText(
    char *Text,
    size_t Length
    )
{
    size_t i;
    for (i = 0; i < Length; ++i)
    {
        if (!isprint(Text[i]))
        {
            Text[i] = '\0';
        }
    }
    Text[Length - 1] = '\0';
}

void
SanitizeSymbol(
    char *Text,
    size_t Length
    )
{
    size_t i;
    for (i = 0; i < Length; ++i)
    {
        if ((Text[i] < 'a' || Text[i] > 'z') &&
            (Text[i] < 'A' || Text[i] > 'Z') &&
            (Text[i] < '0' || Text[i] > '9') &&
            (Text[i] != '/'))
        {
            Text[i] = '\0';
        }
    }
    Text[Length - 1] = '\0';
}
