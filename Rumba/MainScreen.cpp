#include <windows.h>
#include <sstream>
#include <iomanip>

#include "MainScreen.h"
#include "Theme.h"

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


// --- IMPLEMENTACIÓN DE MAIN SCREEN ---

MainScreen::MainScreen(HINSTANCE hInstance, HWND hWnd)
    : m_hInstance(hInstance), m_hWnd(hWnd), m_robotActual("dummy", 0), m_isCalculating(false)
{
    m_zonas.emplace_back("Zona 1", 500.0, 150.0);
    m_zonas.emplace_back("Zona 2", 480.0, 101.0);
    m_zonas.emplace_back("Zona 3", 309.0, 480.0);
    m_zonas.emplace_back("Zona 4", 90.0, 220.0);

    m_robots.emplace_back("Estandar", 1000.0);
    m_robots.emplace_back("Turbo", 1500.0);
    m_robots.emplace_back("Ecologico", 750.0);
    m_robotActual = m_robots[0];
}

MainScreen::~MainScreen() {
    DestroyWindow(m_hBtnCalculate);
    DestroyWindow(m_hBtnClear);
    DestroyWindow(m_hComboRobotType);
    DestroyWindow(m_hBtnSelectAll);
    DestroyWindow(m_hBtnDeselectAll);
    for (size_t i = 0; i < m_hEditsLargo.size(); ++i) DestroyWindow(m_hEditsLargo[i]);
    for (size_t i = 0; i < m_hEditsAncho.size(); ++i) DestroyWindow(m_hEditsAncho[i]);
    for (size_t i = 0; i < m_hChecks.size(); ++i) DestroyWindow(m_hChecks[i]);
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

void MainScreen::HandleCommand(WPARAM wParam, LPARAM lParam) {
    int cmd = LOWORD(wParam);

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
    else if (cmd == ID_COMBO_ROBOT_TYPE && HIWORD(wParam) == CBN_SELCHANGE) {
        UpdateRobotFromSelection();
    }
    else {
        for (size_t i = 0; i < m_hChecks.size(); ++i) {
            if ((HWND)lParam == m_hChecks[i]) {
                m_zonas[i].isSelected = (IsDlgButtonChecked(m_hWnd, (int)(ID_CHECK_ZONA1 + i)) == BST_CHECKED);
                InvalidateRect(m_hWnd, NULL, TRUE);
                break;
            }
        }
    }
}

void MainScreen::OnCalculateClick() {
    if (m_isCalculating) return;

    std::vector<Zona> zonasSeleccionadas;
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        m_zonas[i].isSelected = (IsDlgButtonChecked(m_hWnd, (int)(ID_CHECK_ZONA1 + i)) == BST_CHECKED);

        char buffer[64];
        GetWindowTextA(m_hEditsLargo[i], buffer, 64);
        m_zonas[i].largo = atof(buffer);

        GetWindowTextA(m_hEditsAncho[i], buffer, 64);
        m_zonas[i].ancho = atof(buffer);

        if (m_zonas[i].isSelected) {
            zonasSeleccionadas.push_back(m_zonas[i]);
        }
    }

    if (zonasSeleccionadas.empty()) {
        MessageBoxA(m_hWnd, "Por favor, seleccione al menos una zona para limpiar.", "Advertencia", MB_OK | MB_ICONWARNING);
        return;
    }

    UpdateRobotFromSelection();

    m_isCalculating = true;
    EnableWindow(m_hBtnCalculate, FALSE);
    SetWindowTextA(m_hBtnCalculate, "Calculando...");

    std::thread t([this, zonasSeleccionadas]() {
        ResultadoCalculo res = m_robotActual.ejecutarCalculoDistribuido(zonasSeleccionadas);

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_resultado = res;
        }

        m_isCalculating = false;
        EnableWindow(m_hBtnCalculate, TRUE);
        SetWindowTextA(m_hBtnCalculate, "CALCULAR");
        InvalidateRect(m_hWnd, NULL, TRUE);
        });
    t.detach();
}

void MainScreen::OnClearClick() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resultado = ResultadoCalculo();
    m_isCalculating = false;
    InvalidateRect(m_hWnd, NULL, TRUE);
}

void MainScreen::OnSelectAllClick(bool select) {
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        m_zonas[i].isSelected = select;
        CheckDlgButton(m_hWnd, (int)(ID_CHECK_ZONA1 + i), select ? BST_CHECKED : BST_UNCHECKED);
    }
    InvalidateRect(m_hWnd, NULL, TRUE);
}

void MainScreen::UpdateRobotFromSelection() {
    int index = (int)SendMessage(m_hComboRobotType, CB_GETCURSEL, 0, 0);
    if (index != CB_ERR) {
        m_robotActual = m_robots[index];
    }
}

void MainScreen::CreateControls() {
    HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
    HFONT hFontBold = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");

    int yPos = 125;
    for (size_t i = 0; i < m_zonas.size(); ++i) {
        HWND hCheck = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            35, yPos, 20, 20, m_hWnd, (HMENU)(ID_CHECK_ZONA1 + i), m_hInstance, NULL);
        CheckDlgButton(m_hWnd, (int)(ID_CHECK_ZONA1 + i), m_zonas[i].isSelected ? BST_CHECKED : BST_UNCHECKED);
        m_hChecks.push_back(hCheck);

        std::string largoStr = std::to_string((int)m_zonas[i].largo);
        HWND hEditLargo = CreateWindowExA(0, "EDIT", largoStr.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            180, yPos - 2, 80, 24, m_hWnd, (HMENU)(ID_EDIT_LARGO_ZONA1 + i * 10), m_hInstance, NULL);
        SendMessage(hEditLargo, WM_SETFONT, (WPARAM)hFont, TRUE);
        m_hEditsLargo.push_back(hEditLargo);

        std::string anchoStr = std::to_string((int)m_zonas[i].ancho);
        HWND hEditAncho = CreateWindowExA(0, "EDIT", anchoStr.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER,
            280, yPos - 2, 80, 24, m_hWnd, (HMENU)(ID_EDIT_ANCHO_ZONA1 + i * 10 + 1), m_hInstance, NULL);
        SendMessage(hEditAncho, WM_SETFONT, (WPARAM)hFont, TRUE);
        m_hEditsAncho.push_back(hEditAncho);

        yPos += 35;
    }

    m_hBtnSelectAll = CreateWindowA("BUTTON", "Sel. Todo", WS_CHILD | WS_VISIBLE, 35, yPos, 90, 24, m_hWnd, (HMENU)ID_BTN_SELECT_ALL, m_hInstance, NULL);
    m_hBtnDeselectAll = CreateWindowA("BUTTON", "Des. Todo", WS_CHILD | WS_VISIBLE, 130, yPos, 90, 24, m_hWnd, (HMENU)ID_BTN_DESELECT_ALL, m_hInstance, NULL);
    SendMessage(m_hBtnSelectAll, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessage(m_hBtnDeselectAll, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_hComboRobotType = CreateWindowA("COMBOBOX", "", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 400, yPos, 200, 150, m_hWnd, (HMENU)ID_COMBO_ROBOT_TYPE, m_hInstance, NULL);
    for (size_t i = 0; i < m_robots.size(); ++i) {
        std::string item = m_robots[i].nombre + " (" + std::to_string((int)m_robots[i].tasaLimpiezaCm2ps) + " cm2/s)";
        SendMessage(m_hComboRobotType, CB_ADDSTRING, 0, (LPARAM)item.c_str());
    }
    SendMessage(m_hComboRobotType, CB_SETCURSEL, 0, 0);
    SendMessage(m_hComboRobotType, WM_SETFONT, (WPARAM)hFont, TRUE);

    m_hBtnCalculate = CreateWindowA("BUTTON", "CALCULAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 620, yPos - 2, 120, 30, m_hWnd, (HMENU)ID_BTN_CALCULATE, m_hInstance, NULL);
    m_hBtnClear = CreateWindowA("BUTTON", "LIMPIAR", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 750, yPos - 2, 120, 30, m_hWnd, (HMENU)ID_BTN_CLEAR, m_hInstance, NULL);
    SendMessage(m_hBtnCalculate, WM_SETFONT, (WPARAM)hFontBold, TRUE);
    SendMessage(m_hBtnClear, WM_SETFONT, (WPARAM)hFontBold, TRUE);
}

void MainScreen::DrawUI(HDC hdc, const RECT& clientRect) {
    int width = clientRect.right;
    int height = clientRect.bottom;

    HBRUSH hBrFondo = CreateSolidBrush(COLOR_FONDO);
    FillRect(hdc, &clientRect, hBrFondo);
    DeleteObject(hBrFondo);

    localDibujarTextoCentrado(hdc, "ROBOT ASPIRADOR - Simulador de Limpieza", 0, 15, width, COLOR_TITULO, 24, true);

    DrawZonesPanel(hdc);
    DrawRoomPlan(hdc, width / 2 + 10, 80, width / 2 - 30, 220);
    DrawResultsPanel(hdc, 20, 320, width - 40, 200);

    localDibujarTextoCentrado(hdc, "v2.0 | C++ Multithreading | Win32 GUI", 0, height - 25, width, COLOR_TEXTO_DIM, 11, false);
}

void MainScreen::DrawZonesPanel(HDC hdc) {
    int panelX = 20, panelY = 80, panelW = 900 / 2 - 30, panelH = 220;

    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTexto(hdc, "DEFINIR AREAS Y TIPO DE RUMBA", panelX + 15, panelY + 12, COLOR_TITULO, 15, true);

    int tabX = panelX + 15;
    int tabY = panelY + 40;
    localDibujarTexto(hdc, "Zona", tabX + 40, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Largo (cm)", tabX + 180, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Ancho (cm)", tabX + 280, tabY, COLOR_TEXTO_DIM, 13, true);
    localDibujarTexto(hdc, "Area (cm2)", tabX + 370, tabY, COLOR_TEXTO_DIM, 13, true);

    COLORREF coloresZ[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };
    for (size_t i = 0; i < m_zonas.size(); i++) {
        int fY = tabY + 28 + (int)i * 35;
        localDibujarTexto(hdc, m_zonas[i].nombre, tabX + 60, fY, coloresZ[i], 14, false);

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_resultado.calculado) {
            bool found = false;
            for (size_t j = 0; j < m_resultado.zonasCalculadas.size(); ++j) {
                if (m_resultado.zonasCalculadas[j].nombre == m_zonas[i].nombre) {
                    localDibujarTexto(hdc, localFormatearNumero(m_resultado.zonasCalculadas[j].area), tabX + 370, fY, COLOR_ACENTO, 14, true);
                    found = true;
                    break;
                }
            }
            if (!found) localDibujarTexto(hdc, "---", tabX + 370, fY, COLOR_TEXTO_DIM, 14, false);
        }
        else {
            localDibujarTexto(hdc, "---", tabX + 370, fY, COLOR_TEXTO_DIM, 14, false);
        }
    }

    localDibujarTexto(hdc, "Tipo de Robot:", 290, 275, COLOR_TEXTO_DIM, 13, true);
}


void MainScreen::DrawResultsPanel(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTexto(hdc, "RESULTADOS DEL CALCULO", panelX + 15, panelY + 12, COLOR_TITULO, 15, true);

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isCalculating) {
        localDibujarTextoCentrado(hdc, "Realizando calculo distribuido en hilos...", panelX, panelY + 90, panelW, COLOR_ACENTO2, 18, true);
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

        std::string hilosStr = "Calculo realizado en " + std::to_string(m_resultado.zonasCalculadas.size()) + " hilos concurrentes.";
        localDibujarTexto(hdc, hilosStr, panelX + 30, resY + 110, COLOR_TEXTO_DIM, 12, false);

    }
    else {
        localDibujarTextoCentrado(hdc, "Define las areas, selecciona las zonas a limpiar y presiona 'Calcular'.", panelX, panelY + 90, panelW, COLOR_TEXTO_DIM, 16, false);
    }
}

void MainScreen::DrawRoomPlan(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    localDibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH, COLOR_PANEL, COLOR_BORDE, 10);
    localDibujarTextoCentrado(hdc, "Vista de Planta", panelX, panelY + 10, panelW, COLOR_TITULO, 16, true);

    int drawX = panelX + 20, drawY = panelY + 40, drawW = panelW - 40, drawH = panelH - 60;
    double habLargo = 590.0, habAncho = 480.0;

    double escalaX = (double)drawW / habLargo;
    double escalaY = (double)drawH / habAncho;
    double escala = (escalaX < escalaY) ? escalaX : escalaY;

    int offsetX = drawX + (int)((drawW - habLargo * escala) / 2);
    int offsetY = drawY + (int)((drawH - habAncho * escala) / 2);

    COLORREF coloresZona[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };
    struct ZonaPos { double x, y, largo, ancho; };
    ZonaPos posiciones[] = { {0,0,500,150}, {0,150,480,101}, {0,251,309,229}, {500,0,90,220} };

    for (size_t i = 0; i < 4; i++) {
        if (!m_zonas[i].isSelected) continue;

        int zx = offsetX + (int)(posiciones[i].x * escala);
        int zy = offsetY + (int)(posiciones[i].y * escala);
        int zw = (int)(posiciones[i].largo * escala);
        int zh = (int)(posiciones[i].ancho * escala);

        HBRUSH hBrZ = CreateSolidBrush(coloresZona[i]);
        HPEN hPenZ = CreatePen(PS_SOLID, 2, coloresZona[i]);
        SelectObject(hdc, hBrZ);
        SelectObject(hdc, hPenZ);

        LOGBRUSH lb;
        lb.lbStyle = BS_HATCHED;
        lb.lbColor = coloresZona[i];
        lb.lbHatch = HS_DIAGCROSS;
        HBRUSH hHatchBrush = CreateBrushIndirect(&lb);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hHatchBrush);
        SetBkMode(hdc, TRANSPARENT);

        Rectangle(hdc, zx, zy, zx + zw, zy + zh);

        SelectObject(hdc, hOldBrush);
        DeleteObject(hHatchBrush);
        DeleteObject(hBrZ);
        DeleteObject(hPenZ);

        localDibujarTextoCentrado(hdc, "Z" + std::to_string(i + 1), zx, zy + zh / 2 - 8, zw, coloresZona[i], 14, true);
    }
}