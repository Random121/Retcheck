#pragma once
#include <Windows.h>

namespace PEB {
	typedef struct _LIST_ENTRY {
		struct _LIST_ENTRY *Flink;
		struct _LIST_ENTRY *Blink;
	} LIST_ENTRY, *PLIST_ENTRY, *RESTRICTED_POINTER PRLIST_ENTRY;

	typedef struct _PEB_FREE_BLOCK {
		void*				Next; // PPEB_FREE_BLOCK
		ULONG				Size;
	} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;


	struct THREAD_BASIC_INFORMATION {
		unsigned long ExitStatus;
		unsigned long TEBAddress;
		unsigned long shit[0x5]; //Only to preserve the structure's size
	};

	typedef enum _MEMORY_INFORMATION_CLASS {
		MemoryBasicInformation
	} MEMORY_INFORMATION_CLASS;


	typedef struct _PEB_LDR_DATA {
		UCHAR			Reserved1[8];
		PVOID			Reserved2[3];
		LIST_ENTRY		InMemoryOrderModuleList;
	} PEB_LDR_DATA, *PPEB_LDR_DATA;


	typedef struct _UNICODE_STRING {
		USHORT			Length;
		USHORT			MaximumLength;
		WCHAR *			Buffer;
	} UNICODE_STRING, *PUNICODE_STRING;


	typedef struct _RTL_USER_PROCESS_PARAMETERS {
		UCHAR			Reserved1[16];
		PVOID			Reserved2[10];
		UNICODE_STRING	ImagePathName;
		UNICODE_STRING	CommandLine;
	} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;


	typedef struct _IO_STATUS_BLOCK {
		union {
			LONG		Status; // NTSTATUS
			PVOID		Pointer;
		} DUMMYUNIONNAME;
		ULONG_PTR		Information;
	} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;


	typedef struct _OBJECT_ATTRIBUTES {
		ULONG           Length;
		HANDLE          RootDirectory;
		PUNICODE_STRING ObjectName;
		ULONG           Attributes;
		PVOID           SecurityDescriptor;
		PVOID           SecurityQualityOfService;
	} OBJECT_ATTRIBUTES;
	typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

	struct PEB {
		/*0x000 */ BYTE InheritedAddressSpace;
		/*0x001 */ BYTE ReadImageFileExecOptions;
		/*0x002 */ BYTE BeingDebugged;
		/*0x003 */ BYTE SpareBool;
		/*0x004 */ void* Mutant;
		/*0x008 */ void* ImageBaseAddress;
		/*0x00C */ _PEB_LDR_DATA* Ldr;
		/*0x010 */ _RTL_USER_PROCESS_PARAMETERS* ProcessParameters;
		/*0x014 */ void* SubSystemData;
		/*0x018 */ void* ProcessHeap;
		/*0x01C */ _RTL_CRITICAL_SECTION* FastPebLock;
		/*0x020 */ void* FastPebLockRoutine;
		/*0x024 */ void* FastPebUnlockRoutine;
		/*0x028 */ DWORD EnvironmentUpdateCount;
		/*0x02C */ void* KernelCallbackTable;
		/*0x030 */ DWORD SystemReserved[1];
		/*0x034 */ DWORD ExecuteOptions : 2; // bit offset: 34, len=2
		/*0x034 */ DWORD SpareBits : 30; // bit offset: 34, len=30
		/*0x038 */ _PEB_FREE_BLOCK* FreeList;
		/*0x03C */ DWORD TlsExpansionCounter;
		/*0x040 */ void* TlsBitmap;
		/*0x044 */ DWORD TlsBitmapBits[2];
		/*0x04C */ void* ReadOnlySharedMemoryBase;
		/*0x050 */ void* ReadOnlySharedMemoryHeap;
		/*0x054 */ void** ReadOnlyStaticServerData;
		/*0x058 */ void* AnsiCodePageData;
		/*0x05C */ void* OemCodePageData;
		/*0x060 */ void* UnicodeCaseTableData;
		/*0x064 */ DWORD NumberOfProcessors;
		/*0x068 */ DWORD NtGlobalFlag;
		/*0x070 */ _LARGE_INTEGER CriticalSectionTimeout;
		/*0x078 */ DWORD HeapSegmentReserve;
		/*0x07C */ DWORD HeapSegmentCommit;
		/*0x080 */ DWORD HeapDeCommitTotalFreeThreshold;
		/*0x084 */ DWORD HeapDeCommitFreeBlockThreshold;
		/*0x088 */ DWORD NumberOfHeaps;
		/*0x08C */ DWORD MaximumNumberOfHeaps;
		/*0x090 */ void** ProcessHeaps;
		/*0x094 */ void* GdiSharedHandleTable;
		/*0x098 */ void* ProcessStarterHelper;
		/*0x09C */ DWORD GdiDCAttributeList;
		/*0x0A0 */ void* LoaderLock;
		/*0x0A4 */ DWORD OSMajorVersion;
		/*0x0A8 */ DWORD OSMinorVersion;
		/*0x0AC */ WORD OSBuildNumber;
		/*0x0AE */ WORD OSCSDVersion;
		/*0x0B0 */ DWORD OSPlatformId;
		/*0x0B4 */ DWORD ImageSubsystem;
		/*0x0B8 */ DWORD ImageSubsystemMajorVersion;
		/*0x0BC */ DWORD ImageSubsystemMinorVersion;
		/*0x0C0 */ DWORD ImageProcessAffinityMask;
		/*0x0C4 */ DWORD GdiHandleBuffer[34];
		/*0x14C */ void(*PostProcessInitRoutine)();
		/*0x150 */ void* TlsExpansionBitmap;
		/*0x154 */ DWORD TlsExpansionBitmapBits[32];
		/*0x1D4 */ DWORD SessionId;
		/*0x1D8 */ _ULARGE_INTEGER AppCompatFlags;
		/*0x1E0 */ _ULARGE_INTEGER AppCompatFlagsUser;
		/*0x1E8 */ void* pShimData;
		/*0x1EC */ void* AppCompatInfo;
		/*0x1F0 */ _UNICODE_STRING CSDVersion;
		/*0x1F8 */ void* ActivationContextData;
		/*0x1FC */ void* ProcessAssemblyStorageMap;
		/*0x200 */ void* SystemDefaultActivationContextData;
		/*0x204 */ void* SystemAssemblyStorageMap;
		/*0x208 */ DWORD MinimumStackCommit;
	};
}
