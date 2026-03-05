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
// NIVEL 0: Clase Window (Ventana basica)
// ============================================
class Window {
private:
    int width;
    int height;
    std::vector<std::vector<char>> buffer;

public:
    Window(int w, int h) : width(w), height(h) {
        buffer.resize(height, std::vector<char>(width, ' '));
    }

    void clear() {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                buffer[i][j] = ' ';
            }
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

    void drawText(int x, int y, const char* text) {
        int i = 0;
        while (text[i] != '\0' && x + i < width) {
            drawChar(x + i, y, text[i]);
            i++;
        }
    }

    void render() {
        clearScreen();
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                std::cout << buffer[i][j];
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

    int getWidth() { return width; }
    int getHeight() { return height; }
};

// ============================================
// NIVEL 1: Clase Vacuum (Aspiradora)
// ============================================
class Vacuum {
private:
    Vector2 position;
    Vector2 velocity;
    int direction; // 0=stop, 1=up, 2=down, 3=left, 4=right
    float speed;

public:
    Vacuum(float x, float y) {
        position = Vector2(x, y);
        velocity = Vector2(0, 0);
        direction = 0;
        speed = 1.0f;
    }

    void setDirection(int dir) {
        direction = dir;
        if (direction == 1) {
            velocity = Vector2(0, -speed);
        }
        else if (direction == 2) {
            velocity = Vector2(0, speed);
        }
        else if (direction == 3) {
            velocity = Vector2(-speed, 0);
        }
        else if (direction == 4) {
            velocity = Vector2(speed, 0);
        }
        else {
            velocity = Vector2(0, 0);
        }
    }

    void update(int screenWidth, int screenHeight) {
        position += velocity;

        if (position.x < 1) position.x = 1;
        if (position.y < 1) position.y = 1;
        if (position.x >= screenWidth - 2) position.x = (float)(screenWidth - 3);
        if (position.y >= screenHeight - 2) position.y = (float)(screenHeight - 3);
    }

    void handleInput(char key) {
        if (key == 'w' || key == 'W') {
            setDirection(1);
        }
        else if (key == 's' || key == 'S') {
            setDirection(2);
        }
        else if (key == 'a' || key == 'A') {
            setDirection(3);
        }
        else if (key == 'd' || key == 'D') {
            setDirection(4);
        }
        else if (key == ' ') {
            setDirection(0);
        }
    }

    int getX() { return (int)position.x; }
    int getY() { return (int)position.y; }

    char getSymbol() {
        if (direction == 1) return '^';
        if (direction == 2) return 'v';
        if (direction == 3) return '<';
        if (direction == 4) return '>';
        return 'O';
    }

    const char* getDirectionString() {
        if (direction == 1) return "ARRIBA";
        if (direction == 2) return "ABAJO";
        if (direction == 3) return "IZQUIERDA";
        if (direction == 4) return "DERECHA";
        return "DETENIDO";
    }
};

// ============================================
// PROGRAMA PRINCIPAL
// ============================================
int main() {
    Window window(60, 20);
    Vacuum vacuum(30, 10);
    bool running = true;

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

    while (running) {
        char key = window.getKeyPressed();

        if (key == 'q' || key == 'Q') {
            running = false;
        }
        else if (key != '\0') {
            vacuum.handleInput(key);
        }

        vacuum.update(window.getWidth(), window.getHeight());

        window.clear();
        window.drawBorder();
        window.drawChar(vacuum.getX(), vacuum.getY(), vacuum.getSymbol());

        window.drawText(2, 1, "ASPIRADORA ROBOT");
        window.drawText(2, 17, "Dir:");
        window.drawText(7, 17, vacuum.getDirectionString());
        window.drawText(45, 17, "Q = Salir");

        window.render();

        Sleep(100);
    }

    system("cls");
    std::cout << "\nSimulacion finalizada.\n";

    return 0;
}


