// UE4Net - UE4 Networking Shared Library
// Extracted from Unreal Engine 4.27 source for standalone use
//
// This library provides the complete UE4 networking stack:
// - Bit-level serialization (FBitReader/FBitWriter)
// - Packet sequencing and acknowledgment (FNetPacketNotify)
// - Stateless connection handshake (cookie challenge/response)
// - Channel system (Control, Actor channels)
// - Bunch serialization (partial bunch reassembly)
// - Property replication (FRepLayout)
// - RPC dispatch framework

#pragma once

// Core types and compatibility shim
#include "UE4Net/CoreMinimal.h"

// Serialization
#include "UE4Net/Serialization/BitReader.h"
#include "UE4Net/Serialization/BitWriter.h"

// Networking packet layer
#include "UE4Net/Net/NetPacketNotify.h"
#include "UE4Net/Net/Util/SequenceNumber.h"
#include "UE4Net/Net/Util/SequenceHistory.h"

// Handshake
#include "UE4Net/PacketHandlers/StatelessConnectHandlerComponent.h"
#include "UE4Net/Crypto/SHA1.h"

// Channel system
#include "UE4Net/Channel/DataBunch.h"
#include "UE4Net/Channel/Channel.h"
#include "UE4Net/Channel/ControlChannel.h"
#include "UE4Net/Channel/ActorChannel.h"
#include "UE4Net/Channel/NetConnection.h"
#include "UE4Net/Channel/NetDriver.h"

// Replication
#include "UE4Net/Replication/RepLayout.h"

// RPC
#include "UE4Net/RPC/RPCDispatch.h"
