#include <thread>
#include <future>
#include <chrono>
#include <cstdlib> // para rand()

#include "Robot.h"

Robot::Robot(const std::string& n, double tasa) : nombre(n), tasaLimpiezaCm2ps(tasa) {}

static double calcularAreaInterna(double largo, double ancho) {
    // Simular procesamiento distribuido
    std::this_thread::sleep_for(std::chrono::milliseconds(150 + rand() % 100));
    return largo * ancho;
}

ResultadoCalculo Robot::ejecutarCalculoDistribuido(const std::vector<Zona>& zonasParaCalcular) {
    ResultadoCalculo resultado;
    resultado.tasaLimpieza = this->tasaLimpiezaCm2ps;

    std::vector<std::future<double>> futuros;
    std::vector<Zona> zonasCopia = zonasParaCalcular;

    for (const auto& zona : zonasCopia) {
        futuros.push_back(
            std::async(std::launch::async, calcularAreaInterna, zona.largo, zona.ancho)
        );
    }

    for (size_t i = 0; i < futuros.size(); ++i) {
        zonasCopia[i].area = futuros[i].get();
        resultado.superficieTotal += zonasCopia[i].area;
    }

    if (resultado.tasaLimpieza > 0) {
        resultado.tiempoSegundos = resultado.superficieTotal / resultado.tasaLimpieza;
        resultado.tiempoMinutos = resultado.tiempoSegundos / 60.0;
        resultado.tiempoHoras = resultado.tiempoMinutos / 60.0;
    }

    resultado.zonasCalculadas = zonasCopia;
    resultado.calculado = true;

    return resultado;
}