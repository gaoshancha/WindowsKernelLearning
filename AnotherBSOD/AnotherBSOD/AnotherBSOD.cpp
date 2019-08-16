//
// Based on ENDGAME's MyBSOD1Driver1.c
// (https://www.endgame.com/blog/technical-blog/introduction-windows-kernel-debugging)
//

#include <ntifs.h>

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
);

void
DriverUnload(
	IN     PDRIVER_OBJECT      DriverObject);

#define MY_POOL_TAG 'GTYM'
#define MY_STRING "Hello!"
#define MY_PAGEDPOOL_SIZE 512 * 1024 * 1024
PVOID pointer = NULL;
KSPIN_LOCK SpinLock;

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

	KeInitializeSpinLock(&SpinLock);

	pointer = ExAllocatePoolWithTag(PagedPool, MY_PAGEDPOOL_SIZE, MY_POOL_TAG);
	if (!pointer)
	{
		status = STATUS_NO_MEMORY;
		return status;
	}

	return status;
}

void
DriverUnload(
	IN     PDRIVER_OBJECT      DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	if (pointer)
	{
		KLOCK_QUEUE_HANDLE LockHandle;

		KeAcquireInStackQueuedSpinLock(&SpinLock, &LockHandle);

		// BSOD accessing PagedPool memory inside of a spinlock
		RtlSecureZeroMemory(pointer, MY_PAGEDPOOL_SIZE);

		KeReleaseInStackQueuedSpinLock(&LockHandle);

		ExFreePoolWithTag(pointer, MY_POOL_TAG);
		pointer = NULL;
	}

	DbgPrint("MyBSODDriver1 successfully unloaded!\n");
}
