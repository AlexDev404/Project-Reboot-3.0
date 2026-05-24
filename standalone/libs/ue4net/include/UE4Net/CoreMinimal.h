// Copyright Epic Games, Inc. All Rights Reserved.
// Compatibility shim: Provides minimal UE4 core types for standalone compilation.

#pragma once

// =============================================================================
// Platform & Build Configuration
// =============================================================================

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <memory>
#include <functional>

// Build configuration
#define UE_BUILD_SHIPPING 0
#define UE_BUILD_TEST 0
#define WITH_DEV_AUTOMATION_TESTS 0
#define UE_NET_TRACE_ENABLED 0
#define DO_CHECK 1

// Platform macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX 0
#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 1
#endif

// Export/Import
#if PLATFORM_WINDOWS
    #ifdef UE4NET_BUILDING
        #define ENGINE_API __declspec(dllexport)
        #define CORE_API __declspec(dllexport)
    #else
        #define ENGINE_API __declspec(dllimport)
        #define CORE_API __declspec(dllimport)
    #endif
#else
    #ifdef UE4NET_BUILDING
        #define ENGINE_API __attribute__((visibility("default")))
        #define CORE_API __attribute__((visibility("default")))
    #else
        #define ENGINE_API
        #define CORE_API
    #endif
#endif

// =============================================================================
// Type Aliases (matching UE4)
// =============================================================================

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using SIZE_T = size_t;
using TCHAR = wchar_t;
using UPTRINT = uintptr_t;

// =============================================================================
// UE4 Macros
// =============================================================================

#define FORCEINLINE inline
#define FORCEINLINE_DEBUGGABLE inline
#define FORCENOINLINE
#define TEXT(x) L##x
#define INDEX_NONE (-1)
#define NAME_SIZE 1024
#define PURE_VIRTUAL(Func,...) { __VA_ARGS__ }

#define check(expr) assert(expr)
#define checkSlow(expr) assert(expr)
#define checkf(expr, ...) assert(expr)
#define ensure(expr) (expr)
#define ensureMsgf(expr, ...) (expr)
#define verify(expr) assert(expr)

#define ENUM_CLASS_FLAGS(Enum) \
    inline Enum operator|(Enum A, Enum B) { return static_cast<Enum>(static_cast<uint32>(A) | static_cast<uint32>(B)); } \
    inline Enum operator&(Enum A, Enum B) { return static_cast<Enum>(static_cast<uint32>(A) & static_cast<uint32>(B)); } \
    inline Enum& operator|=(Enum& A, Enum B) { A = A | B; return A; } \
    inline bool operator!(Enum E) { return static_cast<uint32>(E) == 0; }

#define PRAGMA_DISABLE_DEPRECATION_WARNINGS
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS
#define PRAGMA_DISABLE_UNSAFE_TYPECAST_WARNINGS
#define PRAGMA_ENABLE_UNSAFE_TYPECAST_WARNINGS

#define UE_DEPRECATED(Ver, Msg)

// UObject macros (no-ops for standalone)
#define UCLASS(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define GENERATED_UCLASS_BODY() public:
#define GENERATED_BODY() public:

#define DECLARE_LOG_CATEGORY_EXTERN(Name, DefaultVerbosity, CompileTimeVerbosity) \
    struct FLogCategory##Name {};
#define DEFINE_LOG_CATEGORY(Name)

// =============================================================================
// Logging
// =============================================================================

enum { Log, Verbose, Warning, Error, Fatal };

#define UE_LOG(Category, Verbosity, Format, ...) \
    do { std::fprintf(stderr, "[UE4Net] " ); std::fprintf(stderr, "%s\n", ""); } while(0)

// Silence specific log categories used by the networking code
#define LogNet void(0)
#define LogNetSerialization void(0)
#define LogNetTraffic void(0)
#define LogHandshake void(0)
#define LogSerialization void(0)

// =============================================================================
// FMemory
// =============================================================================

struct FMemory
{
    static void* Memzero(void* Dest, SIZE_T Count) { return std::memset(Dest, 0, Count); }
    static void* Memcpy(void* Dest, const void* Src, SIZE_T Count) { return std::memcpy(Dest, Src, Count); }
    static int32 Memcmp(const void* Buf1, const void* Buf2, SIZE_T Count) { return std::memcmp(Buf1, Buf2, Count); }
    static void* Memmove(void* Dest, const void* Src, SIZE_T Count) { return std::memmove(Dest, Src, Count); }
    static void Memset(void* Dest, uint8 Char, SIZE_T Count) { std::memset(Dest, Char, Count); }
    static void* Malloc(SIZE_T Count) { return std::malloc(Count); }
    static void Free(void* Ptr) { std::free(Ptr); }
};

struct FPlatformMemory
{
    static void Memset(void* Dest, uint8 Char, SIZE_T Count) { std::memset(Dest, Char, Count); }
};

// =============================================================================
// Type Traits
// =============================================================================

template<typename T> struct TIsSigned { static constexpr bool Value = std::is_signed<T>::value; };
template<typename T> struct TIsUnsigned { static constexpr bool Value = std::is_unsigned<T>::value; };

// Inline allocator stub (used in template args, not for actual allocation in standalone)
template<uint32 NumElements> struct TInlineAllocator {};

// =============================================================================
// FMath
// =============================================================================

struct FMath
{
    template<typename T>
    static FORCEINLINE T Max(T A, T B) { return (A > B) ? A : B; }

    template<typename T>
    static FORCEINLINE T Min(T A, T B) { return (A < B) ? A : B; }

    template<typename T>
    static FORCEINLINE T Clamp(T Value, T MinVal, T MaxVal) { return Max(MinVal, Min(MaxVal, Value)); }

    static FORCEINLINE uint32 CeilLogTwo(uint32 Value)
    {
        uint32 Result = 0;
        if (Value > 1)
        {
            Value -= 1;
            while (Value > 0)
            {
                Value >>= 1;
                Result++;
            }
        }
        return Result;
    }

    // Compile-time CeilLogTwo (UE4 uses FMath::CeilLogTwo<N>::Value)
    template<uint64 N, uint64 V = N - 1, uint32 R = 0>
    struct CeilLogTwoHelper { static constexpr uint32 Value = (V == 0) ? 0 : CeilLogTwoHelper<N, (V >> 1), R + 1>::Value; };

    template<uint64 N, uint32 R>
    struct CeilLogTwoHelper<N, 0, R> { static constexpr uint32 Value = R; };

    template<SIZE_T N>
    using CeilLogTwo_t = CeilLogTwoHelper<N>;

    static FORCEINLINE uint32 RoundUpToPowerOfTwo(uint32 Value)
    {
        if (Value == 0) return 1;
        Value--;
        Value |= Value >> 1;
        Value |= Value >> 2;
        Value |= Value >> 4;
        Value |= Value >> 8;
        Value |= Value >> 16;
        return Value + 1;
    }

    static FORCEINLINE uint64 RoundUpToPowerOfTwo(uint64 Value)
    {
        if (Value == 0) return 1;
        Value--;
        Value |= Value >> 1;
        Value |= Value >> 2;
        Value |= Value >> 4;
        Value |= Value >> 8;
        Value |= Value >> 16;
        Value |= Value >> 32;
        return Value + 1;
    }

    // SIZE_T overload only needed on 32-bit (where size_t != uint64_t)
    template<typename T>
    static FORCEINLINE typename std::enable_if<!std::is_same<T, uint64>::value && std::is_same<T, SIZE_T>::value, SIZE_T>::type
    RoundUpToPowerOfTwo(T Value)
    {
        return static_cast<SIZE_T>(RoundUpToPowerOfTwo(static_cast<uint64>(Value)));
    }
};

struct FPlatformMath : public FMath {};

// =============================================================================
// FPlatformTime
// =============================================================================

struct FPlatformTime
{
    static double Seconds();
};

// =============================================================================
// TArray (minimal UE4-compatible)
// =============================================================================

template<typename T, typename AllocatorT = void>
class TArray
{
public:
    TArray() = default;
    TArray(const TArray&) = default;
    TArray& operator=(const TArray&) = default;
    TArray(TArray&&) = default;
    TArray& operator=(TArray&&) = default;

    FORCEINLINE int32 Num() const { return static_cast<int32>(Data.size()); }
    FORCEINLINE bool IsEmpty() const { return Data.empty(); }
    FORCEINLINE T* GetData() { return Data.data(); }
    FORCEINLINE const T* GetData() const { return Data.data(); }

    FORCEINLINE T& operator[](int32 Index) { return Data[Index]; }
    FORCEINLINE const T& operator[](int32 Index) const { return Data[Index]; }

    void Add(const T& Item) { Data.push_back(Item); }
    void Append(const T* Items, int32 Count) { Data.insert(Data.end(), Items, Items + Count); }
    void Append(const TArray& Other) { Data.insert(Data.end(), Other.Data.begin(), Other.Data.end()); }
    void Empty(int32 Slack = 0) { Data.clear(); if (Slack > 0) Data.reserve(Slack); }
    void SetNum(int32 Count) { Data.resize(Count); }
    void SetNumUninitialized(int32 Count) { Data.resize(Count); }
    void SetNumZeroed(int32 Count) { Data.resize(Count); std::memset(Data.data(), 0, Count * sizeof(T)); }
    void AddUninitialized(int32 Count) { Data.resize(Data.size() + Count); }
    void AddZeroed(int32 Count) { size_t Old = Data.size(); Data.resize(Old + Count); std::memset(Data.data() + Old, 0, Count * sizeof(T)); }
    void Reserve(int32 Count) { Data.reserve(Count); }
    void SetNumUnsafeInternal(int32 Count) { Data.resize(Count); }
    void Insert(const T& Item, int32 Index) { Data.insert(Data.begin() + Index, Item); }
    void RemoveAt(int32 Index) { Data.erase(Data.begin() + Index); }

    void CountBytes(class FArchive& Ar) const {}

    auto begin() { return Data.begin(); }
    auto end() { return Data.end(); }
    auto begin() const { return Data.begin(); }
    auto end() const { return Data.end(); }

private:
    std::vector<T> Data;
};

// =============================================================================
// TBitArray (minimal)
// =============================================================================

template<typename Allocator = void>
class TBitArray
{
public:
    TBitArray() = default;
    int32 Num() const { return static_cast<int32>(Bits.size()); }
    void Add(bool Value) { Bits.push_back(Value); }
    bool operator[](int32 Index) const { return Bits[Index]; }
private:
    std::vector<bool> Bits;
};

// =============================================================================
// FName (minimal)
// =============================================================================

#define NAME_Control FName(TEXT("Control"))
#define NAME_None FName()

class FName
{
public:
    FName() = default;
    FName(const wchar_t* InName) : Name(InName ? std::wstring(InName) : L"") {}
    FName(const char* InName) { if (InName) { for (const char* c = InName; *c; c++) Name += static_cast<wchar_t>(*c); } }
    FName(const std::string& InName) { for (char c : InName) Name += static_cast<wchar_t>(c); }
    bool operator==(const FName& Other) const { return Name == Other.Name; }
    bool operator!=(const FName& Other) const { return Name != Other.Name; }
    bool IsNone() const { return Name.empty(); }
    std::wstring ToWString() const { return Name; }
    std::string ToString() const { std::string r; for (wchar_t c : Name) r += static_cast<char>(c); return r; }
private:
    std::wstring Name;
};

// =============================================================================
// FString (minimal)
// =============================================================================

class FString
{
public:
    FString() = default;
    FString(const TCHAR* InStr) : Data(InStr ? InStr : L"") {}
    FString(const char* InStr) { if (InStr) { for (const char* c = InStr; *c; c++) Data += static_cast<wchar_t>(*c); } }
    FString(const std::string& InStr) { for (char c : InStr) Data += static_cast<wchar_t>(c); }
    bool IsEmpty() const { return Data.empty(); }
    int32 Len() const { return static_cast<int32>(Data.size()); }
    SIZE_T length() const { return Data.size(); }
    const TCHAR* operator*() const { return Data.c_str(); }
    bool operator==(const FString& Other) const { return Data == Other.Data; }

    // Narrow string access (for networking - addresses etc.)
    const char* c_str() const { UpdateNarrow(); return NarrowCache.c_str(); }
    std::string ToString() const { UpdateNarrow(); return NarrowCache; }

    static FString Printf(const TCHAR* Fmt, ...) { return FString(); }
    static FString FromInt(int32 Value) { char b[16]; std::snprintf(b,16,"%d",Value); return FString(b); }
private:
    std::wstring Data;
    mutable std::string NarrowCache;
    void UpdateNarrow() const { NarrowCache.clear(); for (wchar_t c : Data) NarrowCache += static_cast<char>(c); }
};

// =============================================================================
// FArchive (minimal base for serialization)
// =============================================================================

class FArchive
{
public:
    virtual ~FArchive() = default;

    bool IsLoading() const { return bIsLoading; }
    bool IsSaving() const { return bIsSaving; }
    bool IsPersistent() const { return bIsPersistent; }
    bool IsError() const { return bError; }
    void SetError() { bError = true; }
    void ClearError() { bError = false; }

    bool IsNetArchive() const { return ArIsNetArchive; }

    void SetIsLoading(bool b) { bIsLoading = b; }
    void SetIsSaving(bool b) { bIsSaving = b; }
    void SetIsPersistent(bool b) { bIsPersistent = b; }

    virtual void Reset() { bError = false; }
    virtual void Serialize(void* Data, int64 Num) {}
    virtual void CountBytes(SIZE_T InNum, SIZE_T InMax) {}

    int32 EngineNetVer() const { return ArEngineNetVer; }
    int32 GameNetVer() const { return ArGameNetVer; }
    void SetEngineNetVer(int32 V) { ArEngineNetVer = V; }
    void SetGameNetVer(int32 V) { ArGameNetVer = V; }

    template<typename T>
    FArchive& operator<<(T& Value) { Serialize(&Value, sizeof(T)); return *this; }

    int32 ArMaxSerializeSize = 16 * 1024 * 1024;

protected:
    bool bIsLoading = false;
    bool bIsSaving = false;
    bool bIsPersistent = false;
    bool bError = false;
    bool ArIsNetArchive = false;
    int32 ArEngineNetVer = 0;
    int32 ArGameNetVer = 0;
};

// CVar stub
struct TAutoConsoleVariable
{
    TAutoConsoleVariable(const TCHAR*, int32, const TCHAR*) {}
    int32 GetValueOnAnyThread() const { return 16 * 1024 * 1024; }
};

// Forward declare the CVar used by BitArchive
extern TAutoConsoleVariable CVarMaxNetStringSize;

// =============================================================================
// FBitArchive base (for FBitReader/FBitWriter)
// =============================================================================

struct FBitArchive : public FArchive
{
    virtual void SerializeBits(void* Src, int64 LengthBits) {}
    virtual void SerializeBitsWithOffset(void* Dest, int32 DestBit, int64 LengthBits) {}
    virtual void SerializeInt(uint32& Value, uint32 Max) {}
    virtual void SerializeIntPacked(uint32& Value) {}
};

// =============================================================================
// Minimal UObject stubs (just enough for Channel/Connection classes to compile)
// =============================================================================

class UObject
{
public:
    virtual ~UObject() = default;
    virtual void Serialize(FArchive& Ar) {}
    virtual void BeginDestroy() {}
};

class FObjectInitializer
{
public:
    static FObjectInitializer& Get() { static FObjectInitializer Instance; return Instance; }
};

// Stub for GENERATED macros
class FReferenceCollector {};

// =============================================================================
// Network-specific types
// =============================================================================

struct FNetworkGUID
{
    uint32 Value = 0;
    FNetworkGUID() = default;
    FNetworkGUID(uint32 InValue) : Value(InValue) {}
    bool IsValid() const { return Value != 0; }
    bool IsDynamic() const { return Value > 0 && !(Value & 1); }
    bool IsStatic() const { return (Value & 1) != 0; }
    bool operator==(const FNetworkGUID& Other) const { return Value == Other.Value; }
    bool operator!=(const FNetworkGUID& Other) const { return Value != Other.Value; }
    bool operator<(const FNetworkGUID& Other) const { return Value < Other.Value; }
    FString ToString() const { return FString::FromInt(static_cast<int32>(Value)); }
};

struct FPacketIdRange
{
    int32 First = INDEX_NONE;
    int32 Last = INDEX_NONE;
};

// FInternetAddr stub
class FInternetAddr
{
public:
    virtual ~FInternetAddr() = default;
    virtual FString ToString(bool bAppendPort) const { return FString(); }
    virtual bool operator==(const FInternetAddr& Other) const { return false; }
};

// Packet traits
struct FOutPacketTraits
{
    bool bIsKeepAlive = false;
};

// PacketHandler base
class HandlerComponent
{
public:
    HandlerComponent() = default;
    HandlerComponent(FName InName) : ComponentName(InName) {}
    virtual ~HandlerComponent() = default;

    virtual void CountBytes(FArchive& Ar) const {}
    virtual bool IsValid() const { return true; }
    virtual void NotifyHandshakeBegin() {}
    virtual void Initialize() {}
    virtual void Incoming(struct FBitReader& Packet) {}
    virtual void Outgoing(struct FBitWriter& Packet, FOutPacketTraits& Traits) {}
    virtual void IncomingConnectionless(struct FIncomingPacketRef* PacketRef) {}
    virtual bool CanReadUnaligned() const { return false; }
    virtual int32 GetReservedPacketBits() const { return 0; }
    virtual void Tick(float DeltaTime) {}

    FName ComponentName;
};

// FIncomingPacketRef stub
struct FIncomingPacketRef
{
    void* Packet = nullptr;
    std::shared_ptr<const FInternetAddr> Address;
};

// FNetTraceCollector stub
class FNetTraceCollector {};

// FNetBitWriter/FNetBitReader forward declarations (these extend FBitWriter/FBitReader)
struct FNetBitWriter;
struct FNetBitReader;

// Replication flags
struct FReplicationFlags
{
    uint32 bNetInitial : 1;
    uint32 bNetSimulated : 1;
    uint32 bNetOwner : 1;
    uint32 bRepPhysics : 1;
};

// AActor stub
class AActor : public UObject {};

// TSharedPtr / TSharedRef / TWeakObjectPtr stubs (use std::shared_ptr)
template<typename T>
using TSharedPtr = std::shared_ptr<T>;

template<typename T>
using TSharedRef = std::shared_ptr<T>;

template<typename T>
struct TWeakObjectPtr
{
    T* Ptr = nullptr;
    T* Get() const { return Ptr; }
    bool IsValid() const { return Ptr != nullptr; }
};

// TMap / TSet stubs
#include <unordered_map>
#include <unordered_set>

template<typename K, typename V>
using TMap = std::unordered_map<K, V>;

template<typename T>
using TSet = std::unordered_set<T>;

// LexToString
template<typename T>
const TCHAR* LexToString(T) { return TEXT(""); }

// =============================================================================
// CoreMinimal.h equivalent - includes everything
// =============================================================================
// (This file IS the CoreMinimal equivalent for the standalone build)
