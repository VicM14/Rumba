#include <windows.h>
#include <memory> // Para std::unique_ptr

// Rutas directas sin carpetas
#include "SplashScreen.h"
#include "MainScreen.h"
#include "Theme.h"

enum class ScreenState { SPLASH, MAIN };
static ScreenState g_currentScreen = ScreenState::SPLASH;

// Usamos unique_ptr para cargar y descargar pantallas dinámicamente
static std::unique_ptr<SplashScreen> g_splashScreen;
static std::unique_ptr<MainScreen> g_mainScreen;

static HWND g_hWnd = NULL;
static HINSTANCE g_hInstance = NULL;

void SwitchToMainScreen();

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        g_splashScreen = std::make_unique<SplashScreen>(g_hInstance, hWnd, SwitchToMainScreen);
        g_splashScreen->CreateControls();
        return 0;

    case WM_COMMAND:
        if (g_currentScreen == ScreenState::SPLASH && g_splashScreen) {
            g_splashScreen->HandleCommand(wParam);
        }
        else if (g_currentScreen == ScreenState::MAIN && g_mainScreen) {
            g_mainScreen->HandleCommand(wParam, lParam);
        }
        return 0;

    case WM_TIMER:
        if (g_currentScreen == ScreenState::SPLASH && g_splashScreen) {
            g_splashScreen->HandleTimer(wParam);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (g_currentScreen == ScreenState::SPLASH && g_splashScreen) {
            g_splashScreen->HandlePaint(hdc, ps.rcPaint);
        }
        else if (g_currentScreen == ScreenState::MAIN && g_mainScreen) {
            g_mainScreen->HandlePaint(hdc, ps.rcPaint);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXTO);
        SetBkColor(hdcStatic, COLOR_PANEL);
        return (LRESULT)CreateSolidBrush(COLOR_PANEL);
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

void SwitchToMainScreen() {
    g_splashScreen.reset(); // Destruye la pantalla de carga anterior
    g_currentScreen = ScreenState::MAIN;
    g_mainScreen = std::make_unique<MainScreen>(g_hInstance, g_hWnd);
    g_mainScreen->CreateControls();
    InvalidateRect(g_hWnd, NULL, TRUE); // Redibuja la ventana con la pantalla nueva
}

// Reemplaza el final de main.cpp con esto:

// WinMain con anotaciones de seguridad requeridas por Visual Studio
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    g_hInstance = hInstance;

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_FONDO);
    wc.lpszClassName = "RobotAspiradorProClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        return 1;
    }

    int winW = 920, winH = 600;
    int posX = (GetSystemMetrics(SM_CXSCREEN) - winW) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - winH) / 2;

    g_hWnd = CreateWindowExA(0, "RobotAspiradorProClass", "Robot Rumba PRO v2.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH, NULL, NULL, hInstance, NULL);

    if (!g_hWnd) return 1;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

// ESTO SOLUCIONA EL "EXTERNO SIN RESOLVER":
// Si Visual Studio cree que es una consola, redirigimos main() hacia WinMain()
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}