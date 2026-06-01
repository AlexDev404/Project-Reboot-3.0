// UE4 Native Protocol Network Driver - Implementation
// Integrates ue4net library into the standalone server
//
// Phase 1: Uses FBitReader/FBitWriter from ue4net for all serialization
// Phase 2: Raw UDP sockets + FNetPacketNotify + StatelessConnectHandlerComponent
// Phase 3: UControlChannel (NMT flow) + UActorChannel (per-actor replication)
// Phase 4: FRepLayout for delta property serialization
// Phase 5: FRPCDispatcher for RPC serialize/dispatch

#include "net/ue4_net_driver.h"
#include "util/logging.h"

#include <spdlog/fmt/fmt.h>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET(s) closesocket(s)
    #define INVALID_SOCKET_FD INVALID_SOCKET
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET(s) close(s)
    #define INVALID_SOCKET_FD (-1)
#endif

// =============================================================================
// UE4NetConnection Implementation
// =============================================================================

UE4NetConnection::UE4NetConnection()
{
}

UE4NetConnection::~UE4NetConnection()
{
    Close("Connection destroyed");
}

void UE4NetConnection::SendRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize)
{
    if (State != EState::Open || !InternalConnection) return;

    // Find actor channel for the RPC (typically channel 1 for player controller)
    // For now, send on the first actor channel available
    for (auto& [guid, chIndex] : NetGUIDToChannelIndex)
    {
        UChannel* Ch = InternalConnection->Channels[chIndex];
        if (Ch && Ch->ChType == EChannelType::CHTYPE_Actor)
        {
            UActorChannel* ActorCh = static_cast<UActorChannel*>(Ch);
            FBitWriter Writer(ParamSize * 8 + 64);
            Writer.SerializeBits(const_cast<uint8*>(ParamData), ParamSize * 8);
            ActorCh->SendRPC(FunctionName, Writer, true);
            return;
        }
    }
}

void UE4NetConnection::SendPropertyUpdate(FNetworkGUID NetGUID, uint16 Handle, const uint8* Data, int32 NumBits)
{
    if (State != EState::Open || !InternalConnection) return;

    auto It = NetGUIDToChannelIndex.find(NetGUID.Value);
    if (It == NetGUIDToChannelIndex.end()) return;

    UChannel* Ch = InternalConnection->Channels[It->second];
    if (!Ch || Ch->ChType != EChannelType::CHTYPE_Actor) return;

    // Actor channel handles the property serialization
    UActorChannel* ActorCh = static_cast<UActorChannel*>(Ch);
    ActorCh->ReplicateActor();
}

void UE4NetConnection::Close(const std::string& Reason)
{
    if (State == EState::Closed) return;
    State = EState::Closed;

    if (InternalConnection)
    {
        InternalConnection->Close();
        InternalConnection = nullptr;
    }

    if (!Reason.empty())
    {
        LOG_INFO(LogNet, "UE4 Connection {} closed: {}", ConnectionId, Reason);
    }
}

UActorChannel* UE4NetConnection::OpenActorChannel(FNetworkGUID NetGUID, int32 ChIndex)
{
    if (!InternalConnection || State != EState::Open) return nullptr;

    // Check if already open
    auto It = NetGUIDToChannelIndex.find(NetGUID.Value);
    if (It != NetGUIDToChannelIndex.end())
    {
        return static_cast<UActorChannel*>(InternalConnection->Channels[It->second]);
    }

    // Allocate channel index
    if (ChIndex < 0)
    {
        ChIndex = NextActorChannelIndex++;
    }

    UChannel* Ch = InternalConnection->CreateChannel(EChannelType::CHTYPE_Actor, ChIndex);
    if (!Ch) return nullptr;

    UActorChannel* ActorCh = static_cast<UActorChannel*>(Ch);
    ActorCh->SetChannelActor(nullptr, NetGUID);
    NetGUIDToChannelIndex[NetGUID.Value] = ChIndex;

    return ActorCh;
}

UActorChannel* UE4NetConnection::FindActorChannel(FNetworkGUID NetGUID)
{
    if (!InternalConnection) return nullptr;

    auto It = NetGUIDToChannelIndex.find(NetGUID.Value);
    if (It == NetGUIDToChannelIndex.end()) return nullptr;

    UChannel* Ch = InternalConnection->Channels[It->second];
    if (!Ch || Ch->ChType != EChannelType::CHTYPE_Actor) return nullptr;

    return static_cast<UActorChannel*>(Ch);
}

// =============================================================================
// UE4NetDriver Implementation
// =============================================================================

UE4NetDriver::UE4NetDriver()
{
}

UE4NetDriver::~UE4NetDriver()
{
    Shutdown();
}

bool UE4NetDriver::Initialize(uint16_t Port)
{
#ifdef _WIN32
    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
    {
        LOG_ERROR(LogNet, "WSAStartup failed");
        return false;
    }
#endif

    // Create raw UDP socket
    if (!CreateSocket(Port))
    {
        LOG_ERROR(LogNet, "Failed to create UDP socket on port {}", Port);
        return false;
    }

    // Initialize the ue4net driver
    FString ListenURL("");
    if (!InternalDriver.Init(ListenURL, static_cast<int32>(Port)))
    {
        LOG_ERROR(LogNet, "Failed to initialize UE4 net driver");
        CloseSocket();
        return false;
    }

    // Set up connection factory so LowLevelSend uses our raw UDP socket
    InternalDriver.ConnectionFactory = [this](const FString& Address) -> UNetConnection* {
        auto* Conn = new UE4RawUDPConnection();
        Conn->SetRemoteAddress(Address.ToString());
        Conn->SetSendFunction([this](const uint8* Data, int32 Count, const std::string& Addr) {
            SendRawTo(Data, Count, Addr);
        });

        // When handshake completes (state -> Open), set up control channel handler
        Conn->OnStateChanged = [this](EConnectionState NewState) {
            if (NewState != EConnectionState::USOCK_Open) return;

            // Find the wrapper connection for this internal connection
            for (auto& WrapperConn : ClientConnections)
            {
                if (WrapperConn->InternalConnection && WrapperConn->InternalConnection->State == EConnectionState::USOCK_Open
                    && WrapperConn->State == UE4NetConnection::EState::Pending)
                {
                    WrapperConn->State = UE4NetConnection::EState::Open;
                    WrapperConn->bChallengeSent = false;

                    UControlChannel* CtrlCh = WrapperConn->InternalConnection->GetControlChannel();
                    if (CtrlCh)
                    {
                        CtrlCh->OnControlMessage = [this, ConnPtr = WrapperConn.get()](ENMTType Type, FBitReader& Data) {
                            HandleControlMessage(ConnPtr, Type, Data);
                        };

                        // Some clients do not reliably deliver/parse early NMT_Hello,
                        // so emit a single bootstrap challenge immediately after
                        // handshake and dedupe it against HandleHello.
                        SendChallengeOnce(WrapperConn.get(), "post-handshake");
                    }

                    LOG_INFO(LogNet, "UE4 Connection {} handshake complete", WrapperConn->ConnectionId);
                    break;
                }
            }
        };

        return Conn;
    };

    // Set up driver callbacks (OnConnectionAccepted fires on CreateConnection, before handshake)
    InternalDriver.OnConnectionAccepted = [this](UNetConnection* NewConn) {
        // Connection created but handshake not yet complete - nothing to do here
        // Actual setup happens in OnStateChanged above
    };

    InternalDriver.OnConnectionLost = [this](UNetConnection* LostConn) {
        for (auto& Conn : ClientConnections)
        {
            if (Conn->InternalConnection == LostConn)
            {
                Conn->State = UE4NetConnection::EState::Closed;
                if (OnPlayerDisconnected) OnPlayerDisconnected(Conn.get());
                break;
            }
        }
    };

    bListening = true;
    ListenPort = Port;
    LastSecretRotationTime = 0.0;

    LOG_INFO(LogNet, "UE4 Native Protocol server listening on port {} (raw UDP)", Port);
    return true;
}

void UE4NetDriver::Shutdown()
{
    // Close all connections
    for (auto& Conn : ClientConnections)
    {
        if (Conn && Conn->GetState() != UE4NetConnection::EState::Closed)
        {
            // Send NMT_Failure before closing
            if (Conn->InternalConnection && Conn->InternalConnection->GetControlChannel())
            {
                Conn->InternalConnection->GetControlChannel()->SendFailure(FString("Server shutting down"));
                Conn->InternalConnection->FlushNet();
            }
            Conn->Close("Server shutting down");
        }
    }

    InternalDriver.Shutdown();
    CloseSocket();
    ClientConnections.clear();
    AddressToConnection.clear();
    bListening = false;

#ifdef _WIN32
    WSACleanup();
#endif

    LOG_INFO(LogNet, "UE4 Native Protocol driver shut down");
}

void UE4NetDriver::TickFlush(float DeltaTime)
{
    if (!bListening) return;

    // Rotate handshake secrets periodically
    LastSecretRotationTime += DeltaTime;
    if (LastSecretRotationTime >= SECRET_ROTATION_INTERVAL)
    {
        LastSecretRotationTime = 0.0;
        // Secret rotation handled internally by ue4net's StatelessConnectHandlerComponent
    }

    // Poll for incoming UDP data
    PollSocket();

    // Tick the ue4net driver (processes packets, ticks channels, sends acks)
    InternalDriver.TickDispatch(DeltaTime);
    InternalDriver.TickFlush(DeltaTime);

    // Remove dead connections
    ClientConnections.erase(
        std::remove_if(ClientConnections.begin(), ClientConnections.end(),
            [this](const std::unique_ptr<UE4NetConnection>& Conn) {
                if (Conn->GetState() == UE4NetConnection::EState::Closed)
                {
                    AddressToConnection.erase(Conn->Address);
                    return true;
                }
                return false;
            }),
        ClientConnections.end()
    );
}

bool UE4NetDriver::CreateSocket(uint16_t Port)
{
    SocketFD = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (SocketFD == static_cast<int>(INVALID_SOCKET_FD))
    {
        LOG_ERROR(LogNet, "Failed to create UDP socket: {}", SOCKET_ERROR_CODE);
        return false;
    }

    // Set non-blocking
#ifdef _WIN32
    u_long NonBlocking = 1;
    ioctlsocket(static_cast<SOCKET>(SocketFD), FIONBIO, &NonBlocking);
#else
    int Flags = fcntl(SocketFD, F_GETFL, 0);
    fcntl(SocketFD, F_SETFL, Flags | O_NONBLOCK);
#endif

    // Allow address reuse
    int OptVal = 1;
    setsockopt(SocketFD, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char*>(&OptVal), sizeof(OptVal));

    // Increase recv buffer
    int RecvBufSize = 256 * 1024;
    setsockopt(SocketFD, SOL_SOCKET, SO_RCVBUF,
        reinterpret_cast<const char*>(&RecvBufSize), sizeof(RecvBufSize));

    // Bind
    struct sockaddr_in Addr{};
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(Port);
    Addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(SocketFD, reinterpret_cast<struct sockaddr*>(&Addr), sizeof(Addr)) < 0)
    {
        LOG_ERROR(LogNet, "Failed to bind UDP socket to port {}: {}", Port, SOCKET_ERROR_CODE);
        CLOSE_SOCKET(SocketFD);
        SocketFD = -1;
        return false;
    }

    return true;
}

void UE4NetDriver::CloseSocket()
{
    if (SocketFD >= 0)
    {
        CLOSE_SOCKET(SocketFD);
        SocketFD = -1;
    }
}

void UE4NetDriver::PollSocket()
{
    static constexpr int MAX_PACKET_SIZE = 2048;
    uint8 Buffer[MAX_PACKET_SIZE];
    struct sockaddr_in FromAddr{};
    socklen_t FromLen = sizeof(FromAddr);

    // Read all pending datagrams
    while (true)
    {
        int BytesRead = recvfrom(SocketFD, reinterpret_cast<char*>(Buffer), MAX_PACKET_SIZE, 0,
            reinterpret_cast<struct sockaddr*>(&FromAddr), &FromLen);

        if (BytesRead <= 0) break;

        // Build address string
        char AddrStr[64];
        snprintf(AddrStr, sizeof(AddrStr), "%s:%d",
            inet_ntoa(FromAddr.sin_addr), ntohs(FromAddr.sin_port));
        std::string FromAddress(AddrStr);

        // Route to existing connection or create new one
        auto It = AddressToConnection.find(FromAddress);
        UE4NetConnection* Connection = nullptr;

        if (It != AddressToConnection.end())
        {
            Connection = It->second;
            // Feed raw data to existing connection
            if (Connection->InternalConnection)
            {
                LOG_INFO(LogNet, "Routing {} bytes to connection {} from {} (handshake={})",
                    BytesRead, Connection->ConnectionId, FromAddress,
                    Connection->InternalConnection->bHandshakeComplete ? "done" : "pending");
                Connection->InternalConnection->ReceivedRawPacket(Buffer, BytesRead);
            }
        }
        else
        {
            // New connection - let the internal driver handle handshake initiation
            FString UE4Address(FromAddress.c_str());

            // Create our wrapper
            auto NewConn = std::make_unique<UE4NetConnection>();
            NewConn->ConnectionId = NextConnectionId++;
            NewConn->Address = FromAddress;
            NewConn->State = UE4NetConnection::EState::Pending;

            UE4NetConnection* Ptr = NewConn.get();
            AddressToConnection[FromAddress] = Ptr;
            ClientConnections.push_back(std::move(NewConn));

            {
                std::string HexDump;
                HexDump.reserve(BytesRead * 3);
                char HexBuf[4];
                for (int i = 0; i < BytesRead; i++)
                {
                    snprintf(HexBuf, sizeof(HexBuf), "%02X ", Buffer[i]);
                    HexDump += HexBuf;
                }
                LOG_INFO(LogNet, "New UE4 connection from {} (ID: {}), first packet: {} bytes, data=[{}]",
                    FromAddress, Ptr->ConnectionId, BytesRead, HexDump);
            }

            // Use ProcessIncomingConnection which creates the connection AND sends the challenge
            InternalDriver.ProcessIncomingConnection(Buffer, BytesRead, UE4Address);

            // Find the internal connection that was just created
            UNetConnection* InternalConn = InternalDriver.FindConnectionByAddress(UE4Address);
            if (InternalConn)
            {
                Ptr->InternalConnection = InternalConn;
                LOG_INFO(LogNet, "Internal connection linked for ID {}", Ptr->ConnectionId);
            }
            else
            {
                LOG_ERROR(LogNet, "Failed to find internal connection for {}", FromAddress);
            }
        }
    }
}

void UE4NetDriver::SendRawTo(const uint8* Data, int32 Count, const std::string& Address)
{
    // Parse "ip:port" address
    auto ColonPos = Address.rfind(':');
    if (ColonPos == std::string::npos) return;

    std::string IP = Address.substr(0, ColonPos);
    uint16_t Port = static_cast<uint16_t>(std::stoi(Address.substr(ColonPos + 1)));

    struct sockaddr_in Addr{};
    Addr.sin_family = AF_INET;
    Addr.sin_port = htons(Port);
    inet_pton(AF_INET, IP.c_str(), &Addr.sin_addr);

    int Sent = sendto(SocketFD, reinterpret_cast<const char*>(Data), Count, 0,
        reinterpret_cast<struct sockaddr*>(&Addr), sizeof(Addr));

    LOG_INFO(LogNet, "SendRawTo: {} bytes to {} (result={})", Count, Address, Sent);
}

UE4NetConnection* UE4NetDriver::CreateNewConnection(const std::string& FromAddress)
{
    auto NewConn = std::make_unique<UE4NetConnection>();
    NewConn->ConnectionId = NextConnectionId++;
    NewConn->Address = FromAddress;
    NewConn->State = UE4NetConnection::EState::Pending;

    // Create the ue4net-level connection
    FString UE4Address(FromAddress.c_str());
    UNetConnection* InternalConn = InternalDriver.CreateConnection(UE4Address);
    if (!InternalConn)
    {
        LOG_ERROR(LogNet, "Failed to create internal connection for {}", FromAddress);
        return nullptr;
    }

    // Initialize the internal connection
    InternalConn->InitBase(&InternalDriver, nullptr, UE4Address, EConnectionState::USOCK_Pending);
    InternalConn->RemoteAddressStr = UE4Address;

    // Override LowLevelSend to use our raw UDP socket
    // NOTE: In a full implementation, we'd subclass UNetConnection.
    // For now, we store the address and handle sends via the driver.

    NewConn->InternalConnection = InternalConn;

    LOG_INFO(LogNet, "New UE4 connection from {} (ID: {})", FromAddress, NewConn->ConnectionId);

    UE4NetConnection* Ptr = NewConn.get();
    AddressToConnection[FromAddress] = Ptr;
    ClientConnections.push_back(std::move(NewConn));

    return Ptr;
}

void UE4NetDriver::RemoveConnection(UE4NetConnection* Connection)
{
    if (!Connection) return;

    if (OnPlayerDisconnected) OnPlayerDisconnected(Connection);

    Connection->Close("Removed");
    // Actual removal happens in TickFlush cleanup
}

UE4NetConnection* UE4NetDriver::FindConnection(uint32_t ConnectionId) const
{
    for (auto& Conn : ClientConnections)
    {
        if (Conn && Conn->GetConnectionId() == ConnectionId)
            return Conn.get();
    }
    return nullptr;
}

void UE4NetDriver::KickPlayer(UE4NetConnection* Connection, const std::string& Reason)
{
    if (!Connection) return;

    LOG_INFO(LogNet, "Kicking player {} ({}): {}",
        Connection->GetPlayerName(), Connection->GetConnectionId(), Reason);

    // Send failure message via control channel
    if (Connection->InternalConnection && Connection->InternalConnection->GetControlChannel())
    {
        Connection->InternalConnection->GetControlChannel()->SendFailure(FString(Reason.c_str()));
        Connection->InternalConnection->FlushNet();
    }

    Connection->Close(Reason);
}

// =============================================================================
// Phase 3: NMT Control Message Handling
// =============================================================================

void UE4NetDriver::HandleControlMessage(UE4NetConnection* Connection, ENMTType Type, FBitReader& Data)
{
    switch (Type)
    {
        case ENMTType::NMT_Hello:
            HandleHello(Connection, Data);
            break;
        case ENMTType::NMT_Login:
            HandleLogin(Connection, Data);
            break;
        case ENMTType::NMT_Netspeed:
            HandleNetspeed(Connection, Data);
            break;
        case ENMTType::NMT_Join:
            HandleJoin(Connection, Data);
            break;
        default:
            LOG_INFO(LogNet, "Unhandled NMT message type {} from connection {}",
                static_cast<int>(Type), Connection->GetConnectionId());
            break;
    }
}

void UE4NetDriver::SendChallengeOnce(UE4NetConnection* Connection, const char* SourceTag)
{
    if (!Connection || !Connection->InternalConnection || Connection->bChallengeSent)
    {
        return;
    }

    UControlChannel* CtrlCh = Connection->InternalConnection->GetControlChannel();
    if (!CtrlCh)
    {
        return;
    }

    const std::string ChallengeStr = fmt::format("CHALLENGE_{:08X}",
        Connection->GetConnectionId());

    CtrlCh->SendChallenge(FString(ChallengeStr.c_str()));
    Connection->bChallengeSent = true;
    LOG_INFO(LogNet, "Sent NMT_Challenge to connection {} ({})",
        Connection->GetConnectionId(), SourceTag ? SourceTag : "unknown");
}

void UE4NetDriver::HandleHello(UE4NetConnection* Connection, FBitReader& Data)
{
    // Client sends: protocol version, encryption flag, token
    // We respond with NMT_Challenge

    LOG_INFO(LogNet, "Received NMT_Hello from connection {}", Connection->GetConnectionId());
    SendChallengeOnce(Connection, "hello");
}

void UE4NetDriver::HandleLogin(UE4NetConnection* Connection, FBitReader& Data)
{
    // Client sends: URL, UniqueId, OnlinePlatformName
    // We validate and respond with NMT_Welcome

    LOG_INFO(LogNet, "Received NMT_Login from connection {}", Connection->GetConnectionId());

    // Read login parameters from the bunch
    // In UE4 format: URL string, UniqueId string, Platform string
    FString LoginURL;
    FString UniqueId;
    FString Platform;

    // Deserialize (simplified - in real UE4, these are FStrings serialized via FBitReader)
    uint32 URLLen = 0;
    Data.SerializeIntPacked(URLLen);
    // For now, accept all logins

    // Notify game code of login
    if (OnPlayerLogin)
    {
        OnPlayerLogin(Connection, LoginURL.ToString(), UniqueId.ToString());
    }

    // Send NMT_Welcome with map info
    UControlChannel* CtrlCh = Connection->InternalConnection->GetControlChannel();
    if (CtrlCh)
    {
        // Welcome: MapName, GameName, URL
        FString MapName("/Game/Athena/Maps/Athena_Terrain");
        FString GameName("/Script/FortniteGame.FortGameModeAthena");
        FString WelcomeURL("");

        CtrlCh->SendWelcome(MapName, GameName, WelcomeURL);
    }

    LOG_INFO(LogNet, "Sent NMT_Welcome to connection {}", Connection->GetConnectionId());
}

void UE4NetDriver::HandleNetspeed(UE4NetConnection* Connection, FBitReader& Data)
{
    // Client reports their desired netspeed - acknowledge it
    LOG_INFO(LogNet, "Received NMT_Netspeed from connection {}", Connection->GetConnectionId());
}

void UE4NetDriver::HandleJoin(UE4NetConnection* Connection, FBitReader& Data)
{
    // Client has loaded the map and is ready to join
    // This completes the connection setup

    LOG_INFO(LogNet, "Received NMT_Join from connection {} - player fully connected",
        Connection->GetConnectionId());

    Connection->State = UE4NetConnection::EState::Open;

    // Notify game code
    if (OnPlayerConnected)
    {
        OnPlayerConnected(Connection);
    }
}

// =============================================================================
// Phase 4: Actor Channel / Replication Support
// =============================================================================

UActorChannel* UE4NetDriver::GetOrCreateActorChannel(UE4NetConnection* Connection, FNetworkGUID NetGUID)
{
    if (!Connection) return nullptr;

    UActorChannel* Ch = Connection->FindActorChannel(NetGUID);
    if (Ch) return Ch;

    return Connection->OpenActorChannel(NetGUID);
}

void UE4NetDriver::CloseActorChannel(UE4NetConnection* Connection, FNetworkGUID NetGUID)
{
    if (!Connection || !Connection->InternalConnection) return;

    UActorChannel* Ch = Connection->FindActorChannel(NetGUID);
    if (Ch)
    {
        Ch->Close();
    }
}

// =============================================================================
// Phase 5: RPC Registration and Dispatch
// =============================================================================

void UE4NetDriver::RegisterRPC(const FRPCDefinition& Definition)
{
    RPCDispatcher.RegisterRPC(Definition);
}

void UE4NetDriver::RegisterRPCHandler(const FName& FunctionName, FRPCDispatcher::FRPCCallback Callback, void* UserData)
{
    RPCDispatcher.RegisterHandler(FunctionName, Callback, UserData);
}

void UE4NetDriver::RegisterRepLayout(const FName& ClassName, const FRepLayout& Layout)
{
    RepLayouts[ClassName.ToString()] = Layout;
}

const FRepLayout* UE4NetDriver::FindRepLayout(const FName& ClassName) const
{
    auto It = RepLayouts.find(ClassName.ToString());
    return (It != RepLayouts.end()) ? &It->second : nullptr;
}

void UE4NetDriver::BroadcastRPC(const FName& FunctionName, const uint8* ParamData, int32 ParamSize, UE4NetConnection* Exclude)
{
    for (auto& Conn : ClientConnections)
    {
        if (Conn.get() != Exclude && Conn->IsValid())
        {
            Conn->SendRPC(FunctionName, ParamData, ParamSize);
        }
    }
}
