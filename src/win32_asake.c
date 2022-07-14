#define WIN32_LEAN_AND_MEAN

#include <shellscalingapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#include "resource.h"

typedef struct {
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int BytesPerPixels;
  int Pitch;
} OffscreenBuffer;

typedef struct {
  int Width;
  int Height;
} Win32Dimension;

/* typdef  DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState); */
/* DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration); */

Win32Dimension GetWindowDimension(HWND WindowHandle) {
  Win32Dimension result;

  RECT ClientRect;
  GetClientRect(WindowHandle, &ClientRect);
  result.Width = ClientRect.right - ClientRect.left;
  result.Height = ClientRect.bottom - ClientRect.top;

  return result;
}

static bool RUNNING;
static OffscreenBuffer GlobalBackBuffer;

static void RenderWeirdGradient(OffscreenBuffer Buffer, int BlueOffset, int GreenOffset) {
  int Width = Buffer.Width;
  int Height = Buffer.Height;

  uint8_t *Row = (uint8_t *)Buffer.Memory;
  for (int Y = 0; Y < Buffer.Height; ++Y) {
    uint32_t *Pixels = (uint32_t *)Row;
    for (int X = 0; X < Buffer.Width; ++X) {
      uint8_t Blue = (X + BlueOffset);
      uint8_t Green = (Y + GreenOffset);

      *Pixels++ = ((Green << 8) | Blue);
    }
    Row += Buffer.Pitch;
  }
}

static void Win32ResizeDIBSection(OffscreenBuffer *Buffer, int Width, int Height) {
  // TODO: Maybe don't free first, free after then free first if it fails.

  if (Buffer->Memory) {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;

  int BytesPerPixels = 4;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB; // Uncompressed.

  int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixels;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = Width * BytesPerPixels;
}

static void Win32CopyBufferToWindow(OffscreenBuffer Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight) {
  /* StretchDIBITS(
     DeviceContext,

     [Destination, in this case the Window]
     X-Coordinate of the destination
     Y-Coordinate of the destination
     Width of the destination
     Height of the destination

     [Source, which is the backbuffer]
     X-coordinate of the source
     Y-coordinate of the source
     Width of the source
     Height of the source

     lpBits - pointer to the image bits stored in array of bytes
     lpbmi - pointer to BITMAPINFO struct
     bmiColors
  )
  */
  // TODO(tijani): Aspect ratio correction.

  StretchDIBits(DeviceContext, 0, 0, WindowWidth, WindowHeight, 0, 0, Buffer.Width, Buffer.Height, Buffer.Memory,
                &Buffer.Info, DIB_RGB_COLORS, SRCCOPY);
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

    Win32Dimension Dimension = GetWindowDimension(WindowHandle);
    Win32CopyBufferToWindow(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
    EndPaint(WindowHandle, &Paint);

  case WM_SIZE: {
    //    Win32Dimension Dimension = GetWindowDimension(WindowHandle);

  } break;
  default:
    CallbackResult = DefWindowProc(WindowHandle, Message, WParam, LParam);
    break;
  }
  return CallbackResult;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR Command, int ShowCode) {
  WNDCLASS WindowClass = {0};
  // NOTE()
  WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
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
  // Allow the backbuffer to stretch equally.
  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

  if (WindowHandle == NULL) {
    // ShowLastError(WindowHandle, "Error: Window Creation Failed");
    return 1;
  }

  ShowWindow(WindowHandle, ShowCode);
  UpdateWindow(WindowHandle);

  // Load accelerator table
  HACCEL AcceleratorHandle = LoadAccelerators(Instance, "ID_MENU_ACCELERATOR");

  /* NOTE: Since CS_OWNDC is specified, then get a single device context and use it for the
     lifetime of the program because it is not being shared with anyone.
   */
  HDC DeviceContext = GetDC(WindowHandle);

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
    RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

    Win32Dimension Dimension = GetWindowDimension(WindowHandle);
    Win32CopyBufferToWindow(GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);

    ++XOffset;
    YOffset += 10;
  }
  return (0);
}
