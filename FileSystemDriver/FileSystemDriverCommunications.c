#include "FileSystemDriver.h"


NTSTATUS
ClientHandlerPortConnect(
_In_ PFLT_PORT ClientPort,
_In_opt_ PVOID ServerPortCookie,
_In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
_In_ ULONG SizeOfContext,
_Outptr_result_maybenull_ PVOID *ConnectionCookie
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);
	UNREFERENCED_PARAMETER(ConnectionCookie = NULL);

	FLT_ASSERT(clientPort == NULL);
	FLT_ASSERT(usrProc == NULL);

	usrProc = PsGetCurrentProcess();
	usrProcId = PsGetProcessId(usrProc);
	clientPort = ClientPort;

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("ClientHandlerPortDisconnect!ClientHandlerPortConnect: connected, port=0x%p\n", ClientPort));

	return STATUS_SUCCESS;
}


VOID
ClientHandlerPortDisconnect(
_In_opt_ PVOID ConnectionCookie
)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	PAGED_CODE();
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSystemDriver!ClientHandlerPortDisconnect: disconnected, port=0x%p\n", clientPort));

	FltCloseClientPort(gFilterHandle, &clientPort);

	usrProc = NULL;
	usrProcId = (HANDLE) -1;
	targetPid = (DWORD) -2;
}


NTSTATUS
ClientHandlerPortMessage(
_In_ PVOID PortCookie,
_In_opt_ PVOID InputBuffer,
_In_ ULONG InputBufferLength,
_Out_opt_ PVOID OutputBuffer,
_Out_ ULONG OutputBufferLength,
_Out_ PULONG ReturnOutputBufferLength
) {
	UNREFERENCED_PARAMETER(PortCookie);
	UNREFERENCED_PARAMETER(InputBuffer);
	UNREFERENCED_PARAMETER(InputBufferLength);
	UNREFERENCED_PARAMETER(OutputBuffer);
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(ReturnOutputBufferLength);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSystemDriver!ClientHandlerPortMessage: Entered\n"));

	if (InputBuffer != NULL && InputBufferLength >= sizeof(MSG_BODY_STRUCT)) {
		targetPid = ((MSG_BODY_STRUCT *)InputBuffer)->messageType.processPid.pid;
		PT_DBG_PRINT(PTDBG_INFORMATION,
			("FileSystemDriver!ClientHandlerPortMessage: Target PID: %d\n", targetPid));
		MSG_BODY_STRUCT reply;
		reply.messageType.driverReply.status = 0;
		RtlCopyMemory(OutputBuffer, &reply, sizeof(reply));
		*ReturnOutputBufferLength = sizeof(reply);
	}


	return STATUS_SUCCESS;
}
