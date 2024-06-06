#include "FileSystemDriver.h"


BOOLEAN CheckExtension(_In_ PFILE_OBJECT fileObject) {
	NTSTATUS sts = 0;
	WCHAR txtExtBuffer[] = L".txt";
	UNICODE_STRING txtExt, fileExt;
	txtExt.Buffer = txtExtBuffer;
	fileExt.Length = txtExt.Length = sizeof(txtExtBuffer) - sizeof(WCHAR);
	fileExt.MaximumLength = txtExt.MaximumLength = sizeof(txtExtBuffer);

	if (NULL != fileObject) {
		PT_DBG_PRINT(PTDBG_INFORMATION,
			("Read file: %wZ\n", &fileObject->FileName));
		if (clientPort != NULL) {
			sts = FltSendMessage(gFilterHandle, &clientPort, fileObject->FileName.Buffer, fileObject->FileName.Length, NULL, NULL, NULL);
			PT_DBG_PRINT(PTDBG_TRACE_OPERATION_STATUS, ("FileSystemDriver!DriverEntry: CheckExtension status=%08x\n", sts));
		}
		if (NULL != fileObject->FileName.Buffer && fileObject->FileName.Length >= txtExt.Length) {
			fileExt.Buffer = &fileObject->FileName.Buffer[(fileObject->FileName.Length - txtExt.Length) / sizeof(WCHAR)];
			PT_DBG_PRINT(PTDBG_INFORMATION,
				("Extension: %wZ\n", &fileExt));
			if (RtlEqualUnicodeString(&txtExt, &fileExt, TRUE)) {
				PT_DBG_PRINT(PTDBG_INFORMATION,
					("Text file detected\n"));
				return TRUE;
			}
		}
	}
	return FALSE;
}
