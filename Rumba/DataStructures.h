#pragma once
#include <string>
#include <vector>

/**
 * Estructura que representa una zona editable de la habitacion.
 */
struct Zona {
    std::string nombre;
    double largo;   // en centímetros
    double ancho;   // en centímetros
    double area;    // en centímetros cuadrados (calculada)
    bool isSelected; // Para seleccionar qué zonas limpiar

    Zona(const std::string& n, double l, double a)
        : nombre(n), largo(l), ancho(a), area(0.0), isSelected(true) {
    }
};

/**
 * Estructura que almacena los resultados del cálculo.
 */
struct ResultadoCalculo {
    std::vector<Zona> zonasCalculadas;
    double superficieTotal;
    double tasaLimpieza;        // cm2/segundo
    double tiempoSegundos;
    double tiempoMinutos;
    double tiempoHoras;
    bool calculado;

    ResultadoCalculo() : superficieTotal(0), tasaLimpieza(1000),
        tiempoSegundos(0), tiempoMinutos(0),
        tiempoHoras(0), calculado(false) {
    }
};