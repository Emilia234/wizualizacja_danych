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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
using namespace std;


const char* vertexSource = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;
    layout(location = 2) in vec2 aTexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoord;

    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        TexCoord = aTexCoord;
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char* fragmentSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;
    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main() {
        vec3 color = texture(texture1, TexCoord).rgb;
        FragColor = vec4(color, 1.0);
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
        else if (type == "vt") { 
            TextureCoord texCoord;
            lineStream >> texCoord.u >> texCoord.v;
            model.texCoords.push_back(texCoord);
        }
        else if (type == "vn") {
            Normal normal;
            lineStream >> normal.nx >> normal.ny >> normal.nz;
            model.normals.push_back(normal);
        }
        else if (type == "f") {
            Face face;
            std::string vertexData;
            while (lineStream >> vertexData) {
                std::istringstream vertexStream(vertexData);
                std::string vertexIndex, texCoordIndex, normalIndex;
                if (std::getline(vertexStream, vertexIndex, '/') &&
                    std::getline(vertexStream, texCoordIndex, '/') &&
                    std::getline(vertexStream, normalIndex)) {
                    face.vertexIndices.push_back(std::stoi(vertexIndex) - 1);
                    face.texCoordIndices.push_back(std::stoi(texCoordIndex) - 1);
                    face.normalIndices.push_back(std::stoi(normalIndex) - 1);
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

bool LoadTexture(const char* filePath, unsigned int& textureID,
    GLenum wrapS = GL_REPEAT, GLenum wrapT = GL_REPEAT,
    GLenum minFilter = GL_LINEAR, GLenum magFilter = GL_LINEAR) {
    glGenTextures(1, &textureID); 
    glBindTexture(GL_TEXTURE_2D, textureID); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char* data = stbi_load(filePath, &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        return true;
    }
    else {
        cout << "Failed to load texture: " << filePath << endl;
        stbi_image_free(data);
        return false;
    }
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


    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f); 

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
        for (size_t i = 0; i < face.vertexIndices.size(); ++i) {
            const Vertex& vertex = model.vertices[face.vertexIndices[i]];
            vertices.push_back(vertex.x);
            vertices.push_back(vertex.y);
            vertices.push_back(vertex.z);

            if (!model.normals.empty()) {
                const Normal& normal = model.normals[face.normalIndices[i]];
                vertices.push_back(normal.nx);
                vertices.push_back(normal.ny);
                vertices.push_back(normal.nz);
            }

            if (!model.texCoords.empty()) {
                const TextureCoord& texCoord = model.texCoords[face.texCoordIndices[i]];
                vertices.push_back(texCoord.u);
                vertices.push_back(texCoord.v);
            }
        }
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    glBindVertexArray(0);

    sf::Clock clock;
    sf::Time elapsed;
    window.setFramerateLimit(60);


    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    GLuint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
    glEnableVertexAttribArray(TexCoord);
    glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

    GLuint texture1;
    if (!LoadTexture("metal.jpg", texture1)) {
        std::cerr << "Failed to load texture!" << std::endl;
        return -1;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    GLuint textureLoc = glGetUniformLocation(shaderProgram, "texture1");
    glUniform1i(textureLoc, 0); 


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
        glm::mat4 model = glm::mat4(1.0f);
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