#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
out vec3 Color;
void main() {
    Color = color;
    gl_Position = vec4(position, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
out vec4 outColor;
void main() {
    outColor = vec4(Color, 1.0);
}
)glsl";

// Funkcja do sprawdzania statusu kompilacji shaderów
void checkShaderCompilation(GLuint shader, const std::string& shaderType) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, NULL, log);

        // Wyświetlanie komunikatu o błędzie kompilacji
        std::cerr << "Kompilacja " << shaderType << " ERROR" << std::endl;
        std::cerr << log << std::endl;

        delete[] log;
    }
    else {
        std::cout << "Kompilacja " << shaderType << " OK" << std::endl;
    }
}

// Funkcja do tworzenia wierzchołków wielokąta foremnego
void createRegularPolygonVertices(int sides, float radius, GLfloat*& vertices, GLfloat*& colors) {
    // Alokacja pamięci dla wierzchołków i kolorów
    vertices = new GLfloat[sides * 3]; // X, Y, Z
    colors = new GLfloat[sides * 3];   // R, G, B

    for (int i = 0; i < sides; ++i) {
        float angle = 2.0f * M_PI * i / sides;
        int index = i * 3;

        // Pozycja wierzchołka (X, Y, Z)
        vertices[index] = radius * cos(angle); // X
        vertices[index + 1] = radius * sin(angle); // Y
        vertices[index + 2] = 0.0f; // Z

        // Kolor wierzchołka (R, G, B)
        colors[index] = static_cast<float>(i) / sides;     // R
        colors[index + 1] = static_cast<float>(sides - i) / sides; // G
        colors[index + 2] = 1.0f - static_cast<float>(i) / sides; // B
    }
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;

    // Okno renderingu
    sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

    // Inicjalizacja GLEW 
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Błąd inicjalizacji GLEW: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    // Utworzenie VAO (Vertex Array Object)
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLfloat* vertices = nullptr;
    GLfloat* colors = nullptr;
    int sides = 6; // Zaczynamy od sześciokąta
    const float radius = 0.5f;
    createRegularPolygonVertices(sides, radius, vertices, colors);

    // Utworzenie VBO (Vertex Buffer Object)
    GLuint vbo[2];
    glGenBuffers(2, vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sides * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sides * 3 * sizeof(GLfloat), colors, GL_STATIC_DRAW);

    // Utworzenie i skompilowanie shadera wierzchołków
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader, "shadera wierzcholkow");

    // Utworzenie i skompilowanie shadera fragmentów
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader, "shadera fragmentow");

    // Zlinkowanie obu shaderów w jeden wspólny program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glBindFragDataLocation(shaderProgram, 0, "outColor");
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Specifikacja formatu danych wierzchołkowych
    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Typ prymitywu
    GLenum primitiveType = GL_TRIANGLE_FAN; // Domyślny typ prymitywu

    bool running = true;
    while (running) {
        sf::Event windowEvent;
        while (window.pollEvent(windowEvent)) {
            if (windowEvent.type == sf::Event::Closed) {
                running = false;
            }

            // Obsługa ruchu myszy do zmiany liczby boków
            if (windowEvent.type == sf::Event::MouseMoved) {
                sides = std::max(3, windowEvent.mouseMove.y / 10); // Zmień liczbę boków w zależności od pozycji kursora
                delete[] vertices;
                delete[] colors;
                createRegularPolygonVertices(sides, radius, vertices, colors);

                glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
                glBufferData(GL_ARRAY_BUFFER, sides * 3 * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

                glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
                glBufferData(GL_ARRAY_BUFFER, sides * 3 * sizeof(GLfloat), colors, GL_STATIC_DRAW);
            }

            if (windowEvent.type == sf::Event::KeyPressed) {
                switch (windowEvent.key.code) {
                case sf::Keyboard::Num1: primitiveType = GL_POINTS; break;
                case sf::Keyboard::Num2: primitiveType = GL_LINES; break;
                case sf::Keyboard::Num3: primitiveType = GL_LINE_STRIP; break;
                case sf::Keyboard::Num4: primitiveType = GL_LINE_LOOP; break;
                case sf::Keyboard::Num5: primitiveType = GL_TRIANGLES; break;
                case sf::Keyboard::Num6: primitiveType = GL_TRIANGLE_STRIP; break;
                case sf::Keyboard::Num7: primitiveType = GL_TRIANGLE_FAN; break;
                case sf::Keyboard::Num8: primitiveType = GL_QUADS; break; // Czworokąty nie będą wyświetlane poprawnie, chyba że liczba boków wynosi 4
                case sf::Keyboard::Num9: primitiveType = GL_QUAD_STRIP; break; // Czworokąty nie będą wyświetlane poprawnie, chyba że liczba boków wynosi 4
                case sf::Keyboard::Num0: primitiveType = GL_POLYGON; break; // Dla wielokątów
                default: break;
                }
            }
        }

        // Nadanie scenie koloru czarnego
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Rysowanie wielokąta w zależności od aktualnego typu prymitywu
        glDrawArrays(primitiveType, 0, sides);

        // Wymiana buforów tylni/przedni
        window.display();
    }

    // Kasowanie programu i czyszczenie buforów
    delete[] vertices;
    delete[] colors;
    glDeleteProgram(shaderProgram);
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);
    // Zamknięcie okna renderingu
    window.close();
    return 0;
}
