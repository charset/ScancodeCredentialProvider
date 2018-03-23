#include "CommandWindow.h"

#pragma comment(lib, "ws2_32.lib")

// Custom messages for managing the behavior of the window thread.
#define WM_EXIT_THREAD              WM_USER + 1

const WCHAR c_szClassName[] = L"EventWindow";
const WCHAR c_szConnected[] = L"Connected";
const WCHAR c_szDisconnected[] = L"Disconnected";

SOCKET CCommandWindow::server = 0;

CCommandWindow::CCommandWindow() : _hWnd(NULL), _hInst(NULL), _pCredential(NULL){ }

CCommandWindow::~CCommandWindow() {
    // If we have an active window, we want to post it an exit message.
    if (_hWnd != NULL) {
        PostMessage(_hWnd, WM_EXIT_THREAD, 0, 0);
        _hWnd = NULL;
    }

    // We'll also make sure to release any reference we have to the provider.
    if (_pCredential != NULL) {
        _pCredential->Release();
        _pCredential = NULL;
    }

	WSACleanup();
}

// Performs the work required to spin off our message so we can listen for events.
HRESULT CCommandWindow::Initialize(__in CSampleCredential* pCredential) {
    HRESULT hr = S_OK;

    // Be sure to add a release any existing provider we might have, then add a reference
    // to the provider we're working with now.
    if (_pCredential != NULL) {
        _pCredential->Release();
    }
    _pCredential = pCredential;
    _pCredential->AddRef();
    
    // Create and launch the window thread.
    HANDLE hThread = CreateThread(NULL, 0, _ThreadProc, this, 0, NULL);
	CloseHandle(hThread);

	hThread = CreateThread(NULL, 0, _SocketProc, pCredential, 0, NULL);
	CloseHandle(hThread);

    return hr;
}

//
//  FUNCTION: _MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
HRESULT CCommandWindow::_MyRegisterClass() {
    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc      = _WndProc;
    wcex.hInstance        = _hInst;
    wcex.hIcon            = NULL;
    wcex.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName    = c_szClassName;

    return RegisterClassEx(&wcex) ? S_OK : HRESULT_FROM_WIN32(GetLastError());
}

//
//   FUNCTION: _InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HRESULT CCommandWindow::_InitInstance() {
	HRESULT hr = S_OK;
    // Create our window to receive events.
    // 
    // This dialog is for demonstration purposes only.  It is not recommended to create 
    // dialogs that are visible even before a credential enumerated by this credential 
    // provider is selected.  Additionally, any dialogs that are created by a credential
    // provider should not have a NULL hwndParent, but should be parented to the HWND
    // returned by ICredentialProviderCredentialEvents::OnCreatingWindow.
	_hWnd = CreateWindowEx(WS_EX_TOPMOST, c_szClassName, NULL, WS_POPUP | WS_DLGFRAME,
		(GetSystemMetrics(SM_CXSCREEN) - 208) / 2, 0, 208, 208, NULL, NULL, _hInst, NULL);

	ShowWindow(_hWnd, SW_NORMAL);
	UpdateWindow(_hWnd);

    return hr;
}

// Called from the separate thread to process the next message in the message queue. If
// there are no messages, it'll wait for one.
BOOL CCommandWindow::_ProcessNextMessage() {
    // Grab, translate, and process the message.
    MSG msg;
    GetMessage(&(msg), _hWnd, 0, 0);
    TranslateMessage(&(msg));
    DispatchMessage(&(msg));

    // This section performs some "post-processing" of the message. It's easier to do these
    // things here because we have the handles to the window, its button, and the provider
    // handy.
    switch (msg.message) {
    // Return to the thread loop and let it know to exit.
    case WM_EXIT_THREAD:
		return FALSE;
    }
    return TRUE;
}

// Manages window messages on the window thread.
LRESULT CALLBACK CCommandWindow::_WndProc(__in HWND hWnd, __in UINT message, __in WPARAM wParam, __in LPARAM lParam){
	HDC hDC, hCompatibleDC = NULL;
	static HBITMAP hCompatibleBitmap = NULL;
	PAINTSTRUCT ps;
	zint_symbol* my_symbol = NULL;

    switch (message) {
	case WM_CREATE:
		CUtility::GetContent();
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		hCompatibleDC = CreateCompatibleDC(hDC);
		hCompatibleBitmap = CreateCompatibleBitmap(hCompatibleDC, 200, 200);
		SelectObject(hCompatibleDC, hCompatibleBitmap);

		my_symbol = ZBarcode_Create();
		my_symbol->symbology = BARCODE_QRCODE;
		my_symbol->scale = 2;

		//CHAR str[128]; int len;
		ZBarcode_Encode_and_Buffer(my_symbol, (PUCHAR)CUtility::QRContent, (int)strlen(CUtility::QRContent), 0);

		BITMAPINFO bmpi; ZeroMemory(&bmpi, sizeof(BITMAPINFO));
		bmpi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmpi.bmiHeader.biWidth = my_symbol->bitmap_width;
		bmpi.bmiHeader.biHeight = my_symbol->bitmap_height;
		bmpi.bmiHeader.biPlanes = 1;
		bmpi.bmiHeader.biBitCount = 24;
		bmpi.bmiHeader.biSizeImage = 0;

		SetDIBits(hCompatibleDC, hCompatibleBitmap, 0, my_symbol->bitmap_height, my_symbol->bitmap, &bmpi, DIB_RGB_COLORS);
		BitBlt(hDC, 0, 0, my_symbol->bitmap_width, my_symbol->bitmap_height, hCompatibleDC, 0, 0, SRCCOPY);

		DeleteObject(hCompatibleBitmap);
		DeleteDC(hCompatibleDC);

		ZBarcode_Delete(my_symbol);

		EndPaint(hWnd, &ps);
		//ReleaseDC(hWnd, hDC);
		break;
    // To play it safe, we hide the window when "closed" and post a message telling the 
    // thread to exit.
    case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		PostMessage(hWnd, WM_EXIT_THREAD, 0, 0);
		_CleanupSocket();
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Our thread procedure. We actually do a lot of work here that could be put back on the 
// main thread, such as setting up the window, etc.
DWORD WINAPI CCommandWindow::_ThreadProc(__in LPVOID lpParameter)
{
    CCommandWindow *pCommandWindow = static_cast<CCommandWindow *>(lpParameter);
    if (pCommandWindow == NULL)
    {
        // TODO: What's the best way to raise this error?
        return 0;
    }

    HRESULT hr = S_OK;

    // Create the window.
    pCommandWindow->_hInst = GetModuleHandle(NULL);
    if (pCommandWindow->_hInst != NULL)
    {            
        hr = pCommandWindow->_MyRegisterClass();
        if (SUCCEEDED(hr))
        {
            hr = pCommandWindow->_InitInstance();
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    // ProcessNextMessage will pump our message pump and return false if it comes across
    // a message telling us to exit the thread.
    if (SUCCEEDED(hr))
    {        
        while (pCommandWindow->_ProcessNextMessage()) 
        {
        }
    }
    else
    {
        if (pCommandWindow->_hWnd != NULL)
        {
            pCommandWindow->_hWnd = NULL;
        }
    }

    return 0;
}

BOOL CCommandWindow::_CleanupSocket(){
	closesocket(server);
	WSACleanup();
	return TRUE;
}

DWORD WINAPI CCommandWindow::_SocketProc(__in LPVOID lpParameter){
	CSampleCredential* pCredential = static_cast<CSampleCredential*>(lpParameter);
	WSADATA wsa;

	int err = WSAStartup(MAKEWORD(2, 0), &wsa);
	CUtility::WriteLog(L"WSAStartup:: err=[%d]", err);

	server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN _server;
	_server.sin_addr.S_un.S_addr = INADDR_ANY;
	_server.sin_family = AF_INET;
	_server.sin_port = htons(6000);
	bind(server, (SOCKADDR*)&_server, sizeof(SOCKADDR));
	err = listen(server, 5);

	CUtility::WriteLog(L"Bind & Listen:: err=[%d]", err);

	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
	SOCKET sockets[WSA_MAXIMUM_WAIT_EVENTS];
	int nEvent = 0;

	WSAEVENT event = WSACreateEvent();
	err = WSAEventSelect(server, event, FD_ACCEPT | FD_CLOSE);
	CUtility::WriteLog(L"CreateEvent & Async:: err=[%d]", err);

	events[nEvent] = event;
	sockets[nEvent] = server;
	nEvent++;

	bool running = true;

	while(running) {
		int index = WSAWaitForMultipleEvents(nEvent, events, FALSE, WSA_INFINITE, FALSE);

		if(index == WSA_WAIT_IO_COMPLETION || index == WSA_WAIT_TIMEOUT) {
			CUtility::WriteLog(L"Error occur while waiting for events:: err=[%d]", WSAGetLastError());
			break;
		}

		index -= WSA_WAIT_EVENT_0;
		WSANETWORKEVENTS nwEvents;
		SOCKET sock = sockets[index];
		WSAEnumNetworkEvents(sock, events[index], &nwEvents);

		if(nwEvents.lNetworkEvents & FD_ACCEPT) {
			if(nwEvents.iErrorCode[FD_ACCEPT_BIT] == 0) {
				if(nEvent >= WSA_MAXIMUM_WAIT_EVENTS) {
					CUtility::WriteLog(L"Too many event objects, connection refused:: err=[%d]", nEvent);
					continue;
				}

				SOCKADDR_IN addr;
				int len = sizeof(SOCKADDR_IN);
				SOCKET client = accept(sock, (SOCKADDR*)&addr, &len);
				if(client != INVALID_SOCKET) {
					TCHAR ipaddress[128]; InetNtop(AF_INET, &(addr.sin_addr), ipaddress, sizeof(ipaddress));
					CUtility::WriteLog(L"A client connection accepted:: [%S:%d]", ipaddress, ntohs(addr.sin_port));
					WSAEVENT newEvent = WSACreateEvent();
					WSAEventSelect(client, newEvent, FD_READ | FD_CLOSE | FD_WRITE);
					events[nEvent] = newEvent;
					sockets[nEvent] = client;
					nEvent++;
				}
			}
		} else if(nwEvents.lNetworkEvents & FD_READ) {
			if(nwEvents.iErrorCode[FD_READ_BIT] == 0) {
				char buf[128]; ZeroMemory(buf, 128);
				int nRecv = recv(sock, buf, 128, 0);
				if(nRecv > 0) {
					CUtility::WriteLog(L"Got Message And Feedback:: err=[%S]", buf);
					send(sock, buf, nRecv, 0);
					pCredential->SetStringValue(SFI_EDIT_TEXT, L"CHARSET");
					pCredential->SetStringValue(SFI_PASSWORD, L"1");
					closesocket(sock);
					break;
				}
			}
		} else if(nwEvents.lNetworkEvents & FD_CLOSE) {
			WSACloseEvent(events[index]);
			err = closesocket(sockets[index]);
			CUtility::WriteLog(L"Disconnect:: err=[%d]", err);
			for(int j = index;j < nEvent - 1;j++) {
				events[j] = events[j + 1];
				sockets[j] = sockets[j + 1];
			}
			nEvent--;
		} else if(nwEvents.lNetworkEvents & FD_WRITE) {
			CUtility::WriteLog(L"Allow Write:: err=[%d]", 0);
		}
	}//end of while

	closesocket(server);
	CUtility::WriteLog(L"%S:: Close Socket", __FUNCTION__);
	return 0;
}