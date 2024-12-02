#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/System/Time.hpp>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec2 texCoord;
in vec3 aNormal;

out vec3 Normal;
out vec2 TexCoord;
out vec3 Color;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
    Normal = mat3(transpose(inverse(model))) * aNormal;
    FragPos = vec3(model * vec4(position, 1.0));
    Color = color;
    TexCoord = texCoord;
    gl_Position = proj * view * model * vec4(position, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
out vec4 outColor;
uniform sampler2D texture1;
uniform vec3 lightPos;          
uniform vec3 viewPos;           
uniform vec3 ambientLightColor; 
uniform vec3 diffuseLightColor; 
uniform float ambientStrength;  
uniform float lightStrength; 
uniform bool lightingEnabled;

void main() {

    vec3 ambient = ambientStrength * ambientLightColor;
    vec3 diffuse = vec3(0.0);
    if (lightingEnabled) {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        diffuse = diff * diffuseLightColor * lightStrength;
    }

   vec3 lighting = (ambient + diffuse);
   vec4 texColor = texture(texture1, TexCoord);
   outColor = vec4(lighting, 1.0) * texColor;
}
)glsl";

void checkShaderCompilation(GLuint shader, const std::string& shaderType) {
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        char* log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, NULL, log);
        std::cerr << "Compilation error in " << shaderType << " shader: " << log << std::endl;
        delete[] log;
    }
    else {
        std::cout << shaderType << " shader compilation OK" << std::endl;
    }
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    sf::Window window(sf::VideoMode(800, 600), "OpenGL Cube with Camera Controls", sf::Style::Close, settings);
    window.setFramerateLimit(60);
    window.setMouseCursorGrabbed(true);
    window.setMouseCursorVisible(false);

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "GLEW initialization error: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

       GLfloat vertices[] = {
            // Front
            -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  0.0f,  0.0f,  1.0f,

            // Back
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  0.0f,  0.0f, -1.0f,

            // Left
            -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f, -1.0f,  0.0f,  0.0f,

            // Right
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  1.0f,  0.0f,  0.0f,

             // Bottom
             -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f, -1.0f,  0.0f,
              0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  0.0f, -1.0f,  0.0f,
              0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  0.0f, -1.0f,  0.0f,
             -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  0.0f, -1.0f,  0.0f,

             // Top
             -0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,  0.0f,  1.0f,  0.0f,
              0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  0.0f,  1.0f,  0.0f,
              0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,  0.0f,  1.0f,  0.0f,
             -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,  0.0f,  1.0f,  0.0f,
         };

   GLuint indices[] = {
        // Front
        0, 1, 2, 0, 2, 3,
        // Back
        4, 5, 6, 4, 6, 7,
        // Left
        8, 9,10, 8,10,11,
        // Right
        12,13,14,12,14,15,
        // Bottom
        16,17,18,16,18,19,
        // Top
        20,21,22,20,22,23
    };


    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);
    checkShaderCompilation(vertexShader, "Vertex");

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);
    checkShaderCompilation(fragmentShader, "Fragment");

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(posAttrib);
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);

    GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
    glEnableVertexAttribArray(colAttrib);
    glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    GLint texAttrib = glGetAttribLocation(shaderProgram, "texCoord");
    glEnableVertexAttribArray(texAttrib);
    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));

    GLint NorAttrib = glGetAttribLocation(shaderProgram, "aNormal");
    glEnableVertexAttribArray(NorAttrib);
    glVertexAttribPointer(NorAttrib, 3,GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));


    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("metal.jpg", &width, &height, &nrChannels, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cerr << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = 0.0f;
    float lastX = 400, lastY = 300;
    bool firstMouse = true;
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    float sensitivity = 0.1f;
    float speed = 2.5f;

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
    GLint uniView = glGetUniformLocation(shaderProgram, "view");
    GLint uniModel = glGetUniformLocation(shaderProgram, "model");

    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 ambientLightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 diffuseLightColor(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.1f;
    float lightStrength = 1.0f;

    GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
    GLint uniViewPos = glGetUniformLocation(shaderProgram, "viewPos");
    GLint uniAmbientLightColor = glGetUniformLocation(shaderProgram, "ambientLightColor");
    GLint uniDiffuseLightColor = glGetUniformLocation(shaderProgram, "diffuseLightColor");
    GLint uniAmbientStrength = glGetUniformLocation(shaderProgram, "ambientStrength");
    GLint uniLightStrength = glGetUniformLocation(shaderProgram, "lightStrength");
    GLint uniLightingEnabled = glGetUniformLocation(shaderProgram, "lightingEnabled");

    glUniform3fv(uniLightPos, 1, glm::value_ptr(lightPos));
    glUniform3fv(uniAmbientLightColor, 1, glm::value_ptr(ambientLightColor));
    glUniform3fv(uniDiffuseLightColor, 1, glm::value_ptr(diffuseLightColor));
    glUniform1f(uniAmbientStrength, ambientStrength);
    glUniform1f(uniLightStrength, lightStrength);
    glUniform3fv(uniViewPos, 1, glm::value_ptr(cameraPos));
    glUniform1i(uniLightingEnabled, 1); 

    glEnable(GL_DEPTH_TEST);

    bool lightingEnabled = true;

    sf::Clock clock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        float currentFrame = clock.getElapsedTime().asSeconds();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
            cameraPos += speed * deltaTime * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
            cameraPos -= speed * deltaTime * cameraFront;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
            window.close();
        }

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        if (firstMouse) {
            lastX = mousePos.x;
            lastY = mousePos.y;
            firstMouse = false;
        }

        float xoffset = mousePos.x - lastX;
        float yoffset = lastY - mousePos.y; 
        lastX = mousePos.x;
        lastY = mousePos.y;

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(uniViewPos, 1, glm::value_ptr(cameraPos));


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
            ambientStrength += 0.1f;
            if (ambientStrength > 1.0f) ambientStrength = 1.0f;
            glUniform1f(uniAmbientStrength, ambientStrength);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X)) {
            ambientStrength -= 0.1f;
            if (ambientStrength < 0.0f) ambientStrength = 0.0f;
            glUniform1f(uniAmbientStrength, ambientStrength);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::C)) {
            lightStrength += 0.1f;
            if (lightStrength > 2.0f) lightStrength = 2.0f;
            glUniform1f(uniLightStrength, lightStrength);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::V)) {
           lightStrength -= 0.1f;
            if (lightStrength < 0.0f) lightStrength = 0.0f;
            glUniform1f(uniLightStrength, lightStrength);
        }

        static bool keyPressed = false;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::L)) { 
            if (!keyPressed) { 
                lightingEnabled = !lightingEnabled; 
                glUniform1i(uniLightingEnabled, lightingEnabled ? 1 : 0); 
                keyPressed = true;  
            }
        }
        else {
            keyPressed = false;     
        }


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float time = clock.getElapsedTime().asSeconds();
        glm::mat4 model = glm::mat4(1.0f); // Brak obrotu
      // glm::mat4 model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

       
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(vao);

        window.display();
    }

    glDeleteTextures(1, &texture);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);

    return 0;
}
