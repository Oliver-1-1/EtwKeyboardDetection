#pragma once
#include <Windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <stdbool.h>
#include <stdio.h>
#include <tdh.h>
#include <wchar.h>
#include <stdlib.h> 
#pragma comment(lib, "tdh.lib")

typedef struct _ETW_TOKEN
{
	LPCWSTR Name;
	TRACEHANDLE SetupHandle;
	TRACEHANDLE Handle;
	PVOID CallbackRecord;
	PVOID CallbackBuffer;
	HANDLE Thread;
	BOOL Active;
} ETW_TOKEN, * PETW_TOKEN;

//
// Etw session
//

BOOL
SetupEtwSession(
	__in CONST LPCWSTR Name,
	__in CONST PVOID CallbackRecord,
	__in CONST PVOID CallbackBuffer,
	__inout PETW_TOKEN Token
);

BOOL
StartEtwSession(
	__inout PETW_TOKEN Token
);

BOOL
StartEtwSessionAsync(
	__inout PETW_TOKEN Token
);

BOOL
StopEtwSession(
	__inout PETW_TOKEN Token
);

//
// Etw event parse
//

PTRACE_EVENT_INFO
GetEventData(
	__in PEVENT_RECORD Event,
	__inout PDWORD Osize
);

LPCWSTR
GetPropertyData(
	__in PTRACE_EVENT_INFO Trace,
	__in PEVENT_RECORD Event,
	__in EVENT_PROPERTY_INFO Prop,
	__in UINT Index
);

