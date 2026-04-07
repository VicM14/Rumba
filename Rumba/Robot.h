#pragma once
#include <string>
#include <vector>

#include "DataStructures.h" // <-- Muy importante

class Robot {
public:
    std::string nombre;
    double tasaLimpiezaCm2ps; // Tasa de limpieza en cm2 por segundo

    Robot(const std::string& n, double tasa);

    ResultadoCalculo ejecutarCalculoDistribuido(const std::vector<Zona>& zonasParaCalcular);
};