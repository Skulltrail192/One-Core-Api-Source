#include <Windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char* yesno(bool b)
{
	return b ? "yes" : "no";
}

void print_bitmap(ULONG_PTR mask)
{
	for (int i = sizeof(mask)* 8 - 1; i >= 0; --i)
		printf("%d", (int)(mask >> i) & 1);
}

void print_group_affinity(const GROUP_AFFINITY& affinity)
{
	printf(" Group #%d = ", affinity.Group);
	print_bitmap(affinity.Mask);
	printf("\n");
}

int main(void)
{
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* pBuffer;
	DWORD cbBuffer;
	char dummy[256] = {};

	pBuffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)dummy;
	cbBuffer = 1;
	 if (GetLogicalProcessorInformationEx(RelationAll, pBuffer, &cbBuffer))
	 {
		 // what!?!?
		 printf("GetLogicalProcessorInformationEx returned nothing successfully.\n");
		 return 0;
	 }
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		printf("GetLogicalProcessorInformationEx returned error (1). GetLastError() = %u\n", GetLastError());
		return 1;
	}

	pBuffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)malloc(cbBuffer);
	if (!GetLogicalProcessorInformationEx(RelationAll, pBuffer, &cbBuffer))
	{
		printf("GetLogicalProcessorInformationEx returned error (2). GetLastError() = %u\n", GetLastError());
		return 2;
	}

	printf("GetLogicalProcessorInformationEx returned %u byte data.\n", cbBuffer);

	char* pCur = (char*)pBuffer;
	char* pEnd = pCur + cbBuffer;
	for (int idx = 0; pCur < pEnd; pCur += ((SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)pCur)->Size, ++idx)
	{
		printf("\n");

		pBuffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*)pCur;
		printf("Info #%d:\n", idx);
		static const char* relationship_list[] = {
			"ProcessorCore",
			"NumaNode",
			"Cache",
			"ProcessorPackage",
			"Group",
		};
		const char* relationship_str;
		if (pBuffer->Relationship >= _countof(relationship_list))
			relationship_str = "(reserved)";
		else
			relationship_str = relationship_list[pBuffer->Relationship];
		printf(" Relationship = %s (%d)\n", relationship_str, pBuffer->Relationship);

		switch (pBuffer->Relationship)
		{
		case RelationProcessorCore:
		case RelationProcessorPackage: {
			PROCESSOR_RELATIONSHIP& info = pBuffer->Processor;
			if (pBuffer->Relationship == RelationProcessorCore)
			{
				printf(" SMT Support = %s\n", yesno(info.Flags == LTP_PC_SMT));
				//printf(" Efficiency Class = %d\n", info.EfficiencyClass);
			}
			printf(" GroupCount = %d\n", info.GroupCount);
			for (int i = 0; i < info.GroupCount; ++i)
				print_group_affinity(info.GroupMask[i]);
		}
			break;

		case RelationNumaNode: {
			NUMA_NODE_RELATIONSHIP& info = pBuffer->NumaNode;
			printf(" Node Number = %d\n", info.NodeNumber);
			print_group_affinity(info.GroupMask);
		}
			break;

		case RelationCache: {
			CACHE_RELATIONSHIP& info = pBuffer->Cache;
			static const char* cachetype_list[] = {
				"Unified",
				"Instruction",
				"Data",
				"Trace",
			};
			const char* cachetype_str;
			if (info.Type >= _countof(cachetype_list))
				cachetype_str = "(reserved)";
			else
				cachetype_str = cachetype_list[info.Type];
			printf(" Type = L%d %s\n", info.Level, cachetype_str);
			printf(" Assoc = ");
			if (info.Associativity == 0xff)
				printf("full\n");
			else
				printf("%d\n", info.Associativity);
			printf(" Line Size = %dB\n", info.LineSize);
			printf(" Cache Size = %dKB\n", info.CacheSize / 1024);
			print_group_affinity(info.GroupMask);
		}
			break;

		case RelationGroup: {
			GROUP_RELATIONSHIP& info = pBuffer->Group;
			printf(" Max Group Count = %d\n", info.MaximumGroupCount);
			printf(" Active Group Count = %d\n", info.ActiveGroupCount);
			for (int i = 0; i < info.ActiveGroupCount; ++i)
			{
				PROCESSOR_GROUP_INFO& ginfo = info.GroupInfo[i];
				printf(" Group #%d:\n", i);
				printf("  Max Processor Count = %d\n", ginfo.MaximumProcessorCount);
				printf("  Active Processor Count = %d\n", ginfo.ActiveProcessorCount);
				printf("  Active Processor Mask = ");
				print_bitmap(ginfo.ActiveProcessorMask);
				printf("\n");
			}
		}
			break;
		}
	}
	
	system("pause");
}
