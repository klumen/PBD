#include "Global.h"

unsigned Global::screenWidth = 1600, Global::screenHeight = 1200;

float Global::lastFrame = 0.f;
float Global::deltaTime = 0.f;

void Global::init()
{
    init_screen_quad();
    init_texture_shader();
    init_camera();
}

float Global::screenQuadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};
unsigned Global::screenQuadVAO = 0;
unsigned Global::screenQuadVBO = 0;
void Global::init_screen_quad()
{
    glGenVertexArrays(1, &screenQuadVAO);
    glGenBuffers(1, &screenQuadVBO);

    glBindVertexArray(screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuadVertices), &screenQuadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

PipelineShader* Global::textureShader = nullptr;
void Global::init_texture_shader()
{
    textureShader = new PipelineShader("shader/textureVert.glsl", "shader/textureFrag.glsl");
}
void Global::texture_shader(unsigned texture)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    textureShader->use();
    textureShader->set_int("tex", 0);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(Global::screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

Camera* Global::mainCamera = nullptr;
void Global::init_camera()
{
    mainCamera = new Camera(glm::vec3(0.f, 2.f, 3.f), -35.f);
}

std::vector<DirLight*> Global::dirLights;