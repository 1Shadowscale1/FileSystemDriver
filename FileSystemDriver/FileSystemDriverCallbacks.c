#include "FileSystemDriver.h"


FLT_PREOP_CALLBACK_STATUS
FileSystemDriverPreOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_Flt_CompletionContext_Outptr_ PVOID *ContextCompletion
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(ContextCompletion);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
		("FileSystemDriver!FileSystemDriverPreOperation: Entered\n"));

	PEPROCESS peprocess = IoThreadToProcess(Data->Thread);
	HANDLE pid = PsGetProcessId(peprocess);
	PT_DBG_PRINT(PTDBG_INFORMATION, ("Process PID is: %d\n", pid));

	PCTX_CONTEXT_INSTANCE contextInstance;

	NTSTATUS sts = FltGetInstanceContext(FltObjects->Instance, &contextInstance);
	if (!NT_SUCCESS(sts))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverPreOperation: Fails on FltGetInstanceContext. Status=%08x\n", sts));
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	PT_DBG_PRINT(PTDBG_INFORMATION, ("Reading file from volume with name: %wZ\n", contextInstance->VolumeName));

	PFLT_FILE_NAME_INFORMATION fileInfo = NULL;
	MSG_BODY_STRUCT msg;

	if ((clientPort != NULL) && (((DWORD_PTR)pid == targetPid) || (targetPid == -1)) && (pid != usrProcId)) {
		try {
			if (FltObjects->FileObject != NULL) {
				sts = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &fileInfo);
				if (!NT_SUCCESS(sts)) {
					PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!FileSystemDriverPreOperation: Fails on FltGetFileNameInformation, status=%08x\n", sts));
					leave;
				}

				msg.messageType.operationStatus.ioOpType = Data->Iopb->MajorFunction;
				msg.messageType.operationStatus.pid = (DWORD)(DWORD_PTR)pid;
				
				RtlCopyMemory(msg.messageType.operationStatus.guid,
							  contextInstance->VolumeName.Buffer,
							  contextInstance->VolumeName.Length);
				
				msg.messageType.operationStatus.guid[contextInstance->VolumeName.Length / sizeof(WCHAR)] = L'\0';
				
				RtlCopyMemory(msg.messageType.operationStatus.path,
							  fileInfo->Name.Buffer + (fileInfo->Volume.Length / sizeof(WCHAR) + 1),
							  fileInfo->Name.Length - fileInfo->Volume.Length - 1);
				
				msg.messageType.operationStatus.path[(fileInfo->Name.Length - fileInfo->Volume.Length - 1) / sizeof(WCHAR)] = L'\0';
				
				sts = FltSendMessage(gFilterHandle, &clientPort, &msg, sizeof(MSG_BODY_STRUCT), NULL, NULL, NULL);
				if (!NT_SUCCESS(sts)) {
					PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!FileSystemDriverPreOperation: Fails on FltSendMessage, status=%08x\n", sts));
					leave;
				}
			}
		} finally {
			if (fileInfo != NULL)
				FltReleaseFileNameInformation(fileInfo);
		}
	}

	FltReleaseContext(contextInstance);

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


FLT_POSTOP_CALLBACK_STATUS
FileSystemDriverPostOperation(
_Inout_ PFLT_CALLBACK_DATA Data,
_In_ PCFLT_RELATED_OBJECTS FltObjects,
_In_opt_ PVOID ContextCompletion,
_In_ FLT_POST_OPERATION_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(ContextCompletion);
	UNREFERENCED_PARAMETER(Flags);
	return FLT_POSTOP_FINISHED_PROCESSING;
}

