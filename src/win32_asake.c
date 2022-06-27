#define WIN32_LEAN_AND_MEAN

#include <shellscalingapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <windows.h>

#include "resource.h"

void ShowLastError(HWND hwnd, LPCSTR error_msg) {
  LPVOID lpMsgBuf;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                    GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)) {
    MessageBox(hwnd, (LPCSTR)lpMsgBuf, error_msg, MB_OK | MB_ICONINFORMATION);
    LocalFree(lpMsgBuf);
  }
}

LRESULT CALLBACK WndProc(HWND WindowHandle, UINT msg, WPARAM wParam, LPARAM lParam) {
  static HINSTANCE hInstance;
  switch (msg) {
  // WM_COMMAND - application menu
  case WM_COMMAND:
    switch (LOWORD(wParam)) {

    case ID_QUIT:
      PostQuitMessage(0);
      break;
    }
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

    // NOTE: May want to ask the user if they would really like to exit the
    // program in the event they have unsaved work.
  case WM_CLOSE:
    DestroyWindow(WindowHandle);
    break;

  case WM_PAINT:
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;
    static DWORD Operation = WHITENESS;
    PatBlt(DeviceContext, X, Y, Width, Height, Operation);
    if(Operation == WHITENESS){
      Operation = BLACKNESS;
    }else {
      Operation = WHITENESS;
    }
    EndPaint(WindowHandle, &Paint);

  default:
    return DefWindowProc(WindowHandle, msg, wParam, lParam);
    break;
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Command, int ShowCode) {
  WNDCLASS WindowClass = {0};
  HACCEL haccel;
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = WndProc;
  WindowClass.hInstance = Instance;
  WindowClass.hIcon = LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON));
  WindowClass.hCursor = NULL;
  WindowClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  WindowClass.lpszMenuName = NULL;
  WindowClass.lpszClassName = "AsakeClass";

  if (!RegisterClass(&WindowClass)) {
    ShowLastError(NULL, "Error: Window Registration Failed");
    return 1;
  }

  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  HWND WindowHandle = CreateWindow(WindowClass.lpszClassName, "Asake", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 575, 74,
                                   1080, 720, NULL, NULL, Instance, NULL);

  if (WindowHandle == NULL) {
    ShowLastError(WindowHandle, "Error: Window Creation Failed");
    return 1;
  }

  ShowWindow(WindowHandle, ShowCode);
  UpdateWindow(WindowHandle);

  // Load accelerator table
  haccel = LoadAccelerators(Instance, "IDR_MENU_ACCELERATOR");

  MSG msg = {0};
  while (GetMessage(&msg, NULL, 0, 0)) {
    if (!TranslateAccelerator(WindowHandle, haccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return (int)(msg.wParam);
}
