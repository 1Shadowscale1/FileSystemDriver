#include "FileSystemDriver.h"

NTSTATUS FileSystemDriverInstanceSetup (_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType)
{
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(VolumeDeviceType);
    UNREFERENCED_PARAMETER(VolumeFilesystemType);

    PAGED_CODE();
	
    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Entered\n") );
	
	if (NULL == FltObjects->Volume)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Volume is NULL\n"));
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	ULONG needed = 0;
	NTSTATUS sts = FltGetVolumeGuidName(FltObjects->Volume, NULL, &needed);
	if (STATUS_BUFFER_TOO_SMALL != sts)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Error on first call FltGetVolumeGuidName Status=%08x\n", sts));
		return STATUS_FLT_DO_NOT_ATTACH;
	}
	
	PCTX_CONTEXT_INSTANCE contextInstance = NULL;
	sts = FltAllocateContext(FltObjects->Filter, FLT_INSTANCE_CONTEXT, CTX_INSTANCE_CONTEXT_SIZE, NonPagedPool,	&contextInstance);
	if (!NT_SUCCESS(sts))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Can't allocate context! Status=%08x\n", sts));
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	contextInstance->VolumeName.Length = 0;
	contextInstance->Instance = FltObjects->Instance;
	contextInstance->Volume = FltObjects->Volume;
	contextInstance->VolumeName.MaximumLength = (USHORT) needed;
	
	contextInstance->VolumeName.Buffer = ExAllocatePoolWithTag(PagedPool, contextInstance->VolumeName.MaximumLength, CTX_STRING_TAG);
	if (NULL == contextInstance->VolumeName.Buffer)
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Error on memory allocation!\n"));
		FltReleaseContext(contextInstance);
		return STATUS_FLT_DO_NOT_ATTACH;
	}
	
	sts = FltGetVolumeGuidName(FltObjects->Volume, &contextInstance->VolumeName, &needed);
	if (!NT_SUCCESS(sts))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Can't get volume name! Status=%08x\n", sts));
		FltReleaseContext(contextInstance);
		return STATUS_FLT_DO_NOT_ATTACH;
	}

	PT_DBG_PRINT(PTDBG_INFORMATION, ("Volume name: %wZ\n", contextInstance->VolumeName));

	sts = FltSetInstanceContext(FltObjects->Instance, FLT_SET_CONTEXT_KEEP_IF_EXISTS, contextInstance, NULL);
	if (!NT_SUCCESS(sts))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceSetup: Can't set instance context! Status=%08x\n", sts));
		FltReleaseContext(contextInstance);
		return STATUS_FLT_DO_NOT_ATTACH;
	}
	
	FltReleaseContext(contextInstance);
	
    return STATUS_SUCCESS;
}

VOID CtxInstanceTeardownComplete(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	PCTX_CONTEXT_INSTANCE contextInstance;

	PAGED_CODE();

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceTeardownComplete: Entered\n"));

	NTSTATUS status = FltGetInstanceContext(FltObjects->Instance, &contextInstance);
	if (!NT_SUCCESS(status))
	{
		PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceQueryTeardown: Fails on FltGetInstanceContext. Status=%08x\n", status));
		return;
	}

	PT_DBG_PRINT(PTDBG_INFORMATION, ("Unregistering context with volume name: %wZ\n", contextInstance->VolumeName));

	FltReleaseContext(contextInstance);

	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverInstanceTeardownComplete: Completed!\n"));
}

VOID CtxContextCleanup(_In_ PFLT_CONTEXT Context, _In_ FLT_CONTEXT_TYPE ContextType)
{
	PAGED_CODE();

	switch (ContextType) {
	case FLT_INSTANCE_CONTEXT:
		ExFreePoolWithTag(((PCTX_CONTEXT_INSTANCE)Context)->VolumeName.Buffer, CTX_STRING_TAG);
		break;
	}
}

/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS DriverEntry (_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS sts = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES objAttr;
	UNICODE_STRING unicodeStr;
	PSECURITY_DESCRIPTOR secDesc;

	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);
	targetPid = (DWORD) -2;

    UNREFERENCED_PARAMETER(RegistryPath);

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!DriverEntry: Entered\n"));

	try
	{
		sts = FltRegisterFilter(DriverObject, &FilterRegistration, &gFilterHandle);
		if (!NT_SUCCESS(sts))
		{
			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!DriverEntry: FltREgisterFilter Failed, status=%08x\n", sts));
			leave;
		}

		RtlInitUnicodeString(&unicodeStr, L"\\FileSystemDriver");

		sts = FltBuildDefaultSecurityDescriptor(&secDesc, FLT_PORT_ALL_ACCESS);
		if (!NT_SUCCESS(sts))
		{
			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!DriverEntry: FltBuildDefaultSecurityDescriptor Failed, status=%08x\n", sts));
			leave;
		}

		InitializeObjectAttributes(&objAttr, &unicodeStr,	OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL,	secDesc);

		sts = FltCreateCommunicationPort(gFilterHandle, &serverPort, &objAttr, NULL, ClientHandlerPortConnect, ClientHandlerPortDisconnect, ClientHandlerPortMessage,	1);
		
		FltFreeSecurityDescriptor(secDesc);
		if (!NT_SUCCESS(sts))
		{
			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!DriverEntry: ltCreateCommunicationPort Failed, status=%08x\n", sts));
			leave;
		}

		sts = FltStartFiltering(gFilterHandle);
		if (!NT_SUCCESS(sts))
		{
			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!DriverEntry: FltStartFiltering Failed, status=%08x\n", sts));
			leave;
		}

	} finally
	{
		if (!NT_SUCCESS(sts))
		{
			if (NULL != serverPort)
				FltCloseCommunicationPort(serverPort);

			if (NULL != gFilterHandle)
				FltUnregisterFilter(gFilterHandle);
		}
	}

    return sts;
}

NTSTATUS FileSystemDriverUnload (_In_ FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverUnload: Entered\n"));

	FltCloseCommunicationPort(serverPort);
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverUnload: Communication port closed\n"));

    FltUnregisterFilter(gFilterHandle);
	PT_DBG_PRINT(PTDBG_TRACE_ROUTINES, ("FileSystemDriver!FileSystemDriverUnload: FilterUnregistered\n"));

    return STATUS_SUCCESS;
}
