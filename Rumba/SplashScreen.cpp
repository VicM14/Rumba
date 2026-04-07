#include <windows.h>
#include <string>

#include "SplashScreen.h"
#include "Theme.h"

// Funciones de dibujo internas para que no choquen con las de MainScreen
static void splashDibujarRectRedondeado(HDC hdc, int x, int y, int w, int h, COLORREF colorFondo, COLORREF colorBorde, int radio = 8) {
    HBRUSH hBrush = CreateSolidBrush(colorFondo);
    HPEN hPen = CreatePen(PS_SOLID, 1, colorBorde);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    RoundRect(hdc, x, y, x + w, y + h, radio, radio);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

static void splashDibujarTextoCentrado(HDC hdc, const std::string& texto, int x, int y, int ancho, COLORREF color, int tamano, bool negrita) {
    HFONT hFont = CreateFontA(tamano, 0, 0, 0, negrita ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    SIZE sz;
    GetTextExtentPoint32A(hdc, texto.c_str(), (int)texto.length(), &sz);
    int tx = x + (ancho - sz.cx) / 2;
    TextOutA(hdc, tx, y, texto.c_str(), (int)texto.length());
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

SplashScreen::SplashScreen(HINSTANCE hInstance, HWND hWnd, std::function<void()> onComplete)
    : m_hInstance(hInstance), m_hWnd(hWnd), m_hBtnStart(NULL), m_loadingProgress(0), m_onCompleteCallback(onComplete)
{
    SetTimer(m_hWnd, ID_TIMER_LOADING, 30, NULL);
}

SplashScreen::~SplashScreen() {
    KillTimer(m_hWnd, ID_TIMER_LOADING);
    if (m_hBtnStart) DestroyWindow(m_hBtnStart);
}

void SplashScreen::CreateControls() {
    m_hBtnStart = CreateWindowA("BUTTON", "INICIAR RUMBA",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        325, 420, 250, 50, m_hWnd, (HMENU)ID_BTN_START, m_hInstance, NULL);

    HFONT hFontBtn = CreateFontA(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    SendMessage(m_hBtnStart, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
    EnableWindow(m_hBtnStart, FALSE); // Bloqueado hasta que cargue al 100%
}

void SplashScreen::HandlePaint(HDC hdc, const RECT& clientRect) {
    int width = clientRect.right;
    int height = clientRect.bottom;

    HBRUSH hBrFondo = CreateSolidBrush(COLOR_FONDO);
    FillRect(hdc, &clientRect, hBrFondo);
    DeleteObject(hBrFondo);

    splashDibujarTextoCentrado(hdc, "Robot Rumba PRO", 0, height / 2 - 120, width, COLOR_TITULO, 48, true);
    splashDibujarTextoCentrado(hdc, "Sistemas Concurrentes y Multihilos", 0, height / 2 - 60, width, COLOR_TEXTO_DIM, 20, false);

    int barX = width / 2 - 150;
    int barY = height / 2 + 20;
    int barW = 300;
    int barH = 20;
    splashDibujarRectRedondeado(hdc, barX, barY, barW, barH, COLOR_PANEL_ZONA, COLOR_BORDE, 10);

    if (m_loadingProgress > 0) {
        int progressW = (barW * m_loadingProgress) / 100;
        splashDibujarRectRedondeado(hdc, barX, barY, progressW, barH, COLOR_PROGRESS_BAR, COLOR_PROGRESS_BAR, 10);
    }

    std::string loadingText = "Cargando componentes... " + std::to_string(m_loadingProgress) + "%";
    splashDibujarTextoCentrado(hdc, loadingText, 0, barY + 30, width, COLOR_TEXTO_DIM, 14, false);
}

void SplashScreen::HandleTimer(WPARAM wParam) {
    if (wParam == ID_TIMER_LOADING) {
        m_loadingProgress += 2; // Sube la barra de carga
        if (m_loadingProgress >= 100) {
            m_loadingProgress = 100;
            KillTimer(m_hWnd, ID_TIMER_LOADING);
            EnableWindow(m_hBtnStart, TRUE); // Habilitar el botón cuando termine
        }
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void SplashScreen::HandleCommand(WPARAM wParam) {
    if (LOWORD(wParam) == ID_BTN_START) {
        if (m_onCompleteCallback) {
            m_onCompleteCallback(); // Cambiar de pantalla
        }
    }
}