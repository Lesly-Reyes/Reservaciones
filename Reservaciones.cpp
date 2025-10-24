#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <vector>
#include <cctype>

using namespace std;

struct Habitacion {
    string codigo;
    string nombre;
    int capacidad = 0;
    double precio = 0.0;
};

struct Reservacion {
    string codigo;
    string cliente;
    Habitacion habitaciones[100];
    string habFechaIn[100];
    string habFechaOut[100];
    int habNoches[100];
    int habitacionesCount = 0;
    double total = 0.0;
};

const int MAX_RES = 200;
Reservacion reservas[MAX_RES];
int reservasCount = 0;

Habitacion catalogo[5];
int inventario[5];

void initCatalogo() {
    catalogo[0] = { "IND", "Individual Sencilla", 1, 500.0 };
    catalogo[1] = { "DBL", "Doble Superior", 2, 800.0 };
    catalogo[2] = { "TRP", "Triple Deluxe", 3, 1100.0 };
    catalogo[3] = { "QDR", "Cuadruple Premier", 4, 1400.0 };
    catalogo[4] = { "PRS", "Suite Presidencial", 5, 1700.0 };

    inventario[0] = 40;
    inventario[1] = 20;
    inventario[2] = 20;
    inventario[3] = 15;
    inventario[4] = 5;
}

string formatCodigo(int n) {
    ostringstream ss;
    ss << setw(3) << setfill('0') << n;
    return ss.str();
}

bool parseDate(const string& s, tm& out) {
    if (s.size() != 10 || s[2] != '/' || s[5] != '/') return false;
    try {
        int d = stoi(s.substr(0, 2));
        int m = stoi(s.substr(3, 2));
        int y = stoi(s.substr(6, 4));
        if (d < 1 || d > 31 || m < 1 || m > 12 || y < 1900) return false;
        out = tm();
        out.tm_mday = d;
        out.tm_mon = m - 1;
        out.tm_year = y - 1900;
        out.tm_hour = out.tm_min = out.tm_sec = 0;
        out.tm_isdst = -1;
        return true;
    }
    catch (...) {
        return false;
    }
}

int nightsBetween(const string& start, const string& end) {
    tm t1 = tm(), t2 = tm();
    if (!parseDate(start, t1) || !parseDate(end, t2)) return -1;
    time_t tt1 = mktime(&t1);
    time_t tt2 = mktime(&t2);
    if (tt1 == (time_t)-1 || tt2 == (time_t)-1) return -1;
    double diff = difftime(tt2, tt1);
    return static_cast<int>(diff / (24 * 3600));
}

int catalogIndexByCode(const string& codeRaw) {
    string code = codeRaw;
    for (size_t i = 0; i < code.size(); ++i) code[i] = toupper((unsigned char)code[i]);
    for (int i = 0; i < 5; ++i) {
        string cc = catalogo[i].codigo;
        for (size_t j = 0; j < cc.size(); ++j) cc[j] = toupper((unsigned char)cc[j]);
        if (cc == code) return i;
    }
    return -1;
}

void mostrarCatalogo() {
    cout << "\nCodigo  Descripcion               Precio   Dispon\n";
    cout << "--------------------------------------------------\n";
    for (int i = 0; i < 5; ++i) {
        cout << setw(6) << left << catalogo[i].codigo << " "
            << setw(24) << left << catalogo[i].nombre << " "
            << "Q" << setw(6) << right << fixed << setprecision(0) << catalogo[i].precio
            << "   " << setw(3) << right << inventario[i] << "\n";
    }
}

void ajustarInventarioDesdeReservas() {
    inventario[0] = 40;
    inventario[1] = 20;
    inventario[2] = 20;
    inventario[3] = 15;
    inventario[4] = 5;
    for (int r = 0; r < reservasCount; ++r) {
        for (int h = 0; h < reservas[r].habitacionesCount; ++h) {
            int idx = catalogIndexByCode(reservas[r].habitaciones[h].codigo);
            if (idx >= 0) inventario[idx] = max(0, inventario[idx] - 1);
        }
    }
}

void guardarReservasArchivo() {
    ofstream f("reservaciones.txt");
    if (!f) { cout << "No se pudo abrir reservaciones.txt\n"; return; }
    for (int i = 0; i < reservasCount; ++i) {
        Reservacion& r = reservas[i];
        f << r.codigo << "|" << r.cliente << "|";
        for (int j = 0; j < r.habitacionesCount; ++j) {
            Habitacion& h = r.habitaciones[j];
            string din = r.habFechaIn[j];
            string dout = r.habFechaOut[j];
            f << h.codigo << ":" << h.nombre << ":" << h.capacidad << ":" << fixed << setprecision(2) << h.precio
                << ":" << din << ":" << dout;
            if (j + 1 < r.habitacionesCount) f << ",";
        }
        f << "|" << fixed << setprecision(2) << r.total << "\n";
    }
    f.close();
}

void cargarReservasArchivo() {
    ifstream f("reservaciones.txt");
    if (!f) return;
    reservasCount = 0;
    string line;
    while (getline(f, line) && reservasCount < MAX_RES) {
        if (line.empty()) continue;
        Reservacion r;
        r.habitacionesCount = 0;
        r.total = 0.0;
        size_t p1 = line.find('|');
        size_t p2 = (p1 == string::npos) ? string::npos : line.find('|', p1 + 1);
        if (p1 == string::npos || p2 == string::npos) continue;
        r.codigo = line.substr(0, p1);
        r.cliente = line.substr(p1 + 1, p2 - p1 - 1);
        string rest = line.substr(p2 + 1);
        size_t lastPipe = rest.rfind('|');
        string roomList;
        string totalStr;
        if (lastPipe != string::npos) {
            roomList = rest.substr(0, lastPipe);
            totalStr = rest.substr(lastPipe + 1);
        }
        else {
            roomList = rest;
            totalStr = "0";
        }
        size_t s = 0;
        while (s < roomList.size() && r.habitacionesCount < 100) {
            size_t comma = roomList.find(',', s);
            string part = (comma == string::npos) ? roomList.substr(s) : roomList.substr(s, comma - s);
            vector<string> tokens;
            size_t pos = 0;
            while (true) {
                size_t col = part.find(':', pos);
                if (col == string::npos) {
                    tokens.push_back(part.substr(pos));
                    break;
                }
                else {
                    tokens.push_back(part.substr(pos, col - pos));
                    pos = col + 1;
                }
            }
            if (tokens.size() >= 6) {
                Habitacion h;
                h.codigo = tokens[0];
                h.nombre = tokens[1];
                try { h.capacidad = stoi(tokens[2]); }
                catch (...) { h.capacidad = 0; }
                try { h.precio = stod(tokens[3]); }
                catch (...) { h.precio = 0.0; }
                string din = tokens[4];
                string dout = tokens[5];
                r.habitaciones[r.habitacionesCount] = h;
                r.habFechaIn[r.habitacionesCount] = din;
                r.habFechaOut[r.habitacionesCount] = dout;
                int nights = nightsBetween(din, dout);
                r.habNoches[r.habitacionesCount] = (nights >= 0) ? nights : 0;
                r.habitacionesCount++;
            }
            if (comma == string::npos) break;
            s = comma + 1;
        }
        try { r.total = stod(totalStr); }
        catch (...) { r.total = 0.0; }
        reservas[reservasCount++] = r;
    }
    f.close();
    ajustarInventarioDesdeReservas();
}

bool codigoEnUso(const string& code) {
    for (int i = 0; i < reservasCount; ++i) if (reservas[i].codigo == code) return true;
    return false;
}

string asignarSiguienteCodigo() {
    for (int i = 1; i <= 100; ++i) {
        string c = formatCodigo(i);
        if (!codigoEnUso(c)) return c;
    }
    return "";
}

void mostrarReserva(const Reservacion& r) {
    cout << "\nCodigo: " << r.codigo << "\n";
    cout << "Cliente: " << r.cliente << "\n";
    cout << "Habitaciones:\n";
    for (int i = 0; i < r.habitacionesCount; ++i) {
        double precioNoche = r.habitaciones[i].precio;
        int noches = r.habNoches[i];
        double subtotal = precioNoche * noches;
        cout << " - " << r.habitaciones[i].codigo << " | " << r.habitaciones[i].nombre
            << " | Noches: " << noches
            << " | Q" << fixed << setprecision(2) << subtotal
            << " (Q" << fixed << setprecision(2) << precioNoche << "/noche)\n";
    }
    cout << "Total: Q" << fixed << setprecision(2) << r.total << "\n";
}

void crearReservacion(const string& codigoAsignado, const string& cliente) {
    if (reservasCount >= MAX_RES) { cout << "No se pueden agregar mas reservas.\n"; return; }
    Reservacion r;
    r.codigo = codigoAsignado;
    r.cliente = cliente;
    r.habitacionesCount = 0;
    r.total = 0.0;

    mostrarCatalogo();

    double totalSum = 0.0;
    while (true) {
        cout << "Ingrese codigo de habitacion a seleccionar (IND/DBL/TRP/QDR/PRS) o FIN: ";
        string code; getline(cin, code);
        code.erase(remove_if(code.begin(), code.end(), ::isspace), code.end());
        for (size_t i = 0; i < code.size(); ++i) code[i] = toupper((unsigned char)code[i]);
        if (code.empty()) { cout << "Sin texto.\n"; continue; }
        if (code == "FIN") break;
        bool alpha = true;
        for (char ch : code) if (!isalpha((unsigned char)ch)) alpha = false;
        if (!alpha) { cout << "Codigo invalido. Use IND, DBL, TRP, QDR o PRS.\n"; continue; }
        int idx = catalogIndexByCode(code);
        if (idx < 0) { cout << "Codigo no reconocido.\n"; continue; }
        if (inventario[idx] <= 0) { cout << "No hay disponibilidad para " << code << ".\n"; continue; }

        Habitacion h = catalogo[idx];
        if (code == "PRS") {
            cout << "Ingrese numero de personas (5-8, ENTER = 5): ";
            string ln; getline(cin, ln);
            int personas = 5;
            if (!ln.empty()) {
                try { personas = stoi(ln); }
                catch (...) { personas = 5; }
            }
            if (personas < 5) personas = 5;
            if (personas > 8) personas = 8;
            int extra = max(0, personas - 5);
            h.precio = h.precio + extra * 250.0;
            h.capacidad = personas;
        }

        int pos = r.habitacionesCount;
        r.habitaciones[pos] = h;
        r.habFechaIn[pos] = "";
        r.habFechaOut[pos] = "";
        r.habNoches[pos] = 0;
        r.habitacionesCount++;
        inventario[idx] -= 1;
        cout << "Agregada: " << h.codigo << " - " << h.nombre << " | Q" << fixed << setprecision(2) << h.precio << " por noche\n";

        string mas;
        while (true) {
            cout << "Desea agregar otra habitacion? (si/no): ";
            getline(cin, mas);
            for (size_t i = 0; i < mas.size(); ++i) mas[i] = tolower((unsigned char)mas[i]);
            if (mas == "si") { mostrarCatalogo(); break; }
            if (mas == "no") break;
            cout << "Responda si/no.\n";
        }
        if (mas == "no") break;
        if (r.habitacionesCount >= 100) { cout << "Limite de 100 habitaciones alcanzado.\n"; break; }
    }

    if (r.habitacionesCount == 0) { cout << "Sin habitaciones. Cancelado.\n"; return; }

    for (int i = 0; i < r.habitacionesCount; ++i) {
        while (true) {
            cout << "Fechas para " << r.habitaciones[i].codigo << " - " << r.habitaciones[i].nombre << "\n";
            cout << "  Fecha de entrada (dd/mm/aaaa): ";
            getline(cin, r.habFechaIn[i]);
            cout << "  Fecha de salida (dd/mm/aaaa): ";
            getline(cin, r.habFechaOut[i]);
            int nights = nightsBetween(r.habFechaIn[i], r.habFechaOut[i]);
            if (nights < 1) {
                cout << "  Fechas no validas. La salida debe ser despues. Reingrese.\n";
                continue;
            }
            r.habNoches[i] = nights;
            double sub = r.habitaciones[i].precio * nights;
            cout << "  Noches: " << nights << " | Subtotal por esta habitacion: Q" << fixed << setprecision(2) << sub << "\n";
            totalSum += sub;
            break;
        }
    }

    r.total = totalSum;
    reservas[reservasCount++] = r;
    guardarReservasArchivo();
    cout << "Reservacion creada: Codigo " << r.codigo << " | Total Q" << fixed << setprecision(2) << r.total << "\n";
}

void buscarPorCodigo() {
    cout << "Ingrese codigo de reservacion: ";
    string code; getline(cin, code);
    if (code.empty()) { cout << "Codigo vacio.\n"; return; }
    bool found = false;
    for (int i = 0; i < reservasCount; ++i) {
        if (reservas[i].codigo == code) {
            mostrarReserva(reservas[i]);
            found = true;
            break;
        }
    }
    if (!found) cout << "No existe reservacion con codigo " << code << "\n";
}

void cancelarReservacionAdmin() {
    cout << "Ingresa el codigo de la reservacion a cancelar: ";
    string codigo; getline(cin, codigo);
    if (codigo.empty()) { cout << "Codigo invalido.\n"; return; }
    int pos = -1;
    for (int i = 0; i < reservasCount; ++i) if (reservas[i].codigo == codigo) { pos = i; break; }
    if (pos == -1) { cout << "Codigo no existe.\n"; return; }
    cout << "Cancelando " << reservas[pos].codigo << " de " << reservas[pos].cliente << ". Confirma (SI/NO): ";
    string c; getline(cin, c);
    for (size_t i = 0; i < c.size(); ++i) c[i] = toupper((unsigned char)c[i]);
    if (c != "SI") { cout << "Cancelado.\n"; return; }
    for (int j = 0; j < reservas[pos].habitacionesCount; ++j) {
        int idx = catalogIndexByCode(reservas[pos].habitaciones[j].codigo);
        if (idx >= 0) inventario[idx] += 1;
    }
    for (int k = pos; k + 1 < reservasCount; ++k) reservas[k] = reservas[k + 1];
    reservasCount--;
    guardarReservasArchivo();
    cout << "Reservacion cancelada.\n";
}

void cancelarReservacionCliente() {
    cout << "Ingrese su codigo de reservacion: ";
    string codigo; getline(cin, codigo);
    if (codigo.empty()) { cout << "Codigo invalido.\n"; return; }
    cout << "Ingrese su nombre (tal como figura en la reserva): ";
    string nombre; getline(cin, nombre);
    if (nombre.empty()) { cout << "Nombre vacio.\n"; return; }
    int pos = -1;
    for (int i = 0; i < reservasCount; ++i) {
        if (reservas[i].codigo == codigo && reservas[i].cliente == nombre) { pos = i; break; }
    }
    if (pos == -1) { cout << "No se encontro una reserva con ese codigo y nombre.\n"; return; }
    cout << "Confirmar cancelacion de " << reservas[pos].codigo << " de " << reservas[pos].cliente << " ? (SI/NO): ";
    string c; getline(cin, c);
    for (size_t i = 0; i < c.size(); ++i) c[i] = toupper((unsigned char)c[i]);
    if (c != "SI") { cout << "Cancelacion abortada.\n"; return; }
    for (int j = 0; j < reservas[pos].habitacionesCount; ++j) {
        int idx = catalogIndexByCode(reservas[pos].habitaciones[j].codigo);
        if (idx >= 0) inventario[idx] += 1;
    }
    for (int k = pos; k + 1 < reservasCount; ++k) reservas[k] = reservas[k + 1];
    reservasCount--;
    guardarReservasArchivo();
    cout << "Su reservacion fue cancelada.\n";
}

int main() {
    initCatalogo();

    reservasCount = 0;
    initCatalogo();

    cout << "=====================================\n";
    cout << " BIENVENIDOS AL HOTEL DINAMITA\n";
    cout << "=====================================\n";

    while (true) {
        cout << "\n--- Menu principal ---\n";
        cout << "1) Cliente\n2) Admin\n3) Salir\nSeleccione opcion: ";
        string op; getline(cin, op);

        if (op == "1") {
            while (true) {
                cout << "\n--- Menu Cliente ---\n";
                cout << "1) Crear reservacion\n2) Cancelar mi reservacion\n3) Volver\nElige: ";
                string c; getline(cin, c);
                if (c == "1") {
                    cout << "Nombre: ";
                    string nombre; getline(cin, nombre);
                    if (nombre.empty()) { cout << "Sin texto.\n"; continue; }
                    string codigo = asignarSiguienteCodigo();
                    if (codigo.empty()) { cout << "No hay codigos disponibles. Intente mas tarde.\n"; continue; }
                    cout << "Tu codigo de reservacion es: " << codigo << "\n";
                    cout << "Escribe el codigo para confirmar (o CANCEL): ";
                    string conf; getline(cin, conf);
                    if (conf != codigo) { cout << "Codigo no coincide. Volviendo.\n"; continue; }
                    crearReservacion(codigo, nombre);
                }
                else if (c == "2") {
                    cancelarReservacionCliente();
                }
                else if (c == "3") break;
                else cout << "Opcion no valida.\n";
            }
        }
        else if (op == "2") {
            cout << "\n--- Acceso admin ---\n";
            cout << "Usuario: "; string user; getline(cin, user);
            cout << "Pin: "; string pin; getline(cin, pin);
            if (pin != "000") { cout << "Pin incorrecto.\n"; continue; }
            while (true) {
                cout << "\n--- Menu Admin ---\n";
                cout << "1) Ver reservaciones\n2) Agregar reservacion\n3) Guardar en archivo\n4) Cancelar reservacion\n5) Volver\nElige: ";
                string a; getline(cin, a);
                if (a == "1") {
                    if (reservasCount == 0) {
                        cout << "No hay ninguna reservacion guardada.\n";
                        continue;
                    }
                    for (int i = 0; i < reservasCount; ++i) {
                        cout << reservas[i].codigo << " | " << reservas[i].cliente << " | Q" << fixed << setprecision(2) << reservas[i].total << "\n";
                    }
                }
                else if (a == "2") {
                    cout << "Nombre cliente: ";
                    string nombre; getline(cin, nombre);
                    if (nombre.empty()) { cout << "Sin texto.\n"; continue; }
                    string codigo = asignarSiguienteCodigo();
                    if (codigo.empty()) { cout << "No hay codigos disponibles.\n"; continue; }
                    cout << "Codigo asignado: " << codigo << "\n";
                    crearReservacion(codigo, nombre);
                }
                else if (a == "3") {
                    guardarReservasArchivo();
                    cout << "Exportado a reservaciones.txt\n";
                }
                else if (a == "4") {
                    cancelarReservacionAdmin();
                }
                else if (a == "5") break;
                else cout << "Opcion no valida.\n";
            }
        }
        else if (op == "3") {
            cout << "Saliendo...\n";
            break;
        }
        else {
            cout << "Opcion no valida.\n";
        }
    }

    return 0;
}