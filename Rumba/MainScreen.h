#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <mutex>

#include "DataStructures.h"
#include "Robot.h"

struct RumbaState {
    double xPct, yPct;
    double dxPct, dyPct;
    std::vector<POINT> trail;
    bool active;

    RumbaState() : xPct(0.5), yPct(0.5), dxPct(0.02), dyPct(0.03), active(false) {}
};

class MainScreen {
public:
    MainScreen(HINSTANCE hInstance, HWND hWnd);
    ~MainScreen();

    void HandlePaint(HDC hdc, const RECT& clientRect);
    void HandleCommand(WPARAM wParam, LPARAM lParam);
    void HandleTimer(WPARAM wParam);
    void CreateControls();

private:
    void OnCalculateClick();
    void OnClearClick();
    void OnSelectAllClick(bool select);
    void UpdateRobotFromSelection();
    bool ReadInputsAndValidate(bool showMessages);

    void DrawUI(HDC hdc, const RECT& clientRect);
    void DrawZonesPanel(HDC hdc);
    void DrawResultsPanel(HDC hdc, int panelResX, int panelResY, int panelResW, int panelResH);
    void DrawRoomPlan(HDC hdc, int panelX, int panelY, int panelW, int panelH);

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    std::vector<Zona> m_zonas;
    std::vector<Robot> m_robots;
    Robot m_robotActual;
    ResultadoCalculo m_resultado;
    std::mutex m_mutex;
    bool m_isCalculating;

    // Animación de rumbas
    RumbaState m_rumbas[4];
    bool m_animActive; // <-- Nuevo: Mantiene la animación viva después del cálculo
    bool m_isPaused;   // <-- Nuevo: Detiene las pelotitas momentáneamente

    HWND m_hBtnCalculate, m_hBtnClear, m_hBtnSelectAll, m_hBtnDeselectAll, m_hBtnPause; // <-- Nuevo: botón pausa
    HWND m_hComboRobotType;
    std::vector<HWND> m_hEditsLargo;
    std::vector<HWND> m_hEditsAncho;
    std::vector<HWND> m_hChecks;
};