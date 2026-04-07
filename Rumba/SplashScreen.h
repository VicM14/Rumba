#pragma once
#include <windows.h>
#include <functional> // <-- ¡ESTO ES VITAL PARA std::function!

class SplashScreen {
public:
    SplashScreen(HINSTANCE hInstance, HWND hWnd, std::function<void()> onComplete);
    ~SplashScreen();

    void HandlePaint(HDC hdc, const RECT& clientRect);
    void HandleTimer(WPARAM wParam);
    void HandleCommand(WPARAM wParam);
    void CreateControls();

private:
    HINSTANCE m_hInstance;
    HWND m_hWnd;
    HWND m_hBtnStart;
    int m_loadingProgress;
    std::function<void()> m_onCompleteCallback;
};