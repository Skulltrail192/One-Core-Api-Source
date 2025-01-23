#include "precomp.h"
//#define NDEBUG
#include <debug.h>

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry (
    PDRIVER_OBJECT  DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    DPRINT("ACPI_NEW: Entry\n");
    ACPIInitUACPI();
    return 1;
}
