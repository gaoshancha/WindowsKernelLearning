//
// Based on ENDGAME's MyDeadLockDriver.c 
// (https://www.endgame.com/blog/technical-blog/introduction-windows-kernel-debugging)
//

#include <ntifs.h>

HANDLE gThreadA = NULL;
HANDLE gThreadB = NULL;
ERESOURCE gLockA;
ERESOURCE gLockB;
KEVENT    Event;

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
);

extern "C"
void
DriverUnload(
	IN     PDRIVER_OBJECT      DriverObject);

_IRQL_requires_max_(APC_LEVEL)
VOID ThreadA(
	_In_opt_ PVOID Ctx
)
{
	UNREFERENCED_PARAMETER(Ctx);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&gLockA, TRUE);
	DbgPrint("ThreadA acquired lock A.\n");

	// KeWaitForSingleObject is here to force a deadlock
	KeWaitForSingleObject((PVOID)&Event,
		Executive,
		KernelMode,
		FALSE,
		NULL);


	DbgPrint("ThreadA trying to acquire lock B...\n");
	ExAcquireResourceExclusiveLite(&gLockB, TRUE);
	DbgPrint("ThreadA acquired lock B.\n");

	ExReleaseResourceLite(&gLockB);
	ExReleaseResourceLite(&gLockA);
	KeLeaveCriticalRegion();
}

_IRQL_requires_max_(APC_LEVEL)
VOID ThreadB(
	_In_opt_ PVOID Ctx
)
{
	UNREFERENCED_PARAMETER(Ctx);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&gLockB, TRUE);
	DbgPrint("ThreadB acquired lock B.\n");

	KeSetEvent(&Event, 0, FALSE);

	DbgPrint("ThreadB trying to acquire lock A...\n");
	ExAcquireResourceExclusiveLite(&gLockA, TRUE);
	DbgPrint("ThreadB acquired lock A.\n");

	ExReleaseResourceLite(&gLockA);
	ExReleaseResourceLite(&gLockB);
	KeLeaveCriticalRegion();
}

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status = STATUS_SUCCESS;

	DriverObject->DriverUnload = DriverUnload;

	status = ExInitializeResourceLite(&gLockA);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to initialize resource A.\n");
		return status;
	}

	status = ExInitializeResourceLite(&gLockB);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to initialize resource B.\n");
		return status;
	}

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	status = PsCreateSystemThread(&gThreadA,
		0x1F03FF,
		NULL,
		NULL,
		NULL,
		ThreadA,
		NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to initialize thread A.\n");
		return status;
	}

	status = PsCreateSystemThread(&gThreadB,
		0x1F03FF,
		NULL,
		NULL,
		NULL,
		ThreadB,
		NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to initialize thread B.\n");
		return status;
	}

	return status;
}

extern "C"
void
DriverUnload(
	IN     PDRIVER_OBJECT      DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);
	NTSTATUS status = STATUS_SUCCESS;

	// Wait for thread A to terminate
	status = ZwWaitForSingleObject(gThreadA, FALSE, NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to wait for thread A.\n");
	}

	// Close the handle to thread A
	ZwClose(gThreadA);
	gThreadA = NULL;

	// Wait for thread B to terminate
	status = ZwWaitForSingleObject(gThreadB, FALSE, NULL);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("Failed to wait for thread B.\n");
	}

	// Close the handle to thread B
	ZwClose(gThreadB);
	gThreadB = NULL;

	ExDeleteResourceLite(&gLockA);
	ExDeleteResourceLite(&gLockB);
	DbgPrint("MyDeadlockDriver successfully unloaded!\n");
}
