#define WIN32_LEAN_AND_MEAN

#include <shellscalingapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include "resource.h"

static bool RUNNING;
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixels = 4;

static void RenderWeirdGradient(int BlueOffset, int GreenOffset) {
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  int Pitch = Width * BytesPerPixels;
  uint8_t *Row = (uint8_t *)BitmapMemory;
  for (int Y = 0; Y < BitmapHeight; ++Y) {
    uint32_t *Pixels = (uint32_t *)Row;
    for (int X = 0; X < BitmapWidth; ++X) {
      uint8_t Blue = (X + BlueOffset);
      uint8_t Green = (Y + GreenOffset);

      *Pixels++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

static void Win32ResizeDIBSection(int Width, int Height) {
  // TODO: Maybe don't free first, free after then free first if it fails.

  if (BitmapMemory) {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB; // Uncompressed.

  int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixels;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

static void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect, int Width, int Height) {
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;

  StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0, WindowWidth, WindowHeight, BitmapMemory,
                &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
  LRESULT CallbackResult;
  switch (Message) {
  // WM_COMMAND - application menu, keybindings, etc.
  case WM_COMMAND:
    switch (LOWORD(WParam)) {

    case ID_QUIT:
      PostQuitMessage(0);
      break;
    }
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

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

    RECT ClientRect;
    GetClientRect(WindowHandle, &ClientRect);
    Win32UpdateWindow(DeviceContext, &ClientRect, Width, Height);
    EndPaint(WindowHandle, &Paint);

  case WM_SIZE: {
    RECT ClientRect;
    GetClientRect(WindowHandle, &ClientRect);
    int Width = ClientRect.right - ClientRect.left;
    int Height = ClientRect.bottom - ClientRect.top;

    Win32ResizeDIBSection(Width, Height);
  } break;

  default:
    CallbackResult = DefWindowProc(WindowHandle, Message, WParam, LParam);
    break;
  }
  return CallbackResult;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Command, int ShowCode) {
  WNDCLASS WindowClass = {0};
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  WindowClass.hIcon = LoadIcon(Instance, MAKEINTRESOURCE(ID_ICON));
  WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW); // This would later be set to IDC_IBEAM when editing text.
  WindowClass.lpszMenuName = NULL;
  WindowClass.lpszClassName = "AsakeWindowClass";

  if (!RegisterClass(&WindowClass)) {
    // ShowLastError(NULL, "Error: Window Registration Failed");
    return 1;
  }

  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  HWND WindowHandle = CreateWindow(WindowClass.lpszClassName, "Asake", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 575, 74, 1080,
                                   720, NULL, NULL, Instance, NULL);

  if (WindowHandle == NULL) {
    // ShowLastError(WindowHandle, "Error: Window Creation Failed");
    return 1;
  }

  ShowWindow(WindowHandle, ShowCode);
  UpdateWindow(WindowHandle);

  // Load accelerator table
  HACCEL AcceleratorHandle = LoadAccelerators(Instance, "ID_MENU_ACCELERATOR");

  int XOffset = 0;
  int YOffset = 0;

  RUNNING = true;
  while (RUNNING) {
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
      if (Message.message == WM_QUIT) {
        RUNNING = false;
      }
      if (!TranslateAccelerator(WindowHandle, AcceleratorHandle, &Message)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }
    }
    RenderWeirdGradient(XOffset, YOffset);
    HDC DeviceContext = GetDC(WindowHandle);
    RECT ClientRect;
    GetClientRect(WindowHandle, &ClientRect);
    int WindowWidth = ClientRect.right - ClientRect.left;
    int WindowHeight = ClientRect.bottom - ClientRect.top;
    Win32UpdateWindow(DeviceContext, &ClientRect, WindowWidth, WindowHeight);
    ReleaseDC(WindowHandle, DeviceContext);

    ++XOffset;
    YOffset += 2;
  }
  return (0);
}
