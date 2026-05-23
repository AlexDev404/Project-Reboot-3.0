// SHA-1 Implementation (RFC 3174)
// Used by UE4's StatelessConnectHandlerComponent for HMAC cookie generation

#include "UE4Net/Crypto/SHA1.h"

#define SHA1_ROTL(bits, word) (((word) << (bits)) | ((word) >> (32-(bits))))

FSHA1::FSHA1() { Reset(); }

void FSHA1::Reset()
{
    State[0] = 0x67452301;
    State[1] = 0xEFCDAB89;
    State[2] = 0x98BADCFE;
    State[3] = 0x10325476;
    State[4] = 0xC3D2E1F0;
    Count = 0;
    bFinalized = false;
    FMemory::Memzero(Buffer, sizeof(Buffer));
    FMemory::Memzero(Digest, sizeof(Digest));
}

void FSHA1::Transform(const uint8 Block[64])
{
    uint32 W[80];
    uint32 A, B, C, D, E;

    for (int i = 0; i < 16; i++)
    {
        W[i] = ((uint32)Block[i*4] << 24) | ((uint32)Block[i*4+1] << 16) |
               ((uint32)Block[i*4+2] << 8) | ((uint32)Block[i*4+3]);
    }
    for (int i = 16; i < 80; i++)
    {
        W[i] = SHA1_ROTL(1, W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16]);
    }

    A = State[0]; B = State[1]; C = State[2]; D = State[3]; E = State[4];

    for (int i = 0; i < 80; i++)
    {
        uint32 f, k;
        if (i < 20)      { f = (B & C) | ((~B) & D); k = 0x5A827999; }
        else if (i < 40) { f = B ^ C ^ D;             k = 0x6ED9EBA1; }
        else if (i < 60) { f = (B & C) | (B & D) | (C & D); k = 0x8F1BBCDC; }
        else              { f = B ^ C ^ D;             k = 0xCA62C1D6; }

        uint32 temp = SHA1_ROTL(5, A) + f + E + k + W[i];
        E = D; D = C; C = SHA1_ROTL(30, B); B = A; A = temp;
    }

    State[0] += A; State[1] += B; State[2] += C; State[3] += D; State[4] += E;
}

void FSHA1::Update(const uint8* Data, uint64 Length)
{
    uint64 Index = Count % 64;
    Count += Length;

    for (uint64 i = 0; i < Length; i++)
    {
        Buffer[Index++] = Data[i];
        if (Index == 64)
        {
            Transform(Buffer);
            Index = 0;
        }
    }
}

void FSHA1::Final()
{
    if (bFinalized) return;

    uint64 BitCount = Count * 8;
    uint8 End = 0x80;
    Update(&End, 1);

    while ((Count % 64) != 56)
    {
        uint8 Zero = 0;
        Update(&Zero, 1);
    }

    // Append bit length (big-endian)
    uint8 LenBuf[8];
    for (int i = 7; i >= 0; i--)
    {
        LenBuf[i] = (uint8)(BitCount & 0xFF);
        BitCount >>= 8;
    }
    Update(LenBuf, 8);

    // Store final hash
    for (int i = 0; i < 5; i++)
    {
        Digest[i*4]   = (uint8)(State[i] >> 24);
        Digest[i*4+1] = (uint8)(State[i] >> 16);
        Digest[i*4+2] = (uint8)(State[i] >> 8);
        Digest[i*4+3] = (uint8)(State[i]);
    }
    bFinalized = true;
}

void FSHA1::GetHash(uint8 OutDigest[DigestSize]) const
{
    FMemory::Memcpy(OutDigest, Digest, DigestSize);
}

void FSHA1::HashBuffer(const void* Data, uint64 DataSize, uint8 OutHash[DigestSize])
{
    FSHA1 Sha;
    Sha.Update(static_cast<const uint8*>(Data), DataSize);
    Sha.Final();
    Sha.GetHash(OutHash);
}

// HMAC-SHA1
void FHMACSHA1::Generate(const uint8* Key, int32 KeyLen, const uint8* Data, int32 DataLen, uint8 OutDigest[DigestSize])
{
    const int32 BlockSize = 64;
    uint8 KeyBlock[64];
    FMemory::Memzero(KeyBlock, BlockSize);

    if (KeyLen > BlockSize)
    {
        FSHA1::HashBuffer(Key, KeyLen, KeyBlock);
    }
    else
    {
        FMemory::Memcpy(KeyBlock, Key, KeyLen);
    }

    uint8 IPad[64], OPad[64];
    for (int i = 0; i < BlockSize; i++)
    {
        IPad[i] = KeyBlock[i] ^ 0x36;
        OPad[i] = KeyBlock[i] ^ 0x5C;
    }

    // Inner hash
    FSHA1 InnerHash;
    InnerHash.Update(IPad, BlockSize);
    InnerHash.Update(Data, DataLen);
    InnerHash.Final();
    uint8 InnerDigest[FSHA1::DigestSize];
    InnerHash.GetHash(InnerDigest);

    // Outer hash
    FSHA1 OuterHash;
    OuterHash.Update(OPad, BlockSize);
    OuterHash.Update(InnerDigest, FSHA1::DigestSize);
    OuterHash.Final();
    OuterHash.GetHash(OutDigest);
}
