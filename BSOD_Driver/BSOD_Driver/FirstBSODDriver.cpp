//
// Original Code from ENDGAME's MyFirstBSODDriver.c
// (https://www.endgame.com/blog/technical-blog/introduction-windows-kernel-debugging)
//

#include <ntddk.h>

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath);

extern "C"
void
DriverUnload(
	IN PDRIVER_OBJECT DriverObject);

#define MY_POOL_TAG 'GTYM'
#define MY_STRING "Hello!"

PVOID pointer = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

extern "C"
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status = STATUS_SUCCESS;

	DriverObject->DriverUnload = DriverUnload;

	pointer = ExAllocatePoolWithTag(NonPagedPool, sizeof(MY_STRING), MY_POOL_TAG);
	if (!pointer)
	{
		status = STATUS_NO_MEMORY;
		return status;
	}

	pointer = NULL;
	strcpy((PCHAR)pointer, MY_STRING);

	return status;
}

extern "C"
void
DriverUnload(
	IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	if (pointer)
	{
		DbgPrint("the string at the pointer = %s\n", pointer);
		ExFreePoolWithTag(pointer, MY_POOL_TAG);
		pointer = NULL;
	}

	DbgPrint("MyBSODDriver successfully unloaded!\n");
}
