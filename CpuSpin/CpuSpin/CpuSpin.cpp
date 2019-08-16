//
// Based on ENDGAME's My100CPUDriver.c
// (https://www.endgame.com/blog/technical-blog/introduction-windows-kernel-debugging)
//

#include <ntifs.h>

HANDLE gThreadA = NULL;
HANDLE gThreadB = NULL;

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath);

extern "C"
void
DriverUnload(
	IN PDRIVER_OBJECT DriverObject);

_IRQL_requires_max_(APC_LEVEL)
VOID myThread(
	_In_opt_ PVOID Ctx
)
{
	UNREFERENCED_PARAMETER(Ctx);
	int i = 0;

	// Here's the infinite loop
	for (;;)
	{
		if (i == 65536)
		{
			i = 0;
		}
		else
		{
			i++;
		}
	}

}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status = STATUS_SUCCESS;

	DriverObject->DriverUnload = DriverUnload;

	status = PsCreateSystemThread(&gThreadA,
		0x1F03FF,
		NULL,
		NULL,
		NULL,
		myThread,
		NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to initialize myThread.\n");
		return status;
	}

	/*
	// This part is for demonstrating 2 infinite loop threads
	// Comment out when demonstrating only 1 infinite loop thread
	status = PsCreateSystemThread(&gThreadB,
	0x1F03FF,
	NULL,
	NULL,
	NULL,
	myThread,
	NULL);
	if (!NT_SUCCESS(status))
	{
	DbgPrint("Failed to initialize myThread.\n");
	return status;
	}
	*/

	return status;
}

extern "C"
void
DriverUnload(
	IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	NTSTATUS status = STATUS_SUCCESS;

	// Wait for myThread A to terminate
	status = ZwWaitForSingleObject(gThreadA, FALSE, NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to wait for the thread.\n");
	}

	// Close the handle to myThread A
	ZwClose(gThreadA);
	gThreadA = NULL;

	/*
	// This part is for demonstrating 2 infinite loop threads
	// Comment out when demonstrating only 1 infinite loop thread

	// Wait for myThread B to terminate
	status = ZwWaitForSingleObject(gThreadB, FALSE, NULL);
	if (!NT_SUCCESS(status))
	{
	DbgPrint("Failed to wait for the thread.\n");
	}

	// Close the handle to myThread B
	ZwClose(gThreadB);
	gThreadB = NULL;
	*/

	DbgPrint("My100CPUDriver successfully unloaded!\n");
}