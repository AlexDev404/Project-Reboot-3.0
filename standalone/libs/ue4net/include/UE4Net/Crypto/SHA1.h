// Minimal SHA-1 implementation for StatelessConnectHandlerComponent HMAC
// UE4 uses FSHA1 class for cookie generation

#pragma once

#include "UE4Net/CoreMinimal.h"

class CORE_API FSHA1
{
public:
    static constexpr int32 DigestSize = 20;

    FSHA1();
    void Reset();
    void Update(const uint8* Data, uint64 Length);
    void Final();
    void GetHash(uint8 Digest[DigestSize]) const;

    // Convenience
    static void HashBuffer(const void* Data, uint64 DataSize, uint8 OutHash[DigestSize]);

private:
    uint32 State[5];
    uint64 Count;
    uint8 Buffer[64];
    uint8 Digest[DigestSize];
    bool bFinalized;

    void Transform(const uint8 Block[64]);
};

// HMAC-SHA1 for cookie generation
class CORE_API FHMACSHA1
{
public:
    static constexpr int32 DigestSize = FSHA1::DigestSize;

    static void Generate(const uint8* Key, int32 KeyLen, const uint8* Data, int32 DataLen, uint8 OutDigest[DigestSize]);
};
