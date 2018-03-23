#include "CUtility.h"

#pragma comment(lib, "IPHLPAPI.lib")

const WCHAR szLogFile[] = L"C:\\Users\\CHARSET\\Desktop\\log.txt";

CHAR CUtility::QRContent[128] = { 0 };
void CUtility::WriteLog(LPWSTR pFormat, ...){
	WCHAR buffer[128], prefix[16];
	SYSTEMTIME st;

	GetLocalTime(&st);
	HANDLE hFile = CreateFile(szLogFile, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwWrite = swprintf_s(prefix, L"%02d:%02d:%02d.%03d  ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	DWORD dwWrote = 0;
	WriteFile(hFile, prefix, dwWrite * 2, &dwWrote, NULL);

	va_list args;
	va_start(args, pFormat);
	dwWrite = vswprintf_s(buffer, pFormat, args);
	wcscat_s(buffer, L"\r\n");
	WriteFile(hFile, buffer, (dwWrite + 1) * 2, &dwWrote, NULL);
	CloseHandle(hFile);
}

void CUtility::GetContent(){
	PIP_ADAPTER_ADDRESSES pAddresses = NULL, pCurrent = NULL;
	ULONG ulSize = 0, ulCode = 0, ulFlags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST, ulFamily = AF_INET;
	CHAR lpBuffer[24] = { 0 };
	HANDLE hHeap = GetProcessHeap();

	WriteLog(L"Begin Enumerate Adapter Address", NULL);
	if((ulCode = GetAdaptersAddresses(ulFamily, ulFlags, NULL, pAddresses, &ulSize)) == ERROR_BUFFER_OVERFLOW){
		pAddresses = (PIP_ADAPTER_ADDRESSES)HeapAlloc(hHeap, 0, (SIZE_T)ulSize);
		WriteLog(L"Re-allocate %ld -> %p", ulSize, pAddresses);
	}else{
		WriteLog(L"Failed: %ld", ulCode);
		return;
	}

	GetAdaptersAddresses(ulFamily, ulFlags, NULL, pAddresses, &ulSize);

	pCurrent = pAddresses; ZeroMemory(QRContent, sizeof(QRContent)); 
	while(pCurrent){
		if(pCurrent->PhysicalAddressLength > 0){
			WriteLog(L"MAC-ADD:%02X-%02X-%02X-%02X-%02X-%2X",
				(int)pCurrent->PhysicalAddress[0],
				(int)pCurrent->PhysicalAddress[1],
				(int)pCurrent->PhysicalAddress[2],
				(int)pCurrent->PhysicalAddress[3],
				(int)pCurrent->PhysicalAddress[4],
				(int)pCurrent->PhysicalAddress[5]);
			wsprintfA(lpBuffer, "MAC*{%02X-%02X-%02X-%02X-%02X-%2X}",
				(int)pCurrent->PhysicalAddress[0],
				(int)pCurrent->PhysicalAddress[1],
				(int)pCurrent->PhysicalAddress[2],
				(int)pCurrent->PhysicalAddress[3],
				(int)pCurrent->PhysicalAddress[4],
				(int)pCurrent->PhysicalAddress[5]);
			strcat_s(QRContent, lpBuffer);

			PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrent->FirstUnicastAddress;
			int index = 0;
			strcat_s(QRContent, "ADDR/*");
			while(pUnicast){
				sockaddr_in* _1 = (sockaddr_in*)pUnicast->Address.lpSockaddr;
				IN_ADDR _2 = _1->sin_addr;
				ULONG _3 = _2.S_un.S_addr; PUCHAR q = (PUCHAR)&_3;
				WriteLog(L"[%d]::0x[%02x][%02x][%02x][%02x], => %d.%d.%d.%d", index++,
					q[3], q[2], q[1], q[0], q[0], q[1], q[2], q[3]);
				if(_3 != 0x0100007F){
					wsprintfA(lpBuffer, "%02x%02x%02x%02x", q[0], q[1], q[2], q[3]);
					strcat_s(QRContent, lpBuffer);
				}
				pUnicast = pUnicast->Next;
			}
			strcat_s(QRContent, "*/");
		}

		pCurrent = pCurrent->Next;
	}

	SYSTEMTIME st; GetLocalTime(&st);
	wsprintfA(lpBuffer, "TIME/*GMT+08:00 %04d-%02d-%02dT%02d:%02d:%02d.%03dZ*/",
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	strcat_s(QRContent, lpBuffer);

	HeapFree(hHeap, 0, pAddresses);
	CloseHandle(hHeap);
	WriteLog(L"End Enumerate Adapters", NULL);
}
