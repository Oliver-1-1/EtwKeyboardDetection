#include "etw.h"

ETW_TOKEN token;

UINT32 etwCount;
UINT32 win32Count;
HHOOK keyboardHook;
BOOL active = FALSE;

VOID
EtwCallback(
	__in PEVENT_RECORD Event
)
{
	DWORD size;
	PTRACE_EVENT_INFO trace;
	PWCHAR provider;
	EVENT_PROPERTY_INFO property;

	trace = GetEventData(Event, &size);

	provider = (PCHAR)trace + trace->ProviderNameOffset;

	for (UINT countIndex = 0;
		countIndex < trace->TopLevelPropertyCount;
		countIndex = countIndex + 1)
	{
		EVENT_PROPERTY_INFO  property = trace->EventPropertyInfoArray[countIndex];

		if (!wcscmp(L"fid_UCX_URB_BULK_OR_INTERRUPT_TRANSFER", (PCHAR)trace + property.NameOffset))
		{

			for (INT propertyIndex = property.structType.StructStartIndex;
				propertyIndex < property.structType.StructStartIndex + property.structType.NumOfStructMembers;
				propertyIndex = propertyIndex + 1)
			{
				property = trace->EventPropertyInfoArray[propertyIndex];

				if (!wcscmp(L"fid_URB_TransferBufferLength", (PCHAR)trace + property.NameOffset))
				{

					//
					//Get value of fid_URB_TransferBufferLength
					//

					LPCWSTR string = GetPropertyData(
						trace,
						Event,
						property,
						20); // index for fid_URB_TransferBufferLength

					if (string == NULL)
					{
						continue;
					}

					//
					// Filter out for only keyboard packets by size
					//

					if (!wcscmp(string, L"0xC"))
					{
						etwCount = etwCount + 1;
					}

					free(string);
				}
			}
		}
	}

	free(trace);
}

LRESULT
Win32Callback(
	__in INT    Code,
	__in WPARAM WParam,
	__in LPARAM LParam
)
{
	KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*)LParam;

	//if (key->flags & LLKHF_LOWER_IL_INJECTED || key->flags & LLKHF_INJECTED) {
	//	printf("Simulated keyboard!\n");
	//}

	if (Code >= 0 && WParam == WM_KEYUP)
	{
		if (key->vkCode == VK_F1)
		{
			if (active)
			{
				
				//
				// Each keyboard press has 2 packets. 4 Because up and down
				//

				if (win32Count == (etwCount / 4))
				{
					printf("Not simulated!\n");
				}
				else
				{
					printf("Simulated!\n");
				}
			}
			else
			{
				active = TRUE;
			}

			win32Count = 0;
			etwCount = -4; // Since the packets for F1 will arrive late

			return ERROR_SUCCESS;
		}

		win32Count = win32Count + 1;

	}

	return CallNextHookEx(keyboardHook,
		Code,
		WParam,
		LParam);
}

VOID
main(
)
{
	struct tagMSG msg;
	BOOL status;

	SetupEtwSession(
		L"Zeptaaaaa",
		EtwCallback,
		NULL,
		&token);

	status = StartEtwSessionAsync(&token);

	if (status == FALSE)
	{
		printf("Failed to start etw async seesion!\n");
		return;
	}

	keyboardHook = SetWindowsHookExA(
		WH_KEYBOARD_LL,
		Win32Callback,
		NULL,
		0);

	while (GetMessageA(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

}


