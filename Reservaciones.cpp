#include <iostream>
#include <fstream>
#include <string>

void exportarCSV(const std::string& inPath = "reservaciones.txt",
    const std::string& outPath = "reporte_reservaciones.csv") {
    std::ifstream in(inPath);
    if (!in.is_open()) {
        std::cout << "No se pudo abrir " << inPath << "\n";
        return;
    }
    std::ofstream out(outPath, std::ios::out | std::ios::trunc);
    if (!out.is_open()) {
        std::cout << "No se pudo crear " << outPath << "\n";
        return;
    }

    // Encabezado
    out << "Hotel,Black Orchid Hotel\n";
    out << "Codigo,Cliente,TipoHabitacion,Categoria,CheckIn,CheckOut,Noches,Huespedes,PrecioNoche,Subtotal,Descuento,Total\n";

    std::string linea;
    while (std::getline(in, linea)) {
        // limpiar CR
        while (!linea.empty() && (linea.back() == '\r' || linea.back() == '\n')) linea.pop_back();
        if (linea.empty()) continue;
        for (char& c : linea) if (c == '|') c = ',';
        out << linea << "\n";
    }
    std::cout << "CSV generado en " << outPath << "\n";
}
}
