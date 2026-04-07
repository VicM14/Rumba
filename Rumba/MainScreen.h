#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <mutex>

// Rutas directas sin carpetas
#include "DataStructures.h"
#include "Robot.h"

class MainScreen {
public:
    MainScreen(HINSTANCE hInstance, HWND hWnd);
    ~MainScreen();

    void HandlePaint(HDC hdc, const RECT& clientRect);
    void HandleCommand(WPARAM wParam, LPARAM lParam);
    void CreateControls();

private:
    void OnCalculateClick();
    void OnClearClick();
    void OnSelectAllClick(bool select);
    void UpdateRobotFromSelection();

    void DrawUI(HDC hdc, const RECT& clientRect);
    void DrawZonesPanel(HDC hdc);
    void DrawResultsPanel(HDC hdc, int panelResX, int panelResY, int panelResW, int panelResH);
    void DrawRoomPlan(HDC hdc, int panelX, int panelY, int panelW, int panelH);

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    std::vector<Zona> m_zonas; // Aquí necesita saber qué es Zona
    std::vector<Robot> m_robots; // Aquí necesita saber qué es Robot
    Robot m_robotActual;
    ResultadoCalculo m_resultado; // Aquí necesita saber qué es ResultadoCalculo
    std::mutex m_mutex;
    bool m_isCalculating;

    HWND m_hBtnCalculate, m_hBtnClear, m_hBtnSelectAll, m_hBtnDeselectAll;
    HWND m_hComboRobotType;
    std::vector<HWND> m_hEditsLargo;
    std::vector<HWND> m_hEditsAncho;
    std::vector<HWND> m_hChecks;
};