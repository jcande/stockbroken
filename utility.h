#ifndef _UTILITY_H_
#define _UTILITY_H_

void
DisableBuffering(
    void
    );

char *
MakeGetRequest(
    char *Domain,
    uint16_t Port,
    char *Request
    );

void
SanitizeSymbol(
    char *Text,
    size_t Length
    );

#endif // _UTILITY_H_
