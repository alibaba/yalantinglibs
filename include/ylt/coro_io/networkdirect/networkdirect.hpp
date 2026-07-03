//
// Single-header NetworkDirect helper amalgamation.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// This file is generated from third_party/networkdirect/src/ndutil and embeds
// the mc.exe-generated ndstatus.h plus the ndutil implementation as inline C++.
// It is intended for C++17-or-newer header-only consumers.
//

#pragma once

#ifndef NETWORKDIRECT_SINGLE_HEADER_HPP
#define NETWORKDIRECT_SINGLE_HEADER_HPP

#ifndef __cplusplus
#error networkdirect.hpp requires C++17 or newer.
#endif

#if defined(_MSVC_LANG)
#if _MSVC_LANG < 201703L
#error networkdirect.hpp requires C++17 or newer.
#endif
#elif __cplusplus < 201703L
#error networkdirect.hpp requires C++17 or newer.
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#include <unknwn.h>
#include <guiddef.h>
#include <ifdef.h>
#include <limits.h>
#include <cstring>
#include <string.h>

#ifndef PFL_NETWORKDIRECT_PROVIDER
#define PFL_NETWORKDIRECT_PROVIDER 0x00000010
#endif

#ifndef __fallthrough
#define __fallthrough
#endif


// ===== BEGIN ndstatus.h =====

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

ndstatus.h - NetworkDirect Status Codes

Status codes with a facility of System map to NTSTATUS codes
of similar names.

--*/

#ifndef _NDSTATUS_
#define _NDSTATUS_

#pragma once


//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: ND_SUCCESS
//
// MessageText:
//
//  ND_SUCCESS
//
#define ND_SUCCESS                       ((HRESULT)0x00000000L)

//
// MessageId: ND_TIMEOUT
//
// MessageText:
//
//  ND_TIMEOUT
//
#define ND_TIMEOUT                       ((HRESULT)0x00000102L)

//
// MessageId: ND_PENDING
//
// MessageText:
//
//  ND_PENDING
//
#define ND_PENDING                       ((HRESULT)0x00000103L)

//
// MessageId: ND_BUFFER_OVERFLOW
//
// MessageText:
//
//  ND_BUFFER_OVERFLOW
//
#define ND_BUFFER_OVERFLOW               ((HRESULT)0x80000005L)

//
// MessageId: ND_DEVICE_BUSY
//
// MessageText:
//
//  ND_DEVICE_BUSY
//
#define ND_DEVICE_BUSY                   ((HRESULT)0x80000011L)

//
// MessageId: ND_NO_MORE_ENTRIES
//
// MessageText:
//
//  ND_NO_MORE_ENTRIES
//
#define ND_NO_MORE_ENTRIES               ((HRESULT)0x8000001AL)

//
// MessageId: ND_UNSUCCESSFUL
//
// MessageText:
//
//  ND_UNSUCCESSFUL
//
#define ND_UNSUCCESSFUL                  ((HRESULT)0xC0000001L)

//
// MessageId: ND_ACCESS_VIOLATION
//
// MessageText:
//
//  ND_ACCESS_VIOLATION
//
#define ND_ACCESS_VIOLATION              ((HRESULT)0xC0000005L)

//
// MessageId: ND_INVALID_HANDLE
//
// MessageText:
//
//  ND_INVALID_HANDLE
//
#define ND_INVALID_HANDLE                ((HRESULT)0xC0000008L)

//
// MessageId: ND_INVALID_DEVICE_REQUEST
//
// MessageText:
//
//  ND_INVALID_DEVICE_REQUEST
//
#define ND_INVALID_DEVICE_REQUEST        ((HRESULT)0xC0000010L)

//
// MessageId: ND_INVALID_PARAMETER
//
// MessageText:
//
//  ND_INVALID_PARAMETER
//
#define ND_INVALID_PARAMETER             ((HRESULT)0xC000000DL)

//
// MessageId: ND_NO_MEMORY
//
// MessageText:
//
//  ND_NO_MEMORY
//
#define ND_NO_MEMORY                     ((HRESULT)0xC0000017L)

//
// MessageId: ND_INVALID_PARAMETER_MIX
//
// MessageText:
//
//  ND_INVALID_PARAMETER_MIX
//
#define ND_INVALID_PARAMETER_MIX         ((HRESULT)0xC0000030L)

//
// MessageId: ND_DATA_OVERRUN
//
// MessageText:
//
//  ND_DATA_OVERRUN
//
#define ND_DATA_OVERRUN                  ((HRESULT)0xC000003CL)

//
// MessageId: ND_SHARING_VIOLATION
//
// MessageText:
//
//  ND_SHARING_VIOLATION
//
#define ND_SHARING_VIOLATION             ((HRESULT)0xC0000043L)

//
// MessageId: ND_INSUFFICIENT_RESOURCES
//
// MessageText:
//
//  ND_INSUFFICIENT_RESOURCES
//
#define ND_INSUFFICIENT_RESOURCES        ((HRESULT)0xC000009AL)

//
// MessageId: ND_DEVICE_NOT_READY
//
// MessageText:
//
//  ND_DEVICE_NOT_READY
//
#define ND_DEVICE_NOT_READY              ((HRESULT)0xC00000A3L)

//
// MessageId: ND_IO_TIMEOUT
//
// MessageText:
//
//  ND_IO_TIMEOUT
//
#define ND_IO_TIMEOUT                    ((HRESULT)0xC00000B5L)

//
// MessageId: ND_NOT_SUPPORTED
//
// MessageText:
//
//  ND_NOT_SUPPORTED
//
#define ND_NOT_SUPPORTED                 ((HRESULT)0xC00000BBL)

//
// MessageId: ND_INTERNAL_ERROR
//
// MessageText:
//
//  ND_INTERNAL_ERROR
//
#define ND_INTERNAL_ERROR                ((HRESULT)0xC00000E5L)

//
// MessageId: ND_INVALID_PARAMETER_1
//
// MessageText:
//
//  ND_INVALID_PARAMETER_1
//
#define ND_INVALID_PARAMETER_1           ((HRESULT)0xC00000EFL)

//
// MessageId: ND_INVALID_PARAMETER_2
//
// MessageText:
//
//  ND_INVALID_PARAMETER_2
//
#define ND_INVALID_PARAMETER_2           ((HRESULT)0xC00000F0L)

//
// MessageId: ND_INVALID_PARAMETER_3
//
// MessageText:
//
//  ND_INVALID_PARAMETER_3
//
#define ND_INVALID_PARAMETER_3           ((HRESULT)0xC00000F1L)

//
// MessageId: ND_INVALID_PARAMETER_4
//
// MessageText:
//
//  ND_INVALID_PARAMETER_4
//
#define ND_INVALID_PARAMETER_4           ((HRESULT)0xC00000F2L)

//
// MessageId: ND_INVALID_PARAMETER_5
//
// MessageText:
//
//  ND_INVALID_PARAMETER_5
//
#define ND_INVALID_PARAMETER_5           ((HRESULT)0xC00000F3L)

//
// MessageId: ND_INVALID_PARAMETER_6
//
// MessageText:
//
//  ND_INVALID_PARAMETER_6
//
#define ND_INVALID_PARAMETER_6           ((HRESULT)0xC00000F4L)

//
// MessageId: ND_INVALID_PARAMETER_7
//
// MessageText:
//
//  ND_INVALID_PARAMETER_7
//
#define ND_INVALID_PARAMETER_7           ((HRESULT)0xC00000F5L)

//
// MessageId: ND_INVALID_PARAMETER_8
//
// MessageText:
//
//  ND_INVALID_PARAMETER_8
//
#define ND_INVALID_PARAMETER_8           ((HRESULT)0xC00000F6L)

//
// MessageId: ND_INVALID_PARAMETER_9
//
// MessageText:
//
//  ND_INVALID_PARAMETER_9
//
#define ND_INVALID_PARAMETER_9           ((HRESULT)0xC00000F7L)

//
// MessageId: ND_INVALID_PARAMETER_10
//
// MessageText:
//
//  ND_INVALID_PARAMETER_10
//
#define ND_INVALID_PARAMETER_10          ((HRESULT)0xC00000F8L)

//
// MessageId: ND_CANCELED
//
// MessageText:
//
//  ND_CANCELED
//
#define ND_CANCELED                      ((HRESULT)0xC0000120L)

//
// MessageId: ND_REMOTE_ERROR
//
// MessageText:
//
//  ND_REMOTE_ERROR
//
#define ND_REMOTE_ERROR                  ((HRESULT)0xC000013DL)

//
// MessageId: ND_INVALID_ADDRESS
//
// MessageText:
//
//  ND_INVALID_ADDRESS
//
#define ND_INVALID_ADDRESS               ((HRESULT)0xC0000141L)

//
// MessageId: ND_INVALID_DEVICE_STATE
//
// MessageText:
//
//  ND_INVALID_DEVICE_STATE
//
#define ND_INVALID_DEVICE_STATE          ((HRESULT)0xC0000184L)

//
// MessageId: ND_INVALID_BUFFER_SIZE
//
// MessageText:
//
//  ND_INVALID_BUFFER_SIZE
//
#define ND_INVALID_BUFFER_SIZE           ((HRESULT)0xC0000206L)

//
// MessageId: ND_TOO_MANY_ADDRESSES
//
// MessageText:
//
//  ND_TOO_MANY_ADDRESSES
//
#define ND_TOO_MANY_ADDRESSES            ((HRESULT)0xC0000209L)

//
// MessageId: ND_ADDRESS_ALREADY_EXISTS
//
// MessageText:
//
//  ND_ADDRESS_ALREADY_EXISTS
//
#define ND_ADDRESS_ALREADY_EXISTS        ((HRESULT)0xC000020AL)

//
// MessageId: ND_CONNECTION_REFUSED
//
// MessageText:
//
//  ND_CONNECTION_REFUSED
//
#define ND_CONNECTION_REFUSED            ((HRESULT)0xC0000236L)

//
// MessageId: ND_CONNECTION_INVALID
//
// MessageText:
//
//  ND_CONNECTION_INVALID
//
#define ND_CONNECTION_INVALID            ((HRESULT)0xC000023AL)

//
// MessageId: ND_CONNECTION_ACTIVE
//
// MessageText:
//
//  ND_CONNECTION_ACTIVE
//
#define ND_CONNECTION_ACTIVE             ((HRESULT)0xC000023BL)

//
// MessageId: ND_NETWORK_UNREACHABLE
//
// MessageText:
//
//  ND_NETWORK_UNREACHABLE
//
#define ND_NETWORK_UNREACHABLE           ((HRESULT)0xC000023CL)

//
// MessageId: ND_HOST_UNREACHABLE
//
// MessageText:
//
//  ND_HOST_UNREACHABLE
//
#define ND_HOST_UNREACHABLE              ((HRESULT)0xC000023DL)

//
// MessageId: ND_CONNECTION_ABORTED
//
// MessageText:
//
//  ND_CONNECTION_ABORTED
//
#define ND_CONNECTION_ABORTED            ((HRESULT)0xC0000241L)

//
// MessageId: ND_DEVICE_REMOVED
//
// MessageText:
//
//  ND_DEVICE_REMOVED
//
#define ND_DEVICE_REMOVED                ((HRESULT)0xC00002B6L)

#endif // _NDSTATUS_

// ===== END ndstatus.h =====


// ===== BEGIN nddef.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//    nddef.h
//
// Abstract:
//
//    NetworkDirect Service Provider structure definitions
//
// Environment:
//
//    User mode and kernel mode
//


#ifndef _NDDEF_H_
#define _NDDEF_H_

#pragma once

#define ND_VERSION_1    0x1
#define ND_VERSION_2    0x20000

#ifndef NDVER
#define NDVER      ND_VERSION_2
#endif

#define ND_ADAPTER_FLAG_IN_ORDER_DMA_SUPPORTED              0x00000001
#define ND_ADAPTER_FLAG_CQ_INTERRUPT_MODERATION_SUPPORTED   0x00000004
#define ND_ADAPTER_FLAG_MULTI_ENGINE_SUPPORTED              0x00000008
#define ND_ADAPTER_FLAG_CQ_RESIZE_SUPPORTED                 0x00000100
#define ND_ADAPTER_FLAG_LOOPBACK_CONNECTIONS_SUPPORTED      0x00010000

#define ND_CQ_NOTIFY_ERRORS                                 0
#define ND_CQ_NOTIFY_ANY                                    1
#define ND_CQ_NOTIFY_SOLICITED                              2

#define ND_MR_FLAG_ALLOW_LOCAL_WRITE                        0x00000001
#define ND_MR_FLAG_ALLOW_REMOTE_READ                        0x00000002
#define ND_MR_FLAG_ALLOW_REMOTE_WRITE                       0x00000005
#define ND_MR_FLAG_RDMA_READ_SINK                           0x00000008
#define ND_MR_FLAG_DO_NOT_SECURE_VM                         0x80000000

#define ND_OP_FLAG_SILENT_SUCCESS                           0x00000001
#define ND_OP_FLAG_READ_FENCE                               0x00000002
#define ND_OP_FLAG_SEND_AND_SOLICIT_EVENT                   0x00000004
#define ND_OP_FLAG_ALLOW_READ                               0x00000008
#define ND_OP_FLAG_ALLOW_WRITE                              0x00000010
#if NDVER >= ND_VERSION_2
#define ND_OP_FLAG_INLINE                                   0x00000020
#endif

typedef struct _ND2_ADAPTER_INFO {
    ULONG   InfoVersion;
    UINT16  VendorId;
    UINT16  DeviceId;
    UINT64  AdapterId;
    SIZE_T  MaxRegistrationSize;
    SIZE_T  MaxWindowSize;
    ULONG   MaxInitiatorSge;
    ULONG   MaxReceiveSge;
    ULONG   MaxReadSge;
    ULONG   MaxTransferLength;
    ULONG   MaxInlineDataSize;
    ULONG   MaxInboundReadLimit;
    ULONG   MaxOutboundReadLimit;
    ULONG   MaxReceiveQueueDepth;
    ULONG   MaxInitiatorQueueDepth;
    ULONG   MaxSharedReceiveQueueDepth;
    ULONG   MaxCompletionQueueDepth;
    ULONG   InlineRequestThreshold;
    ULONG   LargeRequestThreshold;
    ULONG   MaxCallerData;
    ULONG   MaxCalleeData;
    ULONG   AdapterFlags;
} ND2_ADAPTER_INFO;

typedef struct _ND2_ADAPTER_INFO32 {
    ULONG   InfoVersion;
    UINT16  VendorId;
    UINT16  DeviceId;
    UINT64  AdapterId;
    ULONG   MaxRegistrationSize;
    ULONG   MaxWindowSize;
    ULONG   MaxInitiatorSge;
    ULONG   MaxReceiveSge;
    ULONG   MaxReadSge;
    ULONG   MaxTransferLength;
    ULONG   MaxInlineDataSize;
    ULONG   MaxInboundReadLimit;
    ULONG   MaxOutboundReadLimit;
    ULONG   MaxReceiveQueueDepth;
    ULONG   MaxInitiatorQueueDepth;
    ULONG   MaxSharedReceiveQueueDepth;
    ULONG   MaxCompletionQueueDepth;
    ULONG   InlineRequestThreshold;
    ULONG   LargeRequestThreshold;
    ULONG   MaxCallerData;
    ULONG   MaxCalleeData;
    ULONG   AdapterFlags;
} ND2_ADAPTER_INFO32;

typedef enum _ND2_REQUEST_TYPE {
    Nd2RequestTypeReceive,
    Nd2RequestTypeSend,
    Nd2RequestTypeBind,
    Nd2RequestTypeInvalidate,
    Nd2RequestTypeRead,
    Nd2RequestTypeWrite
} ND2_REQUEST_TYPE;

typedef struct _ND2_RESULT {
    HRESULT             Status;
    ULONG               BytesTransferred;
    VOID*               QueuePairContext;
    VOID*               RequestContext;
    ND2_REQUEST_TYPE    RequestType;
} ND2_RESULT;

typedef struct _ND2_SGE {
    VOID*   Buffer;
    ULONG   BufferLength;
    UINT32  MemoryRegionToken;
} ND2_SGE;

#endif // _NDDEF_H_

// ===== END nddef.h =====


// ===== BEGIN ndioctl.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
//
// Module Name:
//
//    ndioctl.h
//
// Abstract:
//
//    NetworkDirect Service Provider IOCTL definitions
//
// Environment:
//
//    User mode and kernel mode
//


#ifndef _NDIOCTL_H_
#define _NDIOCTL_H_

#pragma once

#include <ws2def.h>
#include <ws2ipdef.h>
#include <limits.h>
#include <ifdef.h>

#define ND_IOCTL_VERSION    1

#pragma warning(push)
#pragma warning(disable:4201)

typedef enum _ND_MAPPING_TYPE {
    NdMapIoSpace,
    NdMapMemory,
    NdMapMemoryCoallesce,
    NdMapPages,
    NdMapPagesCoallesce,
    NdUnmapIoSpace,
    NdUnmapMemory,
    NdMaximumMapType
} ND_MAPPING_TYPE;

typedef enum _ND_CACHING_TYPE {
    NdNonCached = 0,    // MmNonCached
    NdCached = 1,       // MmCached
    NdWriteCombined = 2,// MmWriteCombined
    NdMaximumCacheType
} ND_CACHING_TYPE;

typedef enum _ND_ACCESS_TYPE {
    NdReadAccess = 0,   // IoReadAccess
    NdWriteAccess = 1,  // IoWriteAccess
    NdModifyAccess = 2  // IoModifyAccess
} ND_ACCESS_TYPE;

typedef struct _ND_MAP_IO_SPACE {
    ND_MAPPING_TYPE MapType;
    ND_CACHING_TYPE CacheType;
    ULONG           CbLength;
} ND_MAP_IO_SPACE;

typedef struct _ND_MAP_MEMORY {
    ND_MAPPING_TYPE MapType;
    ND_ACCESS_TYPE  AccessType;
    UINT64          Address;
    ULONG           CbLength;
} ND_MAP_MEMORY;

typedef struct _ND_MAPPING_ID {
    ND_MAPPING_TYPE MapType;
    UINT64          Id;
} ND_MAPPING_ID;

typedef struct _NDK_MAP_PAGES {
    ND_MAP_MEMORY   Header;
    ULONG           CbLogicalPageAddressesOffset;
} NDK_MAP_PAGES;

typedef union _ND_MAPPING {
    ND_MAPPING_TYPE MapType;
    ND_MAP_IO_SPACE MapIoSpace;
    ND_MAP_MEMORY   MapMemory;
    ND_MAPPING_ID   MappingId;
    NDK_MAP_PAGES   MapPages;
} ND_MAPPING;

typedef struct _ND_MAPPING_RESULT {
    UINT64  Id;
    UINT64  Information;
} ND_MAPPING_RESULT;

typedef struct _ND_RESOURCE_DESCRIPTOR {
    UINT64              Handle;
    ULONG               CeMappingResults;
    ULONG               CbMappingResultsOffset;
} ND_RESOURCE_DESCRIPTOR;

typedef struct _ND_HANDLE {
    ULONG   Version;
    ULONG   Reserved;
    UINT64  Handle;
} ND_HANDLE;

typedef struct _ND_RESOLVE_ADDRESS {
    ULONG           Version;
    ULONG           Reserved;
    SOCKADDR_INET   Address;
} ND_RESOLVE_ADDRESS;

typedef struct _ND_OPEN_ADAPTER {
    ULONG       Version;
    ULONG       Reserved;
    ULONG       CeMappingCount;
    ULONG       CbMappingsOffset;
    UINT64      AdapterId;
} ND_OPEN_ADAPTER;

typedef struct _ND_ADAPTER_QUERY {
    ULONG   Version;
    ULONG   InfoVersion;
    UINT64  AdapterHandle;
} ND_ADAPTER_QUERY;

typedef struct _ND_CREATE_CQ {
    ULONG           Version;
    ULONG           QueueDepth;
    ULONG           CeMappingCount;
    ULONG           CbMappingsOffset;
    UINT64          AdapterHandle;
    GROUP_AFFINITY  Affinity;
} ND_CREATE_CQ;

typedef struct _ND_CREATE_SRQ {
    ULONG           Version;
    ULONG           QueueDepth;
    ULONG           CeMappingCount;
    ULONG           CbMappingsOffset;
    ULONG           MaxRequestSge;
    ULONG           NotifyThreshold;
    UINT64          PdHandle;
    GROUP_AFFINITY  Affinity;
} ND_CREATE_SRQ;

typedef struct _ND_CREATE_QP_HDR {
    ULONG       Version;
    ULONG       CbMaxInlineData;
    ULONG       CeMappingCount;
    ULONG       CbMappingsOffset;
    ULONG       InitiatorQueueDepth;
    ULONG       MaxInitiatorRequestSge;
    UINT64      ReceiveCqHandle;
    UINT64      InitiatorCqHandle;
    UINT64      PdHandle;
} ND_CREATE_QP_HDR;

typedef struct _ND_CREATE_QP {
    ND_CREATE_QP_HDR    Header;
    ULONG               ReceiveQueueDepth;
    ULONG               MaxReceiveRequestSge;
} ND_CREATE_QP;

typedef struct _ND_CREATE_QP_WITH_SRQ {
    ND_CREATE_QP_HDR    Header;
    UINT64              SrqHandle;
} ND_CREATE_QP_WITH_SRQ;

typedef struct _ND_SRQ_MODIFY {
    ULONG       Version;
    ULONG       QueueDepth;
    ULONG       CeMappingCount;
    ULONG       CbMappingsOffset;
    ULONG       NotifyThreshold;
    ULONG       Reserved;
    UINT64      SrqHandle;
} ND_SRQ_MODIFY;

typedef struct _ND_CQ_MODIFY {
    ULONG       Version;
    ULONG       QueueDepth;
    ULONG       CeMappingCount;
    ULONG       CbMappingsOffset;
    UINT64      CqHandle;
} ND_CQ_MODIFY;

typedef struct _ND_CQ_NOTIFY {
    ULONG   Version;
    ULONG   Type;
    UINT64  CqHandle;
} ND_CQ_NOTIFY;

typedef struct _ND_MR_REGISTER_HDR {
    ULONG   Version;
    ULONG   Flags;
    UINT64  CbLength;
    UINT64  TargetAddress;
    UINT64  MrHandle;
} ND_MR_REGISTER_HDR;

typedef struct _ND_MR_REGISTER {
    ND_MR_REGISTER_HDR  Header;
    UINT64              Address;
} ND_MR_REGISTER;

typedef struct _ND_BIND {
    ULONG           Version;
    ULONG           Reserved;
    UINT64          Handle;
    SOCKADDR_INET   Address;
} ND_BIND, NDV_PARTITION_UNBIND_ADDRESS;

typedef struct _ND_READ_LIMITS {
    ULONG   Inbound;
    ULONG   Outbound;
} ND_READ_LIMITS;

typedef struct _ND_CONNECT {
    ULONG               Version;
    ULONG               Reserved;
    ND_READ_LIMITS      ReadLimits;
    ULONG               CbPrivateDataLength;
    ULONG               CbPrivateDataOffset;
    UINT64              ConnectorHandle;
    UINT64              QpHandle;
    SOCKADDR_INET       DestinationAddress;
    IF_PHYSICAL_ADDRESS DestinationHwAddress;
} ND_CONNECT;

typedef struct _ND_ACCEPT {
    ULONG           Version;
    ULONG           Reserved;
    ND_READ_LIMITS  ReadLimits;
    ULONG           CbPrivateDataLength;
    ULONG           CbPrivateDataOffset;
    UINT64          ConnectorHandle;
    UINT64          QpHandle;
} ND_ACCEPT;

typedef struct _ND_REJECT {
    ULONG   Version;
    ULONG   Reserved;
    ULONG   CbPrivateDataLength;
    ULONG   CbPrivateDataOffset;
    UINT64  ConnectorHandle;
} ND_REJECT;

typedef struct _ND_LISTEN {
    ULONG   Version;
    ULONG   Backlog;
    UINT64  ListenerHandle;
} ND_LISTEN;

typedef struct _ND_GET_CONNECTION_REQUEST {
    ULONG   Version;
    ULONG   Reserved;
    UINT64  ListenerHandle;
    UINT64  ConnectorHandle;
} ND_GET_CONNECTION_REQUEST;


#if defined(_DDK_DRIVER_) || defined(_NTIFS_)

typedef enum _NDV_MMIO_TYPE {
    NdPartitionKernelVirtual,
    NdPartitionSystemPhysical,
    NdPartitionGuestPhysical,
    NdMaximumMmioType
} NDV_MMIO_TYPE;

typedef struct _NDV_RESOLVE_ADAPTER_ID {
    ULONG               Version;
    IF_PHYSICAL_ADDRESS HwAddress;
} NDV_RESOLVE_ADAPTER_ID;

typedef struct _NDV_PARTITION_CREATE {
    ULONG               Version;
    NDV_MMIO_TYPE       MmioType;
    UINT64              AdapterId;
    UINT64              XmitCap;
} NDV_PARTITION_CREATE;

typedef struct _NDV_PARTITION_BIND_LUID {
    ULONG               Version;
    ULONG               Reserved;
    UINT64              PartitionHandle;
    IF_PHYSICAL_ADDRESS HwAddress;
    IF_LUID             Luid;
} NDV_PARTITION_BIND_LUID;

typedef struct _NDV_PARTITION_BIND_ADDRESS {
    ULONG               Version;
    ULONG               Reserved;
    UINT64              PartitionHandle;
    SOCKADDR_INET       Address;
    IF_PHYSICAL_ADDRESS GuestHwAddress;
    IF_PHYSICAL_ADDRESS HwAddress;
} NDV_PARTITION_BIND_ADDRESS;

typedef struct _NDK_MR_REGISTER {
    ND_MR_REGISTER_HDR  Header;
    ULONG               CbLogicalPageAddressesOffset;
} NDK_MR_REGISTER;

typedef struct _NDK_BIND {
    ND_BIND Header;
    LUID    AuthenticationId;
    BOOLEAN IsAdmin;
} NDK_BIND;

#endif  // _DDK_DRIVER_

#pragma warning(pop)

#define ND_FUNCTION(r_, i_)    (r_ << 6 | i_)
#define IOCTL_ND(r_, i_)   CTL_CODE( FILE_DEVICE_NETWORK, ND_FUNCTION(r_, i_), METHOD_BUFFERED, FILE_ANY_ACCESS )

#define ND_FUNCTION_FROM_CTL_CODE(ctrlCode_)     ((ctrlCode_ >> 2) & 0xFFF)
#define ND_RESOURCE_FROM_CTL_CODE(ctrlCode_)     (ND_FUNCTION_FROM_CTL_CODE(ctrlCode_) >> 6)
#define ND_OPERATION_FROM_CTRL_CODE(ctrlCode_)   (ND_FUNCTION_FROM_CTL_CODE(ctrlCode_) & 0x3F)

#define ND_DOS_DEVICE_NAME L"\\DosDevices\\Global\\NetworkDirect"
#define ND_WIN32_DEVICE_NAME L"\\\\.\\NetworkDirect"

typedef enum _ND_RESOURCE_TYPE {
    NdProvider = 0,
    NdAdapter = 1,
    NdPd = 2,
    NdCq = 3,
    NdMr = 4,
    NdMw = 5,
    NdSrq = 6,
    NdConnector = 7,
    NdListener = 8,
    NdQp = 9,
    NdVirtualPartition = 10,
    ND_RESOURCE_TYPE_COUNT
} ND_RESOURCE_TYPE;

#define ND_OPERATION_COUNT 14

#define IOCTL_ND_PROVIDER(i_)           IOCTL_ND(NdProvider, i_)
#define IOCTL_ND_ADAPTER(i_)            IOCTL_ND(NdAdapter, i_)
#define IOCTL_ND_PD(i_)                 IOCTL_ND(NdPd, i_)
#define IOCTL_ND_CQ(i_)                 IOCTL_ND(NdCq, i_)
#define IOCTL_ND_MR(i_)                 IOCTL_ND(NdMr, i_)
#define IOCTL_ND_MW(i_)                 IOCTL_ND(NdMw, i_)
#define IOCTL_ND_SRQ(i_)                IOCTL_ND(NdSrq, i_)
#define IOCTL_ND_CONNECTOR(i_)          IOCTL_ND(NdConnector, i_)
#define IOCTL_ND_LISTENER(i_)           IOCTL_ND(NdListener, i_)
#define IOCTL_ND_QP(i_)                 IOCTL_ND(NdQp, i_)
#define IOCTL_ND_VIRTUAL_PARTITION(i_)  IOCTL_ND(NdVirtualPartition, i_)

// Provider IOCTLs
#define IOCTL_ND_PROVIDER_INIT                      IOCTL_ND_PROVIDER( 0 )
#define IOCTL_ND_PROVIDER_BIND_FILE                 IOCTL_ND_PROVIDER( 1 )
#define IOCTL_ND_PROVIDER_QUERY_ADDRESS_LIST        IOCTL_ND_PROVIDER( 2 )
#define IOCTL_ND_PROVIDER_RESOLVE_ADDRESS           IOCTL_ND_PROVIDER( 3 )
#define IOCTL_ND_PROVIDER_MAX_OPERATION                                4

// Adapter IOCTLs
#define IOCTL_ND_ADAPTER_OPEN                       IOCTL_ND_ADAPTER( 0 )
#define IOCTL_ND_ADAPTER_CLOSE                      IOCTL_ND_ADAPTER( 1 )
#define IOCTL_ND_ADAPTER_QUERY                      IOCTL_ND_ADAPTER( 2 )
#define IOCTL_ND_ADAPTER_QUERY_ADDRESS_LIST         IOCTL_ND_ADAPTER( 3 )
#define IOCTL_ND_ADAPTER_MAX_OPERATION                                4

// Protection Domain IOCTLs
#define IOCTL_ND_PD_CREATE                          IOCTL_ND_PD( 0 )
#define IOCTL_ND_PD_FREE                            IOCTL_ND_PD( 1 )
#define IOCTL_ND_PD_MAX_OPERATION                                2

// Completion Queue IOCTLs
#define IOCTL_ND_CQ_CREATE                          IOCTL_ND_CQ( 0 )
#define IOCTL_ND_CQ_FREE                            IOCTL_ND_CQ( 1 )
#define IOCTL_ND_CQ_CANCEL_IO                       IOCTL_ND_CQ( 2 )
#define IOCTL_ND_CQ_GET_AFFINITY                    IOCTL_ND_CQ( 3 )
#define IOCTL_ND_CQ_MODIFY                          IOCTL_ND_CQ( 4 )
#define IOCTL_ND_CQ_NOTIFY                          IOCTL_ND_CQ( 5 )
#define IOCTL_ND_CQ_MAX_OPERATION                                6

// Memory Region IOCTLs
#define IOCTL_ND_MR_CREATE                          IOCTL_ND_MR( 0 )
#define IOCTL_ND_MR_FREE                            IOCTL_ND_MR( 1 )
#define IOCTL_ND_MR_CANCEL_IO                       IOCTL_ND_MR( 2 )
#define IOCTL_ND_MR_REGISTER                        IOCTL_ND_MR( 3 )
#define IOCTL_ND_MR_DEREGISTER                      IOCTL_ND_MR( 4 )
#define IOCTL_NDK_MR_REGISTER                       IOCTL_ND_MR( 5 )
#define IOCTL_ND_MR_MAX_OPERATION                                6

// Memory Window IOCTLs
#define IOCTL_ND_MW_CREATE                          IOCTL_ND_MW( 0 )
#define IOCTL_ND_MW_FREE                            IOCTL_ND_MW( 1 )
#define IOCTL_ND_MW_MAX_OPERATION                                2

// Shared Receive Queue IOCTLs
#define IOCTL_ND_SRQ_CREATE                         IOCTL_ND_SRQ( 0 )
#define IOCTL_ND_SRQ_FREE                           IOCTL_ND_SRQ( 1 )
#define IOCTL_ND_SRQ_CANCEL_IO                      IOCTL_ND_SRQ( 2 )
#define IOCTL_ND_SRQ_GET_AFFINITY                   IOCTL_ND_SRQ( 3 )
#define IOCTL_ND_SRQ_MODIFY                         IOCTL_ND_SRQ( 4 )
#define IOCTL_ND_SRQ_NOTIFY                         IOCTL_ND_SRQ( 5 )
#define IOCTL_ND_SRQ_MAX_OPERATION                                6

// Connector IOCTLs
#define IOCTL_ND_CONNECTOR_CREATE                   IOCTL_ND_CONNECTOR( 0 )
#define IOCTL_ND_CONNECTOR_FREE                     IOCTL_ND_CONNECTOR( 1 )
#define IOCTL_ND_CONNECTOR_CANCEL_IO                IOCTL_ND_CONNECTOR( 2 )
#define IOCTL_ND_CONNECTOR_BIND                     IOCTL_ND_CONNECTOR( 3 )
#define IOCTL_ND_CONNECTOR_CONNECT                  IOCTL_ND_CONNECTOR( 4 )
#define IOCTL_ND_CONNECTOR_COMPLETE_CONNECT         IOCTL_ND_CONNECTOR( 5 )
#define IOCTL_ND_CONNECTOR_ACCEPT                   IOCTL_ND_CONNECTOR( 6 )
#define IOCTL_ND_CONNECTOR_REJECT                   IOCTL_ND_CONNECTOR( 7 )
#define IOCTL_ND_CONNECTOR_GET_READ_LIMITS          IOCTL_ND_CONNECTOR( 8 )
#define IOCTL_ND_CONNECTOR_GET_PRIVATE_DATA         IOCTL_ND_CONNECTOR( 9 )
#define IOCTL_ND_CONNECTOR_GET_PEER_ADDRESS         IOCTL_ND_CONNECTOR( 10 )
#define IOCTL_ND_CONNECTOR_GET_ADDRESS              IOCTL_ND_CONNECTOR( 11 )
#define IOCTL_ND_CONNECTOR_NOTIFY_DISCONNECT        IOCTL_ND_CONNECTOR( 12 )
#define IOCTL_ND_CONNECTOR_DISCONNECT               IOCTL_ND_CONNECTOR( 13 )
#define IOCTL_ND_CONNECTOR_MAX_OPERATION                                14

// Listener IOCTLs
#define IOCTL_ND_LISTENER_CREATE                    IOCTL_ND_LISTENER( 0 )
#define IOCTL_ND_LISTENER_FREE                      IOCTL_ND_LISTENER( 1 )
#define IOCTL_ND_LISTENER_CANCEL_IO                 IOCTL_ND_LISTENER( 2 )
#define IOCTL_ND_LISTENER_BIND                      IOCTL_ND_LISTENER( 3 )
#define IOCTL_ND_LISTENER_LISTEN                    IOCTL_ND_LISTENER( 4 )
#define IOCTL_ND_LISTENER_GET_ADDRESS               IOCTL_ND_LISTENER( 5 )
#define IOCTL_ND_LISTENER_GET_CONNECTION_REQUEST    IOCTL_ND_LISTENER( 6 )
#define IOCTL_ND_LISTENER_MAX_OPERATION                                7

// Queue Pair IOCTLs
#define IOCTL_ND_QP_CREATE                          IOCTL_ND_QP( 0 )
#define IOCTL_ND_QP_CREATE_WITH_SRQ                 IOCTL_ND_QP( 1 )
#define IOCTL_ND_QP_FREE                            IOCTL_ND_QP( 2 )
#define IOCTL_ND_QP_FLUSH                           IOCTL_ND_QP( 3 )
#define IOCTL_ND_QP_MAX_OPERATION                                4

// Kernel-mode only IOCTLs (IRP_MJ_INTERNAL_DEVICE_CONTROL)
#define IOCTL_NDV_PARTITION_RESOLVE_ADAPTER_ID      IOCTL_ND_VIRTUAL_PARTITION( 0 )
#define IOCTL_NDV_PARTITION_CREATE                  IOCTL_ND_VIRTUAL_PARTITION( 1 )
#define IOCTL_NDV_PARTITION_FREE                    IOCTL_ND_VIRTUAL_PARTITION( 2 )
#define IOCTL_NDV_PARTITION_BIND                    IOCTL_ND_VIRTUAL_PARTITION( 3 )
#define IOCTL_NDV_PARTITION_UNBIND                  IOCTL_ND_VIRTUAL_PARTITION( 4 )
#define IOCTL_NDV_PARTITION_BIND_LUID               IOCTL_ND_VIRTUAL_PARTITION( 5 )
#define IOCTL_NDV_PARTITION_MAX_OPERATION                                       6


#if defined(_DDK_DRIVER_) || defined(_NTIFS_)

__inline NTSTATUS
NdValidateMemoryMapping(
    __in const ND_MAPPING* pMapping,
    ND_ACCESS_TYPE AccessType,
    ULONG CbLength
)
{
    if (pMapping->MapType != NdMapMemory && pMapping->MapType != NdMapPages)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (pMapping->MapMemory.AccessType != AccessType ||
        pMapping->MapMemory.CbLength < CbLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

__inline NTSTATUS
NdValidateCoallescedMapping(
    __in const ND_MAPPING* pMapping,
    ND_ACCESS_TYPE AccessType,
    ULONG CbLength
)
{
    if (pMapping->MapType != NdMapMemoryCoallesce && pMapping->MapType != NdMapPagesCoallesce)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (pMapping->MapMemory.AccessType != AccessType ||
        BYTE_OFFSET(pMapping->MapMemory.Address) + pMapping->MapMemory.CbLength > PAGE_SIZE ||
        pMapping->MapMemory.CbLength != CbLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

__inline NTSTATUS
NdValidateIoSpaceMapping(
    __in const ND_MAPPING* pMapping,
    ND_CACHING_TYPE CacheType,
    ULONG CbLength
)
{
    if (pMapping->MapType != NdMapIoSpace ||
        pMapping->MapIoSpace.CacheType != CacheType ||
        pMapping->MapIoSpace.CbLength != CbLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

__inline void
NdThunkAdapterInfo(
    __out ND2_ADAPTER_INFO32* pInfo32,
    __in const ND2_ADAPTER_INFO* pInfo
)
{
    pInfo32->InfoVersion = pInfo->InfoVersion;
    pInfo32->VendorId = pInfo->VendorId;
    pInfo32->DeviceId = pInfo->DeviceId;
    pInfo32->AdapterId = pInfo->AdapterId;
    pInfo32->MaxRegistrationSize = (ULONG)(min(ULONG_MAX, pInfo->MaxRegistrationSize));
    pInfo32->MaxWindowSize = (ULONG)(min(ULONG_MAX, pInfo->MaxWindowSize));
    pInfo32->MaxInitiatorSge = pInfo->MaxInitiatorSge;
    pInfo32->MaxReceiveSge = pInfo->MaxReceiveSge;
    pInfo32->MaxReadSge = pInfo->MaxReadSge;
    pInfo32->MaxTransferLength = pInfo->MaxTransferLength;
    pInfo32->MaxInlineDataSize = pInfo->MaxInlineDataSize;
    pInfo32->MaxInboundReadLimit = pInfo->MaxInboundReadLimit;
    pInfo32->MaxOutboundReadLimit = pInfo->MaxOutboundReadLimit;
    pInfo32->MaxReceiveQueueDepth = pInfo->MaxReceiveQueueDepth;
    pInfo32->MaxInitiatorQueueDepth = pInfo->MaxInitiatorQueueDepth;
    pInfo32->MaxSharedReceiveQueueDepth = pInfo->MaxSharedReceiveQueueDepth;
    pInfo32->MaxCompletionQueueDepth = pInfo->MaxCompletionQueueDepth;
    pInfo32->InlineRequestThreshold = pInfo->InlineRequestThreshold;
    pInfo32->LargeRequestThreshold = pInfo->LargeRequestThreshold;
    pInfo32->MaxCallerData = pInfo->MaxCallerData;
    pInfo32->MaxCalleeData = pInfo->MaxCalleeData;
    pInfo32->AdapterFlags = pInfo->AdapterFlags;
}

#endif  // _DDK_DRIVER_

#endif // _NDSPI_H_

// ===== END ndioctl.h =====


// ===== BEGIN ndspi.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
// Module Name:
//
//    ndspi.h
// 
// Abstract:
//
//    NetworkDirect Service Provider Interfaces
//
// Environment:
//
//    User mode
//


#pragma once

#ifndef _NDSPI_H_
#define _NDSPI_H_

#include <winsock2.h>
#include <unknwn.h>


//
// Overlapped object
//
#undef INTERFACE
#define INTERFACE IND2Overlapped

// {ABF72719-B016-4a40-A6F7-622791A7044C}
inline constexpr IID IID_IND2Overlapped =
    { 0xabf72719, 0xb016, 0x4a40, { 0xa6, 0xf7, 0x62, 0x27, 0x91, 0xa7, 0x4, 0x4c } };

DECLARE_INTERFACE_(IND2Overlapped, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    STDMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    STDMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;
};


//
// Completion Queue
//
#undef INTERFACE
#define INTERFACE IND2CompletionQueue

// {20CC445E-64A0-4cbb-AA75-F6A7251FDA9E}
inline constexpr IID IID_IND2CompletionQueue =
    { 0x20cc445e, 0x64a0, 0x4cbb, { 0xaa, 0x75, 0xf6, 0xa7, 0x25, 0x1f, 0xda, 0x9e } };

DECLARE_INTERFACE_(IND2CompletionQueue, IND2Overlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;

    // *** IND2CompletionQueue methods ***
    STDMETHOD(GetNotifyAffinity)(
        THIS_
        __out USHORT* pGroup,
        __out KAFFINITY* pAffinity
        ) PURE;

    STDMETHOD(Resize)(
        THIS_
        ULONG queueDepth
        ) PURE;

    STDMETHOD(Notify)(
        THIS_
        ULONG type,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD_(ULONG, GetResults)(
        THIS_
        __out_ecount_part(nResults, return) ND2_RESULT results[],
        ULONG nResults
        ) PURE;
};


//
// Shared Receive Queue
//
#undef INTERFACE
#define INTERFACE IND2SharedReceiveQueue

// {AABD67DC-459A-4db1-826B-56CFCC278883}
inline constexpr IID IID_IND2SharedReceiveQueue =
    { 0xaabd67dc, 0x459a, 0x4db1, { 0x82, 0x6b, 0x56, 0xcf, 0xcc, 0x27, 0x88, 0x83 } };

DECLARE_INTERFACE_(IND2SharedReceiveQueue, IND2Overlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;

    // *** IND2SharedReceiveQueue methods ***
    STDMETHOD(GetNotifyAffinity)(
        THIS_
        __out USHORT* pGroup,
        __out KAFFINITY* pAffinity
        ) PURE;

    STDMETHOD(Modify)(
        THIS_
        ULONG queueDepth,
        ULONG notifyThreshold
        ) PURE;

    STDMETHOD(Notify)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Receive)(
        THIS_
        __in VOID* requestContext,
        __in_ecount_opt(nSge) const ND2_SGE sge[],
        ULONG nSge
        ) PURE;
};


//
// Memory Window
//
#undef INTERFACE
#define INTERFACE IND2MemoryWindow

// {070FE1F5-0AB5-4361-88DB-974BA704D4B9}
inline constexpr IID IID_IND2MemoryWindow =
    { 0x70fe1f5, 0xab5, 0x4361, { 0x88, 0xdb, 0x97, 0x4b, 0xa7, 0x4, 0xd4, 0xb9 } };

DECLARE_INTERFACE_(IND2MemoryWindow, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2MemoryWindow methods ***
    STDMETHOD_(UINT32, GetRemoteToken)(
        THIS
        ) PURE;
};


//
// Memory Region
//
#undef INTERFACE
#define INTERFACE IND2MemoryRegion

// {55DFEA2F-BC56-4982-8A45-0301BE46C413}
inline constexpr IID IID_IND2MemoryRegion =
    { 0x55dfea2f, 0xbc56, 0x4982, { 0x8a, 0x45, 0x3, 0x1, 0xbe, 0x46, 0xc4, 0x13 } };

DECLARE_INTERFACE_(IND2MemoryRegion, IND2Overlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;

    // *** IND2MemoryRegion methods ***

    //////////////////////////////////
    // flags - Combination of ND_MR_FLAG_ALLOW_XXX.  Note remote flags imply local.
    STDMETHOD(Register)(
        THIS_
        __in_bcount(cbBuffer) const VOID* pBuffer,
        SIZE_T cbBuffer,
        ULONG flags,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Deregister)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD_(UINT32, GetLocalToken)(
        THIS
        ) PURE;

    STDMETHOD_(UINT32, GetRemoteToken)(
        THIS
        ) PURE;
};


//
// QueuePair
//
#undef INTERFACE
#define INTERFACE IND2QueuePair

// {EEF2F332-B75D-4063-BCE3-3A0BAD2D02CE}
inline constexpr IID IID_IND2QueuePair =
    { 0xeef2f332, 0xb75d, 0x4063, { 0xbc, 0xe3, 0x3a, 0xb, 0xad, 0x2d, 0x2, 0xce } };

DECLARE_INTERFACE_(IND2QueuePair, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2QueuePair methods ***
    STDMETHOD(Flush)(
        THIS
        ) PURE;

    STDMETHOD(Send)(
        THIS_
        __in_opt VOID* requestContext,
        __in_ecount_opt(nSge) const ND2_SGE sge[],
        ULONG nSge,
        ULONG flags
        ) PURE;

    STDMETHOD(Receive)(
        THIS_
        __in_opt VOID* requestContext,
        __in_ecount_opt(nSge) const ND2_SGE sge[],
        ULONG nSge
        ) PURE;

    // RemoteToken available thorugh IND2Mw::GetRemoteToken.
    STDMETHOD(Bind)(
        THIS_
        __in_opt VOID* requestContext,
        __in IUnknown* pMemoryRegion,
        __inout IUnknown* pMemoryWindow,
        __in_bcount(cbBuffer) const VOID* pBuffer,
        SIZE_T cbBuffer,
        ULONG flags
        ) PURE;

    STDMETHOD(Invalidate)(
        THIS_
        __in_opt VOID* requestContext,
        __in IUnknown* pMemoryWindow,
        ULONG flags
        ) PURE;

    STDMETHOD(Read)(
        THIS_
        __in_opt VOID* requestContext,
        __in_ecount_opt(nSge) const ND2_SGE sge[],
        ULONG nSge,
        UINT64 remoteAddress,
        UINT32 remoteToken,
        ULONG flags
        ) PURE;

    STDMETHOD(Write)(
        THIS_
        __in_opt VOID* requestContext,
        __in_ecount_opt(nSge) const ND2_SGE sge[],
        ULONG nSge,
        UINT64 remoteAddress,
        UINT32 remoteToken,
        ULONG flags
        ) PURE;
};


//
// Connector
//
#undef INTERFACE
#define INTERFACE IND2Connector

// {7DD615C4-6B4C-4866-950C-F3B1D25A5302}
inline constexpr IID IID_IND2Connector =
    { 0x7dd615c4, 0x6b4c, 0x4866, { 0x95, 0xc, 0xf3, 0xb1, 0xd2, 0x5a, 0x53, 0x2 } };

DECLARE_INTERFACE_(IND2Connector, IND2Overlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;

    // *** IND2Connector methods ***
    STDMETHOD(Bind)(
        THIS_
        __in_bcount(cbAddress) const struct sockaddr* pAddress,
        ULONG cbAddress
        ) PURE;

    STDMETHOD(Connect)(
        THIS_
        __in IUnknown* pQueuePair,
        __in_bcount(cbDestAddress) const struct sockaddr* pDestAddress,
        ULONG cbDestAddress,
        ULONG inboundReadLimit,
        ULONG outboundReadLimit,
        __in_bcount_opt(cbPrivateData) const VOID* pPrivateData,
        ULONG cbPrivateData,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(CompleteConnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Accept)(
        THIS_
        __in IUnknown* pQueuePair,
        ULONG inboundReadLimit,
        ULONG outboundReadLimit,
        __in_bcount_opt(cbPrivateData) const VOID* pPrivateData,
        ULONG cbPrivateData,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Reject)(
        THIS_
        __in_bcount_opt(cbPrivateData) const VOID* pPrivateData,
        ULONG cbPrivateData
        ) PURE;

    STDMETHOD(GetReadLimits)(
        THIS_
        __out_opt ULONG* pInboundReadLimit,
        __out_opt ULONG* pOutboundReadLimit
        ) PURE;

    STDMETHOD(GetPrivateData)(
        THIS_
        __out_bcount_opt(*pcbPrivateData) VOID* pPrivateData,
        __inout ULONG* pcbPrivateData
        ) PURE;

    STDMETHOD(GetLocalAddress)(
        THIS_
        __out_bcount_part_opt(*pcbAddress, *pcbAddress) struct sockaddr* pAddress,
        __inout ULONG* pcbAddress
        ) PURE;

    STDMETHOD(GetPeerAddress)(
        THIS_
        __out_bcount_part_opt(*pcbAddress, *pcbAddress) struct sockaddr* pAddress,
        __inout ULONG* pcbAddress
        ) PURE;

    STDMETHOD(NotifyDisconnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Disconnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;
};


//
// Listener
//
#undef INTERFACE
#define INTERFACE IND2Listener

// {65D23D83-3A57-4E02-86A4-350165C2D130}
inline constexpr IID IID_IND2Listener =
    { 0x65d23d83, 0x3a57, 0x4e02, { 0x86, 0xa4, 0x35, 0x1, 0x65, 0xc2, 0xd1, 0x30 } };

DECLARE_INTERFACE_(IND2Listener, IND2Overlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Overlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __in OVERLAPPED* pOverlapped,
        BOOL wait
        ) PURE;

    // *** IND2Listen methods ***
    STDMETHOD(Bind)(
        THIS_
        __in_bcount(cbAddress) const struct sockaddr* pAddress,
        ULONG cbAddress
        ) PURE;

    STDMETHOD(Listen)(
        THIS_
        ULONG backlog
        ) PURE;

    STDMETHOD(GetLocalAddress)(
        THIS_
        __out_bcount_part_opt(*pcbAddress, *pcbAddress) struct sockaddr* pAddress,
        __inout ULONG* pcbAddress
        ) PURE;

    STDMETHOD(GetConnectionRequest)(
        THIS_
        __inout IUnknown* pConnector,
        __inout OVERLAPPED* pOverlapped
        ) PURE;
};


//
// Adapter
//
#undef INTERFACE
#define INTERFACE IND2Adapter

// {D89C798C-4823-4D69-846C-DFDCCFF9E5F3}
inline constexpr IID IID_IND2Adapter =
    { 0xd89c798c, 0x4823, 0x4d69, { 0x84, 0x6c, 0xdf, 0xdc, 0xcf, 0xf9, 0xe5, 0xf3 } };

DECLARE_INTERFACE_(IND2Adapter, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Adapter methods ***
    STDMETHOD(CreateOverlappedFile)(
        THIS_
        __deref_out HANDLE* phOverlappedFile
        ) PURE;

    STDMETHOD(Query)(
        THIS_
        __inout_bcount_opt(*pcbInfo) ND2_ADAPTER_INFO* pInfo,
        __inout ULONG* pcbInfo
        ) PURE;

    STDMETHOD(QueryAddressList)(
        THIS_
        __out_bcount_part_opt(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
        __inout ULONG* pcbAddressList
        ) PURE;

    STDMETHOD(CreateCompletionQueue)(
        THIS_
        __in REFIID iid,
        __in HANDLE hOverlappedFile,
        ULONG queueDepth,
        USHORT group,
        KAFFINITY affinity,
        __deref_out VOID** ppCompletionQueue
        ) PURE;

    STDMETHOD(CreateMemoryRegion)(
        THIS_
        __in REFIID iid,
        __in HANDLE hOverlappedFile,
        __deref_out VOID** ppMemoryRegion
        ) PURE;

    STDMETHOD(CreateMemoryWindow)(
        THIS_
        __in REFIID iid,
        __deref_out VOID** ppMemoryWindow
        ) PURE;

    STDMETHOD(CreateSharedReceiveQueue)(
        THIS_
        __in REFIID iid,
        __in HANDLE hOverlappedFile,
        ULONG queueDepth,
        ULONG maxRequestSge,
        ULONG notifyThreshold,
        USHORT group,
        KAFFINITY affinity,
        __deref_out VOID** ppSharedReceiveQueue
        ) PURE;

    STDMETHOD(CreateQueuePair)(
        THIS_
        __in REFIID iid,
        __in IUnknown* pReceiveCompletionQueue,
        __in IUnknown* pInitiatorCompletionQueue,
        __in_opt VOID* context,
        ULONG receiveQueueDepth,
        ULONG initiatorQueueDepth,
        ULONG maxReceiveRequestSge,
        ULONG maxInitiatorRequestSge,
        ULONG inlineDataSize,
        __deref_out VOID** ppQueuePair
        ) PURE;

    STDMETHOD(CreateQueuePairWithSrq)(
        THIS_
        __in REFIID iid,
        __in IUnknown* pReceiveCompletionQueue,
        __in IUnknown* pInitiatorCompletionQueue,
        __in IUnknown* pSharedReceiveQueue,
        __in_opt VOID* context,
        ULONG initiatorQueueDepth,
        ULONG maxInitiatorRequestSge,
        ULONG inlineDataSize,
        __deref_out VOID** ppQueuePair
        ) PURE;

    STDMETHOD(CreateConnector)(
        THIS_
        __in REFIID iid,
        __in HANDLE hOverlappedFile,
        __deref_out VOID** ppConnector
        ) PURE;

    STDMETHOD(CreateListener)(
        THIS_
        __in REFIID iid,
        __in HANDLE hOverlappedFile,
        __deref_out VOID** ppListener
        ) PURE;
};


//
// Provider
//
#undef INTERFACE
#define INTERFACE IND2Provider

// {49EAE6C1-76C4-46D0-8003-5C2EAA2C9A8E}
inline constexpr IID IID_IND2Provider =
    { 0x49eae6c1, 0x76c4, 0x46d0, { 0x80, 0x3, 0x5c, 0x2e, 0xaa, 0x2c, 0x9a, 0x8e } };

DECLARE_INTERFACE_(IND2Provider, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** IND2Provider methods ***
    STDMETHOD(QueryAddressList)(
        THIS_
        __out_bcount_part_opt(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
        __inout ULONG* pcbAddressList
        ) PURE;

    STDMETHOD(ResolveAddress)(
        THIS_
        __in_bcount(cbAddress) const struct sockaddr* pAddress,
        ULONG cbAddress,
        __out UINT64* pAdapterId
        ) PURE;

    STDMETHOD(OpenAdapter)(
        THIS_
        __in REFIID iid,
        UINT64 adapterId,
        __deref_out VOID** ppAdapter
        ) PURE;
};


///////////////////////////////////////////////////////////////////////////////
//
// HPC Pack 2008 SDK interface
//
///////////////////////////////////////////////////////////////////////////////

DECLARE_HANDLE(ND_MR_HANDLE);


typedef struct _ND_ADAPTER_INFO1
{
    UINT32 VendorId;
    UINT32 DeviceId;
    SIZE_T MaxInboundSge;
    SIZE_T MaxInboundRequests;
    SIZE_T MaxInboundLength;
    SIZE_T MaxOutboundSge;
    SIZE_T MaxOutboundRequests;
    SIZE_T MaxOutboundLength;
    SIZE_T MaxInlineData;
    SIZE_T MaxInboundReadLimit;
    SIZE_T MaxOutboundReadLimit;
    SIZE_T MaxCqEntries;
    SIZE_T MaxRegistrationSize;
    SIZE_T MaxWindowSize;
    SIZE_T LargeRequestThreshold;
    SIZE_T MaxCallerData;
    SIZE_T MaxCalleeData;

} ND_ADAPTER_INFO1;
#define ND_ADAPTER_INFO ND_ADAPTER_INFO1

typedef struct _ND_RESULT
{
    HRESULT Status;
    SIZE_T BytesTransferred;

} ND_RESULT;

#pragma pack( push, 1 )
typedef struct _ND_MW_DESCRIPTOR
{
    UINT64 Base;    // Network byte order
    UINT64 Length;  // Network byte order
    UINT32 Token;   // Network byte order

} ND_MW_DESCRIPTOR;
#pragma pack( pop )

typedef struct _ND_SGE
{
    VOID* pAddr;
    SIZE_T Length;
    ND_MR_HANDLE hMr;

} ND_SGE;

//
// Overlapped object
//
#undef INTERFACE
#define INTERFACE INDOverlapped

// {C859E15E-75E2-4fe3-8D6D-0DFF36F02442}
inline constexpr IID IID_INDOverlapped =
    { 0xc859e15e, 0x75e2, 0x4fe3, { 0x8d, 0x6d, 0xd, 0xff, 0x36, 0xf0, 0x24, 0x42 } };

DECLARE_INTERFACE_(INDOverlapped, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDOverlapped methods ***
    STDMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    STDMETHOD(GetOverlappedResult)(
        THIS_
        __inout OVERLAPPED *pOverlapped,
        __out SIZE_T *pNumberOfBytesTransferred,
        BOOL bWait
        ) PURE;
};


//
// Completion Queue
//
#undef INTERFACE
#define INTERFACE INDCompletionQueue

// {1245A633-2A32-473a-830C-E05D1F869D02}
inline constexpr IID IID_INDCompletionQueue =
    { 0x1245a633, 0x2a32, 0x473a, { 0x83, 0xc, 0xe0, 0x5d, 0x1f, 0x86, 0x9d, 0x2 } };

DECLARE_INTERFACE_(INDCompletionQueue, INDOverlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDOverlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __inout OVERLAPPED *pOverlapped,
        __out SIZE_T *pNumberOfBytesTransferred,
        BOOL bWait
        ) PURE;

    // *** INDCompletionQueue methods ***
    STDMETHOD(Resize)(
        THIS_
        SIZE_T nEntries
        ) PURE;

    STDMETHOD(Notify)(
        THIS_
        DWORD Type,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD_(SIZE_T, GetResults)(
        THIS_
        __out_ecount_part_opt(nResults, return) ND_RESULT* pResults[],
        SIZE_T nResults
        ) PURE;
};


//
// Remove View
//
#undef INTERFACE
#define INTERFACE INDMemoryWindow

// {070FE1F5-0AB5-4361-88DB-974BA704D4B9}
inline constexpr IID IID_INDMemoryWindow =
    { 0x70fe1f5, 0xab5, 0x4361, { 0x88, 0xdb, 0x97, 0x4b, 0xa7, 0x4, 0xd4, 0xb9 } };

DECLARE_INTERFACE_(INDMemoryWindow, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;
};


//
// Endpoint
//
#undef INTERFACE
#define INTERFACE INDEndpoint

// {DBD00EAB-B679-44a9-BD65-E82F3DE12D1A}
inline constexpr IID IID_INDEndpoint =
    { 0xdbd00eab, 0xb679, 0x44a9, { 0xbd, 0x65, 0xe8, 0x2f, 0x3d, 0xe1, 0x2d, 0x1a } };

DECLARE_INTERFACE_(INDEndpoint, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDEndpoint methods ***
    STDMETHOD(Flush)(
        THIS
        ) PURE;

    STDMETHOD_(void, StartRequestBatch)(
        THIS
        ) PURE;

    STDMETHOD_(void, SubmitRequestBatch)(
        THIS
        ) PURE;

    STDMETHOD(Send)(
        THIS_
        __out ND_RESULT* pResult,
        __in_ecount_opt(nSge) const ND_SGE* pSgl,
        SIZE_T nSge,
        DWORD Flags
        ) PURE;

    STDMETHOD(SendAndInvalidate)(
        THIS_
        __out ND_RESULT* pResult,
        __in_ecount_opt(nSge) const ND_SGE* pSgl,
        SIZE_T nSge,
        __in const ND_MW_DESCRIPTOR* pRemoteMwDescriptor,
        DWORD Flags
        ) PURE;

    STDMETHOD(Receive)(
        THIS_
        __out ND_RESULT* pResult,
        __in_ecount_opt(nSge) const ND_SGE* pSgl,
        SIZE_T nSge
        ) PURE;

    STDMETHOD(Bind)(
        THIS_
        __out ND_RESULT* pResult,
        __in ND_MR_HANDLE hMr,
        __in INDMemoryWindow* pMw,
        __in_bcount(BufferSize) const void* pBuffer,
        SIZE_T BufferSize,
        DWORD Flags,
        __out ND_MW_DESCRIPTOR* pMwDescriptor
        ) PURE;

    STDMETHOD(Invalidate)(
        THIS_
        __out ND_RESULT* pResult,
        __in INDMemoryWindow* pMw,
        DWORD Flags
        ) PURE;

    STDMETHOD(Read)(
        THIS_
        __out ND_RESULT* pResult,
        __in_ecount_opt(nSge) const ND_SGE* pSgl,
        SIZE_T nSge,
        __in const ND_MW_DESCRIPTOR* pRemoteMwDescriptor,
        ULONGLONG Offset,
        DWORD Flags
        ) PURE;

    STDMETHOD(Write)(
        THIS_
        __out ND_RESULT* pResult,
        __in_ecount_opt(nSge) const ND_SGE* pSgl,
        SIZE_T nSge,
        __in const ND_MW_DESCRIPTOR* pRemoteMwDescriptor,
        ULONGLONG Offset,
        DWORD Flags
        ) PURE;
};


//
// Connector
//
#undef INTERFACE
#define INTERFACE INDConnector

// {1BCAF2D1-E274-4aeb-AC57-CD5D4376E0B7}
inline constexpr IID IID_INDConnector =
    { 0x1bcaf2d1, 0xe274, 0x4aeb, { 0xac, 0x57, 0xcd, 0x5d, 0x43, 0x76, 0xe0, 0xb7 } };

DECLARE_INTERFACE_(INDConnector, INDOverlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDOverlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __inout OVERLAPPED *pOverlapped,
        __out SIZE_T *pNumberOfBytesTransferred,
        BOOL bWait
        ) PURE;

    // *** INDConnector methods ***
    STDMETHOD(CreateEndpoint)(
        THIS_
        __in INDCompletionQueue* pInboundCq,
        __in INDCompletionQueue* pOutboundCq,
        SIZE_T nInboundEntries,
        SIZE_T nOutboundEntries,
        SIZE_T nInboundSge,
        SIZE_T nOutboundSge,
        SIZE_T InboundReadLimit,
        SIZE_T OutboundReadLimit,
        __out_opt SIZE_T* pMaxInlineData,
        __deref_out INDEndpoint** ppEndpoint
        ) PURE;

    STDMETHOD(Connect)(
        THIS_
        __in INDEndpoint* pEndpoint,
        __in_bcount(AddressLength) const struct sockaddr* pAddress,
        SIZE_T AddressLength,
        INT Protocol,
        USHORT LocalPort,
        __in_bcount_opt(PrivateDataLength) const void* pPrivateData,
        SIZE_T PrivateDataLength,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(CompleteConnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Accept)(
        THIS_
        __in INDEndpoint* pEndpoint,
        __in_bcount_opt(PrivateDataLength) const void* pPrivateData,
        SIZE_T PrivateDataLength,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Reject)(
        THIS_
        __in_bcount_opt(PrivateDataLength) const void* pPrivateData,
        SIZE_T PrivateDataLength
        ) PURE;

    STDMETHOD(GetConnectionData)(
        THIS_
        __out_opt SIZE_T* pInboundReadLimit,
        __out_opt SIZE_T* pOutboundReadLimit,
        __out_bcount_part_opt(*pPrivateDataLength, *pPrivateDataLength) void* pPrivateData,
        __inout_opt SIZE_T* pPrivateDataLength
        ) PURE;

    STDMETHOD(GetLocalAddress)(
        THIS_
        __out_bcount_part_opt(*pAddressLength, *pAddressLength) struct sockaddr* pAddress,
        __inout SIZE_T* pAddressLength
        ) PURE;

    STDMETHOD(GetPeerAddress)(
        THIS_
        __out_bcount_part_opt(*pAddressLength, *pAddressLength) struct sockaddr* pAddress,
        __inout SIZE_T* pAddressLength
        ) PURE;

    STDMETHOD(NotifyDisconnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(Disconnect)(
        THIS_
        __inout OVERLAPPED* pOverlapped
        ) PURE;
};


//
// Listen
//
#undef INTERFACE
#define INTERFACE INDListen

// {BB902588-BA3F-4441-9FE1-3B6795E4E668}
inline constexpr IID IID_INDListen =
    { 0xbb902588, 0xba3f, 0x4441, { 0x9f, 0xe1, 0x3b, 0x67, 0x95, 0xe4, 0xe6, 0x68 } };

DECLARE_INTERFACE_(INDListen, INDOverlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDOverlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __inout OVERLAPPED *pOverlapped,
        __out SIZE_T *pNumberOfBytesTransferred,
        BOOL bWait
        ) PURE;

    // *** INDListen methods ***
    STDMETHOD(GetConnectionRequest)(
        THIS_
        __inout INDConnector* pConnector,
        __inout OVERLAPPED* pOverlapped
        ) PURE;
};


//
// INDAdapter
//
#undef INTERFACE
#define INTERFACE INDAdapter

// {A023C5A0-5B73-43bc-8D20-33AA07E9510F}
inline constexpr IID IID_INDAdapter =
    { 0xa023c5a0, 0x5b73, 0x43bc, { 0x8d, 0x20, 0x33, 0xaa, 0x7, 0xe9, 0x51, 0xf } };

DECLARE_INTERFACE_(INDAdapter, INDOverlapped)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDOverlapped methods ***
    IFACEMETHOD(CancelOverlappedRequests)(
        THIS
        ) PURE;

    IFACEMETHOD(GetOverlappedResult)(
        THIS_
        __inout OVERLAPPED *pOverlapped,
        __out SIZE_T *pNumberOfBytesTransferred,
        BOOL bWait
        ) PURE;

    // *** INDAdapter methods ***
    STDMETHOD_(HANDLE, GetFileHandle)(
        THIS
        ) PURE;

    STDMETHOD(Query)(
        THIS_
        DWORD VersionRequested,
        __out_bcount_part_opt(*pBufferSize, *pBufferSize) ND_ADAPTER_INFO* pInfo,
        __inout_opt SIZE_T* pBufferSize
        ) PURE;

    STDMETHOD(Control)(
        THIS_
        DWORD IoControlCode,
        __in_bcount_opt(InBufferSize) const void* pInBuffer,
        SIZE_T InBufferSize,
        __out_bcount_opt(OutBufferSize) void* pOutBuffer,
        SIZE_T OutBufferSize,
        __out SIZE_T* pBytesReturned,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(CreateCompletionQueue)(
        THIS_
        SIZE_T nEntries,
        __deref_out INDCompletionQueue** ppCq
        ) PURE;

    STDMETHOD(RegisterMemory)(
        THIS_
        __in_bcount(BufferSize) const void* pBuffer,
        SIZE_T BufferSize,
        __inout OVERLAPPED* pOverlapped,
        __deref_out ND_MR_HANDLE* phMr
        ) PURE;

    STDMETHOD(DeregisterMemory)(
        THIS_
        __in ND_MR_HANDLE hMr,
        __inout OVERLAPPED* pOverlapped
        ) PURE;

    STDMETHOD(CreateMemoryWindow)(
        THIS_
        __out ND_RESULT* pInvalidateResult,
        __deref_out INDMemoryWindow** ppMw
        ) PURE;

    STDMETHOD(CreateConnector)(
        THIS_
        __deref_out INDConnector** ppConnector
        ) PURE;

    STDMETHOD(Listen)(
        THIS_
        SIZE_T Backlog,
        INT Protocol,
        USHORT Port,
        __out_opt USHORT* pAssignedPort,
        __deref_out INDListen** ppListen
        ) PURE;
};


//
// INDProvider
//
#undef INTERFACE
#define INTERFACE INDProvider

// {0C5DD316-5FDF-47e6-B2D0-2A6EDA8D39DD}
inline constexpr IID IID_INDProvider =
    { 0xc5dd316, 0x5fdf, 0x47e6, { 0xb2, 0xd0, 0x2a, 0x6e, 0xda, 0x8d, 0x39, 0xdd } };

DECLARE_INTERFACE_(INDProvider, IUnknown)
{
    // *** IUnknown methods ***
    IFACEMETHOD(QueryInterface)(
        THIS_
        REFIID riid,
        __deref_out LPVOID* ppvObj
        ) PURE;

    IFACEMETHOD_(ULONG, AddRef)(
        THIS
        ) PURE;

    IFACEMETHOD_(ULONG, Release)(
        THIS
        ) PURE;

    // *** INDProvider methods ***
    STDMETHOD(QueryAddressList)(
        THIS_
        __out_bcount_part_opt(*pBufferSize, *pBufferSize) SOCKET_ADDRESS_LIST* pAddressList,
        __inout SIZE_T* pBufferSize
        ) PURE;

    STDMETHOD(OpenAdapter)(
        THIS_
        __in_bcount(AddressLength) const struct sockaddr* pAddress,
        SIZE_T AddressLength,
        __deref_out INDAdapter** ppAdapter
        ) PURE;
};

//
// Map version 1 error values to version 2.
//
#define ND_LOCAL_LENGTH         ND_DATA_OVERRUN
#define ND_INVALIDATION_ERROR   ND_INVALID_DEVICE_REQUEST

#endif // _NDSPI_H_

// ===== END ndspi.h =====


// ===== BEGIN ndsupport.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
// Net Direct Helper Interface
//

#pragma once

#ifndef _NETDIRECT_H_
#define _NETDIRECT_H_



#ifdef __cplusplus
extern "C"
{
#endif  // __cplusplus

#define ND_HELPER_API  __stdcall


//
// Initialization
//
HRESULT ND_HELPER_API
NdStartup(
    VOID
    );

HRESULT ND_HELPER_API
NdCleanup(
    VOID
    );

VOID ND_HELPER_API
NdFlushProviders(
    VOID
    );

//
// Network capabilities
//
#define ND_QUERY_EXCLUDE_EMULATOR_ADDRESSES 0x00000001
#define ND_QUERY_EXCLUDE_NDv1_ADDRESSES     0x00000002
#define ND_QUERY_EXCLUDE_NDv2_ADDRESSES     0x00000004

HRESULT ND_HELPER_API
NdQueryAddressList(
    _In_ DWORD flags,
    _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
    _Inout_ SIZE_T* pcbAddressList
    );


HRESULT ND_HELPER_API
NdResolveAddress(
    _In_bytecount_(cbRemoteAddress) const struct sockaddr* pRemoteAddress,
    _In_ SIZE_T cbRemoteAddress,
    _Out_bytecap_(*pcbLocalAddress) struct sockaddr* pLocalAddress,
    _Inout_ SIZE_T* pcbLocalAddress
    );


HRESULT ND_HELPER_API
NdCheckAddress(
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress
    );


HRESULT ND_HELPER_API
NdOpenAdapter(
    _In_ REFIID iid,
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress,
    _Deref_out_ VOID** ppIAdapter
    );


HRESULT ND_HELPER_API
NdOpenV1Adapter(
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress,
    _Deref_out_ INDAdapter** ppIAdapter
    );

#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // _NETDIRECT_H_

// ===== END ndsupport.h =====


// ===== BEGIN assertutil.h =====

// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
//
// Summary:
//  This file contains the assert utility macros.
//    Note: the "Invariant" versions that are included in retail.
//
//   InvariantAssert/Assert(exp_) :
//      This uses interupt 2c, which enables new assert break and skip features.
//          If this assert fires, it will raise a fatal error and crash the host process.
//          If AeDebug key is set, this will cause the debugger to launch
//          When debugger attached, the 'ahi' command can be used to ignore the assertion and continue.
//        NOTE: Unlike the CRT assert, the strings for these asserts are stored in the symbols,
//              so asserts do not add strings to the image.
//
//   InvariantAssertP/AssertP(exp_) :
//      This is the same as the InvariantAssert/Assert macro, except that it will only passively fire
//          when a debugger is actually attached.
//
//

#define InvariantAssert(exp_) \
    ((!(exp_)) ? \
        (__annotation(L"Debug", L"AssertFail", L#exp_), \
         __int2c(), FALSE) : \
        TRUE)

#define InvariantAssertP( exp_ ) \
    ((!(exp_) && IsDebuggerPresent()) ? \
        (__annotation(L"Debug", L"PassiveAssertFail", L#exp_), \
         __int2c(), FALSE) : \
        TRUE)

#if DBG
#  define Assert(exp_)      __analysis_assume(exp_);InvariantAssert(exp_)
#  define AssertP( exp_ )   __analysis_assume(exp_);InvariantAssertP(exp_)
#else
#  define Assert( exp_ )  __analysis_assume(exp_)
#  define AssertP( exp_ ) __analysis_assume(exp_)
#endif

// ===== END assertutil.h =====


// ===== BEGIN ndutil.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//

#pragma once

namespace NetworkDirect
{
#ifndef ASSERT
#define ASSERT Assert
#if DBG
#define ASSERT_BENIGN( exp )    (void)(!(exp)?OutputDebugStringA("Assertion Failed:" #exp "\n"),FALSE:TRUE)
#else
#define ASSERT_BENIGN( exp )
#endif  // DBG
#endif  // ASSERT


    //---------------------------------------------------------
    // Lock wrapper.
    //
    class Lock
    {
        CRITICAL_SECTION* m_pLock;

    public:
        Lock(CRITICAL_SECTION* pLock) { m_pLock = pLock; EnterCriticalSection(m_pLock); }

        _Releases_lock_(*this->m_pLock)
            ~Lock() { LeaveCriticalSection(m_pLock); }
    };
    //---------------------------------------------------------

    extern HANDLE ghHeap;
} // namespace NetworkDirect

// ===== END ndutil.h =====


// ===== BEGIN list.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//
//
//    List and List::iterator, implements an intrusive double linked list and
//    iterator template
//
//    List is defined as a circular double linked list. With actions to insert
//    and remove entries.
//
//       List
//      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+
//     -|     |<--|     |-|     |-|     |-|     |-|     |
//      | head|   | data| | data| | data| | data| | data|
//      |     |-->|     |-|     |-|     |-|     |-|     |-
//      +-----+   +-----+ +-----+ +-----+ +-----+ +-----+
//
//                      Linked list diagram
//
//    An iteration is defined for the list using the member type named
//    iterator. To declare an interator variable, use fully qualified
//    name. e.g., List<T>::iterator. An iterator variable is analogous
//    to type T pointer. Dereference '*' and arrow '->' operators are
//    overloaded for this type so you can (almost) freely use it as a
//    T pointer.
//
//      Example:
//
//        for(List<T>::iterator p = list.begin(); p != list.end(); ++p)
//        {
//            p->doSomeThing();
//        }
//

#pragma once

#ifndef _MSMQ_LIST_H_
#define _MSMQ_LIST_H_

#ifndef __drv_aliasesMem
#define __drv_aliasesMem
#endif

//---------------------------------------------------------
//
//  class ListHelper
//
//---------------------------------------------------------
template<class T>
class ListHelper
{
public:
    enum { Offset = FIELD_OFFSET(T, m_link) };
};


//---------------------------------------------------------
//
//  class List
//
//---------------------------------------------------------
template<class T, int Offset = ListHelper<T>::Offset>
class List
{
private:
    LIST_ENTRY m_head;

public:
    class iterator;

public:
    List();

#if DBG
    ~List();
#endif

    iterator begin() const;
    iterator end() const;

    bool empty() const;

    T& front() const;
    T& back() const;
    void push_front(_Inout_ __drv_aliasesMem T* item);
    void push_back(_Inout_ __drv_aliasesMem T* item);
    void pop_front();
    void pop_back();

    iterator insert(iterator it, T& item);
    iterator erase(iterator it);
    void remove(T& item);

public:
    static LIST_ENTRY* item2entry(T&);
    static T& entry2item(LIST_ENTRY*);
    static void RemoveEntry(LIST_ENTRY*);
    static void InsertBefore(LIST_ENTRY* pNext, LIST_ENTRY*);
    static void InsertAfter(LIST_ENTRY* pPrev, LIST_ENTRY*);

private:
    List(const List&);
    List& operator=(const List&);

public:

    //
    // class List<T, Offset>::iterator
    //
    class iterator
    {
    private:
        LIST_ENTRY * m_current;

    public:
        explicit iterator(LIST_ENTRY* pEntry) :
            m_current(pEntry)
        {
        }


        iterator& operator++()
        {
            m_current = m_current->Flink;
            return *this;
        }


        iterator& operator--()
        {
            m_current = m_current->Blink;
            return *this;
        }


        T& operator*() const
        {
            return entry2item(m_current);
        }


        T* operator->() const
        {
            return (&**this);
        }


        bool operator==(const iterator& it) const
        {
            return (m_current == it.m_current);
        }


        bool operator!=(const iterator& it) const
        {
            return !(*this == it);
        }
    };
    //
    // end class iterator decleration
    //
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------
template<class T, int Offset>
inline List<T, Offset>::List()
{
    m_head.Flink = &m_head;
    m_head.Blink = &m_head;
}

#if DBG
template<class T, int Offset>
inline List<T, Offset>::~List()
{
    ASSERT_BENIGN(empty());
}
#endif

template<class T, int Offset>
inline LIST_ENTRY* List<T, Offset>::item2entry(T& item)
{
    return ((LIST_ENTRY*)(PVOID)((PCHAR)&item + Offset));
}


template<class T, int Offset>
inline T& List<T, Offset>::entry2item(LIST_ENTRY* pEntry)
{
    return *((T*)(PVOID)((PCHAR)pEntry - Offset));
}


template<class T, int Offset>
inline void List<T, Offset>::InsertBefore(LIST_ENTRY* pNext, LIST_ENTRY* pEntry)
{
    pEntry->Flink = pNext;
    pEntry->Blink = pNext->Blink;
    pNext->Blink->Flink = pEntry;
    pNext->Blink = pEntry;
}


template<class T, int Offset>
inline void List<T, Offset>::InsertAfter(LIST_ENTRY* pPrev, LIST_ENTRY* pEntry)
{
    pEntry->Blink = pPrev;
    pEntry->Flink = pPrev->Flink;
    pPrev->Flink->Blink = pEntry;
    pPrev->Flink = pEntry;
}


template<class T, int Offset>
inline void List<T, Offset>::RemoveEntry(LIST_ENTRY* pEntry)
{
    LIST_ENTRY* Blink = pEntry->Blink;
    LIST_ENTRY* Flink = pEntry->Flink;

    Blink->Flink = Flink;
    Flink->Blink = Blink;

    pEntry->Flink = pEntry->Blink = nullptr;
}


template<class T, int Offset>
inline typename List<T, Offset>::iterator List<T, Offset>::begin() const
{
    return iterator(m_head.Flink);
}


template<class T, int Offset>
inline typename List<T, Offset>::iterator List<T, Offset>::end() const
{
    return iterator(const_cast<LIST_ENTRY*>(&m_head));
}


template<class T, int Offset>
inline bool List<T, Offset>::empty() const
{
    return (m_head.Flink == &m_head);
}


template<class T, int Offset>
inline T& List<T, Offset>::front() const
{
    ASSERT(!empty());
    return entry2item(m_head.Flink);
}


template<class T, int Offset>
inline T& List<T, Offset>::back() const
{
    ASSERT(!empty());
    return entry2item(m_head.Blink);
}


#pragma prefast(disable:28194, "Linking the item into the list aliases the memory.");
template<class T, int Offset>
inline void List<T, Offset>::push_front(_Inout_ __drv_aliasesMem T* item)
{
    LIST_ENTRY* pEntry = item2entry(*item);
    InsertAfter(&m_head, pEntry);
}


#pragma prefast(disable:28194, "Linking the item into the list aliases the memory.");
template<class T, int Offset>
inline void List<T, Offset>::push_back(_Inout_ __drv_aliasesMem T* item)
{
    LIST_ENTRY* pEntry = item2entry(*item);
    InsertBefore(&m_head, pEntry);
}


template<class T, int Offset>
inline void List<T, Offset>::pop_front()
{
    ASSERT(!empty());
    RemoveEntry(m_head.Flink);
}


template<class T, int Offset>
inline void List<T, Offset>::pop_back()
{
    ASSERT(!empty());
    RemoveEntry(m_head.Blink);
}


template<class T, int Offset>
inline typename List<T, Offset>::iterator List<T, Offset>::insert(iterator it, T& item)
{
    LIST_ENTRY* pEntry = item2entry(item);
    LIST_ENTRY* pNext = item2entry(*it);
    InsertBefore(pNext, pEntry);
    return iterator(pEntry);
}


template<class T, int Offset>
inline typename List<T, Offset>::iterator List<T, Offset>::erase(iterator it)
{
    ASSERT(it != end());
    iterator next = it;
    ++next;
    remove(*it);
    return next;
}


template<class T, int Offset>
inline void List<T, Offset>::remove(T& item)
{
    ASSERT(&item != &*end());
    LIST_ENTRY* pEntry = item2entry(item);
    RemoveEntry(pEntry);
}

#endif // _MSMQ_LIST_H_

// ===== END list.h =====


// ===== BEGIN ndaddr.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//


#pragma once


namespace NetworkDirect
{

    class Provider;

    class Address
    {
        friend class ListHelper<Address>;

        LIST_ENTRY m_link;
        Provider* m_pProvider;
        SOCKADDR_INET m_Addr;

    public:
        Address(_In_ const struct sockaddr& addr, _In_ Provider& provider);

        bool Matches(const struct sockaddr* pMatchAddr) const;
        Provider* GetProvider() const { return m_pProvider; }

        short AF() const { return m_Addr.si_family; };
        SIZE_T CopySockaddr(_Out_writes_(len) BYTE* pBuf, _In_ SIZE_T len) const;

    };

} // namespace NetworkDirect

// ===== END ndaddr.h =====


// ===== BEGIN ndprov.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//


#pragma once

namespace NetworkDirect
{

    typedef HRESULT
    (*DLLGETCLASSOBJECT)(
        REFCLSID rclsid,
        REFIID rrid,
        LPVOID* ppv
        );

    typedef HRESULT
    (*DLLCANUNLOADNOW)(void);


    class Provider
    {
        friend class ListHelper<Provider>;

        GUID m_Guid;
        LIST_ENTRY m_link;
        HMODULE m_hProvider;
        DLLGETCLASSOBJECT m_pfnDllGetClassObject;
        DLLCANUNLOADNOW m_pfnDllCanUnloadNow;
        WCHAR* m_Path;
        int m_Version;
        bool m_Active;

    public:
        Provider(int version);
        virtual ~Provider(void);
        HRESULT Init(GUID& ProviderGuid);
        void MarkActive(void) { m_Active = true; }
        void MarkInactive(void) { m_Active = false; }
        bool IsActive(void) const { return m_Active; }
        int GetVersion(void) const { return m_Version; }

        //
        // GetClassObject and TryUnload require the caller to provide
        // proper serialization.
        //
        bool TryUnload(void);

        virtual HRESULT OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ ULONG cbAddress,
            _Deref_out_ VOID** ppIAdapter
        ) PURE;

        virtual HRESULT
            QueryAddressList(
                _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList)
                SOCKET_ADDRESS_LIST* pAddressList,
                _Inout_ ULONG* pcbAddressList
            ) PURE;

        bool operator ==(const GUID& providerGuid)
        {
            return InlineIsEqualGUID(m_Guid, providerGuid) == TRUE;
        }

    protected:
        //
        // GetClassObject and TryUnload require the caller to provide
        // proper serialization.
        //
        HRESULT GetClassObject(_In_ const IID& iid, _Out_ void** ppInterface);
    };


    class NdV1Provider : public Provider
    {
    public:
        NdV1Provider();
        ~NdV1Provider();

        HRESULT OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ ULONG cbAddress,
            _Deref_out_ VOID** ppIAdapter
        ) override;

        HRESULT QueryAddressList(
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ ULONG* pcbAddressList
        ) override;

    private:
        HRESULT GetProvider(INDProvider** ppIProvider);
    };


    class NdProvider : public Provider
    {
    public:
        NdProvider();
        ~NdProvider();

        HRESULT OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ ULONG cbAddress,
            _Deref_out_ VOID** ppIAdapter
        ) override;

        HRESULT QueryAddressList(
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ ULONG* pcbAddressList
        ) override;
    };

} // namespace NetworkDirect

// ===== END ndprov.h =====


// ===== BEGIN ndfrmwrk.h =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//


#pragma once

namespace NetworkDirect
{

    enum ND_NOTIFY_TYPE
    {
        ND_NOTIFY_PROVIDER_CHANGE,
        ND_NOTIFY_ADDR_CHANGE,
        ND_NOTIFY_MAX
    };


    class Framework
    {
        // IOCP for provider and address changes.
        HANDLE m_hIocp;
        HANDLE m_hProviderChange;
        // Socket for address list change and address resolution.
        SOCKET m_Socket;
        OVERLAPPED m_Ov[ND_NOTIFY_MAX];

        // Lock protecting the provider and address lists.
        CRITICAL_SECTION m_lock;

        List<Provider> m_ProviderList;
        List<Address> m_NdAddrList;
        List<Address> m_NdV1AddrList;

        volatile LONG m_nRef;


    public:
        Framework(void);
        ~Framework(void);

        HRESULT Init(void);

        ULONG AddRef(void);
        ULONG Release(void);

        HRESULT QueryAddressList(
            _In_ DWORD flags,
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ SIZE_T* pcbAddressList
        );

        HRESULT ResolveAddress(
            _In_bytecount_(cbRemoteAddress) const struct sockaddr* pRemoteAddress,
            _In_ SIZE_T cbRemoteAddress,
            _Out_bytecap_(*pcbLocalAddress) struct sockaddr* pLocalAddress,
            _Inout_ SIZE_T* pcbLocalAddress
        );

        HRESULT CheckAddress(
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress
        );

        HRESULT OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress,
            _Deref_out_ VOID** ppIAdapter
        );

        void FlushProvidersForUser();

    private:
        static HRESULT ValidateAddress(
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress
        );

        static void CountAddresses(
            _In_ const List<Address>& list,
            _Inout_ SIZE_T* pnV4,
            _Inout_ SIZE_T* pnV6
        );

        static void
            BuildAddressList(
                _In_ Provider& prov,
                _In_ const SOCKET_ADDRESS_LIST& addrList,
                _Inout_ List<Address>* pList
            );

        static void CopyAddressList(
            _In_ const List<Address>& list,
            _Inout_ SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ BYTE** ppBuf,
            _Inout_ SIZE_T* pcbBuf
        );

        void ProcessUpdates(void);

        void ProcessProviderChange(void);
        void ProcessAddressChange(void);
        void FlushProviders(void);
    };

} // namespace NetworkDirect

// ===== END ndfrmwrk.h =====


namespace NetworkDirect::detail {
inline constexpr IID iid_i_class_factory =
    { 0x00000001, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
} // namespace NetworkDirect::detail


// ===== BEGIN inline implementation: ndaddr.cpp =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//



namespace NetworkDirect
{

    inline Address::Address(
        _In_ const struct sockaddr& addr,
        _In_ Provider& provider
    ) :
        m_pProvider(&provider)
    {
        m_link.Flink = &m_link;
        m_link.Blink = &m_link;

        ASSERT(addr.sa_family == AF_INET || addr.sa_family == AF_INET6);

        switch (addr.sa_family)
        {
        case AF_INET:
            ::CopyMemory(&m_Addr.Ipv4, &addr, sizeof(m_Addr.Ipv4));
            break;
        case AF_INET6:
            ::CopyMemory(&m_Addr.Ipv6, &addr, sizeof(m_Addr.Ipv6));
            break;
        default:
            ASSERT(addr.sa_family == AF_INET ||
                addr.sa_family == AF_INET6);
            m_Addr.si_family = AF_UNSPEC;
        }
    }


    inline bool Address::Matches(const struct sockaddr* pAddr) const
    {
        switch (pAddr->sa_family)
        {
        case AF_INET:
            return reinterpret_cast<const struct sockaddr_in*>(pAddr)->sin_addr.S_un.S_addr ==
                m_Addr.Ipv4.sin_addr.S_un.S_addr;

        case AF_INET6:
            return (::memcmp(reinterpret_cast<const sockaddr_in6*>(pAddr)->sin6_addr.u.Byte,
                m_Addr.Ipv6.sin6_addr.u.Byte, sizeof(m_Addr.Ipv6.sin6_addr)) == 0);

        default:
            return false;
        }
    }


    inline SIZE_T Address::CopySockaddr(_Out_writes_(len) BYTE* pBuf, _In_ SIZE_T len) const
    {
        switch (m_Addr.si_family)
        {
        case AF_INET:
            if (len < sizeof(m_Addr.Ipv4))
            {
                return 0;
            }

            ::CopyMemory(pBuf, &m_Addr.Ipv4, sizeof(m_Addr.Ipv4));
            return sizeof(m_Addr.Ipv4);

        case AF_INET6:
            if (len < sizeof(m_Addr.Ipv6))
            {
                return 0;
            }

            ::CopyMemory(pBuf, &m_Addr.Ipv6, sizeof(m_Addr.Ipv6));
            return sizeof(m_Addr.Ipv6);

        default:
            return 0;
        }
    }

} // namespace NetworkDirect

// ===== END inline implementation: ndaddr.cpp =====


// ===== BEGIN inline implementation: ndprov.cpp =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//




namespace NetworkDirect
{

    inline Provider::Provider(int version) :
        m_Guid{},
        m_hProvider(nullptr),
        m_pfnDllGetClassObject(nullptr),
        m_pfnDllCanUnloadNow(nullptr),
        m_Path(nullptr),
        m_Version(version),
        m_Active(true)
    {
        m_link.Flink = &m_link;
        m_link.Blink = &m_link;
    }


    inline Provider::~Provider()
    {
        if (m_hProvider != nullptr)
        {
            ::FreeLibrary(m_hProvider);
        }

        if (m_Path)
        {
            ::HeapFree(ghHeap, 0, m_Path);
        }
    }


    inline HRESULT Provider::Init(GUID& ProviderGuid)
    {
        INT pathLen;
        INT ret, err;
        WCHAR* pPath;

        // Get the path length for the provider DLL.
        pPath = static_cast<WCHAR*>(
            ::HeapAlloc(ghHeap, 0, sizeof(WCHAR) * MAX_PATH)
            );
        if (pPath == nullptr)
        {
            return ND_NO_MEMORY;
        }

        pathLen = MAX_PATH;
        ret = ::WSCGetProviderPath(&ProviderGuid, pPath, &pathLen, &err);
        if (ret != 0)
        {
            ::HeapFree(ghHeap, 0, pPath);
            return HRESULT_FROM_WIN32(err);
        }

        pathLen = ::ExpandEnvironmentStringsW(pPath, nullptr, 0);
        if (pathLen == 0)
        {
            ::HeapFree(ghHeap, 0, pPath);
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        m_Path = static_cast<WCHAR*>(
            ::HeapAlloc(ghHeap, 0, sizeof(WCHAR) * pathLen)
            );
        if (m_Path == nullptr)
        {
            ::HeapFree(ghHeap, 0, pPath);
            return ND_NO_MEMORY;
        }

        ret = ::ExpandEnvironmentStringsW(pPath, m_Path, pathLen);

        // We don't need the un-expanded path anymore.
        ::HeapFree(ghHeap, 0, pPath);

        if (ret != pathLen)
        {
            return ND_UNSUCCESSFUL;
        }

        m_Guid = ProviderGuid;
        return S_OK;
    }


    //
    // Serialization with TryUnload must be provided by the caller.  Multiple
    // callers may call this function concurrently.
    //
    inline HRESULT Provider::GetClassObject(
        _In_ const IID& iid,
        _Out_ void** ppInterface
    )
    {
        if (m_hProvider == nullptr)
        {
            HMODULE hProvider;
            hProvider = ::LoadLibraryExW(m_Path, nullptr, 0);
            if (hProvider == nullptr)
            {
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            m_pfnDllGetClassObject = reinterpret_cast<DLLGETCLASSOBJECT>(
                ::GetProcAddress(hProvider, "DllGetClassObject")
                );
            if (!m_pfnDllGetClassObject)
            {
                ::FreeLibrary(hProvider);
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            m_pfnDllCanUnloadNow = reinterpret_cast<DLLCANUNLOADNOW>(
                ::GetProcAddress(hProvider, "DllCanUnloadNow")
                );
            if (!m_pfnDllCanUnloadNow)
            {
                ::FreeLibrary(hProvider);
                return HRESULT_FROM_WIN32(::GetLastError());
            }

            HMODULE hCurrentProvider = static_cast<HMODULE>(
                ::InterlockedCompareExchangePointer(
                    reinterpret_cast<void**>(&m_hProvider), hProvider, nullptr)
                );
            if (hCurrentProvider)
            {
                ASSERT(hCurrentProvider == hProvider);
                ::FreeLibrary(hProvider);
            }
        }

        HRESULT hr = m_pfnDllGetClassObject(
            m_Guid,
            iid,
            ppInterface
        );

        return hr;
    }


    //
    // Strict serialization must be provided by the caller.
    //
    inline bool Provider::TryUnload(void)
    {
        if (m_hProvider == nullptr)
        {
            return true;
        }

        ASSERT(m_pfnDllCanUnloadNow != nullptr);

        HRESULT hr = m_pfnDllCanUnloadNow();
        if (hr != S_OK)
        {
            return false;
        }

        ::FreeLibrary(m_hProvider);
        m_hProvider = nullptr;

        return true;
    }


    inline NdV1Provider::NdV1Provider() :
        Provider(ND_VERSION_1)
    {
    }

    inline NdV1Provider::~NdV1Provider() = default;


    //
    // Serialization with TryUnload must be provided by the caller.  Multiple
    // callers may call this function concurrently.
    //
    inline HRESULT NdV1Provider::GetProvider(INDProvider** ppIProvider)
    {
        IClassFactory* pClassFactory;
        HRESULT hr = GetClassObject(
            detail::iid_i_class_factory,
            reinterpret_cast<void**>(&pClassFactory)
        );
        if (FAILED(hr))
        {
            return hr;
        }

        hr = pClassFactory->CreateInstance(
            nullptr,
            IID_INDProvider,
            reinterpret_cast<void**>(ppIProvider)
        );

        // Now that we asked for the provider, we don't need the class factory.
        pClassFactory->Release();
        return hr;
    }


    inline HRESULT
        NdV1Provider::OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ ULONG cbAddress,
            _Deref_out_ VOID** ppIAdapter
        )
    {
        if (iid != IID_INDAdapter)
        {
            return E_NOINTERFACE;
        }

        INDProvider* pIProvider;
        HRESULT hr = GetProvider(&pIProvider);
        if (FAILED(hr))
        {
            TryUnload();
            return ND_INVALID_ADDRESS;
        }

        hr = pIProvider->OpenAdapter(
            pAddress,
            cbAddress,
            reinterpret_cast<INDAdapter**>(ppIAdapter)
        );

        pIProvider->Release();
        TryUnload();

        return hr;
    }


    inline HRESULT
        NdV1Provider::QueryAddressList(
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ ULONG* pcbAddressList
        )
    {
        INDProvider *pIProvider;
        HRESULT hr = GetProvider(&pIProvider);
        if (FAILED(hr))
        {
            TryUnload();
            return ND_DEVICE_NOT_READY;
        }

        SIZE_T cbAddressList = *pcbAddressList;
        hr = pIProvider->QueryAddressList(pAddressList, &cbAddressList);
        *pcbAddressList = static_cast<ULONG>(cbAddressList);

        pIProvider->Release();
        TryUnload();

        return hr;
    }


    inline NdProvider::NdProvider() :
        Provider(ND_VERSION_2)
    {
    }

    inline NdProvider::~NdProvider() = default;


    //
    // Serialization with TryUnload must be provided by the caller.  Multiple
    // callers may call this function concurrently.
    //
    inline HRESULT NdProvider::OpenAdapter(
        _In_ REFIID iid,
        _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
        _In_ ULONG cbAddress,
        _Deref_out_ VOID** ppIAdapter
    )
    {
        IND2Provider* pIProvider;
        HRESULT hr = GetClassObject(
            IID_IND2Provider,
            reinterpret_cast<void**>(&pIProvider)
        );
        if (FAILED(hr))
        {
            TryUnload();
            return ND_INVALID_ADDRESS;
        }

        UINT64 id;
        hr = pIProvider->ResolveAddress(pAddress, cbAddress, &id);
        if (FAILED(hr))
        {
            pIProvider->Release();
            TryUnload();
            return ND_INVALID_ADDRESS;
        }

        hr = pIProvider->OpenAdapter(iid, id, ppIAdapter);

        pIProvider->Release();
        TryUnload();

        return hr;
    }


    inline HRESULT
        NdProvider::QueryAddressList(
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ ULONG* pcbAddressList
        )
    {
        IND2Provider *pIProvider;
        HRESULT hr = GetClassObject(
            IID_IND2Provider,
            reinterpret_cast<void**>(&pIProvider)
        );
        if (FAILED(hr))
        {
            TryUnload();
            return ND_DEVICE_NOT_READY;
        }

        hr = pIProvider->QueryAddressList(pAddressList, pcbAddressList);

        pIProvider->Release();
        TryUnload();

        return hr;
    }


} // namespace NetworkDirect

// ===== END inline implementation: ndprov.cpp =====


// ===== BEGIN inline implementation: ndfrmwrk.cpp =====

//
// Copyright(c) Microsoft Corporation.All rights reserved.
// Licensed under the MIT License.
//




namespace NetworkDirect
{

    //
    // PFL_NETWORKDIRECT_PROVIDER should come from WinSock2.h, but may not be
    // defined depending on which SDK is being used.  We define it here in case
    // it isn't defined yet.
    //
#ifndef PFL_NETWORKDIRECT_PROVIDER
#define PFL_NETWORKDIRECT_PROVIDER 0x00000010
#endif

//
// Flags for checking that a WSAPROTOCOL_INFO structure is a ND provider.
//
    inline constexpr int ND_SERVICE_FLAGS1 = (XP1_GUARANTEED_DELIVERY | XP1_GUARANTEED_ORDER |
        XP1_MESSAGE_ORIENTED | XP1_CONNECT_DATA);
    inline constexpr int ND_PROVIDER_FLAGS = (PFL_HIDDEN | PFL_NETWORKDIRECT_PROVIDER);


    inline volatile LONG gInitializing = 0;
    inline HANDLE ghHeap = nullptr;
    inline Framework* gpFramework = nullptr;


    inline Framework::Framework() :
        m_hIocp(nullptr),
        m_hProviderChange(nullptr),
        m_Socket(INVALID_SOCKET),
        m_nRef(0)
    {
        InitializeCriticalSection(&m_lock);
        ::ZeroMemory(m_Ov, sizeof(m_Ov));
    }


    inline Framework::~Framework()
    {
        while (!m_NdAddrList.empty())
        {
            Address* pAddr = &m_NdAddrList.front();
            m_NdAddrList.pop_front();
            delete pAddr;
        }

        while (!m_NdV1AddrList.empty())
        {
            Address* pAddr = &m_NdV1AddrList.front();
            m_NdV1AddrList.pop_front();
            delete pAddr;
        }

        FlushProviders();

        while (!m_ProviderList.empty())
        {
            Provider* pProvider = &m_ProviderList.front();
            m_ProviderList.pop_front();
            delete pProvider;
        }

        if (m_hProviderChange != nullptr)
        {
            ::CloseHandle(m_hProviderChange);
        }

        if (m_Socket != INVALID_SOCKET)
        {
            ::closesocket(m_Socket);
        }

        if (m_hIocp != nullptr)
        {
            ::CloseHandle(m_hIocp);
        }

        DeleteCriticalSection(&m_lock);
    }


    inline HRESULT
        Framework::Init()
    {
        int ret;
        WSADATA data;

        ASSERT(m_nRef == 0);

        ret = ::WSAStartup(MAKEWORD(2, 2), &data);
        if (ret != 0)
        {
            return HRESULT_FROM_WIN32(ret);
        }

        // Create an IOCP to get all the different notifications:
        // - provider change
        // - address change
        m_hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
        if (m_hIocp == nullptr)
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        // Create a socket for address changes.
        m_Socket = ::WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
        if (m_Socket == INVALID_SOCKET)
        {
            return HRESULT_FROM_WIN32(::WSAGetLastError());
        }

        // Bind the socket change handle to the IOCP.
        HANDLE hIocp = ::CreateIoCompletionPort(
            reinterpret_cast<HANDLE>(m_Socket), m_hIocp, ND_NOTIFY_ADDR_CHANGE, 0);
        if (hIocp != m_hIocp)
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        // Get provider change notification handle.
        ret = ::WSAProviderConfigChange(&m_hProviderChange, nullptr, nullptr);
        if (ret != 0)
        {
            return HRESULT_FROM_WIN32(::WSAGetLastError());
        }

        // Bind the provider change handle to the IOCP.
        __analysis_assume(m_hProviderChange != nullptr);
        hIocp = ::CreateIoCompletionPort(
            m_hProviderChange, m_hIocp, ND_NOTIFY_PROVIDER_CHANGE, 0);
        if (hIocp != m_hIocp)
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        // Request address change notification.
        DWORD BytesRet;
        ret = ::WSAIoctl(m_Socket, SIO_ADDRESS_LIST_CHANGE, nullptr, 0, nullptr,
            0, &BytesRet, &m_Ov[ND_NOTIFY_ADDR_CHANGE], nullptr);
        if (ret != 0 && WSAGetLastError() != WSA_IO_PENDING)
        {
            return HRESULT_FROM_WIN32(::WSAGetLastError());
        }

        // Generate a provider change notification, so that the next call that
        // looks at the provider catalog will update it first.
        ret = ::PostQueuedCompletionStatus(m_hIocp, 0, ND_NOTIFY_PROVIDER_CHANGE,
            &m_Ov[ND_NOTIFY_PROVIDER_CHANGE]);
        if (ret == FALSE)
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        return S_OK;
    }


    inline ULONG
        Framework::AddRef()
    {
        return ::InterlockedIncrement(&m_nRef);
    }


    inline ULONG
        Framework::Release()
    {
        ASSERT(m_nRef > 0);

        ULONG nRef = ::InterlockedDecrement(&m_nRef);

        if (nRef == 0)
        {
            delete this;
        }

        return nRef;
    }


    inline HRESULT
        Framework::QueryAddressList(
            _In_ DWORD flags,
            _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ SIZE_T* pcbAddressList
        )
    {
        // Sync our provider and address lists
        ProcessUpdates();

        Lock lock(&m_lock);

        // Calculate the size of the buffer we need.  We count the number of v4 and
        // v6 addresses.
        SIZE_T nV4 = 0;
        SIZE_T nV6 = 0;

        if ((flags & ND_QUERY_EXCLUDE_NDv2_ADDRESSES) == 0)
        {
            CountAddresses(m_NdAddrList, &nV4, &nV6);
        }

        if ((flags & ND_QUERY_EXCLUDE_NDv1_ADDRESSES) == 0)
        {
            CountAddresses(m_NdV1AddrList, &nV4, &nV6);
        }

        if (nV4 == 0 && nV6 == 0)
        {
            *pcbAddressList = 0;
            return ND_SUCCESS;
        }

        SIZE_T cbRequired = sizeof(SOCKET_ADDRESS_LIST) - sizeof(SOCKET_ADDRESS) +
            (sizeof(SOCKET_ADDRESS) * (nV4 + nV6)) +
            (sizeof(sockaddr_in) * nV4) + (sizeof(sockaddr_in6) * nV6);

        if (pAddressList == nullptr || cbRequired > *pcbAddressList)
        {
            *pcbAddressList = cbRequired;
            return ND_BUFFER_OVERFLOW;
        }

        BYTE* pBuf = reinterpret_cast<BYTE*>(
            &pAddressList->Address[(nV4 + nV6)]);
        SIZE_T cbRemaining = *pcbAddressList -
            FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address[(nV4 + nV6)]);

        pAddressList->iAddressCount = 0;

        if ((flags & ND_QUERY_EXCLUDE_NDv2_ADDRESSES) == 0)
        {
            CopyAddressList(m_NdAddrList, pAddressList, &pBuf, &cbRemaining);
        }

        if ((flags & ND_QUERY_EXCLUDE_NDv1_ADDRESSES) == 0)
        {
            CopyAddressList(m_NdV1AddrList, pAddressList, &pBuf, &cbRemaining);
        }

        return S_OK;
    }


    inline HRESULT
        Framework::ResolveAddress(
            _In_bytecount_(cbRemoteAddress) const struct sockaddr* pRemoteAddress,
            _In_ SIZE_T cbRemoteAddress,
            _Out_bytecap_(*pcbLocalAddress) struct sockaddr* pLocalAddress,
            _Inout_ SIZE_T* pcbLocalAddress
        )
    {
        //
        // Sync our provider and address lists
        //
        ProcessUpdates();

        Lock lock(&m_lock);

        //
        // Cap to max DWORD value.  This has the added benefit of zeroing the upper
        // bits on 64-bit platforms, so that the returned value is correct.
        //
        if (*pcbLocalAddress > UINT_MAX)
        {
            *pcbLocalAddress = UINT_MAX;
        }

        //
        // We store the original length so we can distinguish from different
        // errors that return WSAEFAULT.
        //
        SIZE_T len = *pcbLocalAddress;
        int ret = ::WSAIoctl(
            m_Socket,
            SIO_ROUTING_INTERFACE_QUERY,
            const_cast<sockaddr*>(pRemoteAddress),
            static_cast<DWORD>(cbRemoteAddress),
            pLocalAddress,
            static_cast<DWORD>(len),
            reinterpret_cast<DWORD*>(pcbLocalAddress),
            nullptr,
            nullptr
        );

        if (ret == SOCKET_ERROR)
        {
            switch (::GetLastError())
            {
            case WSAEFAULT:
                if (len < *pcbLocalAddress)
                {
                    return ND_BUFFER_OVERFLOW;
                }
                __fallthrough;
            default:
                return ND_UNSUCCESSFUL;
            case WSAEINVAL:
                return ND_INVALID_ADDRESS;
            case WSAENETUNREACH:
            case WSAENETDOWN:
                return ND_NETWORK_UNREACHABLE;
            }
        }

        //
        // We found a local address.  Now make sure that the we have a provider
        // that supports it.
        //
        for (List<Address>::iterator pAddr = m_NdAddrList.begin();
            pAddr != m_NdAddrList.end();
            ++pAddr)
        {
            if (pAddr->Matches(pLocalAddress))
            {
                return S_OK;
            }
        }

        for (List<Address>::iterator pAddr = m_NdV1AddrList.begin();
            pAddr != m_NdV1AddrList.end();
            ++pAddr)
        {
            if (pAddr->Matches(pLocalAddress))
            {
                return S_OK;
            }
        }

        return ND_INVALID_ADDRESS;
    }


    inline HRESULT
        Framework::CheckAddress(
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress
        )
    {
        ASSERT(pAddress);

        HRESULT hr = ValidateAddress(pAddress, cbAddress);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // Sync our provider and address lists.
        //
        ProcessUpdates();

        Lock lock(&m_lock);
        for (List<Address>::iterator pAddr = m_NdAddrList.begin();
            pAddr != m_NdAddrList.end();
            ++pAddr)
        {
            if (pAddr->Matches(pAddress))
            {
                return ND_SUCCESS;
            }
        }

        for (List<Address>::iterator pAddr = m_NdV1AddrList.begin();
            pAddr != m_NdV1AddrList.end();
            ++pAddr)
        {
            if (pAddr->Matches(pAddress))
            {
                return ND_SUCCESS;
            }
        }

        return ND_INVALID_ADDRESS;
    }


    inline HRESULT
        Framework::OpenAdapter(
            _In_ REFIID iid,
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress,
            _Deref_out_ VOID** ppIAdapter
        )
    {
        ASSERT(pAddress);
        ASSERT(ppIAdapter);
        HRESULT hr = ValidateAddress(pAddress, cbAddress);
        if (FAILED(hr))
        {
            return hr;
        }

        //
        // Sync our provider and address lists
        //
        ProcessUpdates();

        Lock lock(&m_lock);
        const List<Address>* pList;
        if (InlineIsEqualGUID(iid, IID_INDAdapter))
        {
            pList = &m_NdV1AddrList;
        }
        else
        {
            pList = &m_NdAddrList;
        }

        //
        // Find the provider for the given address.
        //
        hr = ND_INVALID_ADDRESS;
        for (List<Address>::iterator pAddr = pList->begin();
            pAddr != pList->end();
            ++pAddr)
        {
            if (!pAddr->Matches(pAddress))
            {
                continue;
            }

            ASSERT(pAddr->GetProvider() != nullptr);
            hr = pAddr->GetProvider()->OpenAdapter(
                iid,
                pAddress,
                static_cast<ULONG>(cbAddress),
                ppIAdapter
            );
            if (FAILED(hr))
            {
                continue;   // In case another provider handles the same address.
            }

            break;
        }
        return hr;
    }


    inline void
        Framework::FlushProvidersForUser()
    {
        Lock lock(&m_lock);
        FlushProviders();
    }


    inline HRESULT
        Framework::ValidateAddress(
            _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
            _In_ SIZE_T cbAddress
        )
    {
        if (cbAddress < sizeof(struct sockaddr))
        {
            return ND_INVALID_PARAMETER_2;
        }

        if (cbAddress > ULONG_MAX)
        {
            return ND_INVALID_PARAMETER_2;
        }

        switch (pAddress->sa_family)
        {
        case AF_INET:
            if (cbAddress < sizeof(struct sockaddr_in))
            {
                return ND_INVALID_PARAMETER_2;
            }
            break;

        case AF_INET6:
            if (cbAddress < sizeof(struct sockaddr_in6))
            {
                return ND_INVALID_PARAMETER_2;
            }
            break;

        default:
            return ND_INVALID_ADDRESS;
        }

        return ND_SUCCESS;
    }


    inline void
        Framework::CountAddresses(
            _In_ const List<Address>& list,
            _Inout_ SIZE_T* pnV4,
            _Inout_ SIZE_T* pnV6
        )
    {
        for (List<Address>::iterator pAddr = list.begin();
            pAddr != list.end() && (*pnV4 + *pnV6) < INT_MAX;
            ++pAddr)
        {
            switch (pAddr->AF())
            {
            case AF_INET:
                (*pnV4)++;
                break;
            case AF_INET6:
                (*pnV6)++;
                break;
            default:
                continue;
            }
        }
    }


    inline void
        Framework::BuildAddressList(
            _In_ Provider& prov,
            _In_ const SOCKET_ADDRESS_LIST& addrList,
            _Inout_ List<Address>* pList
        )
    {
        for (int i = 0; i < addrList.iAddressCount; i++)
        {
            // We only handle IPv4 and IPv6 addresses.
            if (addrList.Address[i].iSockaddrLength < sizeof(struct sockaddr))
            {
                continue;
            }

            switch (addrList.Address[i].lpSockaddr->sa_family)
            {
            case AF_INET:
                if (addrList.Address[i].iSockaddrLength <
                    sizeof(struct sockaddr_in))
                {
                    continue;
                }
                break;

            case AF_INET6:
                if (addrList.Address[i].iSockaddrLength <
                    sizeof(struct sockaddr_in6))
                {
                    continue;
                }
                break;

            default:
                continue;
            }

            Address* pAddr =
                new Address(*addrList.Address[i].lpSockaddr, prov);
            if (pAddr != nullptr)
            {
                pList->push_back(pAddr);
            }
        }
    }


    inline void
        Framework::CopyAddressList(
            _In_ const List<Address>& list,
            _Inout_ SOCKET_ADDRESS_LIST* pAddressList,
            _Inout_ BYTE** ppBuf,
            _Inout_ SIZE_T* pcbBuf
        )
    {
        INT nAddress = pAddressList->iAddressCount;

        for (List<Address>::iterator pAddr = list.begin();
            pAddr != list.end() && nAddress < INT_MAX;
            ++pAddr)
        {
            SIZE_T len = pAddr->CopySockaddr(*ppBuf, *pcbBuf);
            if (len == 0)
            {
                continue;
            }

            ASSERT(len < INT_MAX);
            pAddressList->Address[nAddress].iSockaddrLength =
                static_cast<INT>(len);
            pAddressList->Address[nAddress].lpSockaddr =
                reinterpret_cast<LPSOCKADDR>(*ppBuf);
            (*ppBuf) += len;
            (*pcbBuf) -= len;

            ++nAddress;
        }

        pAddressList->iAddressCount = nAddress;
    }


    inline void
        Framework::ProcessUpdates()
    {
        BOOL ret;

        // Check the IOCP for completion of any of our requests.
        do
        {
            DWORD len;
            ULONG_PTR key;
            OVERLAPPED* pOv;
            INT status;
            DWORD bytesRet;

            ret = ::GetQueuedCompletionStatus(m_hIocp, &len, &key, &pOv, 0);

            if (ret == TRUE)
            {
                Lock lock(&m_lock);

                switch (key)
                {
                case ND_NOTIFY_PROVIDER_CHANGE:
                    // Issue the next request for protocol catalog changes,
                    // in case things change while we are processing this event.
                    status = ::WSAProviderConfigChange(
                        &m_hProviderChange, &m_Ov[ND_NOTIFY_PROVIDER_CHANGE], nullptr);
                    ASSERT(status == 0 || ::WSAGetLastError() == WSA_IO_PENDING);

                    ProcessProviderChange();
                    break;

                case ND_NOTIFY_ADDR_CHANGE:
                    // Issue the next request for address changes, in case
                    // things change while we are processing this event.
                    status = ::WSAIoctl(m_Socket, SIO_ADDRESS_LIST_CHANGE, nullptr, 0, nullptr,
                        0, &bytesRet, &m_Ov[ND_NOTIFY_ADDR_CHANGE], nullptr);
                    ASSERT(status == 0);

                    ProcessAddressChange();
                    break;

                default:
                    ASSERT(key == ND_NOTIFY_PROVIDER_CHANGE ||
                        key == ND_NOTIFY_ADDR_CHANGE);
                    break;
                }

                FlushProviders();
            }
            // TODO: Should we re-issue requests if they have failed?
            // What if (can?) they immediately fail again to the IOCP?
            // We'd end up stuck in this loop.

        } while (ret == TRUE ||
            (::GetLastError() != WAIT_TIMEOUT /*&&
            ::GetLastError() != ERROR_ABANDONED_WAIT*/));
    }


    inline void
        Framework::ProcessProviderChange()
    {
        // Enumerate the provider catalog, and rebuild our list of providers.
        DWORD len = 0;
        INT err;
        INT ret = ::WSCEnumProtocols(nullptr, nullptr, &len, &err);
        ASSERT(ret == SOCKET_ERROR);
        ASSERT(err == WSAENOBUFS);
        if (ret != SOCKET_ERROR || err != WSAENOBUFS)
        {
            return;
        }

        // We try only once - if the required buffer size changes then our
        // request for provider changes will get completed and we'll come back
        ASSERT((len % sizeof(WSAPROTOCOL_INFOW)) == 0);
        WSAPROTOCOL_INFOW* pProtocols = static_cast<WSAPROTOCOL_INFOW*>(::HeapAlloc(
            ghHeap,
            0,
            len
        ));
        if (pProtocols == nullptr)
        {
            return;
        }

        ret = ::WSCEnumProtocols(nullptr, pProtocols, &len, &err);
        if (ret == SOCKET_ERROR)
        {
            ::HeapFree(ghHeap, 0, pProtocols);
            return;
        }

        // Mark all existing providers inactive.
        for (List<Provider>::iterator pProv = m_ProviderList.begin();
            pProv != m_ProviderList.end();
            ++pProv)
        {
            pProv->MarkInactive();
        }

        for (DWORD i = 0; i < len / sizeof(WSAPROTOCOL_INFOW); i++)
        {
            if ((pProtocols[i].dwServiceFlags1 & ND_SERVICE_FLAGS1) !=
                ND_SERVICE_FLAGS1)
            {
                continue;
            }

            switch (pProtocols[i].iVersion)
            {
            case ND_VERSION_1:
                // NDv1 providers don't always set the PFL_NETWORKDIRECT flag.
                if ((pProtocols[i].dwProviderFlags & PFL_HIDDEN) != PFL_HIDDEN)
                {
                    continue;
                }
                break;

            case ND_VERSION_2:
                if ((pProtocols[i].dwProviderFlags & ND_PROVIDER_FLAGS) !=
                    ND_PROVIDER_FLAGS)
                {
                    continue;
                }
                break;

            default:
                continue;
            }

            if (pProtocols[i].iAddressFamily != AF_INET &&
                pProtocols[i].iAddressFamily != AF_INET6)
            {
                continue;
            }

            if (pProtocols[i].iSocketType != -1)
            {
                continue;
            }

            if (pProtocols[i].iProtocol != 0)
            {
                continue;
            }

            if (pProtocols[i].iProtocolMaxOffset != 0)
            {
                continue;
            }

            // We found a network direct provider.  See if it's already in the list.
            List<Provider>::iterator pProv = m_ProviderList.begin();
            while (pProv != m_ProviderList.end())
            {
                if (*pProv == pProtocols[i].ProviderId)
                {
                    pProv->MarkActive();
                    break;
                }
                ++pProv;
            }
            if (pProv != m_ProviderList.end())
            {
                //
                // We found a match in our existing provider list,
                // no need to create a new provider.
                //
                continue;
            }

            // New provider, add it to the list.
            Provider* pProvider;
            if (pProtocols[i].iVersion == ND_VERSION_1)
            {
                pProvider = new NdV1Provider();
            }
            else
            {
                pProvider = new NdProvider();
            }

            if (pProvider == nullptr)
            {
                continue;
            }

            HRESULT hr = pProvider->Init(pProtocols[i].ProviderId);
            if (FAILED(hr))
            {
                delete pProvider;
                continue;
            }

            m_ProviderList.push_back(pProvider);
        }
        ::HeapFree(ghHeap, 0, pProtocols);

        // We now have an up-to-date provider list.  Populate the address table.
        ProcessAddressChange();
    }


    inline void
        Framework::ProcessAddressChange()
    {
        // Remove all existing addresses.
        while (!m_NdAddrList.empty())
        {
            Address* pAddr = &m_NdAddrList.front();
            m_NdAddrList.pop_front();
            delete pAddr;
        }

        while (!m_NdV1AddrList.empty())
        {
            Address* pAddr = &m_NdV1AddrList.front();
            m_NdV1AddrList.pop_front();
            delete pAddr;
        }

        SOCKET_ADDRESS_LIST* pAddrList = nullptr;
        ULONG len = 0;

        for (List<Provider>::iterator pProv = m_ProviderList.begin();
            pProv != m_ProviderList.end();
            ++pProv)
        {
            if (!pProv->IsActive())
            {
                continue;
            }

            HRESULT hr = pProv->QueryAddressList(pAddrList, &len);
            if (hr == ND_BUFFER_OVERFLOW)
            {
                if (pAddrList != nullptr)
                {
                    ::HeapFree(ghHeap, 0, pAddrList);
                }

                // If the allocated buffer is not large enough, our request for
                // address change notifcation will pick up the change.
                pAddrList =
                    static_cast<SOCKET_ADDRESS_LIST*>(::HeapAlloc(ghHeap, 0, len));
                if (pAddrList == nullptr)
                {
                    continue;
                }

                hr = pProv->QueryAddressList(pAddrList, &len);
            }

            if (FAILED(hr))
            {
                continue;
            }

            __analysis_assume(pAddrList);

            if (pProv->GetVersion() == ND_VERSION_1)
            {
                BuildAddressList(*pProv, *pAddrList, &m_NdV1AddrList);
            }
            else
            {
                BuildAddressList(*pProv, *pAddrList, &m_NdAddrList);
            }
        }

        if (pAddrList != nullptr)
        {
            ::HeapFree(ghHeap, 0, pAddrList);
        }
    }


    inline void
        Framework::FlushProviders()
    {
        List<Provider>::iterator iter = m_ProviderList.begin();
        while (iter != m_ProviderList.end())
        {
            Provider* pProv = &*iter;
            //
            // Move to the next item now as we might remove the provider.
            //
            ++iter;

            if (pProv->TryUnload() == true && pProv->IsActive() == false)
            {
                m_ProviderList.remove(*pProv);
                delete pProv;
            }
        }
    }

} // namespace NetworkDirect


//
// Initialization
//


EXTERN_C inline HRESULT ND_HELPER_API
NdStartup(
    VOID
)
{
    LONG init;
    do
    {
        init = ::InterlockedCompareExchange(&NetworkDirect::gInitializing, 1, 0);
    } while (init == 1);

    if (NetworkDirect::gpFramework == nullptr)
    {
        NetworkDirect::ghHeap = ::HeapCreate(0, 0, 0);
        if (NetworkDirect::ghHeap == nullptr)
        {
            ::InterlockedDecrement(&NetworkDirect::gInitializing);
            return HRESULT_FROM_WIN32(::GetLastError());
        }

        NetworkDirect::gpFramework = new NetworkDirect::Framework();
        if (NetworkDirect::gpFramework == nullptr)
        {
            ::HeapDestroy(NetworkDirect::ghHeap);
            NetworkDirect::ghHeap = nullptr;
            ::InterlockedDecrement(&NetworkDirect::gInitializing);
            return ND_NO_MEMORY;
        }
        HRESULT hr = NetworkDirect::gpFramework->Init();
        if (FAILED(hr))
        {
            delete(NetworkDirect::gpFramework);
            NetworkDirect::gpFramework = nullptr;
            ::HeapDestroy(NetworkDirect::ghHeap);
            NetworkDirect::ghHeap = nullptr;
            ::InterlockedDecrement(&NetworkDirect::gInitializing);
            return hr;
        }
    }

    NetworkDirect::gpFramework->AddRef();
    ::InterlockedDecrement(&NetworkDirect::gInitializing);
    return S_OK;
}


EXTERN_C inline HRESULT ND_HELPER_API
NdCleanup(
    VOID
)
{
    LONG init;
    do
    {
        init = ::InterlockedCompareExchange(&NetworkDirect::gInitializing, 1, 0);
    } while (init == 1);

    if (NetworkDirect::gpFramework == nullptr)
    {
        ::InterlockedDecrement(&NetworkDirect::gInitializing);
        return ND_DEVICE_NOT_READY;
    }

    if (NetworkDirect::gpFramework->Release() == 0)
    {
        NetworkDirect::gpFramework = nullptr;
        ::HeapDestroy(NetworkDirect::ghHeap);
        NetworkDirect::ghHeap = nullptr;
    }

    ::InterlockedDecrement(&NetworkDirect::gInitializing);
    return S_OK;
}


EXTERN_C inline VOID ND_HELPER_API
NdFlushProviders(
    VOID
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return;
    }

    NetworkDirect::gpFramework->FlushProvidersForUser();
}


//
// Network capabilities
//
EXTERN_C inline HRESULT ND_HELPER_API
NdQueryAddressList(
    _In_ DWORD flags,
    _Out_opt_bytecap_post_bytecount_(*pcbAddressList, *pcbAddressList) SOCKET_ADDRESS_LIST* pAddressList,
    _Inout_ SIZE_T* pcbAddressList
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return ND_DEVICE_NOT_READY;
    }

    return NetworkDirect::gpFramework->QueryAddressList(flags, pAddressList, pcbAddressList);
}


EXTERN_C inline HRESULT ND_HELPER_API
NdResolveAddress(
    _In_bytecount_(cbRemoteAddress) const struct sockaddr* pRemoteAddress,
    _In_ SIZE_T cbRemoteAddress,
    _Out_bytecap_(*pcbLocalAddress) struct sockaddr* pLocalAddress,
    _Inout_ SIZE_T* pcbLocalAddress
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return ND_DEVICE_NOT_READY;
    }

    return NetworkDirect::gpFramework->ResolveAddress(
        pRemoteAddress,
        cbRemoteAddress,
        pLocalAddress,
        pcbLocalAddress
    );
}


EXTERN_C inline HRESULT ND_HELPER_API
NdCheckAddress(
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return ND_DEVICE_NOT_READY;
    }

    return NetworkDirect::gpFramework->CheckAddress(pAddress, cbAddress);
}


EXTERN_C inline HRESULT ND_HELPER_API
NdOpenAdapter(
    _In_ REFIID iid,
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress,
    _Deref_out_ VOID** ppIAdapter
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return ND_DEVICE_NOT_READY;
    }

    return NetworkDirect::gpFramework->OpenAdapter(iid, pAddress, cbAddress, ppIAdapter);
}


EXTERN_C inline HRESULT ND_HELPER_API
NdOpenV1Adapter(
    _In_bytecount_(cbAddress) const struct sockaddr* pAddress,
    _In_ SIZE_T cbAddress,
    _Deref_out_ INDAdapter** ppIAdapter
)
{
    if (NetworkDirect::gpFramework == nullptr)
    {
        return ND_DEVICE_NOT_READY;
    }

    return NetworkDirect::gpFramework->OpenAdapter(
        IID_INDAdapter,
        pAddress,
        cbAddress,
        reinterpret_cast<VOID**>(ppIAdapter)
    );
}

// ===== END inline implementation: ndfrmwrk.cpp =====


#endif // NETWORKDIRECT_SINGLE_HEADER_HPP
