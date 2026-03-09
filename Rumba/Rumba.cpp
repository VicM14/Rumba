

/**********************************************************************
PROYECTO: SIMULADOR DE ROBOT ASPIRADOR CON CALCULO DISTRIBUIDO
AUTOR: Proyecto académico de programación concurrente
DESCRIPCIÓN:

Este programa simula el cálculo de la superficie que debe limpiar
un robot aspirador dentro de una habitación dividida en zonas.

Cada zona se procesa de forma concurrente mediante programación
paralela en C++ utilizando std::async y std::future.

FUNCIONALIDADES PRINCIPALES:

1. Representación de zonas mediante estructuras y map
2. Cálculo de área por zona
3. Procesamiento concurrente
4. Suma de superficies
5. Estimación del tiempo de limpieza
6. Entrada dinámica de zonas
7. Lectura opcional desde archivo
8. Manejo de errores
9. Simulación de control del robot
10. Estadísticas finales del sistema

**********************************************************************/

#include <iostream>
#include <map>
#include <vector>
#include <future>
#include <thread>
#include <chrono>
#include <fstream>
#include <iomanip>

using namespace std;

/**********************************************************************
ESTRUCTURA ZONA
Representa una zona de limpieza
**********************************************************************/
struct Zona
{
    double largo;
    double ancho;
};

/**********************************************************************
FUNCION: calcularArea
Recibe largo y ancho y devuelve el área
**********************************************************************/
double calcularArea(double largo, double ancho)
{
    if (largo <= 0 || ancho <= 0)
        throw invalid_argument("Dimensiones invalidas");

    double area = largo * ancho;

    /* Simulación de procesamiento pesado */
    this_thread::sleep_for(chrono::milliseconds(200));

    return area;
}

/**********************************************************************
FUNCION: mostrarLineaSeparadora
**********************************************************************/
void mostrarLinea()
{
    cout << "---------------------------------------------------\n";
}

/**********************************************************************
FUNCION: convertirTiempo
Convierte segundos a minutos y horas
**********************************************************************/
void convertirTiempo(double segundos)
{
    double minutos = segundos / 60.0;
    double horas = segundos / 3600.0;

    cout << "\nTiempo estimado de limpieza\n";
    mostrarLinea();
    cout << "Segundos : " << segundos << endl;
    cout << "Minutos  : " << minutos << endl;
    cout << "Horas    : " << horas << endl;
}

/**********************************************************************
FUNCION: leerZonasUsuario
Permite introducir zonas manualmente
**********************************************************************/
map<string, Zona> leerZonasUsuario()
{
    map<string, Zona> zonas;

    int cantidad;
    cout << "\nCuantas zonas deseas introducir? ";
    cin >> cantidad;

    for (int i = 0; i < cantidad; i++)
    {
        string nombre;
        Zona z;

        cout << "\nNombre zona: ";
        cin >> nombre;

        cout << "Largo (cm): ";
        cin >> z.largo;

        cout << "Ancho (cm): ";
        cin >> z.ancho;

        zonas[nombre] = z;
    }

    return zonas;
}

/**********************************************************************
FUNCION: leerZonasArchivo
Lee zonas desde un archivo de texto

Formato del archivo:
Zona1 500 150
Zona2 480 101
**********************************************************************/
map<string, Zona> leerZonasArchivo(string nombreArchivo)
{
    map<string, Zona> zonas;

    ifstream archivo(nombreArchivo);

    if (!archivo)
    {
        cout << "No se pudo abrir el archivo\n";
        return zonas;
    }

    string nombre;
    Zona z;

    while (archivo >> nombre >> z.largo >> z.ancho)
    {
        zonas[nombre] = z;
    }

    archivo.close();

    return zonas;
}

/**********************************************************************
FUNCION: mostrarZonas
**********************************************************************/
void mostrarZonas(const map<string, Zona>& zonas)
{
    cout << "\nZONAS DEFINIDAS\n";
    mostrarLinea();

    for (auto& z : zonas)
    {
        cout << z.first
            << " -> Largo: " << z.second.largo
            << " cm  Ancho: " << z.second.ancho
            << " cm\n";
    }
}

/**********************************************************************
FUNCION: calcularAreasConcurrentemente
Realiza el cálculo distribuido
**********************************************************************/
map<string, double> calcularAreasConcurrentemente(map<string, Zona> zonas)
{
    map<string, double> resultados;

    vector<future<double>> tareas;
    vector<string> nombres;

    cout << "\nIniciando procesamiento concurrente...\n";

    for (auto& z : zonas)
    {
        nombres.push_back(z.first);

        tareas.push_back(
            async(launch::async,
                calcularArea,
                z.second.largo,
                z.second.ancho)
        );
    }

    for (size_t i = 0; i < tareas.size(); i++)
    {
        try
        {
            double area = tareas[i].get();

            resultados[nombres[i]] = area;

            cout << "Area calculada de "
                << nombres[i]
                << " = "
                << area
                << " cm²\n";
        }
        catch (exception& e)
        {
            cout << "Error calculando "
                << nombres[i]
                << " : "
                << e.what()
                << endl;
        }
    }

    return resultados;
}

/**********************************************************************
FUNCION: calcularSuperficieTotal
**********************************************************************/
double calcularSuperficieTotal(map<string, double> areas)
{
    double total = 0;

    for (auto& a : areas)
        total += a.second;

    return total;
}

/**********************************************************************
FUNCION: controlRobot
Simula inicio y parada del robot
**********************************************************************/
void controlRobot()
{
    cout << "\nSistema de control del robot\n";
    mostrarLinea();

    cout << "Robot listo para iniciar limpieza...\n";

    this_thread::sleep_for(chrono::milliseconds(800));

    cout << "Robot iniciando sistema de limpieza...\n";

    this_thread::sleep_for(chrono::milliseconds(800));
}

/**********************************************************************
FUNCION PRINCIPAL
**********************************************************************/
int main()
{
    cout << "=============================================\n";
    cout << "SIMULADOR ROBOT ASPIRADOR - SISTEMA DISTRIBUIDO\n";
    cout << "=============================================\n";

    map<string, Zona> zonas;

    int opcion;

    cout << "\n1. Usar zonas predefinidas\n";
    cout << "2. Introducir zonas manualmente\n";
    cout << "3. Leer zonas desde archivo\n";

    cout << "Selecciona opcion: ";
    cin >> opcion;

    if (opcion == 1)
    {
        zonas["Zona1"] = { 500,150 };
        zonas["Zona2"] = { 480,101 };
        zonas["Zona3"] = { 309,480 };
        zonas["Zona4"] = { 90,220 };
    }
    else if (opcion == 2)
    {
        zonas = leerZonasUsuario();
    }
    else if (opcion == 3)
    {
        zonas = leerZonasArchivo("zonas.txt");
    }
    else
    {
        cout << "Opcion invalida\n";
        return 0;
    }

    mostrarZonas(zonas);

    controlRobot();

    /* Calculo concurrente */
    map<string, double> areas = calcularAreasConcurrentemente(zonas);

    mostrarLinea();

    double superficieTotal = calcularSuperficieTotal(areas);

    cout << "\nSUPERFICIE TOTAL A LIMPIAR\n";
    cout << superficieTotal << " cm²\n";

    /* Tasa de limpieza */
    double tasaLimpieza;

    cout << "\nIntroduce tasa de limpieza (cm²/seg): ";
    cin >> tasaLimpieza;

    if (tasaLimpieza <= 0)
    {
        cout << "Tasa invalida\n";
        return 0;
    }

    double tiempo = superficieTotal / tasaLimpieza;

    convertirTiempo(tiempo);

    mostrarLinea();

    cout << "\nRobot finalizo el calculo de limpieza\n";
    cout << "Sistema apagandose...\n";

    this_thread::sleep_for(chrono::milliseconds(1000));

    cout << "Programa finalizado correctamente\n";

    return 0;
}


