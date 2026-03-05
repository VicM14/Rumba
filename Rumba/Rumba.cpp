// Simulador de Aspiradora Robot - Niveles 0 y 1
// Compilar en Visual Studio: Ctrl+F5 para ejecutar

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <conio.h>
#include <windows.h>

// ============================================
// NIVEL 0: Clase Vector2
// ============================================
class Vector2 {
public:
    float x;
    float y;

    Vector2() : x(0), y(0) {}
    Vector2(float x, float y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& other) const {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2& operator+=(const Vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2 operator-(const Vector2& other) const {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 operator*(float scalar) const {
        return Vector2(x * scalar, y * scalar);
    }

    bool operator==(const Vector2& other) const {
        return (x == other.x && y == other.y);
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    float distanceTo(const Vector2& other) const {
        return (*this - other).magnitude();
    }
};

// ============================================
// NIVEL 0: Clase Window (Ventana básica)
// ============================================
class Window {
private:
    int width;
    int height;
    std::vector<std::vector<char>> buffer;
    bool isRunning;

public:
    Window(int w = 60, int h = 20) : width(w), height(h), isRunning(true) {
        buffer.resize(height, std::vector<char>(width, ' '));
    }

    void clear() {
        for (auto& row : buffer) {
            std::fill(row.begin(), row.end(), ' ');
        }
    }

    void clearScreen() {
        system("cls");
    }

    void drawChar(int x, int y, char c) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            buffer[y][x] = c;
        }
    }

    void drawBorder() {
        // Esquinas
        drawChar(0, 0, '+');
        drawChar(width - 1, 0, '+');
        drawChar(0, height - 1, '+');
        drawChar(width - 1, height - 1, '+');

        // Horizontales
        for (int i = 1; i < width - 1; i++) {
            drawChar(i, 0, '-');
            drawChar(i, height - 1, '-');
        }

        // Verticales
        for (int i = 1; i < height - 1; i++) {
            drawChar(0, i, '|');
            drawChar(width - 1, i, '|');
        }
    }

    void drawText(int x, int y, const std::string& text) {
        for (size_t i = 0; i < text.length() && x + i < (size_t)width; i++) {
            drawChar(x + (int)i, y, text[i]);
        }
    }

    void render() {
        clearScreen();
        for (const auto& row : buffer) {
            for (char c : row) {
                std::cout << c;
            }
            std::cout << '\n';
        }
    }

    char getKeyPressed() {
        if (_kbhit()) {
            return _getch();
        }
        return '\0';
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool getIsRunning() const { return isRunning; }
    void close() { isRunning = false; }
};

// ============================================
// NIVEL 1: Clase Vacuum (Aspiradora)
// ============================================
enum class Direction {
    STOP,
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class Vacuum {
private:
    Vector2 position;
    Vector2 velocity;
    Direction direction;
    float speed;

public:
    Vacuum(float x, float y) {
        position = Vector2(x, y);
        velocity = Vector2(0, 0);
        direction = Direction::STOP;
        speed = 1.0f;
    }

    void setDirection(Direction dir) {
        direction = dir;
        switch (direction) {
        case Direction::UP:    velocity = Vector2(0, -speed); break;
        case Direction::DOWN:  velocity = Vector2(0, speed);  break;
        case Direction::LEFT:  velocity = Vector2(-speed, 0); break;
        case Direction::RIGHT: velocity = Vector2(speed, 0);  break;
        default:               velocity = Vector2(0, 0);      break;
        }
    }

    void update(int screenWidth, int screenHeight) {
        position += velocity;

        // Limitar dentro de la pantalla
        if (position.x < 1) position.x = 1;
        if (position.y < 1) position.y = 1;
        if (position.x >= screenWidth - 2) position.x = (float)(screenWidth - 3);
        if (position.y >= screenHeight - 2) position.y = (float)(screenHeight - 3);
    }

    void handleInput(char key) {
        switch (key) {
        case 'w': case 'W': setDirection(Direction::UP);    break;
        case 's': case 'S': setDirection(Direction::DOWN);  break;
        case 'a': case 'A': setDirection(Direction::LEFT);  break;
        case 'd': case 'D': setDirection(Direction::RIGHT); break;
        case ' ':           setDirection(Direction::STOP);  break;
        }
    }

    int getX() const { return (int)position.x; }
    int getY() const { return (int)position.y; }

    char getSymbol() const {
        switch (direction) {
        case Direction::UP:    return '^';
        case Direction::DOWN:  return 'v';
        case Direction::LEFT:  return '<';
        case Direction::RIGHT: return '>';
        default:               return 'O';
        }
    }

    std::string getDirectionString() const {
        switch (direction) {
        case Direction::UP:    return "ARRIBA";
        case Direction::DOWN:  return "ABAJO";
        case Direction::LEFT:  return "IZQUIERDA";
        case Direction::RIGHT: return "DERECHA";
        default:               return "DETENIDO";
        }
    }
};

// ============================================
// PROGRAMA PRINCIPAL
// ============================================
int main() {
    // Crear ventana
    Window window(60, 20);

    // Crear aspiradora en el centro
    Vacuum vacuum(30, 10);

    bool running = true;

    // Mensaje inicial
    std::cout << "========================================\n";
    std::cout << "    SIMULADOR DE ASPIRADORA ROBOT\n";
    std::cout << "         Niveles 0 y 1\n";
    std::cout << "========================================\n";
    std::cout << "\n";
    std::cout << "Controles:\n";
    std::cout << "  W - Arriba\n";
    std::cout << "  S - Abajo\n";
    std::cout << "  A - Izquierda\n";
    std::cout << "  D - Derecha\n";
    std::cout << "  ESPACIO - Detener\n";
    std::cout << "  Q - Salir\n";
    std::cout << "\n";
    std::cout << "Presione ENTER para comenzar...";
    std::cin.get();

    // Game loop
    while (running) {
        // Procesar input
        char key = window.getKeyPressed();
        if (key == 'q' || key == 'Q') {
            running = false;
        }
        else if (key != '\0') {
            vacuum.handleInput(key);
        }

        // Actualizar
        vacuum.update(window.getWidth(), window.getHeight());

        // Renderizar
        window.clear();
        window.drawBorder();

        // Dibujar aspiradora
        window.drawChar(vacuum.getX(), vacuum.getY(), vacuum.getSymbol());

        // Información
        window.drawText(2, 1, "ASPIRADORA ROBOT");
        window.drawText(2, window.getHeight() - 3, "Pos: (" + std::to_string(vacuum.getX()) + "," + std::to_string(vacuum.getY()) + ")");
        window.drawText(2, window.getHeight() - 2, "Dir: " + vacuum.getDirectionString());
        window.drawText(35, window.getHeight() - 2, "Q=Salir");

        window.render();

        // Esperar (control de velocidad)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Mensaje final
    window.clearScreen();
    std::cout << "\nSimulacion finalizada.\n";

    return 0;
}
}


