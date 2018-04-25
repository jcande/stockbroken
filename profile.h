#ifndef _PROFILE_H_
#define _PROFILE_H_

#define MAX_LOOKUPS         100

typedef struct _Profile {
    RtlListHead StockSymbolList;
    uint64_t PreserveShareprice;
    uint64_t LookupCount;
} Profile, *PProfile;

// XXX clean this up
typedef struct _SerializeContext {
    //
    // This is the calculated length of the serialized saved state.
    //
    uint16_t Length;

    //
    // The save state itself.
    //
    char *State;

    uint32_t FirstElement;
    uint32_t PreserveShareprice;

    //
    // An intermediate representation of the save state.
    //
    PRtlString String;

    int Status;
} SerializeContext, *PSerializeContext;

void
ProfileTeardown(
    PProfile Settings
    );

int
ProfileInitialize(
    PProfile Settings
    );

int
ProfileAlloc(
    PProfile *Settings
    );

void
ProfileSerializeContextTeardown(
    PSerializeContext Context
    );

int
ProfileSerializeContextInitialize(
    PSerializeContext Context,
    PProfile Settings
    );

int
ProfileSerialize(
    PSerializeContext Context,
    PProfile Settings
    );

int
ProfileUnserialize(
    PProfile Settings,
    char *State,
    size_t Length
    );

#endif // _PROFILE_H_
