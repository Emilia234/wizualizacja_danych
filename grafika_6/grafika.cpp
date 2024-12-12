#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const char* vertexSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
)";

struct Vertex { float x, y, z; };

struct TextureCoord { float u, v; };

struct Normal { float nx, ny, nz; };

struct Face {
    vector<int> vertexIndices;
    vector<int> texCoordIndices;
    vector<int> normalIndices;
};

struct ObjModel {
    vector<Vertex> vertices;
    vector<TextureCoord> texCoords;
    vector<Normal> normals;
    vector<Face> faces;
};

ObjModel loadObjModel(const std::string& filePath) {
    ObjModel model;
    ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Nie można otworzyć pliku: " << filePath << std::endl;
        return model;
    }

    string line;
    while (std::getline(file, line)) {
        std::istringstream lineStream(line);
        std::string type;
        lineStream >> type;

        if (type == "v") {
            Vertex vertex;
            lineStream >> vertex.x >> vertex.y >> vertex.z;
            model.vertices.push_back(vertex);
        }
        else if (type == "f") {
            Face face;
            std::string vertexData;
            while (lineStream >> vertexData) {
                std::istringstream vertexStream(vertexData);
                std::string index;
                if (std::getline(vertexStream, index, '/')) {
                    int vertexIndex = std::stoi(index) - 1;
                    face.vertexIndices.push_back(vertexIndex);
                }
            }
            model.faces.push_back(face);
        }
    }

    file.close();
    return model;
}

void check_Shader(GLuint shader, const string& shaderType) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength + 1];
        glGetShaderInfoLog(shader, logLength, nullptr, log);
        log[logLength] = '\0';
        cerr << shaderType << " shader compilation failed: " << log << "\n";
        delete[] log;
    }
    else {
        cout << shaderType << " shader compilation OK\n";
    }
}

GLuint createShader(GLenum shaderType, const char* source) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;

    sf::Window window(sf::VideoMode(800, 600), "OpenGL FPS Camera", sf::Style::Titlebar | sf::Style::Close, settings);
    glewExperimental = GL_TRUE;
    glewInit();

    glEnable(GL_DEPTH_TEST);

    glm::vec3 cameraPos(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);
    glm::vec3 cameraFront(0.0f, 0.0f, -1.0f);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float cameraSpeed = 0.1f;
    float sensitivity = 0.08f;
    float obrot = 0.0f;
    sf::Vector2i lastMousePos = sf::Mouse::getPosition(window);


    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f); //

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    check_Shader(vertexShader, "Vertex");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    check_Shader(fragmentShader, "Fragment");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);


    ObjModel model = loadObjModel("stół3.obj");
    vector<float> vertices;
    for (const auto& face : model.faces) {
        for (int index : face.vertexIndices) {
            const Vertex& vertex = model.vertices[index];
            vertices.push_back(vertex.x);
            vertices.push_back(vertex.y);
            vertices.push_back(vertex.z);
        }
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

  
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);


    glBindVertexArray(0);

    sf::Clock clock;
    sf::Time elapsed;
    window.setFramerateLimit(60); 


    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");


    bool running = true;
    while (running) {

        elapsed = clock.restart(); 
        float deltaTime = elapsed.asSeconds();
        float cameraSpeed = 2.5f * deltaTime;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) running = false;
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) running = false;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) cameraPos += cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) cameraPos -= cameraSpeed * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))  cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q))  cameraPos.y += cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::E))  cameraPos.y -= cameraSpeed;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))  obrot -= cameraSpeed;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  obrot += cameraSpeed;


        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float xOffset = (mousePos.x - lastMousePos.x) * sensitivity;
        float yOffset = (lastMousePos.y - mousePos.y) * sensitivity;
        lastMousePos = mousePos;

        yaw += xOffset;
        pitch += yOffset;


        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;


        cameraFront.x = cos(glm::radians(yaw + obrot)) * cos(glm::radians(pitch));
        cameraFront.y = sin(glm::radians(pitch));
        cameraFront.z = sin(glm::radians(yaw + obrot)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(cameraFront);

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(proj));

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        GLint uniView = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);
       // glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 0.2f, 0.2f));
        GLint uniModel = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        float fps = 1.0f / deltaTime;
        window.display();
    }

    glDeleteProgram(shaderProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    return 0;
}
