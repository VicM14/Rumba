#include <windows.h>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "MainScreen.h"
#include "Theme.h"

#define ID_TIMER_RUMBA_ANIM 2

const double MAX_CASA_ANCHO = 2000.0;
const double MAX_CASA_ALTO = 2000.0;

// --- FUNCIONES AUXILIARES DE DIBUJO ---
static void localDibujarRectRedondeado(HDC hdc, int x, int y, int w, int h, COLORREF colorFondo, COLORREF colorBorde, int radio = 8) {
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

static void localDibujarTexto(HDC hdc, const std::string& texto, int x, int y, COLORREF color, int tamano, bool negrita) {
    HFONT hFont = CreateFontA(tamano, 0, 0, 0, negrita ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, x, y, texto.c_str(), (int)texto.length());
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

static void localDibujarTextoCentrado(HDC hdc, const std::string& texto, int x, int y, int ancho, COLORREF color, int tamano, bool negrita) {
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

static std::string localFormatearNumero(double num, int decimales = 0) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << num;
    std::string s = oss.str();
    size_t puntoPos = s.find('.');
    std::string parteEntera = (puntoPos != std::string::npos) ? s.substr(0, puntoPos) : s;
    std::string parteDecimal = (puntoPos != std::string::npos) ? s.substr(puntoPos) : "";
    std::string resultado;
    int count = 0;
    for (int i = (int)parteEntera.length() - 1; i >= 0; i--) {
        if (count > 0 && count % 3 == 0) resultado = "." + resultado;
        resultado = parteEntera[i] + resultado;
        count++;
    }
    return resultado + parteDecimal;
}


MainScreen::MainScreen(HINSTANCE hInstance, HWND hWnd)
    : m_hInstance(hInstance), m_hWnd(hWnd), m_isCalculating(false), m_animActive(false), m_isPaused(false),
    m_hBtnCalculate(NULL), m_hBtnClear(NULL), m_hBtnSelectAll(NULL), m_hBtnDeselectAll(NULL), m_hBtnPause(NULL),
    m_hComboRobotType(NULL), m_robotActual("dummy", 0)
{
    m_zonas.emplace_back("Zona 1", 500.0, 150.0);
    m_zonas.emplace_back("Zona 2", 480.0, 101.0);
    m_zonas.emplace_back("Zona 3", 309.0, 480.0);
    m_zonas.emplace_back("Zona 4", 90.0, 220.0);

    m_robots.emplace_back("Estandar", 1000.0);
    m_robots.emplace_back("Turbo", 1500.0);
    m_robots.emplace_back("Ecologico", 750.0);
    m_robotActual = m_robots[0];

    for (int i = 0; i < 4; i++) {
        m_rumbas[i].active = false;
        m_rumbas[i].xPct = 0.5; m_rumbas[i].yPct = 0.5;
        m_rumbas[i].dxPct = 0.02; m_rumbas[i].dyPct = 0.03;
    }
}

MainScreen::~MainScreen() {
    KillTimer(m_hWnd, ID_TIMER_RUMBA_ANIM);
    if (m_hBtnPause) DestroyWindow(m_hBtnPause);
}

void MainScreen::HandlePaint(HDC hdc, const RECT& clientRect) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBmp = CreateCompatibleBitmap(hdc, clientRect.right, clientRect.bottom);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    DrawUI(hdcMem, clientRect);

    BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
}

bool MainScreen::ReadInputsAndValidate(bool showMessages) {
    if (m_hEditsLargo.size() != m_zonas.size() || m_hEditsAncho.size() != m_zonas.size()) {
        return false;
    }

    for (size_t i = 0; i < m_zonas.size(); ++i) {
        if (!m_hEditsLargo[i] || !m_hEditsAncho[i]) return false;

        char buffer[64];
        GetWindowTextA(m_hEditsLargo[i], buffer, 64);
        m_zonas[i].largo = atof(buffer);

        GetWindowTextA(m_hEditsAncho[i], buffer, 64);
        m_zonas[i].ancho = atof(buffer);
    }

    double maxCol1Largo = (m_zonas[0].largo > m_zonas[1].largo) ? m_zonas[0].largo : m_zonas[1].largo;
    double maxCol2Largo = (m_zonas[2].largo > m_zonas[3].largo) ? m_zonas[2].largo : m_zonas[3].largo;
    double totalLargo = maxCol1Largo + maxCol2Largo;

    double maxFila1Ancho = (m_zonas[0].ancho > m_zonas[2].ancho) ? m_zonas[0].ancho : m_zonas[2].ancho;
    double maxFila2Ancho = (m_zonas[1].ancho > m_zonas[3].ancho) ? m_zonas[1].ancho : m_zonas[3].ancho;
    double totalAncho = maxFila1Ancho + maxFila2Ancho;

    if (totalLargo > MAX_CASA_ANCHO || totalAncho > MAX_CASA_ALTO) {
        if (showMessages) {
            MessageBoxA(m_hWnd, "¡Dimensiones demasiado grandes para el plano!", "Límite Excedido", MB_OK | MB_ICONERROR);
        }
        return false;
    }

    return true;
}

void MainScreen::HandleCommand(WPARAM wParam, LPARAM lParam) {
    int cmd = LOWORD(wParam);

    if (HIWORD(wParam) == EN_CHANGE) {
        ReadInputsAndValidate(false);
        InvalidateRect(m_hWnd, NULL, FALSE);
        return;
    }

    if (cmd == ID_BTN_CALCULATE) {
        OnCalculateClick();
    }
    else if (cmd == ID_BTN_CLEAR) {
        OnClearClick();
    }
    else if (cmd == ID_BTN_SELECT_ALL) {
        OnSelectAllClick(true);
    }
    else if (cmd == ID_BTN_DESELECT_ALL) {
        OnSelectAllClick(false);
    }
    else if (cmd == ID_BTN_QUIT) {
        PostMessageA(m_hWnd, WM_CLOSE, 0, 0);
    }
    else if (cmd == ID_BTN_PAUSE) { // LÓGICA DE PAUSA
        m_isPaused = !m_isPaused;
        if (m_isPaused) SetWindowTextA(m_hBtnPause, "REANUDAR");
        else SetWindowTextA(m_hBtnPause, "PAUSAR");
    }
    else if (cmd == ID_COMBO_ROBOT_TYPE && HIWORD(wParam) == CBN_SELCHANGE) {
        UpdateRobotFromSelection();
    }
    else {
        for (size_t i = 0; i < m_hChecks.size(); ++i) {
            if ((HWND)lParam == m_hChecks[i]) {
                m_zonas[i].isSelected = (IsDlgButtonChecked(m_hWnd, (int)(ID_CHECK_ZONA1 + i)) == BST_CHECKED);
                InvalidateRect(m_hWnd, NULL, FALSE);
                break;
            }
        }
    }
}

void MainScreen::HandleTimer(WPARAM wParam) {
    if (wParam == ID_TIMER_RUMBA_ANIM && m_animActive && !m_isPaused) {
        for (int i = 0; i < 4; i++) {
            if (!m_rumbas[i].active) continue;

            m_rumbas[i].xPct += m_rumbas[i].dxPct;
            m_rumbas[i].yPct += m_rumbas[i].dyPct;

            if (m_rumbas[i].xPct <= 0.05 || m_rumbas[i].xPct >= 0.95) m_rumbas[i].dxPct = -m_rumbas[i].dxPct;
            if (m_rumbas[i].yPct <= 0.05 || m_rumbas[i].yPct >= 0.95) m_rumbas[i].dyPct = -m_rumbas[i].dyPct;
        }
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

void MainScreen::OnCalculateClick() {
    if (m_isCalculating) return;

    if (!ReadInputsAndValidate(true)) return;

    std::vector<Zona> zonasSeleccionadas;
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        m_rumbas[i].active = m_zonas[i].isSelected;
        m_rumbas[i].xPct = 0.1 + (rand() % 80) / 100.0;
        m_rumbas[i].yPct = 0.1 + (rand() % 80) / 100.0;
        m_rumbas[i].trail.clear();

        if (m_zonas[i].isSelected) {
            zonasSeleccionadas.push_back(m_zonas[i]);
        }
    }

    if (zonasSeleccionadas.empty()) {
        MessageBoxA(m_hWnd, "Selecciona al menos una zona para limpiar.", "Advertencia", MB_OK | MB_ICONWARNING);
        return;
    }

    UpdateRobotFromSelection();

    m_isCalculating = true;
    m_animActive = true; // Empieza la rumba
    EnableWindow(m_hBtnCalculate, FALSE);
    SetWindowTextA(m_hBtnCalculate, "Calculando...");

    SetTimer(m_hWnd, ID_TIMER_RUMBA_ANIM, 25, NULL); // Reloj encendido

    std::thread t([this, zonasSeleccionadas]() {
        ResultadoCalculo res = m_robotActual.ejecutarCalculoDistribuido(zonasSeleccionadas);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_resultado = res;
        }

        m_isCalculating = false;
        EnableWindow(m_hBtnCalculate, TRUE);
        SetWindowTextA(m_hBtnCalculate, "CALCULAR");
        InvalidateRect(m_hWnd, NULL, FALSE);
        });
    t.detach();
}

void MainScreen::OnClearClick() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resultado = ResultadoCalculo();
    m_isCalculating = false;
    m_animActive = false; // Matar rumbas
    m_isPaused = false;
    SetWindowTextA(m_hBtnPause, "PAUSAR");

    for (int i = 0; i < 4; i++) {
        m_rumbas[i].active = false;
        m_rumbas[i].trail.clear();
    }
    KillTimer(m_hWnd, ID_TIMER_RUMBA_ANIM);
    InvalidateRect(m_hWnd, NULL, FALSE);
}

void MainScreen::OnSelectAllClick(bool select) {
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        m_zonas[i].isSelected = select;
        CheckDlgButton(m_hWnd, (int)(ID_CHECK_ZONA1 + i), select ? BST_CHECKED : BST_UNCHECKED);
    }
    InvalidateRect(m_hWnd, NULL, FALSE);
}

void MainScreen::UpdateRobotFromSelection() {
    if (!m_hComboRobotType) return;
    int index = (int)SendMessageA(m_hComboRobotType, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR && index < (int)m_robots.size()) {
        m_robotActual = m_robots[index];
    }
}

void MainScreen::CreateControls() {
    HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hFontBold = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");

    int yPos = 155; // BAJADO A 155 PARA QUE NO TOQUE LOS TÍTULOS
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        HWND hCheck = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            35, yPos + 3, 20, 20, m_hWnd, (HMENU)(ID_CHECK_ZONA1 + i), m_hInstance, NULL);
        CheckDlgButton(m_hWnd, (int)(ID_CHECK_ZONA1 + i), m_zonas[i].isSelected ? BST_CHECKED : BST_UNCHECKED);
        m_hChecks.push_back(hCheck);

        std::string largoStr = std::to_string((int)m_zonas[i].largo);
        HWND hEditLargo = CreateWindowExA(0, "EDIT", largoStr.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            160, yPos, 80, 24, m_hWnd, (HMENU)(ID_EDIT_LARGO_ZONA1 + i * 10), m_hInstance, NULL);
        SendMessageA(hEditLargo, WM_SETFONT, (WPARAM)hFont, TRUE);
        m_hEditsLargo.push_back(hEditLargo);

        std::string anchoStr = std::to_string((int)m_zonas[i].ancho);
        HWND hEditAncho = CreateWindowExA(0, "EDIT", anchoStr.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            260, yPos, 80, 24, m_hWnd, (HMENU)(ID_EDIT_ANCHO_ZONA1 + i * 10 + 1), m_hInstance, NULL);
        SendMessageA(hEditAncho, WM_SETFONT, (WPARAM)hFont, TRUE);
        m_hEditsAncho.push_back(hEditAncho);

        yPos += 40;
    }

    m_hBtnSelectAll = CreateWindowA("BUTTON", "Sel. Todo", WS_CHILD | WS_VISIBLE, 35, 330, 90, 25, m_hWnd, (HMENU)ID_BTN_SELECT_ALL, m_hInstance, NULL);
    m_hBtnDeselectAll = CreateWindowA("BUTTON", "Des. Todo", WS_CHILD | WS_VISIBLE, 130, 330, 90, 25, m_hWnd, (HMENU)ID_BTN_DESELECT_ALL, m_hInstance, NULL);
    SendMessageA(m_hBtnSelectAll, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(m_hBtnDeselectAll, WM_SETFONT, (WPARAM)hFont, TRUE);

    // FILA DE ACCIONES DISTRIBUIDAS (ORDENADAS)
    m_hComboRobotType = CreateWindowA("COMBOBOX", "", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 35, 383, 200, 150, m_hWnd, (HMENU)ID_COMBO_ROBOT_TYPE, m_hInstance, NULL);
    for (size_t i = 0; i < m_robots.size(); ++i) {
        std::string item = m_robots[i].nombre + " (" + std::to_string((int)m_robots[i].tasaLimpiezaCm2ps) + " cm2/s)";
        SendMessageA(m_hComboRobotType, CB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
    SendMessageA(m_hComboRobotType, CB_SETCURSEL, 0, 0);
    SendMessageA(m_hComboRobotType, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_hBtnCalculate = CreateWindowA("BUTTON", "CALCULAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 250, 380, 130, 32, m_hWnd, (HMENU)ID_BTN_CALCULATE, m_hInstance, NULL);
    m_hBtnClear = CreateWindowA("BUTTON", "LIMPIAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 400, 380, 130, 32, m_hWnd, (HMENU)ID_BTN_CLEAR, m_hInstance, NULL);
    m_hBtnPause = CreateWindowA("BUTTON", "PAUSAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 550, 380, 130, 32, m_hWnd, (HMENU)ID_BTN_PAUSE, m_hInstance, NULL);
    HWND hBtnQuit = CreateWindowA("BUTTON", "SALIR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 755, 380, 130, 32, m_hWnd, (HMENU)ID_BTN_QUIT, m_hInstance, NULL);

    SendMessageA(m_hBtnCalculate, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessageA(m_hBtnClear, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessageA(m_hBtnPause, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessageA(hBtnQuit, WM_SETFONT, (WPARAM)hFontBold, TRUE);
}

void MainScreen::DrawUI(HDC hdc, const RECT& clientRect) {
    int width = clientRect.right;
    int height = clientRect.bottom;

    HBRUSH hBrFondo = CreateSolidBrush(COLOR_FONDO);
    FillRect(hdc, &clientRect, hBrFondo);
    DeleteObject(hBrFondo);

    localDibujarTextoCentrado(hdc, "ROBOT ASPIRADOR - Simulador de Limpieza", 0, 15, width, COLOR_TITULO, 24, true);

    DrawZonesPanel(hdc);
    DrawRoomPlan(hdc, width / 2 + 10, 80, width / 2 - 30, 280);
    DrawResultsPanel(hdc, 20, 440, width - 40, 200);

    localDibujarTextoCentrado(hdc, "v2.0 | C++ Multithreading | Win32 GUI", 0, height - 25, width, COLOR_TEXTO_DIM, 11, false);
}

void MainScreen::DrawZonesPanel(HDC hdc) {
    int panelX = 20, panelY = 80, panelW = 920 / 2 - 30, panelH = 280;

    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTexto(hdc, "DEFINIR AREAS Y DIMENSIONES", panelX + 15, panelY + 12, COLOR_TITULO, 15, true);

    int tabX = panelX + 15;
    int tabY = panelY + 45; // ARRIBA DEL TODO PARA QUE NO SE TRASLAPEN
    localDibujarTexto(hdc, "Zona", tabX + 35, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Largo (cm)", tabX + 130, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Ancho (cm)", tabX + 230, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Area (cm2)", tabX + 330, tabY, COLOR_TEXTO_DIM, 13, true);

    COLORREF coloresZ[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };

    int labelY = 160; // COORDINA EXACTAMENTE CON CAJAS EDIT
    for (size_t i = 0; i < m_zonas.size(); i++) {
        localDibujarTexto(hdc, m_zonas[i].nombre, tabX + 35, labelY, coloresZ[i], 14, false);

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_resultado.calculado) {
            bool found = false;
            for (size_t j = 0; j < m_resultado.zonasCalculadas.size(); ++j) {
                if (m_resultado.zonasCalculadas[j].nombre == m_zonas[i].nombre) {
                    localDibujarTexto(hdc, localFormatearNumero(m_resultado.zonasCalculadas[j].area), tabX + 330, labelY, COLOR_ACENTO, 14, true);
                    found = true;
                    break;
                }
            }
            if (!found) localDibujarTexto(hdc, "---", tabX + 330, labelY, COLOR_TEXTO_DIM, 14, false);
        }
        else {
            localDibujarTexto(hdc, "---", tabX + 330, labelY, COLOR_TEXTO_DIM, 14, false);
        }
        labelY += 40;
    }
}


void MainScreen::DrawResultsPanel(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTexto(hdc, "RESULTADOS DEL CALCULO", panelX + 15, panelY + 12, COLOR_TITULO, 15, true);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isCalculating) {
        localDibujarTextoCentrado(hdc, "Calculando areas en hilos distribuidos concurrentes...", panelX, panelY + 90, panelW, COLOR_ACENTO2, 18, true);
    }
    else if (m_resultado.calculado) {
        int resY = panelY + 45;
        int col1X = panelX + 30, col2X = panelX + panelW / 3 + 10, col3X = panelX + 2 * panelW / 3 + 10;
        int cardW = panelW / 3 - 30;

        localDibujarRectRedondeado(hdc, col1X - 10, resY - 5, cardW, 90, COLOR_PANEL_ZONA, COLOR_BORDE, 8);
        localDibujarTextoCentrado(hdc, "Superficie Total", col1X - 10, resY, cardW, COLOR_TEXTO_DIM, 12, false);
        std::string supStr = localFormatearNumero(m_resultado.superficieTotal) + " cm2";
        localDibujarTextoCentrado(hdc, supStr, col1X - 10, resY + 22, cardW, COLOR_ACENTO, 20, true);
        double supM2 = m_resultado.superficieTotal / 10000.0;
        std::ostringstream ossM2; ossM2 << std::fixed << std::setprecision(2) << supM2 << " m2";
        localDibujarTextoCentrado(hdc, ossM2.str(), col1X - 10, resY + 50, cardW, COLOR_TEXTO_DIM, 13, false);

        localDibujarRectRedondeado(hdc, col2X - 10, resY - 5, cardW, 90, COLOR_PANEL_ZONA, COLOR_BORDE, 8);
        localDibujarTextoCentrado(hdc, "Tasa de Limpieza", col2X - 10, resY, cardW, COLOR_TEXTO_DIM, 12, false);
        std::string tasaStr = localFormatearNumero(m_resultado.tasaLimpieza) + " cm2/s";
        localDibujarTextoCentrado(hdc, tasaStr, col2X - 10, resY + 22, cardW, COLOR_ACENTO2, 20, true);
        localDibujarTextoCentrado(hdc, m_robotActual.nombre, col2X - 10, resY + 50, cardW, COLOR_TEXTO_DIM, 13, false);

        localDibujarRectRedondeado(hdc, col3X - 10, resY - 5, cardW, 90, COLOR_PANEL_ZONA, COLOR_BORDE, 8);
        localDibujarTextoCentrado(hdc, "Tiempo Estimado", col3X - 10, resY, cardW, COLOR_TEXTO_DIM, 12, false);
        std::ostringstream ossTiempo;
        if (m_resultado.tiempoMinutos >= 60) ossTiempo << std::fixed << std::setprecision(1) << m_resultado.tiempoHoras << " hrs";
        else ossTiempo << std::fixed << std::setprecision(1) << m_resultado.tiempoMinutos << " min";
        localDibujarTextoCentrado(hdc, ossTiempo.str(), col3X - 10, resY + 22, cardW, COLOR_ACENTO3, 20, true);
        std::ostringstream ossSeg; ossSeg << std::fixed << std::setprecision(1) << m_resultado.tiempoSegundos << " segundos";
        localDibujarTextoCentrado(hdc, ossSeg.str(), col3X - 10, resY + 50, cardW, COLOR_TEXTO_DIM, 13, false);

    }
    else {
        localDibujarTextoCentrado(hdc, "Selecciona las areas a limpiar, tipo de robot y presiona 'Calcular'.", panelX, panelY + 90, panelW, COLOR_TEXTO_DIM, 16, false);
    }
}

void MainScreen::DrawRoomPlan(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTextoCentrado(hdc, "Vista de Planta", panelX, panelY + 10, panelW, COLOR_TITULO, 16, true);

    int drawX = panelX + 15, drawY = panelY + 40, drawW = panelW - 30, drawH = panelH - 60;

    double col1Largo = (m_zonas[0].largo > m_zonas[1].largo) ? m_zonas[0].largo : m_zonas[1].largo;
    double col2Largo = (m_zonas[2].largo > m_zonas[3].largo) ? m_zonas[2].largo : m_zonas[3].largo;
    double totalAnchoX = col1Largo + col2Largo;

    double fila1Ancho = (m_zonas[0].ancho > m_zonas[2].ancho) ? m_zonas[0].ancho : m_zonas[2].ancho;
    double fila2Ancho = (m_zonas[1].ancho > m_zonas[3].ancho) ? m_zonas[1].ancho : m_zonas[3].ancho;
    double totalAltoY = fila1Ancho + fila2Ancho;

    double escalaX = (double)drawW / totalAnchoX;
    double escalaY = (double)drawH / totalAltoY;
    double escala = (escalaX < escalaY) ? escalaX : escalaY;

    int offsetX = drawX + (int)((drawW - totalAnchoX * escala) / 2);
    int offsetY = drawY + (int)((drawH - totalAltoY * escala) / 2);

    COLORREF coloresZ[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };

    struct ZonaLayout { int x, y, w, h; };
    ZonaLayout layouts[4];

    layouts[0] = { offsetX, offsetY, (int)(m_zonas[0].largo * escala), (int)(m_zonas[0].ancho * escala) };
    layouts[1] = { offsetX, offsetY + (int)(fila1Ancho * escala), (int)(m_zonas[1].largo * escala), (int)(m_zonas[1].ancho * escala) };
    layouts[2] = { offsetX + (int)(col1Largo * escala), offsetY, (int)(m_zonas[2].largo * escala), (int)(m_zonas[2].ancho * escala) };
    layouts[3] = { offsetX + (int)(col1Largo * escala), offsetY + (int)(fila1Ancho * escala), (int)(m_zonas[3].largo * escala), (int)(m_zonas[3].ancho * escala) };

    for (int i = 0; i < 4; i++) {
        if (!m_zonas[i].isSelected) continue;

        HBRUSH hBrZ = CreateSolidBrush(RGB(50, 50, 70));
        HPEN hPenZ = CreatePen(PS_SOLID, 2, coloresZ[i]);
        SelectObject(hdc, hBrZ);
        SelectObject(hdc, hPenZ);

        Rectangle(hdc, layouts[i].x, layouts[i].y, layouts[i].x + layouts[i].w, layouts[i].y + layouts[i].h);

        DeleteObject(hBrZ);
        DeleteObject(hPenZ);

        localDibujarTextoCentrado(hdc, "Z" + std::to_string(i + 1), layouts[i].x, layouts[i].y + layouts[i].h / 2 - 8, layouts[i].w, coloresZ[i], 14, true);
    }

    if (m_animActive) { // ESTELAS SEGUIDAS DIBUJADAS POR ENCIMA
        for (int i = 0; i < 4; i++) {
            if (!m_rumbas[i].active) continue;

            int ballX = layouts[i].x + (int)(m_rumbas[i].xPct * layouts[i].w);
            int ballY = layouts[i].y + (int)(m_rumbas[i].yPct * layouts[i].h);

            m_rumbas[i].trail.push_back({ ballX, ballY });

            HPEN hPenTrail = CreatePen(PS_SOLID, 8, coloresZ[i]);
            SelectObject(hdc, hPenTrail);
            if (m_rumbas[i].trail.size() > 1) {
                MoveToEx(hdc, m_rumbas[i].trail[0].x, m_rumbas[i].trail[0].y, NULL);
                for (size_t t = 1; t < m_rumbas[i].trail.size(); ++t) {
                    LineTo(hdc, m_rumbas[i].trail[t].x, m_rumbas[i].trail[t].y);
                }
            }
            DeleteObject(hPenTrail);
        }
    }

    if (m_animActive) { // BOLITAS DIBUJADAS POR ENCIMA
        for (int i = 0; i < 4; i++) {
            if (!m_rumbas[i].active) continue;

            int ballX = layouts[i].x + (int)(m_rumbas[i].xPct * layouts[i].w);
            int ballY = layouts[i].y + (int)(m_rumbas[i].yPct * layouts[i].h);

            HBRUSH hBrushRumba = CreateSolidBrush(coloresZ[i]);
            HPEN hPenRumba = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
            SelectObject(hdc, hBrushRumba);
            SelectObject(hdc, hPenRumba);

            Ellipse(hdc, ballX - 6, ballY - 6, ballX + 6, ballY + 6);

            DeleteObject(hBrushRumba);
            DeleteObject(hPenRumba);
        }
    }
}