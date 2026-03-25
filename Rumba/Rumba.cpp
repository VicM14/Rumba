/**
 * =============================================================================
 * ROBOT ASPIRADOR - CALCULADOR DE SUPERFICIE Y TIEMPO DE LIMPIEZA
 * =============================================================================
 *
 * Descripción:
 *   Este programa calcula la superficie total de una habitación dividida en
 *   zonas y estima el tiempo de limpieza de un robot aspirador. Utiliza
 *   programación concurrente (hilos/threads) para el cálculo distribuido
 *   de las áreas de cada zona. Incluye una interfaz gráfica Win32.
 *
 * Enfoque distribuido:
 *   Se usa std::thread y std::future/std::async para lanzar el cálculo
 *   de cada zona en un hilo independiente, simulando procesamiento distribuido.
 *   Los resultados se sincronizan mediante std::future y se integran en el
 *   hilo principal.
 *
 * Compilación (MinGW):
 *   g++ -o robot_aspirador robot_aspirador.cpp -lgdi32 -mwindows -static
 *
 * Compilación (MSVC):
 *   cl robot_aspirador.cpp /EHsc /link user32.lib gdi32.lib
 *
 * Autor: Estudiante
 * Fecha: 2025
 * =============================================================================
 */

#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <thread>
#include <future>
#include <mutex>
#include <chrono>
#include <cmath>

 // =============================================================================
 // TITULO DE LA VENTANA (EDITABLE)
 // Cambia este texto para personalizar el titulo de la barra superior.
 // =============================================================================
#define WINDOW_TITLE     L"Robot Aspirador - Calculador de Superficie (C++ Multithreading)"
#define WINDOW_CLASS     L"RobotAspiradorClass"

 // =============================================================================
 // ESTRUCTURAS DE DATOS
 // =============================================================================

 /**
  * Estructura que representa una zona de la habitación.
  * Cada zona tiene un nombre, largo, ancho y área calculada.
  */
struct Zona {
    std::string nombre;
    double largo;   // en centímetros
    double ancho;   // en centímetros
    double area;    // en centímetros cuadrados (calculada)
    int threadId;   // ID del hilo que calculó esta zona

    Zona(const std::string& n, double l, double a)
        : nombre(n), largo(l), ancho(a), area(0.0), threadId(0) {
    }
};

/**
 * Estructura que almacena los resultados del cálculo.
 */
struct ResultadoCalculo {
    std::vector<Zona> zonas;
    double superficieTotal;
    double tasaLimpieza;        // cm²/segundo
    double tiempoSegundos;
    double tiempoMinutos;
    double tiempoHoras;
    bool calculado;

    ResultadoCalculo() : superficieTotal(0), tasaLimpieza(1000),
        tiempoSegundos(0), tiempoMinutos(0),
        tiempoHoras(0), calculado(false) {
    }
};

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static ResultadoCalculo g_resultado;
static HWND g_hWnd = NULL;
static HWND g_hBtnCalcular = NULL;
static HWND g_hBtnLimpiar = NULL;
static HWND g_hEditTasa = NULL;
static HWND g_hLabelTasa = NULL;
static std::mutex g_mutex;

// Identificadores de controles
#define ID_BTN_CALCULAR  1001
#define ID_BTN_LIMPIAR   1002
#define ID_EDIT_TASA     1003

// Colores personalizados
#define COLOR_FONDO         RGB(30, 30, 46)
#define COLOR_PANEL         RGB(45, 45, 65)
#define COLOR_PANEL_ZONA    RGB(55, 55, 80)
#define COLOR_TITULO        RGB(137, 180, 250)
#define COLOR_TEXTO         RGB(205, 214, 244)
#define COLOR_TEXTO_DIM     RGB(147, 153, 178)
#define COLOR_ACENTO        RGB(166, 227, 161)
#define COLOR_ACENTO2       RGB(249, 226, 175)
#define COLOR_ACENTO3       RGB(243, 139, 168)
#define COLOR_ZONA1         RGB(137, 180, 250)
#define COLOR_ZONA2         RGB(166, 227, 161)
#define COLOR_ZONA3         RGB(249, 226, 175)
#define COLOR_ZONA4         RGB(243, 139, 168)
#define COLOR_BTN           RGB(137, 180, 250)
#define COLOR_BTN_TEXT      RGB(30, 30, 46)
#define COLOR_BORDE         RGB(88, 91, 112)

// =============================================================================
// FUNCIONES DE CÁLCULO DISTRIBUIDO
// =============================================================================

/**
 * Función que calcula el área de una zona.
 * Diseñada para ejecutarse en un hilo independiente.
 * Incluye un pequeño delay para simular procesamiento distribuido.
 *
 * @param largo  - Largo de la zona en cm
 * @param ancho  - Ancho de la zona en cm
 * @return El área calculada (largo * ancho) en cm²
 */
double calcularArea(double largo, double ancho) {
    // Simular trabajo de procesamiento distribuido
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return largo * ancho;
}

/**
 * Función que calcula el área de una zona y devuelve un par con
 * el índice de la zona y el área calculada, junto con el ID del hilo.
 *
 * @param indice - Índice de la zona en el vector
 * @param largo  - Largo de la zona en cm
 * @param ancho  - Ancho de la zona en cm
 * @return Par (índice, área calculada)
 */
std::pair<int, double> calcularAreaConIndice(int indice, double largo, double ancho) {
    double area = calcularArea(largo, ancho);
    return std::make_pair(indice, area);
}

/**
 * Función principal de cálculo distribuido.
 * Lanza un hilo (std::async) por cada zona para calcular su área
 * de forma concurrente. Luego recopila los resultados y calcula
 * la superficie total y el tiempo estimado de limpieza.
 *
 * @param tasaLimpieza - Tasa de limpieza del robot en cm²/segundo
 */
void ejecutarCalculoDistribuido(double tasaLimpieza) {
    // Inicializar las zonas con los datos del enunciado (según la foto)
    // Zona 1: franja superior, 500 cm x 150 cm
    // Zona 2: franja izquierda (debajo de Z1), 101 cm x 480 cm
    // Zona 3: bloque derecho (al lado de Z2), 309 cm x 480 cm
    // Zona 4: franja inferior, 500 cm x 220 cm
    std::vector<Zona> zonas;
    zonas.emplace_back("Zona 1", 500.0, 150.0);
    zonas.emplace_back("Zona 2", 101.0, 480.0);
    zonas.emplace_back("Zona 3", 309.0, 480.0);
    zonas.emplace_back("Zona 4", 500.0, 220.0);

    // =========================================================================
    // CÁLCULO DISTRIBUIDO: Lanzar un hilo por cada zona usando std::async
    // Cada std::async crea un hilo independiente que calcula el área de una zona
    // =========================================================================
    std::vector<std::future<std::pair<int, double>>> futuros;

    for (int i = 0; i < (int)zonas.size(); i++) {
        // Lanzar cálculo asíncrono para cada zona
        // std::launch::async garantiza que se ejecute en un hilo separado
        futuros.push_back(
            std::async(std::launch::async, calcularAreaConIndice,
                i, zonas[i].largo, zonas[i].ancho)
        );
    }

    // =========================================================================
    // SINCRONIZACIÓN: Recopilar resultados de todos los hilos
    // =========================================================================
    double superficieTotal = 0.0;

    for (auto& futuro : futuros) {
        // .get() bloquea hasta que el hilo termine y devuelve el resultado
        auto resultado = futuro.get();
        int indice = resultado.first;
        double area = resultado.second;

        zonas[indice].area = area;
        superficieTotal += area;
    }

    // =========================================================================
    // CÁLCULO DEL TIEMPO DE LIMPIEZA
    // Tiempo = Superficie Total / Tasa de Limpieza
    // =========================================================================
    double tiempoSeg = superficieTotal / tasaLimpieza;
    double tiempoMin = tiempoSeg / 60.0;
    double tiempoHrs = tiempoMin / 60.0;

    // =========================================================================
    // ALMACENAR RESULTADOS (con protección de mutex)
    // =========================================================================
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_resultado.zonas = zonas;
        g_resultado.superficieTotal = superficieTotal;
        g_resultado.tasaLimpieza = tasaLimpieza;
        g_resultado.tiempoSegundos = tiempoSeg;
        g_resultado.tiempoMinutos = tiempoMin;
        g_resultado.tiempoHoras = tiempoHrs;
        g_resultado.calculado = true;
    }

    // Solicitar repintado de la ventana
    InvalidateRect(g_hWnd, NULL, TRUE);
}

// =============================================================================
// FUNCIONES AUXILIARES DE DIBUJO
// =============================================================================

/**
 * Dibuja un rectángulo redondeado relleno.
 */
void dibujarRectRedondeado(HDC hdc, int x, int y, int w, int h,
    COLORREF colorFondo, COLORREF colorBorde, int radio = 8) {
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

/**
 * Dibuja texto con color y fuente especificados.
 */
void dibujarTexto(HDC hdc, const std::string& texto, int x, int y,
    COLORREF color, int tamano = 14, bool negrita = false,
    const char* fuente = "Segoe UI") {
    HFONT hFont = CreateFontA(tamano, 0, 0, 0, negrita ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, fuente);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutA(hdc, x, y, texto.c_str(), (int)texto.length());
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

/**
 * Dibuja texto centrado horizontalmente dentro de un ancho dado.
 */
void dibujarTextoCentrado(HDC hdc, const std::string& texto, int x, int y, int ancho,
    COLORREF color, int tamano = 14, bool negrita = false) {
    HFONT hFont = CreateFontA(tamano, 0, 0, 0, negrita ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
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

/**
 * Formatea un número double a string con separador de miles.
 */
std::string formatearNumero(double num, int decimales = 0) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << num;
    std::string s = oss.str();

    // Encontrar el punto decimal
    size_t puntoPos = s.find('.');
    std::string parteEntera = (puntoPos != std::string::npos) ? s.substr(0, puntoPos) : s;
    std::string parteDecimal = (puntoPos != std::string::npos) ? s.substr(puntoPos) : "";

    // Agregar separadores de miles
    std::string resultado;
    int count = 0;
    for (int i = (int)parteEntera.length() - 1; i >= 0; i--) {
        if (count > 0 && count % 3 == 0) {
            resultado = "." + resultado;
        }
        resultado = parteEntera[i] + resultado;
        count++;
    }

    return resultado + parteDecimal;
}

// =============================================================================
// FUNCIÓN DE DIBUJO DE LA VISTA DE PLANTA DE LA HABITACIÓN
// =============================================================================

/**
 * Dibuja una representación visual (vista de planta) de las zonas
 * de la habitación, mostrando las zonas como rectángulos a escala.
 */
void dibujarVistaPlanta(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    // Panel de fondo
    dibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH,
        COLOR_PANEL, COLOR_BORDE, 10);

    // Título
    dibujarTextoCentrado(hdc, "Vista de Planta de la Habitacion",
        panelX, panelY + 10, panelW, COLOR_TITULO, 16, true);

    // Área de dibujo dentro del panel
    int drawX = panelX + 20;
    int drawY = panelY + 40;
    int drawW = panelW - 40;
    int drawH = panelH - 70;

    double habAncho = 560.0;
    double habAlto = 690.0;

    // Escala para ajustar al área de dibujo
    double escalaX = drawW / habAncho;
    double escalaY = drawH / habAlto;
    double escala = (escalaX < escalaY) ? escalaX : escalaY;

    // Centrar el dibujo
    int offsetX = drawX + (int)((drawW - habAncho * escala) / 2);
    int offsetY = drawY + (int)((drawH - habAlto * escala) / 2);

    // Dibujar contorno de la habitación completa
    HPEN hPenHab = CreatePen(PS_DASH, 1, COLOR_TEXTO_DIM);
    HBRUSH hBrushHab = CreateSolidBrush(RGB(35, 35, 55));
    HBRUSH hOldBrHab = (HBRUSH)SelectObject(hdc, hBrushHab);
    HPEN hOldPenHab = (HPEN)SelectObject(hdc, hPenHab);
    Rectangle(hdc, offsetX, offsetY,
        offsetX + (int)(habAncho * escala),
        offsetY + (int)(habAlto * escala));
    SelectObject(hdc, hOldBrHab);
    SelectObject(hdc, hOldPenHab);
    DeleteObject(hPenHab);
    DeleteObject(hBrushHab);

    // Colores para cada zona
    COLORREF coloresZona[] = {
        RGB(137, 180, 250),
        RGB(166, 227, 161),
        RGB(249, 226, 175),
        RGB(243, 139, 168)
    };
    COLORREF coloresZonaDim[] = {
        RGB(80, 110, 170),
        RGB(90, 150, 90),
        RGB(170, 150, 100),
        RGB(170, 80, 100)
    };

    struct ZonaPos { double x, y, ancho, alto; };
    ZonaPos posiciones[] = {
        {   0,   0, 500, 150},
        {   0, 150, 101, 480},
        { 251, 150, 309, 480},
        {   0, 470, 500, 220}
    };

    // Dibujar cada zona
    for (int i = 0; i < 4; i++) {
        int zx = offsetX + (int)(posiciones[i].x * escala);
        int zy = offsetY + (int)(posiciones[i].y * escala);
        int zw = (int)(posiciones[i].ancho * escala);
        int zh = (int)(posiciones[i].alto * escala);

        HBRUSH hBrZ = CreateSolidBrush(coloresZonaDim[i]);
        HPEN hPenZ = CreatePen(PS_SOLID, 2, coloresZona[i]);
        HBRUSH hOldBrZ = (HBRUSH)SelectObject(hdc, hBrZ);
        HPEN hOldPenZ = (HPEN)SelectObject(hdc, hPenZ);
        Rectangle(hdc, zx, zy, zx + zw, zy + zh);
        SelectObject(hdc, hOldBrZ);
        SelectObject(hdc, hOldPenZ);
        DeleteObject(hBrZ);
        DeleteObject(hPenZ);

        std::string label = "Z" + std::to_string(i + 1);
        dibujarTextoCentrado(hdc, label, zx, zy + zh / 2 - 8, zw,
            coloresZona[i], 14, true);
    }

    // Mueble
    int muebleX = offsetX + (int)(101 * escala);
    int muebleY = offsetY + (int)(150 * escala);
    int muebleW = (int)(150 * escala);
    int muebleH = (int)(320 * escala);

    HBRUSH hBrMueble = CreateSolidBrush(RGB(80, 73, 69));
    HPEN hPenMueble = CreatePen(PS_SOLID, 2, RGB(120, 110, 100));
    HBRUSH hOldBrM = (HBRUSH)SelectObject(hdc, hBrMueble);
    HPEN hOldPenM = (HPEN)SelectObject(hdc, hPenMueble);
    Rectangle(hdc, muebleX, muebleY, muebleX + muebleW, muebleY + muebleH);
    SelectObject(hdc, hOldBrM);
    SelectObject(hdc, hOldPenM);
    DeleteObject(hBrMueble);
    DeleteObject(hPenMueble);

    dibujarTextoCentrado(hdc, "MUEBLE", muebleX, muebleY + muebleH / 2 - 8,
        muebleW, RGB(180, 170, 160), 11, true);

    // Leyenda
    int legY = offsetY + (int)(habAlto * escala) + 8;
    for (int i = 0; i < 4; i++) {
        int lx = drawX + i * (drawW / 4);
        HBRUSH hBrL = CreateSolidBrush(coloresZona[i]);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrL);
        Rectangle(hdc, lx, legY, lx + 12, legY + 12);
        SelectObject(hdc, hOldBr);
        DeleteObject(hBrL);
        dibujarTexto(hdc, "Zona " + std::to_string(i + 1), lx + 16, legY - 1,
            COLOR_TEXTO, 12, false);
    }
}

// =============================================================================
// FUNCIÓN PRINCIPAL DE PINTADO
// =============================================================================

void pintarVentana(HDC hdc, RECT& rect) {
    int anchoVentana = rect.right - rect.left;
    int altoVentana = rect.bottom - rect.top;

    // Fondo general
    HBRUSH hBrFondo = CreateSolidBrush(COLOR_FONDO);
    FillRect(hdc, &rect, hBrFondo);
    DeleteObject(hBrFondo);

    // TÍTULO PRINCIPAL
    dibujarTextoCentrado(hdc, "ROBOT ASPIRADOR - Calculador de Superficie",
        0, 15, anchoVentana, COLOR_TITULO, 24, true);
    dibujarTextoCentrado(hdc, "Calculo distribuido con hilos concurrentes",
        0, 45, anchoVentana, COLOR_TEXTO_DIM, 13, false);

    // PANEL IZQUIERDO: DATOS DE LAS ZONAS
    int panelIzqX = 20;
    int panelIzqY = 80;
    int panelIzqW = anchoVentana / 2 - 30;
    int panelIzqH = 220;

    dibujarRectRedondeado(hdc, panelIzqX, panelIzqY, panelIzqW, panelIzqH,
        COLOR_PANEL, COLOR_BORDE, 10);

    dibujarTexto(hdc, "DATOS DE LAS ZONAS", panelIzqX + 15, panelIzqY + 12,
        COLOR_TITULO, 15, true);

    int tabX = panelIzqX + 15;
    int tabY = panelIzqY + 40;
    dibujarTexto(hdc, "Zona", tabX, tabY, COLOR_TEXTO_DIM, 13, true);
    dibujarTexto(hdc, "Largo (cm)", tabX + 100, tabY, COLOR_TEXTO_DIM, 13, true);
    dibujarTexto(hdc, "Ancho (cm)", tabX + 210, tabY, COLOR_TEXTO_DIM, 13, true);
    dibujarTexto(hdc, "Area (cm2)", tabX + 320, tabY, COLOR_TEXTO_DIM, 13, true);

    HPEN hPenSep = CreatePen(PS_SOLID, 1, COLOR_BORDE);
    SelectObject(hdc, hPenSep);
    MoveToEx(hdc, tabX, tabY + 20, NULL);
    LineTo(hdc, tabX + panelIzqW - 30, tabY + 20);
    DeleteObject(hPenSep);

    COLORREF coloresZ[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };
    double datosZonas[][2] = { {500,150}, {101,480}, {309,480}, {500,220} };

    for (int i = 0; i < 4; i++) {
        int fY = tabY + 28 + i * 35;

        HBRUSH hBrInd = CreateSolidBrush(coloresZ[i]);
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrInd);
        HPEN hPenN = CreatePen(PS_SOLID, 1, coloresZ[i]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenN);
        RoundRect(hdc, tabX, fY + 2, tabX + 8, fY + 16, 3, 3);
        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBrInd);
        DeleteObject(hPenN);

        dibujarTexto(hdc, "Zona " + std::to_string(i + 1), tabX + 14, fY,
            coloresZ[i], 14, false);

        std::ostringstream ossL, ossA;
        ossL << std::fixed << std::setprecision(0) << datosZonas[i][0];
        ossA << std::fixed << std::setprecision(0) << datosZonas[i][1];
        dibujarTexto(hdc, ossL.str(), tabX + 120, fY, COLOR_TEXTO, 14, false);
        dibujarTexto(hdc, ossA.str(), tabX + 230, fY, COLOR_TEXTO, 14, false);

        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_resultado.calculado && i < (int)g_resultado.zonas.size()) {
            std::string areaStr = formatearNumero(g_resultado.zonas[i].area);
            dibujarTexto(hdc, areaStr, tabX + 330, fY, COLOR_ACENTO, 14, true);
        }
        else {
            dibujarTexto(hdc, "---", tabX + 330, fY, COLOR_TEXTO_DIM, 14, false);
        }
    }

    // PANEL DERECHO: VISTA DE PLANTA
    int panelDerX = anchoVentana / 2 + 10;
    int panelDerY = 80;
    int panelDerW = anchoVentana / 2 - 30;
    int panelDerH = 220;

    dibujarVistaPlanta(hdc, panelDerX, panelDerY, panelDerW, panelDerH);

    // PANEL INFERIOR: RESULTADOS
    int panelResX = 20;
    int panelResY = 320;
    int panelResW = anchoVentana - 40;
    int panelResH = 200;

    dibujarRectRedondeado(hdc, panelResX, panelResY, panelResW, panelResH,
        COLOR_PANEL, COLOR_BORDE, 10);

    dibujarTexto(hdc, "RESULTADOS DEL CALCULO DISTRIBUIDO",
        panelResX + 15, panelResY + 12, COLOR_TITULO, 15, true);

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_resultado.calculado) {
            int resY = panelResY + 45;
            int col1X = panelResX + 30;
            int col2X = panelResX + panelResW / 3 + 10;
            int col3X = panelResX + 2 * panelResW / 3 + 10;

            int cardW = panelResW / 3 - 30;
            dibujarRectRedondeado(hdc, col1X - 10, resY - 5, cardW, 90,
                COLOR_PANEL_ZONA, COLOR_BORDE, 8);
            dibujarTextoCentrado(hdc, "Superficie Total", col1X - 10, resY, cardW,
                COLOR_TEXTO_DIM, 12, false);
            std::string supStr = formatearNumero(g_resultado.superficieTotal) + " cm2";
            dibujarTextoCentrado(hdc, supStr, col1X - 10, resY + 22, cardW,
                COLOR_ACENTO, 20, true);
            double supM2 = g_resultado.superficieTotal / 10000.0;
            std::ostringstream ossM2;
            ossM2 << std::fixed << std::setprecision(2) << supM2 << " m2";
            dibujarTextoCentrado(hdc, ossM2.str(), col1X - 10, resY + 50, cardW,
                COLOR_TEXTO_DIM, 13, false);

            dibujarRectRedondeado(hdc, col2X - 10, resY - 5, cardW, 90,
                COLOR_PANEL_ZONA, COLOR_BORDE, 8);
            dibujarTextoCentrado(hdc, "Tasa de Limpieza", col2X - 10, resY, cardW,
                COLOR_TEXTO_DIM, 12, false);
            std::string tasaStr = formatearNumero(g_resultado.tasaLimpieza) + " cm2/s";
            dibujarTextoCentrado(hdc, tasaStr, col2X - 10, resY + 22, cardW,
                COLOR_ACENTO2, 20, true);
            dibujarTextoCentrado(hdc, "Velocidad del robot", col2X - 10, resY + 50, cardW,
                COLOR_TEXTO_DIM, 13, false);

            dibujarRectRedondeado(hdc, col3X - 10, resY - 5, cardW, 90,
                COLOR_PANEL_ZONA, COLOR_BORDE, 8);
            dibujarTextoCentrado(hdc, "Tiempo Estimado", col3X - 10, resY, cardW,
                COLOR_TEXTO_DIM, 12, false);

            std::ostringstream ossTiempo;
            if (g_resultado.tiempoMinutos >= 60) {
                ossTiempo << std::fixed << std::setprecision(1)
                    << g_resultado.tiempoHoras << " hrs";
            }
            else {
                ossTiempo << std::fixed << std::setprecision(1)
                    << g_resultado.tiempoMinutos << " min";
            }
            dibujarTextoCentrado(hdc, ossTiempo.str(), col3X - 10, resY + 22, cardW,
                COLOR_ACENTO3, 20, true);

            std::ostringstream ossSeg;
            ossSeg << std::fixed << std::setprecision(1)
                << g_resultado.tiempoSegundos << " segundos";
            dibujarTextoCentrado(hdc, ossSeg.str(), col3X - 10, resY + 50, cardW,
                COLOR_TEXTO_DIM, 13, false);

            int detY = resY + 100;
            dibujarTexto(hdc, "Calculo realizado con 4 hilos concurrentes (std::async + std::future)",
                panelResX + 30, detY, COLOR_TEXTO_DIM, 12, false);

            std::ostringstream ossFormula;
            ossFormula << "Tiempo = Superficie Total / Tasa = "
                << formatearNumero(g_resultado.superficieTotal)
                << " / " << formatearNumero(g_resultado.tasaLimpieza)
                << " = " << std::fixed << std::setprecision(1)
                << g_resultado.tiempoSegundos << " seg";
            dibujarTexto(hdc, ossFormula.str(), panelResX + 30, detY + 18,
                COLOR_TEXTO_DIM, 12, false);
        }
        else {
            dibujarTextoCentrado(hdc, "Presiona 'Calcular' para iniciar el calculo distribuido",
                panelResX, panelResY + 80, panelResW,
                COLOR_TEXTO_DIM, 16, false);
            dibujarTextoCentrado(hdc, "Se lanzaran 4 hilos concurrentes, uno por cada zona",
                panelResX, panelResY + 105, panelResW,
                COLOR_TEXTO_DIM, 13, false);
        }
    }

    // PIE DE PÁGINA
    dibujarTextoCentrado(hdc, "Robot Aspirador v1.0 | C++ Multithreading | Win32 GUI",
        0, altoVentana - 25, anchoVentana, COLOR_TEXTO_DIM, 11, false);
}

// =============================================================================
// PROCEDIMIENTO DE VENTANA (Win32 Message Handler)
// =============================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hLabelTasa = CreateWindowA("STATIC", "Tasa (cm2/s):",
            WS_CHILD | WS_VISIBLE | SS_RIGHT,
            20, 540, 120, 25, hWnd, NULL, NULL, NULL);

        g_hEditTasa = CreateWindowExA(0, "EDIT", "1000",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            145, 538, 100, 28, hWnd, (HMENU)ID_EDIT_TASA, NULL, NULL);

        g_hBtnCalcular = CreateWindowA("BUTTON", "  CALCULAR  ",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            260, 536, 140, 32, hWnd, (HMENU)ID_BTN_CALCULAR, NULL, NULL);

        g_hBtnLimpiar = CreateWindowA("BUTTON", "  LIMPIAR  ",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            410, 536, 120, 32, hWnd, (HMENU)ID_BTN_LIMPIAR, NULL, NULL);

        HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");
        SendMessage(g_hLabelTasa, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(g_hEditTasa, WM_SETFONT, (WPARAM)hFont, TRUE);

        HFONT hFontBtn = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");
        SendMessage(g_hBtnCalcular, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        SendMessage(g_hBtnLimpiar, WM_SETFONT, (WPARAM)hFontBtn, TRUE);

        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, COLOR_TEXTO);
        SetBkColor(hdcStatic, COLOR_FONDO);
        static HBRUSH hBrStatic = CreateSolidBrush(COLOR_FONDO);
        return (LRESULT)hBrStatic;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_CALCULAR: {
            char buffer[64];
            GetWindowTextA(g_hEditTasa, buffer, sizeof(buffer));
            double tasa = atof(buffer);
            if (tasa <= 0) tasa = 1000.0;

            EnableWindow(g_hBtnCalcular, FALSE);

            std::thread t([tasa]() {
                ejecutarCalculoDistribuido(tasa);
                EnableWindow(g_hBtnCalcular, TRUE);
                });
            t.detach();
            break;
        }
        case ID_BTN_LIMPIAR: {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_resultado = ResultadoCalculo();
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rect;
        GetClientRect(hWnd, &rect);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBmp = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

        pintarVentana(hdcMem, rect);

        BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

        SelectObject(hdcMem, hOldBmp);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

// =============================================================================
// PUNTO DE ENTRADA PRINCIPAL
// =============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    // =========================================================================
    // Usar WNDCLASSEXW (Unicode) para que el titulo se muestre correctamente
    // =========================================================================
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_FONDO);
    wc.lpszClassName = WINDOW_CLASS;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Error al registrar la clase de ventana", L"Error", MB_OK);
        return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 900;
    int winH = 620;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    // =========================================================================
    // Usar CreateWindowExW (Unicode) con WINDOW_TITLE para el titulo
    // =========================================================================
    g_hWnd = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) {
        MessageBoxW(NULL, L"Error al crear la ventana", L"Error", MB_OK);
        return 1;
    }

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}