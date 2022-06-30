#define WIN32_LEAN_AND_MEAN

#include <shellscalingapi.h>
#include <stdint.h>
#include <windows.h>

#include "resource.h"

static bool RUNNING;
static BITMAPINFO BitmapInfo;
static void *BitmapMemory;
static int BitmapWidth;
static int BitmapHeight;
static int BytesPerPixels = 4;

typedef uint8_t uint8;

void ShowLastError(HWND Handle, LPCSTR ErrorMessage) {
  LPVOID lpMsgBuf;
  if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                    GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL)) {
    MessageBox(Handle, (LPCSTR)lpMsgBuf, ErrorMessage, MB_OK | MB_ICONINFORMATION);
    LocalFree(lpMsgBuf);
  }
}

static void RenderWeirdGradient(int XOffset, int YOffset) {
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  uint8_t *Row = (uint8_t *)BitmapMemory;
  int Pitch = Width * BytesPerPixels;

  for (int Y = 0; Y < BitmapHeight; ++Y) {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < BitmapWidth; ++X) {

      uint8_t Green = (Y + YOffset);
      uint8_t Blue = (X + XOffset);

      *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

// DIB stands for Device Independent Bitmap
// Resize the DIB section or create one if it as never been made.
static void Win32ResizeDIBSection(int Width, int Height) {
  // TODO(tijani): Bulletproof this.
  // Maybe don't free first, free after, then free first if that fails.
  if (BitmapMemory) {
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  // BITMAPINFO BitmapInfo;
  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32; // using 32 because cpus are byte sized aligned.
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BytesPerPixels = 4;
  int BitmapMemorySize = (Width * Height) * BytesPerPixels;
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  RenderWeirdGradient(139, 0);
}

static void Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height) {
  int WindowWidth = WindowRect->right - WindowRect->left;
  int WindowHeight = WindowRect->bottom - WindowRect->top;

  StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, X, Y, WindowWidth, WindowHeight, BitmapMemory,
                &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WndProc(HWND WindowHandle, UINT msg, WPARAM wParam, LPARAM lParam) {
  static HINSTANCE hInstance;
  switch (msg) {
    //  WM_COMMAND - application menu
  case WM_COMMAND:
    switch (LOWORD(wParam)) {

    case ID_QUIT:
      RUNNING = false;
      PostQuitMessage(WM_CLOSE);
      break;
    }
    break;

  case WM_SIZE: {
    RECT ClientRect;
    GetClientRect(WindowHandle, &ClientRect);
    int Width = ClientRect.right - ClientRect.left;
    int Height = ClientRect.bottom - ClientRect.top;

    Win32ResizeDIBSection(Width, Height);
  } break;

  case WM_DESTROY:
    RUNNING = false;
    PostQuitMessage(WM_CLOSE);
    break;

    // NOTE: May want to ask the user if they would really like to exit the
    // program in the event they have unsaved work.
  case WM_CLOSE:
    RUNNING = false;
    DestroyWindow(WindowHandle);
    break;

  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
    int X = Paint.rcPaint.left;
    int Y = Paint.rcPaint.top;
    int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
    int Width = Paint.rcPaint.right - Paint.rcPaint.left;

    RECT ClientRect;
    GetClientRect(WindowHandle, &ClientRect);
    Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
    EndPaint(WindowHandle, &Paint);
  } break;
  default:
    return DefWindowProc(WindowHandle, msg, wParam, lParam);
    break;
  }
  return 0;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Command, int ShowCode) {
  WNDCLASS WindowClass = {0};
  WindowClass.style = CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = WndProc;
  WindowClass.hInstance = Instance;
  WindowClass.hIcon = LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON));
  WindowClass.hCursor = NULL;
  WindowClass.lpszMenuName = NULL;
  WindowClass.lpszClassName = "AsakeClass";

  if (!RegisterClass(&WindowClass)) {
    ShowLastError(NULL, "Error: Window Registration Failed");
    return 1;
  }

  SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

  HWND WindowHandle = CreateWindow(WindowClass.lpszClassName, "Asake", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 575, 74, 1080,
                                   720, NULL, NULL, Instance, NULL);

  if (WindowHandle == NULL) {
    ShowLastError(WindowHandle, "Error: Window Creation Failed");
    return 1;
  }

  ShowWindow(WindowHandle, ShowCode);
  UpdateWindow(WindowHandle);

  // Load accelerator table
  HACCEL AcceleratorHandle = LoadAccelerators(Instance, "IDR_MENU_ACCELERATOR");

  RUNNING = true;

  int XOffset = 0;
  int YOffset = 0;

  while (RUNNING) {
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
      if (!TranslateAccelerator(WindowHandle, AcceleratorHandle, &Message)) {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }

      RenderWeirdGradient(XOffset, YOffset);

      HDC DeviceContext = GetDC(WindowHandle);
      RECT WindowRect;
      GetClientRect(WindowHandle, &WindowRect);
      int WindowWidth = WindowRect.right - WindowRect.left;
      int WindowHeight = WindowRect.bottom - WindowRect.top;

      Win32UpdateWindow(DeviceContext, &WindowRect, 0, 0, WindowWidth, WindowHeight);
      ReleaseDC(WindowHandle, DeviceContext);
      --XOffset;
      YOffset -= 2;
    }
  }
  return 0;
}
