#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include <mutex>

#include "DataStructures.h"
#include "Robot.h"

struct RumbaState {
    double xPct, yPct;
    double speedPct;
    int dirX;
    std::vector<POINT> trail;
    bool active;
    bool finished;

    RumbaState() : xPct(0.05), yPct(0.05), speedPct(0.02), dirX(1), active(false), finished(false) {}
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
    void DrawRobotSelectionScreen(HDC hdc, const RECT& clientRect);

    HINSTANCE m_hInstance;
    HWND m_hWnd;
    std::vector<Zona> m_zonas;
    std::vector<Robot> m_robots;
    Robot m_robotActual;
    ResultadoCalculo m_resultado;
    std::mutex m_mutex;
    bool m_isCalculating;

    RumbaState m_rumbas[4];
    bool m_animActive;
    bool m_isPaused;
    bool m_allFinished;

    bool m_showRobotSelection;
    int m_selectedRobotIndex;
    int m_previewX;
    int m_previewDir;

    HWND m_hBtnCalculate, m_hBtnClear, m_hBtnSelectAll, m_hBtnDeselectAll, m_hBtnPause, m_hBtnQuit; // <-- m_hBtnQuit agregado
    HWND m_hComboRobotType;
    HWND m_hBtnConfirmRobot;
    std::vector<HWND> m_hEditsLargo;
    std::vector<HWND> m_hEditsAncho;
    std::vector<HWND> m_hChecks;
};