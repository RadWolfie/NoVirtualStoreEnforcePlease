// main.c : Defines the entry point for the application.
//

#include "stdafx.h"
#include "main.h"

typedef enum CONVERT_STEP {
    CONVERT_STEP_READY_COMPLETE = -1,
    CONVERT_STEP_READY = 0,
    CONVERT_STEP_1 = 1,
    CONVERT_STEP_INVALID_FILE = -2,
    CONVERT_STEP_MANIFEST_INCLUDED = -3,
    CONVERT_STEP_NOPERM = -4,
    CONVERT_STEP_2 = 2,
    CONVERT_STEP_APP_RESOURCE_FAIL = -5,
    CONVERT_STEP_3 = 3,
    CONVERT_STEP_UNABLE_MOVE_FILE = -6,
    CONVERT_STEP_UNABLE_COPY_FILE = -7,
    CONVERT_STEP_4 = 4,
    CONVERT_STEP_UNABLE_WRITE_FILE = -8,
    CONVERT_STEP_UNABLE_ADD_RES = -9,
    CONVERT_STEP_UNABLE_SAVE_RES = -10,
} CONVERT_STEP;

#define MAX_LOADSTRING 100

#define MAX_SIZE 500
CHAR * mbMsg = "Program is designed to process one file at time, please drop one file only.";
CHAR * mbCaption = "One at a Time";

CHAR * strReady = "Drag and drop a file here.";
CHAR * strReadyCompleted = "Completed! - Drag and drop a file here.";
CHAR * strProcessing1 = "Verifying exe file...";
CHAR * strErrorInvalidFile = "Error - Invalid file, please try again.";
CHAR * strErrorNoPerm = "Error - You don't have full rights to this file.";
CHAR * strErrorManifestIncluded = "Error - Manifest file is already included.";
CHAR * strProcessing2 = "Getting resource file...";
CHAR * strErrorAppResourceMissing = "Error - Embedded resource file is missing or is not functional properly.";
CHAR * strProcessing3 = "Creating backup of original exe file...";
CHAR * strErrorUnableMoveFile = "Error - Unable to move exe file.";
CHAR * strErrorUnableCopyFile = "Error - Unable to copy exe file.";
CHAR * strProcessing4 = "Updating file, please do not close this program during this process...";
CHAR * strErrorUnableWriteFile = "Error - Unable to write to new exe file.";
CHAR * strErrorUnableAddRes = "Error - Unable to add manifest into new exe file.";
CHAR * strErrorUnableSaveRes = "Error - Unable to save manifest into new exe file.";

CHAR * strDefault = "Unknown step has occurred, contact developer.";
CHAR * strFileName = "File:";
CHAR * strFilePath = NULL;
CRITICAL_SECTION m_cs;
HANDLE hThread, hThreadHandle;
RECT rectScreen, rectTopLeft, rectTopRight, rectBottom;

CONVERT_STEP conStep = CONVERT_STEP_READY;

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name


// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_NOVIRTUALSTOREENFORCEPLEASE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOVIRTUALSTOREENFORCEPLEASE));
    InitializeCriticalSection(&m_cs);

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DeleteCriticalSection(&m_cs);
    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style            = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra        = 0;
    wcex.cbWndExtra        = 0;
    wcex.hInstance        = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MYICON));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground    = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_NOVIRTUALSTOREENFORCEPLEASE);
    wcex.lpszClassName    = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MYICON));

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_EX_ACCEPTFILES,
      CW_USEDEFAULT, 0, 350, 200, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   GetClientRect(hWnd, &rectScreen);

   rectTopLeft = rectScreen;
   rectTopLeft.bottom /= 2;

   rectTopRight = rectTopLeft;

   rectTopRight.left += 30;

   rectTopLeft.right = rectTopLeft.left + 30;

   rectBottom = rectScreen;
   rectBottom.top += rectBottom.bottom / 2;

   DragAcceptFiles(hWnd, TRUE);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    HGLOBAL hResLoad;   // handle to loaded resource
    HRSRC hRes;         // handle/ptr. to res. info. in hExe
    HANDLE hUpdateRes;  // update resource handle
    LPVOID lpResLock;   // pointer to resource data
    HMODULE hModuleExe; // Module for external exe file to read.
    BOOL result;
#define IDD_MANIFEST_RESOURCE   1
    char hExeFileNameTemp[MAX_SIZE] = "";
    workerInfo* worker = (workerInfo*)lpParam;

    conStep = CONVERT_STEP_1;
    strFilePath = worker->fileStr;
    RedrawWindow(worker->hWnd, NULL, NULL, RDW_INVALIDATE);

    //Step 1: Read exe file if a manifest is already included.
    hModuleExe = LoadLibraryExA(strFilePath, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hModuleExe == NULL) {
        conStep = CONVERT_STEP_INVALID_FILE;
        goto skipToFinalStep;
    }
    // Locate the dialog box resource in the .EXE file.
    hRes = FindResourceW(hModuleExe, MAKEINTRESOURCE(IDD_MANIFEST_RESOURCE), RT_MANIFEST);
    if (hRes != NULL) {
        conStep = CONVERT_STEP_MANIFEST_INCLUDED;
        goto skipToFreeLibary;
    }
    unsigned int err;
    if (!isDirFileFullPermission(L"\\", &err)) {
        conStep = CONVERT_STEP_NOPERM;
    skipToFreeLibary:
        FreeLibrary(hModuleExe);
        goto skipToFinalStep;
    }
    FreeLibrary(hModuleExe);

    //Step 2: Get manifest resource file from inside exe file.
    conStep = CONVERT_STEP_2;
    RedrawWindow(worker->hWnd, NULL, NULL, RDW_INVALIDATE);
    hRes = FindResourceW(GetCurrentModule(), MAKEINTRESOURCE(IDD_MANIFEST_RESOURCE), RT_MANIFEST);
    // Load the dialog box into global memory.
    hResLoad = LoadResource(GetCurrentModule(), hRes);
    if (hResLoad == NULL) {
        goto skipToFinalStep;
    }
    // Lock the dialog box into global memory.
    lpResLock = LockResource(hResLoad);
    if (lpResLock == NULL) {
        conStep = CONVERT_STEP_APP_RESOURCE_FAIL;
        skipToFreeResource:
        FreeResource(hResLoad);
        goto skipToFinalStep;
    }

    //Step 3: Create backup of original exe of user request to add manifest in it.
    conStep = CONVERT_STEP_3;
    RedrawWindow(worker->hWnd, NULL, NULL, RDW_INVALIDATE);
    strcatA(hExeFileNameTemp, MAX_SIZE, strFilePath);
    strcatA(hExeFileNameTemp, MAX_SIZE, "_backup");
    if (!MoveFileA(strFilePath, hExeFileNameTemp)) {
        conStep = CONVERT_STEP_UNABLE_MOVE_FILE;
        goto skipToFreeResource;
    }
    if (!CopyFileA(hExeFileNameTemp, strFilePath, FALSE)) {
        conStep = CONVERT_STEP_UNABLE_COPY_FILE;
        goto skipToFreeResource;
    }

    //Step 4: Add manifest to the exe file.
    // Open the file to which you want to add the dialog box resource.
    conStep = CONVERT_STEP_4;
    RedrawWindow(worker->hWnd, NULL, NULL, RDW_INVALIDATE);
    hUpdateRes = BeginUpdateResourceA(strFilePath, FALSE);
    if (hUpdateRes == NULL) {
        conStep = CONVERT_STEP_UNABLE_WRITE_FILE;
        goto skipToFreeResource;
    }
    // Add the dialog box resource to the update list.
    result = UpdateResource(hUpdateRes,    // update resource handle
        RT_MANIFEST,                         // change dialog box resource
        MAKEINTRESOURCE(IDD_MANIFEST_RESOURCE),         // dialog box id
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),  // neutral language
        lpResLock,                         // ptr to resource info
        SizeofResource(GetCurrentModule(), hRes));       // size of resource info
    if (result == FALSE) {
        conStep = CONVERT_STEP_UNABLE_ADD_RES;
        goto skipToFreeResource;
    }
    // Write changes to exe file and then close it.
    if (!EndUpdateResource(hUpdateRes, FALSE)) {
        conStep = CONVERT_STEP_UNABLE_SAVE_RES;
        goto skipToFreeResource;
    }
    FreeResource(hResLoad);

    //Final step
    conStep = CONVERT_STEP_READY_COMPLETE;
skipToFinalStep:
    RedrawWindow(worker->hWnd, NULL, NULL, RDW_INVALIDATE);
    free(worker);
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;
    CHAR * strTemp;
    HDROP hDropInfo;

    switch (message) {
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        SetBkColor(hdc, RGB(0, 0, 0));
        FillRect(hdc, &rectScreen, GetStockBrush(BLACK_BRUSH));
        SetTextColor(hdc, RGB(255, 255, 255));
        DrawTextA(hdc, strFileName, strlen(strFileName), &rectTopLeft, DT_RIGHT | DT_SINGLELINE);
        if (strFilePath)
            DrawTextA(hdc, strFilePath, strlen(strFilePath), &rectTopRight, DT_LEFT | DT_EDITCONTROL | DT_WORDBREAK | DT_PATH_ELLIPSIS);
        switch (conStep) {
            case CONVERT_STEP_READY_COMPLETE:
                strTemp = strReadyCompleted;
                break;
            case CONVERT_STEP_READY:
                strTemp = strReady;
                break;
            //Step 1 processing
            case CONVERT_STEP_1:
                strTemp = strProcessing1;
                break;
            case CONVERT_STEP_INVALID_FILE:
                strTemp = strErrorInvalidFile;
                break;
            case CONVERT_STEP_MANIFEST_INCLUDED:
                strTemp = strErrorManifestIncluded;
                break;
            case CONVERT_STEP_NOPERM:
                strTemp = strErrorNoPerm;
                break;
            //Step 2 processing
            case CONVERT_STEP_2:
                strTemp = strProcessing2;
                break;
            case CONVERT_STEP_APP_RESOURCE_FAIL:
                strTemp = strErrorAppResourceMissing;
                break;
            //Step 3 processing
            case CONVERT_STEP_3:
                strTemp = strProcessing3;
                break;
            case CONVERT_STEP_UNABLE_MOVE_FILE:
                strTemp = strErrorUnableMoveFile;
                break;
            case CONVERT_STEP_UNABLE_COPY_FILE:
                strTemp = strErrorUnableCopyFile;
                break;
            //Step 4 processing
            case CONVERT_STEP_4:
                strTemp = strProcessing4;
                break;
            case CONVERT_STEP_UNABLE_WRITE_FILE:
                strTemp = strErrorUnableWriteFile;
                break;
            case CONVERT_STEP_UNABLE_ADD_RES:
                strTemp = strErrorUnableAddRes;
                break;
            case CONVERT_STEP_UNABLE_SAVE_RES:
                strTemp = strErrorUnableSaveRes;
                break;
            default:
                strTemp = strDefault;
                break;
        }
        DrawTextA(hdc, strTemp, strlen(strTemp), &rectBottom, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        if (strFilePath) free(strFilePath);
        PostQuitMessage(0);
        break;
    case WM_DROPFILES:
        hDropInfo = (HDROP)wParam;
        if (hThreadHandle) {
            DWORD dwWaitResult = WaitForSingleObject(hThreadHandle, 0);

            switch (dwWaitResult) {
                case STATUS_WAIT_0:
                case STATUS_ABANDONED_WAIT_0:
                    CloseHandle(hThreadHandle);
                    hThreadHandle = 0;
                    break;
                default:
                userTryMultiFiles:
                    MessageBoxA(hWnd, mbMsg, mbCaption, MB_OK | MB_ICONWARNING);
                    goto doNotProcessNewFile;
            }
        }
        UINT numFiles = DragQueryFileA(hDropInfo, -1, NULL, 0);
        if (numFiles > 1) {
            goto userTryMultiFiles;
        }
        EnterCriticalSection(&m_cs);
        UINT buffsize = MAX_SIZE;
        CHAR * buf = malloc(MAX_SIZE);
        workerInfo* worker = malloc(sizeof(workerInfo));
        if (strFilePath) {
            free(strFilePath);
            strFilePath = NULL;
        }
        DragQueryFileA(hDropInfo, 0, buf, buffsize);
        worker->fileStr = buf;
        worker->hWnd = hWnd;
        hThreadHandle = CreateThread(NULL, 0, WorkerThread, worker, 0, (LPDWORD)&hThread);
        LeaveCriticalSection(&m_cs);
    doNotProcessNewFile:
        //This is required to be at bottom for less coding.
        DragFinish(hDropInfo);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
