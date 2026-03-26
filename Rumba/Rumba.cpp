/**
 * =============================================================================
 * ROBOT ASPIRADOR - SELECCION DE ZONA, RUMBA Y SISTEMA DE BATERIA
 * =============================================================================
 *
 * Descripcion:
 *   Interfaz completa donde el usuario elige la zona a limpiar y el modelo
 *   de Rumba a usar. Cada Rumba tiene ventajas en zonas especificas.
 *   Incluye sistema de bateria grafico que puede agotarse.
 *   Conserva las zonas originales del proyecto (Zona 1-4).
 *
 * Compilacion (MinGW):
 *   g++ -o robot_aspirador robot_aspirador.cpp -lgdi32 -mwindows -static
 *
 * Compilacion (MSVC):
 *   cl robot_aspirador.cpp /EHsc /link user32.lib gdi32.lib
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
#include <algorithm>
#include <functional>

 // =============================================================================
 // ESTRUCTURAS DE DATOS
 // =============================================================================

enum TipoZona {
    ZONA_ALFOMBRA = 0,
    ZONA_PISO_LISO,
    ZONA_MUEBLES,
    ZONA_COCINA,
    ZONA_BANO,
    ZONA_JARDIN_INTERIOR,
    NUM_ZONAS
};

enum ModeloRumba {
    RUMBA_TURBO_CARPET = 0,
    RUMBA_SMOOTH_GLIDE,
    RUMBA_NAVIGATOR_PRO,
    RUMBA_ALL_TERRAIN,
    NUM_RUMBAS
};

/**
 * Zona original del proyecto con medidas fijas.
 */
struct ZonaOriginal {
    std::string nombre;
    double largo;   // cm
    double ancho;   // cm
    double area;    // cm2 calculada
    int threadId;

    ZonaOriginal(const std::string& n, double l, double a)
        : nombre(n), largo(l), ancho(a), area(0.0), threadId(0) {
    }
};

struct InfoZona {
    std::string nombre;
    std::string descripcion;
    std::string icono;
    double areaBase;
    double factorDificultad;
    COLORREF color;
    COLORREF colorOscuro;
    std::string tamanos[3];
    double multiplicadores[3];
};

struct InfoRumba {
    std::string nombre;
    std::string descripcion;
    std::string especialidad;
    double velocidadBase;
    double bateriaMah;
    double consumoPorCm2;
    TipoZona zonaMejor;
    double bonificacion;
    double penalizacion;
    COLORREF color;
    COLORREF colorOscuro;
    double eficiencia[NUM_ZONAS];
};

struct ResultadoZonasOriginales {
    std::vector<ZonaOriginal> zonas;
    double superficieTotal;
    bool calculado;
    ResultadoZonasOriginales() : superficieTotal(0), calculado(false) {}
};

struct EstadoApp {
    int zonaSeleccionada;
    int rumbaSeleccionada;
    int tamanoSeleccionado;

    double bateriaPorcentaje;
    double bateriaMaxMah;
    double bateriaActualMah;
    bool bateriaAgotada;

    bool limpiando;
    bool limpiezaCompleta;
    bool detenidoPrematuramente;
    double progreso;
    double areaTotal;
    double areaLimpiada;
    double tiempoEstimado;
    double tiempoTranscurrido;
    double velocidadEfectiva;
    std::string mensajeEstado;
    std::string mensajeBateria;

    double eficienciaActual;
    bool esZonaOptima;

    ResultadoZonasOriginales resOriginal;

    EstadoApp() : zonaSeleccionada(-1), rumbaSeleccionada(-1),
        tamanoSeleccionado(1), bateriaPorcentaje(100.0),
        bateriaMaxMah(0), bateriaActualMah(0),
        bateriaAgotada(false), limpiando(false),
        limpiezaCompleta(false), detenidoPrematuramente(false),
        progreso(0), areaTotal(0), areaLimpiada(0),
        tiempoEstimado(0), tiempoTranscurrido(0),
        velocidadEfectiva(0), eficienciaActual(0),
        esZonaOptima(false) {
    }
};

// =============================================================================
// VARIABLES GLOBALES
// =============================================================================

static EstadoApp g_estado;
static HWND g_hWnd = NULL;
static std::mutex g_mutex;
static bool g_cancelarLimpieza = false;
static bool g_abortarProceso = false;

static HWND g_hBtnIniciar = NULL;
static HWND g_hBtnDetener = NULL;
static HWND g_hBtnAbortar = NULL;
static HWND g_hBtnReset = NULL;

#define ID_BTN_INICIAR  2001
#define ID_BTN_DETENER  2002
#define ID_BTN_RESET    2003
#define ID_BTN_ABORTAR  2004

// Colores
#define COLOR_FONDO         RGB(20, 22, 34)
#define COLOR_PANEL         RGB(35, 38, 55)
#define COLOR_PANEL_HOVER   RGB(45, 48, 70)
#define COLOR_PANEL_SEL     RGB(55, 58, 85)
#define COLOR_TITULO        RGB(137, 180, 250)
#define COLOR_TEXTO         RGB(205, 214, 244)
#define COLOR_TEXTO_DIM     RGB(127, 133, 158)
#define COLOR_ACENTO        RGB(166, 227, 161)
#define COLOR_ACENTO2       RGB(249, 226, 175)
#define COLOR_ACENTO3       RGB(243, 139, 168)
#define COLOR_BORDE         RGB(68, 71, 92)
#define COLOR_BORDE_SEL     RGB(137, 180, 250)
#define COLOR_BAT_FULL      RGB(166, 227, 161)
#define COLOR_BAT_MED       RGB(249, 226, 175)
#define COLOR_BAT_LOW       RGB(243, 139, 168)
#define COLOR_BAT_BG        RGB(45, 48, 65)
#define COLOR_PROGRESO      RGB(137, 180, 250)
#define COLOR_PROGRESO_BG   RGB(45, 48, 65)

#define COLOR_ZONA1         RGB(137, 180, 250)
#define COLOR_ZONA2         RGB(166, 227, 161)
#define COLOR_ZONA3         RGB(249, 226, 175)
#define COLOR_ZONA4         RGB(243, 139, 168)

// =============================================================================
// DATOS DE ZONAS Y RUMBAS
// =============================================================================

static InfoZona g_zonas[NUM_ZONAS] = {
    {"Alfombra", "Superficie de alfombra o tapete. Requiere mayor succion.",
     "[~]", 50000.0, 1.4, RGB(180, 142, 173), RGB(120, 92, 123),
     {"Pequena (3m2)", "Mediana (5m2)", "Grande (8m2)"}, {0.6, 1.0, 1.6}},

    {"Piso Liso", "Baldosa, porcelanato o piso laminado. Facil de limpiar.",
     "[=]", 60000.0, 0.8, RGB(137, 180, 250), RGB(87, 130, 200),
     {"Pequeno (4m2)", "Mediano (6m2)", "Grande (10m2)"}, {0.66, 1.0, 1.66}},

    {"Area con Muebles", "Sala o habitacion con muchos obstaculos.",
     "[#]", 45000.0, 1.8, RGB(249, 226, 175), RGB(199, 176, 125),
     {"Pocas piezas (3m2)", "Normal (4.5m2)", "Muchas piezas (7m2)"}, {0.66, 1.0, 1.55}},

    {"Cocina", "Piso de cocina, puede tener grasa y particulas.",
     "[K]", 35000.0, 1.2, RGB(166, 227, 161), RGB(116, 177, 111),
     {"Pequena (2m2)", "Mediana (3.5m2)", "Grande (6m2)"}, {0.57, 1.0, 1.71}},

    {"Bano", "Piso de bano, posible humedad. Area pequena.",
     "[B]", 20000.0, 1.3, RGB(148, 226, 213), RGB(98, 176, 163),
     {"Pequeno (1.5m2)", "Mediano (2m2)", "Grande (3.5m2)"}, {0.75, 1.0, 1.75}},

    {"Jardin Interior", "Terraza o jardin interior con tierra y hojas.",
     "[J]", 70000.0, 1.6, RGB(243, 139, 168), RGB(193, 89, 118),
     {"Pequeno (4m2)", "Mediano (7m2)", "Grande (12m2)"}, {0.57, 1.0, 1.71}}
};

static InfoRumba g_rumbas[NUM_RUMBAS] = {
    {"Rumba TurboCarpet X1", "Motor de alta succion para fibras profundas",
     "Alfombras y tapetes", 800.0, 5200.0, 0.012, ZONA_ALFOMBRA, 1.5, 0.85,
     RGB(180, 142, 173), RGB(130, 92, 123),
     {1.5, 0.9, 0.85, 0.9, 0.85, 0.7}},

    {"Rumba SmoothGlide S3", "Diseno optimizado para superficies lisas",
     "Pisos lisos y baldosas", 1100.0, 4800.0, 0.010, ZONA_PISO_LISO, 1.6, 0.8,
     RGB(137, 180, 250), RGB(87, 130, 200),
     {0.7, 1.6, 0.8, 1.3, 1.2, 0.6}},

    {"Rumba NavigatorPro N7", "Sensores LIDAR para navegacion entre muebles",
     "Areas con obstaculos", 900.0, 5500.0, 0.013, ZONA_MUEBLES, 1.7, 0.9,
     RGB(249, 226, 175), RGB(199, 176, 125),
     {0.85, 0.9, 1.7, 1.1, 1.0, 0.9}},

    {"Rumba AllTerrain AT9", "Versatil y equilibrado para todo tipo de superficie",
     "Todas las superficies", 950.0, 6000.0, 0.011, ZONA_COCINA, 1.15, 1.0,
     RGB(166, 227, 161), RGB(116, 177, 111),
     {1.05, 1.1, 1.05, 1.15, 1.1, 1.1}}
};

// =============================================================================
// AREAS DE CLIC
// =============================================================================

struct AreaClic {
    RECT rect;
    int indice;
    int tipo;   // 0=zona, 1=rumba, 2=tamano
};

static std::vector<AreaClic> g_areasClic;

// =============================================================================
// FUNCIONES AUXILIARES DE DIBUJO
// =============================================================================

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

void dibujarRectRedondeadoBorde(HDC hdc, int x, int y, int w, int h,
    COLORREF colorFondo, COLORREF colorBorde, int grosor, int radio = 8) {
    HBRUSH hBrush = CreateSolidBrush(colorFondo);
    HPEN hPen = CreatePen(PS_SOLID, grosor, colorBorde);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    RoundRect(hdc, x, y, x + w, y + h, radio, radio);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

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

std::string formatearNumero(double num, int decimales = 0) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimales) << num;
    return oss.str();
}

std::string formatearConMiles(double num) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0) << num;
    std::string s = oss.str();
    std::string resultado;
    int count = 0;
    for (int i = (int)s.length() - 1; i >= 0; i--) {
        if (count > 0 && count % 3 == 0) resultado = "." + resultado;
        resultado = s[i] + resultado;
        count++;
    }
    return resultado;
}

// =============================================================================
// CALCULO DISTRIBUIDO DE ZONAS ORIGINALES
// =============================================================================

double calcularAreaAsync(double largo, double ancho) {
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    return largo * ancho;
}

void calcularZonasOriginales() {
    std::vector<ZonaOriginal> zonas;
    zonas.emplace_back("Zona 1", 500.0, 150.0);
    zonas.emplace_back("Zona 2", 480.0, 101.0);
    zonas.emplace_back("Zona 3", 309.0, 480.0);
    zonas.emplace_back("Zona 4", 90.0, 220.0);

    std::vector<std::future<double>> futuros;
    for (int i = 0; i < (int)zonas.size(); i++) {
        futuros.push_back(
            std::async(std::launch::async, calcularAreaAsync,
                zonas[i].largo, zonas[i].ancho)
        );
    }

    double total = 0.0;
    for (int i = 0; i < (int)futuros.size(); i++) {
        zonas[i].area = futuros[i].get();
        total += zonas[i].area;
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_estado.resOriginal.zonas = zonas;
        g_estado.resOriginal.superficieTotal = total;
        g_estado.resOriginal.calculado = true;
    }
    InvalidateRect(g_hWnd, NULL, TRUE);
}

// =============================================================================
// DIBUJO DE BATERIA
// =============================================================================

void dibujarBateria(HDC hdc, int x, int y, int ancho, int alto, double porcentaje) {
    int capAncho = 10;
    int capAlto = 6;
    dibujarRectRedondeado(hdc, x + ancho / 2 - capAncho / 2, y - capAlto,
        capAncho, capAlto + 3, RGB(100, 105, 130), RGB(120, 125, 150), 3);

    dibujarRectRedondeadoBorde(hdc, x, y, ancho, alto,
        COLOR_BAT_BG, RGB(100, 105, 130), 2, 6);

    COLORREF colorNivel;
    std::string estadoBat;
    if (porcentaje > 60) { colorNivel = COLOR_BAT_FULL; estadoBat = "OK"; }
    else if (porcentaje > 25) { colorNivel = COLOR_BAT_MED; estadoBat = "MEDIA"; }
    else if (porcentaje > 0) { colorNivel = COLOR_BAT_LOW; estadoBat = "BAJA!"; }
    else { colorNivel = RGB(80, 80, 80); estadoBat = "AGOTADA"; }

    int margen = 3;
    int numBarras = 8;
    int barraH = (alto - 2 * margen) / numBarras;
    int rellenoW = ancho - 2 * margen;
    int barrasLlenas = (int)(numBarras * porcentaje / 100.0);

    for (int i = 0; i < numBarras; i++) {
        int barraY = y + margen + (numBarras - 1 - i) * barraH;
        COLORREF colorBarra = (i < barrasLlenas) ? colorNivel : RGB(40, 42, 58);
        HBRUSH hBr = CreateSolidBrush(colorBarra);
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(30, 32, 48));
        HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBr);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        RoundRect(hdc, x + margen, barraY + 1,
            x + margen + rellenoW, barraY + barraH - 1, 2, 2);
        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBr);
        DeleteObject(hPen);
    }

    if (porcentaje > 0 && porcentaje <= 100) {
        int cx = x + ancho / 2;
        int cy = y + alto / 2;
        HPEN hPenRayo = CreatePen(PS_SOLID, 2, RGB(255, 255, 200));
        HPEN hOldP = (HPEN)SelectObject(hdc, hPenRayo);
        MoveToEx(hdc, cx + 2, cy - 8, NULL);
        LineTo(hdc, cx - 3, cy + 2);
        LineTo(hdc, cx + 3, cy - 2);
        LineTo(hdc, cx - 2, cy + 8);
        SelectObject(hdc, hOldP);
        DeleteObject(hPenRayo);
    }

    std::ostringstream ossPct;
    ossPct << (int)porcentaje << "%";
    dibujarTextoCentrado(hdc, ossPct.str(), x - 5, y + alto + 5, ancho + 10,
        colorNivel, 14, true);
    dibujarTextoCentrado(hdc, estadoBat, x - 10, y + alto + 22, ancho + 20,
        colorNivel, 11, false);
}

// =============================================================================
// BARRA DE PROGRESO
// =============================================================================

void dibujarBarraProgreso(HDC hdc, int x, int y, int w, int h,
    double progreso, COLORREF color) {
    dibujarRectRedondeado(hdc, x, y, w, h, COLOR_PROGRESO_BG, COLOR_BORDE, h / 2);
    int pw = (int)(w * progreso / 100.0);
    if (pw > 4) {
        dibujarRectRedondeado(hdc, x, y, pw, h, color, color, h / 2);
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << progreso << "%";
    dibujarTextoCentrado(hdc, oss.str(), x, y + 1, w, COLOR_TEXTO, h - 4, true);
}

// =============================================================================
// DIBUJO DE VISTA DE PLANTA (ZONAS ORIGINALES)
// =============================================================================

void dibujarVistaPlanta(HDC hdc, int panelX, int panelY, int panelW, int panelH) {
    dibujarRectRedondeado(hdc, panelX, panelY, panelW, panelH,
        COLOR_PANEL, COLOR_BORDE, 10);
    dibujarTexto(hdc, "ZONAS ORIGINALES (Vista de Planta)",
        panelX + 10, panelY + 8, COLOR_TITULO, 13, true);

    int drawX = panelX + 15;
    int drawY = panelY + 28;
    int drawW = panelW - 30;
    int drawH = panelH - 80;

    double habLargo = 590.0;
    double habAncho = 480.0;
    double escalaX = drawW / habLargo;
    double escalaY = drawH / habAncho;
    double escala = (escalaX < escalaY) ? escalaX : escalaY;

    int offsetX = drawX + (int)((drawW - habLargo * escala) / 2);
    int offsetY = drawY + (int)((drawH - habAncho * escala) / 2);

    // Contorno habitacion
    HPEN hPenHab = CreatePen(PS_DASH, 1, COLOR_TEXTO_DIM);
    HBRUSH hBrushHab = CreateSolidBrush(RGB(35, 35, 55));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPenHab);
    HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, hBrushHab);
    Rectangle(hdc, offsetX, offsetY,
        offsetX + (int)(habLargo * escala), offsetY + (int)(habAncho * escala));
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBr);
    DeleteObject(hPenHab);
    DeleteObject(hBrushHab);

    COLORREF coloresZona[] = { COLOR_ZONA1, COLOR_ZONA2, COLOR_ZONA3, COLOR_ZONA4 };
    COLORREF coloresZonaDim[] = {
        RGB(80, 110, 170), RGB(90, 150, 90),
        RGB(170, 150, 100), RGB(170, 80, 100)
    };

    struct ZonaPos { double x, y, largo, ancho; };
    ZonaPos posiciones[] = {
        {0, 0, 500, 150}, {0, 150, 480, 101},
        {0, 251, 309, 229}, {500, 0, 90, 220}
    };

    for (int i = 0; i < 4; i++) {
        int zx = offsetX + (int)(posiciones[i].x * escala);
        int zy = offsetY + (int)(posiciones[i].y * escala);
        int zw = (int)(posiciones[i].largo * escala);
        int zh = (int)(posiciones[i].ancho * escala);

        HBRUSH hBrZ = CreateSolidBrush(coloresZonaDim[i]);
        HPEN hPenZ = CreatePen(PS_SOLID, 2, coloresZona[i]);
        SelectObject(hdc, hBrZ);
        SelectObject(hdc, hPenZ);
        Rectangle(hdc, zx, zy, zx + zw, zy + zh);
        DeleteObject(hBrZ);
        DeleteObject(hPenZ);

        std::string label = "Z" + std::to_string(i + 1);
        dibujarTextoCentrado(hdc, label, zx, zy + zh / 2 - 6, zw,
            coloresZona[i], 12, true);
    }

    // Mueble
    int muebleX = offsetX + (int)(309 * escala);
    int muebleY = offsetY + (int)(251 * escala);
    int muebleW = (int)(281 * escala);
    int muebleH = (int)(229 * escala);
    HBRUSH hBrM = CreateSolidBrush(RGB(80, 73, 69));
    HPEN hPenM = CreatePen(PS_SOLID, 2, RGB(120, 110, 100));
    SelectObject(hdc, hBrM);
    SelectObject(hdc, hPenM);
    Rectangle(hdc, muebleX, muebleY, muebleX + muebleW, muebleY + muebleH);
    DeleteObject(hBrM);
    DeleteObject(hPenM);
    dibujarTextoCentrado(hdc, "MUEBLE", muebleX, muebleY + muebleH / 2 - 6,
        muebleW, RGB(180, 170, 160), 10, true);

    // Tabla datos originales
    int tabY = offsetY + (int)(habAncho * escala) + 8;
    double datosZ[][2] = { {500,150}, {480,101}, {309,480}, {90,220} };

    std::lock_guard<std::mutex> lock(g_mutex);
    for (int i = 0; i < 4; i++) {
        int lx = panelX + 10 + i * (panelW / 4);
        HBRUSH hBrL = CreateSolidBrush(coloresZona[i]);
        HBRUSH oldB = (HBRUSH)SelectObject(hdc, hBrL);
        Rectangle(hdc, lx, tabY, lx + 10, tabY + 10);
        SelectObject(hdc, oldB);
        DeleteObject(hBrL);

        std::ostringstream oss;
        oss << "Z" << (i + 1) << ":";
        if (g_estado.resOriginal.calculado && i < (int)g_estado.resOriginal.zonas.size()) {
            oss << formatearConMiles(g_estado.resOriginal.zonas[i].area);
        }
        else {
            oss << (int)datosZ[i][0] << "x" << (int)datosZ[i][1];
        }
        dibujarTexto(hdc, oss.str(), lx + 13, tabY - 1, COLOR_TEXTO, 11, false);
    }

    if (g_estado.resOriginal.calculado) {
        std::string totStr = "Total: " + formatearConMiles(g_estado.resOriginal.superficieTotal) + " cm2";
        dibujarTextoCentrado(hdc, totStr, panelX, tabY + 14, panelW, COLOR_ACENTO, 12, true);
    }
}

// =============================================================================
// FUNCION PRINCIPAL DE PINTADO
// =============================================================================

void pintarVentana(HDC hdc, RECT& rect) {
    int W = rect.right - rect.left;
    int H = rect.bottom - rect.top;

    g_areasClic.clear();

    HBRUSH hBrFondo = CreateSolidBrush(COLOR_FONDO);
    FillRect(hdc, &rect, hBrFondo);
    DeleteObject(hBrFondo);

    // =========================================================================
    // TITULO
    // =========================================================================
    dibujarTextoCentrado(hdc, "ROBOT ASPIRADOR INTELIGENTE",
        0, 8, W, COLOR_TITULO, 24, true);
    dibujarTextoCentrado(hdc, "Selecciona zona, tamano y modelo de Rumba para limpiar",
        0, 35, W, COLOR_TEXTO_DIM, 12, false);

    // =========================================================================
    // LAYOUT: 3 columnas
    //   Izquierda: Zonas + Tamano
    //   Centro: Rumbas + Eficiencia
    //   Derecha: Zonas originales (vista planta) + Botones
    // =========================================================================
    int margenX = 12;
    int col1X = margenX;
    int col1W = (W - margenX * 4) * 30 / 100;
    int col2X = col1X + col1W + margenX;
    int col2W = (W - margenX * 4) * 38 / 100;
    int col3X = col2X + col2W + margenX;
    int col3W = W - col3X - margenX;

    int topY = 52;

    // =========================================================================
    // COLUMNA 1: SELECCION DE ZONA + TAMANO
    // =========================================================================
    int secZonaH = 310;
    dibujarRectRedondeado(hdc, col1X, topY, col1W, secZonaH,
        COLOR_PANEL, COLOR_BORDE, 10);
    dibujarTexto(hdc, "1. ZONA A LIMPIAR", col1X + 10, topY + 6,
        COLOR_TITULO, 13, true);

    int cardH = 42;
    int cardMargen = 4;

    for (int i = 0; i < NUM_ZONAS; i++) {
        int cx = col1X + 8;
        int cy = topY + 26 + i * (cardH + cardMargen);
        int cw = col1W - 16;

        bool sel = (g_estado.zonaSeleccionada == i);
        COLORREF bg = sel ? COLOR_PANEL_SEL : RGB(40, 43, 62);
        COLORREF borde = sel ? g_zonas[i].color : COLOR_BORDE;

        dibujarRectRedondeadoBorde(hdc, cx, cy, cw, cardH,
            bg, borde, sel ? 2 : 1, 6);

        dibujarTexto(hdc, g_zonas[i].icono, cx + 6, cy + 3,
            g_zonas[i].color, 14, true);
        dibujarTexto(hdc, g_zonas[i].nombre, cx + 30, cy + 3,
            sel ? g_zonas[i].color : COLOR_TEXTO, 13, true);

        std::string dif;
        COLORREF colorDif;
        if (g_zonas[i].factorDificultad <= 0.9) { dif = "Facil"; colorDif = COLOR_ACENTO; }
        else if (g_zonas[i].factorDificultad <= 1.2) { dif = "Normal"; colorDif = COLOR_ACENTO2; }
        else if (g_zonas[i].factorDificultad <= 1.5) { dif = "Dificil"; colorDif = COLOR_ACENTO3; }
        else { dif = "Muy dificil"; colorDif = COLOR_ACENTO3; }
        dibujarTexto(hdc, dif, cx + 30, cy + 22, colorDif, 11, false);

        if (sel) dibujarTexto(hdc, "[OK]", cx + cw - 35, cy + 8, COLOR_ACENTO, 11, true);

        AreaClic ac;
        ac.rect = { cx, cy, cx + cw, cy + cardH };
        ac.indice = i;
        ac.tipo = 0;
        g_areasClic.push_back(ac);
    }

    int tamY = topY + 26 + NUM_ZONAS * (cardH + cardMargen) + 4;
    if (g_estado.zonaSeleccionada >= 0) {
        dibujarTexto(hdc, "Tamano:", col1X + 10, tamY, COLOR_TITULO, 12, true);
        InfoZona& zona = g_zonas[g_estado.zonaSeleccionada];
        int btnTamW = (col1W - 30) / 3;
        for (int i = 0; i < 3; i++) {
            int bx = col1X + 8 + i * (btnTamW + 3);
            int by = tamY + 16;
            bool sel = (g_estado.tamanoSeleccionado == i);
            COLORREF bg = sel ? COLOR_PANEL_SEL : RGB(40, 43, 62);
            COLORREF borde = sel ? COLOR_ACENTO2 : COLOR_BORDE;
            dibujarRectRedondeadoBorde(hdc, bx, by, btnTamW, 20, bg, borde, sel ? 2 : 1, 5);
            dibujarTextoCentrado(hdc, zona.tamanos[i], bx, by + 3, btnTamW,
                sel ? COLOR_ACENTO2 : COLOR_TEXTO_DIM, 10, sel);

            AreaClic ac;
            ac.rect = { bx, by, bx + btnTamW, by + 20 };
            ac.indice = i;
            ac.tipo = 2;
            g_areasClic.push_back(ac);
        }
    }

    // =========================================================================
    // COLUMNA 2: SELECCION DE RUMBA
    // =========================================================================
    int secRumbaH = 310;
    dibujarRectRedondeado(hdc, col2X, topY, col2W, secRumbaH,
        COLOR_PANEL, COLOR_BORDE, 10);
    dibujarTexto(hdc, "2. MODELO DE RUMBA", col2X + 10, topY + 6,
        COLOR_TITULO, 13, true);

    int rCardH = 62;
    for (int i = 0; i < NUM_RUMBAS; i++) {
        int rx = col2X + 8;
        int ry = topY + 26 + i * (rCardH + 4);
        int rw = col2W - 16;

        bool sel = (g_estado.rumbaSeleccionada == i);
        COLORREF bg = sel ? COLOR_PANEL_SEL : RGB(40, 43, 62);
        COLORREF borde = sel ? g_rumbas[i].color : COLOR_BORDE;

        dibujarRectRedondeadoBorde(hdc, rx, ry, rw, rCardH,
            bg, borde, sel ? 2 : 1, 6);

        dibujarTexto(hdc, g_rumbas[i].nombre, rx + 8, ry + 3,
            sel ? g_rumbas[i].color : COLOR_TEXTO, 13, true);
        dibujarTexto(hdc, g_rumbas[i].descripcion, rx + 8, ry + 20,
            COLOR_TEXTO_DIM, 10, false);
        dibujarTexto(hdc, "Mejor en: " + g_rumbas[i].especialidad,
            rx + 8, ry + 34, COLOR_ACENTO2, 10, true);

        std::ostringstream ossInfo;
        ossInfo << "Bat:" << (int)g_rumbas[i].bateriaMah << "mAh | Vel:"
            << (int)g_rumbas[i].velocidadBase << "cm2/s";
        dibujarTexto(hdc, ossInfo.str(), rx + 8, ry + 48, COLOR_TEXTO_DIM, 10, false);

        if (g_estado.zonaSeleccionada >= 0) {
            double efic = g_rumbas[i].eficiencia[g_estado.zonaSeleccionada] * 100;
            std::ostringstream ossE;
            ossE << (int)efic << "%";
            COLORREF ce = efic >= 130 ? COLOR_ACENTO : (efic >= 100 ? COLOR_ACENTO2 : COLOR_ACENTO3);
            dibujarTexto(hdc, ossE.str(), rx + rw - 40, ry + 5, ce, 13, true);
        }

        if (sel) dibujarTexto(hdc, "[OK]", rx + rw - 40, ry + 48, COLOR_ACENTO, 10, true);

        AreaClic ac;
        ac.rect = { rx, ry, rx + rw, ry + rCardH };
        ac.indice = i;
        ac.tipo = 1;
        g_areasClic.push_back(ac);
    }

    if (g_estado.zonaSeleccionada >= 0 && g_estado.rumbaSeleccionada >= 0) {
        InfoRumba& rumba = g_rumbas[g_estado.rumbaSeleccionada];
        InfoZona& zona = g_zonas[g_estado.zonaSeleccionada];
        double efic = rumba.eficiencia[g_estado.zonaSeleccionada];

        int msgY = topY + 26 + NUM_RUMBAS * (rCardH + 4) + 2;
        std::string msgEfic;
        COLORREF colorMsg;
        if (efic >= 1.3) {
            msgEfic = "EXCELENTE! Ideal para " + zona.nombre;
            colorMsg = COLOR_ACENTO;
        }
        else if (efic >= 1.0) {
            msgEfic = "BUENO para " + zona.nombre;
            colorMsg = COLOR_ACENTO2;
        }
        else if (efic >= 0.8) {
            msgEfic = "ACEPTABLE en " + zona.nombre;
            colorMsg = COLOR_ACENTO2;
        }
        else {
            msgEfic = "NO RECOMENDADO para " + zona.nombre;
            colorMsg = COLOR_ACENTO3;
        }
        dibujarTexto(hdc, msgEfic, col2X + 10, msgY, colorMsg, 12, true);
    }

    // =========================================================================
    // COLUMNA 3: ZONAS ORIGINALES (vista de planta) + BOTONES DE CONTROL
    // =========================================================================
    int vistaPlantaH = secRumbaH - 50; // Dejar espacio para botones
    dibujarVistaPlanta(hdc, col3X, topY, col3W, vistaPlantaH);

    // --- BOTONES DE CONTROL (debajo de la vista de planta, columna derecha) ---
    int btnAreaY = topY + vistaPlantaH + 6;
    int btnW = (col3W - 12) / 2;  // 2 botones por fila
    int btnH = 26;

    if (g_hBtnIniciar) {
        // Fila 1: Iniciar Limpieza (ancho completo)
        SetWindowPos(g_hBtnIniciar, NULL,
            col3X, btnAreaY,
            col3W, btnH, SWP_NOZORDER);

        // Fila 2: Detener | Abortar Proceso
        SetWindowPos(g_hBtnDetener, NULL,
            col3X, btnAreaY + btnH + 4,
            btnW, btnH, SWP_NOZORDER);

        SetWindowPos(g_hBtnAbortar, NULL,
            col3X + btnW + 6, btnAreaY + btnH + 4,
            col3W - btnW - 6, btnH, SWP_NOZORDER);

        // Fila 3: Reiniciar (ancho completo)
        SetWindowPos(g_hBtnReset, NULL,
            col3X, btnAreaY + (btnH + 4) * 2,
            col3W, btnH, SWP_NOZORDER);
    }

    // =========================================================================
    // PANEL INFERIOR: LIMPIEZA + BATERIA
    // =========================================================================
    int panelLimpY = topY + secZonaH + 8;
    int panelLimpH = H - panelLimpY - 22;
    int panelLimpW = W - 2 * margenX;

    dibujarRectRedondeado(hdc, margenX, panelLimpY, panelLimpW, panelLimpH,
        COLOR_PANEL, COLOR_BORDE, 10);
    dibujarTexto(hdc, "3. PANEL DE LIMPIEZA", margenX + 12, panelLimpY + 6,
        COLOR_TITULO, 13, true);

    {
        std::lock_guard<std::mutex> lock(g_mutex);

        // Bateria (lado derecho)
        int batX = margenX + panelLimpW - 70;
        int batY = panelLimpY + 28;
        int batW = 40;
        int batH = panelLimpH - 65;
        if (batH > 100) batH = 100;
        if (batH < 40) batH = 40;

        dibujarTexto(hdc, "BATERIA", batX - 5, batY - 14, COLOR_TEXTO_DIM, 11, true);
        dibujarBateria(hdc, batX, batY, batW, batH, g_estado.bateriaPorcentaje);

        if (g_estado.bateriaMaxMah > 0) {
            std::ostringstream ossBatInfo;
            ossBatInfo << (int)g_estado.bateriaActualMah << "/" << (int)g_estado.bateriaMaxMah;
            dibujarTextoCentrado(hdc, ossBatInfo.str(), batX - 15, batY + batH + 40,
                batW + 30, COLOR_TEXTO_DIM, 10, false);
        }

        // Informacion central
        int infoX = margenX + 16;
        int infoY = panelLimpY + 26;
        int infoW = panelLimpW - 110;

        if (g_estado.limpiando || g_estado.limpiezaCompleta ||
            g_estado.bateriaAgotada || g_estado.detenidoPrematuramente) {
            COLORREF colorEstado;
            if (g_estado.limpiezaCompleta) colorEstado = COLOR_ACENTO;
            else if (g_estado.bateriaAgotada) colorEstado = COLOR_ACENTO3;
            else if (g_estado.detenidoPrematuramente) colorEstado = COLOR_ACENTO2;
            else colorEstado = COLOR_ACENTO2;

            dibujarTexto(hdc, g_estado.mensajeEstado, infoX, infoY, colorEstado, 14, true);

            COLORREF colorBarra = COLOR_PROGRESO;
            if (g_estado.bateriaAgotada) colorBarra = COLOR_ACENTO3;
            else if (g_estado.detenidoPrematuramente) colorBarra = COLOR_ACENTO2;

            dibujarBarraProgreso(hdc, infoX, infoY + 22, infoW - 20, 20,
                g_estado.progreso, colorBarra);

            int sY = infoY + 48;
            std::ostringstream o1, o2, o3, o4;
            o1 << "Area: " << formatearConMiles(g_estado.areaTotal) << " cm2 ("
                << std::fixed << std::setprecision(2) << g_estado.areaTotal / 10000.0 << " m2)";
            o2 << "Limpiado: " << formatearConMiles(g_estado.areaLimpiada) << " cm2 ("
                << std::setprecision(1) << g_estado.progreso << "%)";
            o3 << "Velocidad: " << std::setprecision(0) << g_estado.velocidadEfectiva << " cm2/s";
            o4 << "Tiempo: " << std::setprecision(1) << g_estado.tiempoTranscurrido << "s / "
                << std::setprecision(1) << g_estado.tiempoEstimado << "s est.";

            dibujarTexto(hdc, o1.str(), infoX, sY, COLOR_TEXTO, 12, false);
            dibujarTexto(hdc, o2.str(), infoX, sY + 16, COLOR_TEXTO, 12, false);
            dibujarTexto(hdc, o3.str(), infoX + infoW / 2, sY, COLOR_ACENTO2, 12, false);
            dibujarTexto(hdc, o4.str(), infoX + infoW / 2, sY + 16, COLOR_TEXTO, 12, false);

            if (!g_estado.mensajeBateria.empty()) {
                COLORREF cB = g_estado.bateriaAgotada ? COLOR_ACENTO3 :
                    (g_estado.detenidoPrematuramente ? COLOR_ACENTO2 : COLOR_ACENTO2);
                dibujarTexto(hdc, g_estado.mensajeBateria, infoX, sY + 36, cB, 12, true);
            }

        }
        else {
            if (g_estado.zonaSeleccionada < 0 || g_estado.rumbaSeleccionada < 0) {
                dibujarTexto(hdc, "Selecciona una zona y una Rumba para comenzar",
                    infoX, infoY + 10, COLOR_TEXTO_DIM, 14, false);
                if (g_estado.zonaSeleccionada < 0)
                    dibujarTexto(hdc, "-> Falta seleccionar zona", infoX + 15, infoY + 32,
                        COLOR_ACENTO3, 12, false);
                if (g_estado.rumbaSeleccionada < 0)
                    dibujarTexto(hdc, "-> Falta seleccionar rumba", infoX + 15, infoY + 50,
                        COLOR_ACENTO3, 12, false);
            }
            else {
                InfoZona& zona = g_zonas[g_estado.zonaSeleccionada];
                InfoRumba& rumba = g_rumbas[g_estado.rumbaSeleccionada];
                double area = zona.areaBase * zona.multiplicadores[g_estado.tamanoSeleccionado];
                double efic = rumba.eficiencia[g_estado.zonaSeleccionada];
                double vel = rumba.velocidadBase * efic / zona.factorDificultad;
                double tiempo = area / vel;
                double batNecesaria = area * rumba.consumoPorCm2;
                bool alcanza = batNecesaria <= rumba.bateriaMah;

                dibujarTexto(hdc, "Listo para limpiar:", infoX, infoY, COLOR_ACENTO, 14, true);

                std::ostringstream oi;
                oi << zona.nombre << " (" << zona.tamanos[g_estado.tamanoSeleccionado]
                    << ") con " << rumba.nombre;
                dibujarTexto(hdc, oi.str(), infoX, infoY + 18, COLOR_TEXTO, 12, false);

                std::ostringstream oa, ot, ob;
                oa << "Area: " << formatearConMiles(area) << " cm2 ("
                    << std::fixed << std::setprecision(2) << area / 10000.0 << " m2)";
                ot << "Tiempo estimado: " << std::setprecision(1) << tiempo << " seg ("
                    << std::setprecision(1) << tiempo / 60.0 << " min)";
                ob << "Bateria necesaria: " << std::setprecision(0) << batNecesaria
                    << " / " << (int)rumba.bateriaMah << " mAh";

                dibujarTexto(hdc, oa.str(), infoX, infoY + 36, COLOR_TEXTO, 12, false);
                dibujarTexto(hdc, ot.str(), infoX, infoY + 52, COLOR_TEXTO, 12, false);
                dibujarTexto(hdc, ob.str(), infoX, infoY + 68,
                    alcanza ? COLOR_ACENTO : COLOR_ACENTO3, 12, true);

                if (!alcanza) {
                    double pct = (rumba.bateriaMah / batNecesaria) * 100;
                    std::ostringstream ow;
                    ow << "ADVERTENCIA: Bateria solo alcanza para " << std::setprecision(0)
                        << pct << "% del area!";
                    dibujarTexto(hdc, ow.str(), infoX, infoY + 86, COLOR_ACENTO3, 12, true);
                }
            }
        }
    }

    // Pie
    dibujarTextoCentrado(hdc, "Robot Aspirador v2.0 | Zonas + Rumbas + Bateria | C++ Win32",
        0, H - 18, W, COLOR_TEXTO_DIM, 10, false);
}

// =============================================================================
// FUNCION DE LIMPIEZA (hilo separado)
// =============================================================================

void ejecutarLimpieza() {
    int zonaIdx, rumbaIdx, tamIdx;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        zonaIdx = g_estado.zonaSeleccionada;
        rumbaIdx = g_estado.rumbaSeleccionada;
        tamIdx = g_estado.tamanoSeleccionado;
    }
    if (zonaIdx < 0 || rumbaIdx < 0) return;

    InfoZona& zona = g_zonas[zonaIdx];
    InfoRumba& rumba = g_rumbas[rumbaIdx];

    double area = zona.areaBase * zona.multiplicadores[tamIdx];
    double efic = rumba.eficiencia[zonaIdx];
    double velEfectiva = rumba.velocidadBase * efic / zona.factorDificultad;
    double tiempoEstimado = area / velEfectiva;
    double consumoTotal = area * rumba.consumoPorCm2;

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_estado.limpiando = true;
        g_estado.limpiezaCompleta = false;
        g_estado.bateriaAgotada = false;
        g_estado.detenidoPrematuramente = false;
        g_estado.areaTotal = area;
        g_estado.areaLimpiada = 0;
        g_estado.progreso = 0;
        g_estado.tiempoEstimado = tiempoEstimado;
        g_estado.tiempoTranscurrido = 0;
        g_estado.velocidadEfectiva = velEfectiva;
        g_estado.bateriaMaxMah = rumba.bateriaMah;
        g_estado.bateriaActualMah = rumba.bateriaMah;
        g_estado.bateriaPorcentaje = 100.0;
        g_estado.eficienciaActual = efic * 100;
        g_estado.esZonaOptima = (zonaIdx == (int)rumba.zonaMejor);
        g_estado.mensajeEstado = "Limpiando " + zona.nombre + "...";
        g_estado.mensajeBateria = "";
    }

    int numSubareas = 10;
    double areaPorSub = area / numSubareas;
    double consumoPorSub = consumoTotal / numSubareas;

    for (int s = 0; s < numSubareas; s++) {
        if (g_cancelarLimpieza || g_abortarProceso) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_estado.limpiando = false;
            if (g_abortarProceso) {
                g_estado.detenidoPrematuramente = true;
                g_estado.mensajeEstado = "PROCESO ABORTADO! Limpieza detenida al "
                    + formatearNumero(g_estado.progreso, 1) + "%";
                g_estado.mensajeBateria = "El usuario detuvo el proceso prematuramente. "
                    "Bateria restante: " + formatearNumero(g_estado.bateriaPorcentaje, 1) + "%";
            }
            else {
                g_estado.detenidoPrematuramente = true;
                g_estado.mensajeEstado = "Limpieza detenida por el usuario ("
                    + formatearNumero(g_estado.progreso, 1) + "% completado)";
                g_estado.mensajeBateria = "Detenido. Bateria restante: "
                    + formatearNumero(g_estado.bateriaPorcentaje, 1) + "%";
            }
            InvalidateRect(g_hWnd, NULL, TRUE);
            return;
        }

        auto futuro = std::async(std::launch::async, [&]() {
            double tiempoSub = areaPorSub / velEfectiva;
            int pasos = 8;
            for (int p = 0; p < pasos && !g_cancelarLimpieza && !g_abortarProceso; p++) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds((int)(tiempoSub * 100 / pasos)));
            }
            });
        futuro.get();

        if (g_cancelarLimpieza || g_abortarProceso) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_estado.limpiando = false;
            g_estado.detenidoPrematuramente = true;
            if (g_abortarProceso) {
                g_estado.mensajeEstado = "PROCESO ABORTADO! Detenido al "
                    + formatearNumero(g_estado.progreso, 1) + "%";
                g_estado.mensajeBateria = "Abortado por el usuario. Bateria: "
                    + formatearNumero(g_estado.bateriaPorcentaje, 1) + "%";
            }
            else {
                g_estado.mensajeEstado = "Limpieza detenida ("
                    + formatearNumero(g_estado.progreso, 1) + "% completado)";
                g_estado.mensajeBateria = "Detenido. Bateria: "
                    + formatearNumero(g_estado.bateriaPorcentaje, 1) + "%";
            }
            InvalidateRect(g_hWnd, NULL, TRUE);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_estado.areaLimpiada += areaPorSub;
            g_estado.bateriaActualMah -= consumoPorSub;
            g_estado.tiempoTranscurrido += areaPorSub / velEfectiva;

            if (g_estado.bateriaActualMah < 0) g_estado.bateriaActualMah = 0;
            g_estado.bateriaPorcentaje = (g_estado.bateriaActualMah / g_estado.bateriaMaxMah) * 100;
            g_estado.progreso = (g_estado.areaLimpiada / g_estado.areaTotal) * 100;

            if (g_estado.bateriaPorcentaje <= 10 && g_estado.bateriaPorcentaje > 0)
                g_estado.mensajeBateria = "!! BATERIA CRITICA !!";
            else if (g_estado.bateriaPorcentaje <= 25)
                g_estado.mensajeBateria = "! Bateria baja !";
            else if (g_estado.bateriaPorcentaje <= 50)
                g_estado.mensajeBateria = "Bateria al 50%...";
            else
                g_estado.mensajeBateria = "";

            if (g_estado.bateriaActualMah <= 0) {
                g_estado.bateriaAgotada = true;
                g_estado.limpiando = false;
                g_estado.bateriaPorcentaje = 0;
                g_estado.mensajeEstado = "BATERIA AGOTADA! Limpieza incompleta ("
                    + formatearNumero(g_estado.progreso, 1) + "%)";
                g_estado.mensajeBateria = "Rumba detenida. Usa area mas pequena o Rumba con mas bateria.";
                InvalidateRect(g_hWnd, NULL, TRUE);
                return;
            }

            if (g_estado.esZonaOptima)
                g_estado.mensajeEstado = "Limpiando " + zona.nombre + " [ZONA OPTIMA!]";
            else
                g_estado.mensajeEstado = "Limpiando " + zona.nombre + "...";
        }
        InvalidateRect(g_hWnd, NULL, TRUE);
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_estado.limpiando = false;
        g_estado.limpiezaCompleta = true;
        g_estado.progreso = 100.0;
        g_estado.areaLimpiada = g_estado.areaTotal;
        g_estado.mensajeEstado = "LIMPIEZA COMPLETADA! " + zona.nombre + " impecable";
        std::ostringstream ossFin;
        ossFin << "Bateria restante: " << std::fixed << std::setprecision(1)
            << g_estado.bateriaPorcentaje << "% | Tiempo: "
            << std::setprecision(1) << g_estado.tiempoTranscurrido << "s";
        g_estado.mensajeBateria = ossFin.str();
    }
    InvalidateRect(g_hWnd, NULL, TRUE);
}

// =============================================================================
// PROCEDIMIENTO DE VENTANA
// =============================================================================

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFontBtn = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, "Segoe UI");

        // Botones - posicion temporal, se reubican en pintarVentana
        g_hBtnIniciar = CreateWindowA("BUTTON", "INICIAR LIMPIEZA",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            10, 800, 160, 28, hWnd, (HMENU)ID_BTN_INICIAR, NULL, NULL);
        g_hBtnDetener = CreateWindowA("BUTTON", "DETENER",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            180, 800, 110, 28, hWnd, (HMENU)ID_BTN_DETENER, NULL, NULL);
        g_hBtnAbortar = CreateWindowA("BUTTON", "ABORTAR PROCESO",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            300, 800, 130, 28, hWnd, (HMENU)ID_BTN_ABORTAR, NULL, NULL);
        g_hBtnReset = CreateWindowA("BUTTON", "REINICIAR TODO",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            440, 800, 110, 28, hWnd, (HMENU)ID_BTN_RESET, NULL, NULL);

        SendMessage(g_hBtnIniciar, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        SendMessage(g_hBtnDetener, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        SendMessage(g_hBtnAbortar, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        SendMessage(g_hBtnReset, WM_SETFONT, (WPARAM)hFontBtn, TRUE);
        EnableWindow(g_hBtnDetener, FALSE);
        EnableWindow(g_hBtnAbortar, FALSE);

        // Calcular zonas originales al iniciar
        std::thread t(calcularZonasOriginales);
        t.detach();

        return 0;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case ID_BTN_INICIAR: {
            bool puede;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                puede = (g_estado.zonaSeleccionada >= 0 &&
                    g_estado.rumbaSeleccionada >= 0 &&
                    !g_estado.limpiando);
            }
            if (puede) {
                g_cancelarLimpieza = false;
                g_abortarProceso = false;
                EnableWindow(g_hBtnIniciar, FALSE);
                EnableWindow(g_hBtnDetener, TRUE);
                EnableWindow(g_hBtnAbortar, TRUE);
                std::thread t([]() {
                    ejecutarLimpieza();
                    EnableWindow(g_hBtnIniciar, TRUE);
                    EnableWindow(g_hBtnDetener, FALSE);
                    EnableWindow(g_hBtnAbortar, FALSE);
                    });
                t.detach();
            }
            else {
                MessageBoxA(hWnd,
                    "Selecciona una zona y una Rumba antes de iniciar la limpieza.",
                    "Robot Aspirador", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case ID_BTN_DETENER: {
            g_cancelarLimpieza = true;
            EnableWindow(g_hBtnDetener, FALSE);
            EnableWindow(g_hBtnAbortar, FALSE);
            break;
        }
        case ID_BTN_ABORTAR: {
            int resp = MessageBoxA(hWnd,
                "Deseas ABORTAR el proceso de limpieza?\n\n"
                "El progreso actual se perdera y la Rumba se detendra inmediatamente.",
                "Abortar Proceso", MB_YESNO | MB_ICONWARNING);
            if (resp == IDYES) {
                g_abortarProceso = true;
                g_cancelarLimpieza = true;
                EnableWindow(g_hBtnDetener, FALSE);
                EnableWindow(g_hBtnAbortar, FALSE);
            }
            break;
        }
        case ID_BTN_RESET: {
            g_cancelarLimpieza = true;
            g_abortarProceso = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                auto resOrig = g_estado.resOriginal;
                g_estado = EstadoApp();
                g_estado.resOriginal = resOrig;
            }
            g_cancelarLimpieza = false;
            g_abortarProceso = false;
            EnableWindow(g_hBtnIniciar, TRUE);
            EnableWindow(g_hBtnDetener, FALSE);
            EnableWindow(g_hBtnAbortar, FALSE);
            InvalidateRect(hWnd, NULL, TRUE);
            break;
        }
        }
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = LOWORD(lParam);
        int my = HIWORD(lParam);

        bool limpiando;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            limpiando = g_estado.limpiando;
        }
        if (limpiando) break;

        for (auto& ac : g_areasClic) {
            if (mx >= ac.rect.left && mx <= ac.rect.right &&
                my >= ac.rect.top && my <= ac.rect.bottom) {

                std::lock_guard<std::mutex> lock(g_mutex);
                if (ac.tipo == 0) {
                    g_estado.zonaSeleccionada = ac.indice;
                    g_estado.limpiezaCompleta = false;
                    g_estado.bateriaAgotada = false;
                    g_estado.detenidoPrematuramente = false;
                    g_estado.progreso = 0;
                }
                else if (ac.tipo == 1) {
                    g_estado.rumbaSeleccionada = ac.indice;
                    g_estado.limpiezaCompleta = false;
                    g_estado.bateriaAgotada = false;
                    g_estado.detenidoPrematuramente = false;
                    g_estado.progreso = 0;
                    g_estado.bateriaPorcentaje = 100;
                }
                else if (ac.tipo == 2) {
                    g_estado.tamanoSeleccionado = ac.indice;
                    g_estado.limpiezaCompleta = false;
                    g_estado.bateriaAgotada = false;
                    g_estado.detenidoPrematuramente = false;
                    g_estado.progreso = 0;
                }
                break;
            }
        }
        InvalidateRect(hWnd, NULL, TRUE);
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
        g_cancelarLimpieza = true;
        g_abortarProceso = true;
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
}

// =============================================================================
// PUNTO DE ENTRADA
// =============================================================================

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {

    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(COLOR_FONDO);
    wc.lpszClassName = "RobotAspiradorV2";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "Error al registrar la clase de ventana", "Error", MB_OK);
        return 1;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 1100;
    int winH = 750;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    g_hWnd = CreateWindowExA(
        0,
        "RobotAspiradorV2",
        "Robot Aspirador v2.0 - Zona, Rumba y Bateria",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd) {
        MessageBoxA(NULL, "Error al crear la ventana", "Error", MB_OK);
        return 1;
    }

    SetWindowTextA(g_hWnd, "Robot Aspirador v2.0 - Zona, Rumba y Bateria");

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