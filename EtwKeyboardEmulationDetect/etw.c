#include "etw.h"

CONST GUID ucx = { 0x36DA592D, 0xE43A, 0x4E28, {0xAF,0x6F, 0x4B, 0xC5, 0x7C, 0x5A, 0x11, 0xE8} };
CONST GUID zepta = { 0x11111011, 0x1345, 0x01bcd, { 0xAA, 0x22, 0x71, 0x00, 0x00, 0x00, 0x00, 0xF3 } };

BOOL
SetupEtwSession(
	__in CONST LPCWSTR Name,
	__in CONST PVOID CallbackRecord,
	__in CONST PVOID CallbackBuffer,
	__inout PETW_TOKEN Token
)
{
	ULONG size;
	ULONG status = 0;
	PVOID buffer;
	PEVENT_TRACE_PROPERTIES properties;

	if (Name == NULL || Token == NULL)
	{
		return FALSE;
	}

	Token->Name = Name;
	Token->Handle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
	Token->SetupHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

	Token->CallbackRecord = CallbackRecord;
	Token->CallbackBuffer = CallbackBuffer;
	Token->Thread = NULL;
	Token->Active = false;

	size = sizeof(EVENT_TRACE_PROPERTIES) + (wcslen(Token->Name) + 1) * sizeof(Token->Name[0]);
	buffer = malloc(size);

	if (buffer == NULL)
	{
		return FALSE;
	}

	RtlZeroMemory(buffer, size);

	properties = (PEVENT_TRACE_PROPERTIES)buffer;

	properties->Wnode.BufferSize = size;
	properties->Wnode.Guid = zepta;
	properties->Wnode.ClientContext = 1;
	properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
	properties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
	properties->FlushTimer = 1;
	properties->LogFileNameOffset = 0;
	properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
	properties->EnableFlags = 0;

	status = StartTraceW(
		&Token->SetupHandle,
		Token->Name,
		properties);

	if (status != ERROR_SUCCESS &&
		status != ERROR_ALREADY_EXISTS)
	{
		printf("Error on StartTraceW. Code: %x\n", status);
		return FALSE;
	}
	
	status = EnableTraceEx2(
		Token->SetupHandle,
		&ucx,
		EVENT_CONTROL_CODE_ENABLE_PROVIDER,
		TRACE_LEVEL_VERBOSE,
		0,
		0,
		0,
		NULL);

	if (status != ERROR_SUCCESS && status != ERROR_INVALID_PARAMETER)
	{
		printf("Error on EnableTraceEx2. Code: %x\n", status);
		return FALSE;
	}

	return TRUE;
}

BOOL
StartEtwSession(
	__inout PETW_TOKEN Token
)
{
	ULONG status;
	EVENT_TRACE_LOGFILE trace;

	if (Token == NULL ||
		Token->SetupHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE ||
		Token->Active == TRUE)
	{
		return FALSE;
	}

	ZeroMemory(&trace, sizeof(EVENT_TRACE_LOGFILE));

	trace.LoggerName = (LPWSTR)Token->Name;
	trace.LogFileName = NULL;
	trace.EventRecordCallback = (PEVENT_RECORD_CALLBACK)Token->CallbackRecord;
	trace.BufferCallback = (PEVENT_TRACE_BUFFER_CALLBACKW)Token->CallbackBuffer;
	trace.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD | PROCESS_TRACE_MODE_REAL_TIME;

	Token->Handle = OpenTraceW(&trace);

	if (INVALID_PROCESSTRACE_HANDLE == Token->Handle)
	{
		printf("Error on OpenTraceW. Code: %x\n", GetLastError());
		CloseTrace(Token->Handle);
		return FALSE;
	}

	Token->Active = TRUE;

	status = ProcessTrace(
		&Token->Handle,
		1,
		NULL,
		NULL);

	if (status != ERROR_SUCCESS && status != ERROR_CANCELLED)
	{
		printf("Error on ProcessTrace. Code: %x\n", status);
		CloseTrace(Token->Handle);
		return FALSE;
	}

	return TRUE;
}

BOOL
StartEtwSessionAsync(
	__inout PETW_TOKEN Token
)
{
	Token->Thread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)StartEtwSession,
		Token,
		0,
		NULL);

	return Token->Thread ? TRUE : FALSE;
}

BOOL
StopEtwSession(
	__inout PETW_TOKEN Token
)
{
	if (Token == NULL ||
		Token->Active == FALSE ||
		Token->Handle == INVALID_PROCESSTRACE_HANDLE)
	{
		return FALSE;
	}

	CloseTrace(Token->Handle);

	if (Token->Thread)
	{
		WaitForSingleObject(Token->Thread, INFINITE);
	}

	return TRUE;
}

PTRACE_EVENT_INFO
GetEventData(
	__in PEVENT_RECORD Event,
	__inout PDWORD Osize
)
{
	ULONG status;
	ULONG size = 0;
	PCHAR buffer;

	if (Event == NULL)
	{
		return NULL;
	}

	status = TdhGetEventInformation(
		Event,
		0,
		NULL,
		NULL,
		&size);

	if (status == ERROR_INSUFFICIENT_BUFFER)
	{
		buffer = (PCHAR)calloc(size, sizeof(PCHAR));

		status = TdhGetEventInformation(
			Event,
			0,
			NULL,
			(PTRACE_EVENT_INFO)buffer,
			&size);

		if (status != ERROR_SUCCESS)
		{
			return NULL;
		}
	}

	*Osize = size;
	return (PTRACE_EVENT_INFO)buffer;
}

//
// Not my proudest moment.
//
LPCWSTR
GetPropertyData(
	__in PTRACE_EVENT_INFO Trace,
	__in PEVENT_RECORD Event,
	__in EVENT_PROPERTY_INFO Prop,
	__in UINT Index
)
{
	ULONG size = 5;
	PVOID buffer = 0;
	ULONG used = 0;
	USHORT sizeFormat;
	ULONG status;

	INT i = 0;
	while (TRUE)
	{
		if (buffer)
		{
			free(buffer);
			buffer = malloc(size);
		}
		else
		{
			buffer = malloc(size);
		}

		status = TdhFormatProperty(
			Trace,
			NULL,
			8,
			Prop.nonStructType.InType,
			Prop.nonStructType.OutType,
			4,
			(USHORT)((PCHAR)Event->UserData + used + Event->UserDataLength) - Event->UserDataLength,
			(PBYTE)((PBYTE)Event->UserData + used),
			&size,
			(PWCHAR)buffer,
			&sizeFormat);

		if (status == ERROR_INSUFFICIENT_BUFFER)
		{
			size = size * 2;
			continue;
		}

		if (status == ERROR_SUCCESS)
		{
			i = i + 1;
			if (i == Index)
			{
				return (LPCWSTR)buffer;
			}
			used += sizeFormat;

		}

		if (used > Event->UserDataLength)
		{
			free(buffer);
			break;
		}

		if (status != ERROR_SUCCESS)
		{
			free(buffer);
			break;
		}
	}
	return NULL;
}
