#ifndef _FILE_SYSTEM_DRIVER_H
#define _FILE_SYSTEM_DRIVER_H

#ifndef CLIENT_SIDE
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <windef.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;

// Global data
PFLT_PORT serverPort;
PEPROCESS usrProc;
HANDLE usrProcId;
PFLT_PORT clientPort;
DWORD targetPid;

//tags
#define CTX_STRING_TAG 'CTXt'
#define CTX_OBJECT_TAG 'CTXo'

#define MAX_BUFFER_SIZE 2048

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002
#define PTDBG_INFORMATION               0x00000004
#define PTDBG_WARNING                   0x00000008
#define PTDBG_ERROR                     0x00000010


static ULONG gTraceFlags =
	PTDBG_TRACE_OPERATION_STATUS |
	PTDBG_INFORMATION |
	PTDBG_WARNING |
	PTDBG_ERROR;


#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))

typedef struct _CTX_CONTEXT_INSTANCE {
	PFLT_INSTANCE Instance;
	PFLT_VOLUME Volume;
	UNICODE_STRING VolumeName;
} CTX_CONTEXT_INSTANCE, *PCTX_CONTEXT_INSTANCE;


#define CTX_INSTANCE_CONTEXT_SIZE         sizeof( CTX_CONTEXT_INSTANCE )

void
FileClearCache(
PFILE_OBJECT pFileObject
);

VOID
ClientHandlerPortDisconnect(
_In_opt_ PVOID CookieConnection
);

NTSTATUS
ClientHandlerPortConnect(
_In_ PFLT_PORT ClientPort,
_In_opt_ PVOID CookieServerPort,
_In_reads_bytes_opt_(SizeOfContext) PVOID ContextConnection,
_In_ ULONG SizeOfContext,
_Outptr_result_maybenull_ PVOID *ConnectionCookie
);

NTSTATUS
ClientHandlerPortMessage(
_In_ PVOID CookiePort,
_In_opt_ PVOID InputBuffer,
_In_ ULONG InputBufferLen,
_Out_opt_ PVOID OutputBuffer,
_Out_ ULONG OutputBufferLen,
_Out_ PULONG ReturnOutputBufferLen
);

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
_In_ PDRIVER_OBJECT DriverObject,
_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
FileSystemDriverInstanceSetup(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_SETUP_FLAGS Flags,
_In_ DEVICE_TYPE VolumeDeviceType,
_In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);


NTSTATUS
FileSystemDriverUnload(
_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

VOID CtxInstanceTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags);

NTSTATUS
FileSystemDriverInstanceQueryTeardown(
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FileSystemDriverPreOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *ContextCompletion
);

FLT_POSTOP_CALLBACK_STATUS
FileSystemDriverPostOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID ContextCompletion,
_In_ FLT_POST_OPERATION_FLAGS Flags
);

VOID CtxContextCleanup(_In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType);

BOOLEAN CheckExtension(_In_ PFILE_OBJECT fileObject);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, FileSystemDriverUnload)
#pragma alloc_text(PAGE, CtxInstanceTeardownComplete)
#pragma alloc_text(PAGE, FileSystemDriverInstanceSetup)
#pragma alloc_text(PAGE, CtxContextCleanup)
#endif

static const FLT_CONTEXT_REGISTRATION ContextRegistration[] = {

	{ FLT_INSTANCE_CONTEXT,
	0,
	CtxContextCleanup,
	CTX_INSTANCE_CONTEXT_SIZE,
	CTX_OBJECT_TAG },

	{ FLT_CONTEXT_END }
};

static CONST FLT_OPERATION_REGISTRATION Callbacks[] = {

	{ IRP_MJ_CREATE,
	0,
	FileSystemDriverPreOperation,
	FileSystemDriverPostOperation },

	{ IRP_MJ_READ,
	0,
	FileSystemDriverPreOperation,
	FileSystemDriverPostOperation },

	{ IRP_MJ_WRITE,
	0,
	FileSystemDriverPreOperation,
	FileSystemDriverPostOperation },

	{ IRP_MJ_OPERATION_END }
};

static CONST FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),           //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags
	ContextRegistration,                //  Context
	Callbacks,                          //  Operation callbacks
	FileSystemDriverUnload,             //  MiniFilterUnload
	FileSystemDriverInstanceSetup,      //  InstanceSetup
	NULL,								//  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	CtxInstanceTeardownComplete,        //  InstanceTeardownComplete
	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent

};

#endif

#ifdef CLIENT_SIDE
#define RTL_GUID_STRING_SIZE 38
#endif

enum OPER_TYPE {
	ReadOp, WriteOp
};

typedef struct _MSG_BODY_STRUCT {
	union {
		struct {
			UINT8 ioOpType;
			// + size of prefix "\??\Volume"
			WCHAR guid[64];
			WCHAR path[512];
			DWORD pid;
		} operationStatus;
		struct {
			DWORD pid;
		} processPid;
		struct {
			DWORD status;
		} driverReply;
	} messageType;

} MSG_BODY_STRUCT, *PMSG_BODY_STRUCT;

typedef struct _MSG_STRUCT {
	FILTER_MESSAGE_HEADER header;
	MSG_BODY_STRUCT body;
} MSG_STRUCT, *PMSG_STRUCT;

#endif