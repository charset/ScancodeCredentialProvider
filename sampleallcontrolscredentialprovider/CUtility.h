#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <stdio.h>

#pragma once

class CUtility{
public:
	static void WriteLog(LPWSTR format, ...);
	static CHAR QRContent[128];
	static void GetContent();
};