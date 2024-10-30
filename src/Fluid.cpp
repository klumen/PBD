#include "Fluid.h"

ComputeShader* Fluid::predictPositionCompShader = nullptr;
ComputeShader* Fluid::neighborhoodSearchCompShader = nullptr;
ComputeShader* Fluid::calculateLambdaCompShader = nullptr;
ComputeShader* Fluid::solveIncompressibilityConstraintCompShader = nullptr;
ComputeShader* Fluid::solveEnvironmentCollisionConstraintCompShader = nullptr;
ComputeShader* Fluid::solveParticleCollisionConstraintShader = nullptr;
ComputeShader* Fluid::updatePredictPositionCompShader = nullptr;
ComputeShader* Fluid::updateVelocityCompShader = nullptr;
ComputeShader* Fluid::calculateOmegaCompShader = nullptr;
ComputeShader* Fluid::applyVorticityConfinementCompShader = nullptr;
ComputeShader* Fluid::applyXSPHVViscosityCompShader = nullptr;
ComputeShader* Fluid::updatePositionCompShader = nullptr;

PipelineShader* Fluid::SSFGetDepthShader = nullptr;
PipelineShader* Fluid::SSFGetThicknessShader = nullptr;
PipelineShader* Fluid::SSFBlurShader = nullptr;
PipelineShader* Fluid::SSFRenderShader = nullptr;

void Fluid::create_shaders()
{
    predictPositionCompShader = new ComputeShader("shader/fluid/predictPositionComp.glsl");
    neighborhoodSearchCompShader = new ComputeShader("shader/fluid/neighborhoodSearchComp.glsl");
    calculateLambdaCompShader = new ComputeShader("shader/fluid/calculateLambdaComp.glsl");
    solveIncompressibilityConstraintCompShader = new ComputeShader("shader/fluid/solveIncompressibilityConstraintComp.glsl");
    solveEnvironmentCollisionConstraintCompShader = new ComputeShader("shader/fluid/solveEnvironmentCollisionConstraintComp.glsl");
    solveParticleCollisionConstraintShader = new ComputeShader("shader/fluid/solveParticleCollisionConstraintComp.glsl");
    updatePredictPositionCompShader = new ComputeShader("shader/fluid/updatePredictPositionComp.glsl");
    updateVelocityCompShader = new ComputeShader("shader/fluid/updateVelocityComp.glsl");
    calculateOmegaCompShader = new ComputeShader("shader/fluid/calculateOmegaComp.glsl");
    applyVorticityConfinementCompShader = new ComputeShader("shader/fluid/applyVorticityConfinementComp.glsl");
    applyXSPHVViscosityCompShader = new ComputeShader("shader/fluid/applyXSPHVViscosityComp.glsl");
    updatePositionCompShader = new ComputeShader("shader/fluid/updatePositionComp.glsl");
    
    SSFGetDepthShader = new PipelineShader("shader/fluid/SSFGetDepthVert.glsl", "shader/fluid/SSFGetDepthFrag.glsl");
    SSFGetThicknessShader = new PipelineShader("shader/fluid/SSFGetThicknessVert.glsl", "shader/fluid/SSFGetThicknessFrag.glsl");
    SSFBlurShader = new PipelineShader("shader/fluid/SSFBlurVert.glsl", "shader/fluid/SSFBlurFrag.glsl");
    SSFRenderShader = new PipelineShader("shader/fluid/SSFRenderVert.glsl", "shader/fluid/SSFRenderFrag.glsl");
}

void Fluid::delete_shaders()
{
    if (predictPositionCompShader != nullptr)
    {
        delete predictPositionCompShader;
        predictPositionCompShader = nullptr;
    }
    if (neighborhoodSearchCompShader != nullptr)
    {
        delete neighborhoodSearchCompShader;
        neighborhoodSearchCompShader = nullptr;
    }
    if (calculateLambdaCompShader != nullptr)
    {
        delete calculateLambdaCompShader;
        calculateLambdaCompShader = nullptr;
    }
    if (solveIncompressibilityConstraintCompShader != nullptr)
    {
        delete solveIncompressibilityConstraintCompShader;
        solveIncompressibilityConstraintCompShader = nullptr;
    }
    if (solveEnvironmentCollisionConstraintCompShader != nullptr)
    {
        delete solveEnvironmentCollisionConstraintCompShader;
        solveEnvironmentCollisionConstraintCompShader = nullptr;
    }
    if (updatePredictPositionCompShader != nullptr)
    {
        delete updatePredictPositionCompShader;
        updatePredictPositionCompShader = nullptr;
    }
    if (updateVelocityCompShader != nullptr)
    {
        delete updateVelocityCompShader;
        updateVelocityCompShader = nullptr;
    }
    if (calculateOmegaCompShader != nullptr)
    {
        delete calculateOmegaCompShader;
        calculateOmegaCompShader = nullptr;
    }
    if (applyVorticityConfinementCompShader != nullptr)
    {
        delete applyVorticityConfinementCompShader;
        applyVorticityConfinementCompShader = nullptr;
    }
    if (applyXSPHVViscosityCompShader != nullptr)
    {
        delete applyXSPHVViscosityCompShader;
        applyXSPHVViscosityCompShader = nullptr;
    }
    if (updatePositionCompShader != nullptr)
    {
        delete updatePositionCompShader;
        updatePositionCompShader = nullptr;
    }

    if (SSFGetDepthShader != nullptr)
    {
        delete SSFGetDepthShader;
        SSFGetDepthShader = nullptr;
    }
    if (SSFGetThicknessShader != nullptr)
    {
        delete SSFGetThicknessShader;
        SSFGetThicknessShader = nullptr;
    }
    if (SSFBlurShader != nullptr)
    {
        delete SSFBlurShader;
        SSFBlurShader = nullptr;
    }
    if (SSFRenderShader != nullptr)
    {
        delete SSFRenderShader;
        SSFRenderShader = nullptr;
    }
}

Fluid::Fluid(Mesh* mesh) : mesh(mesh)
{
    std::vector<glm::vec3> p;
    mesh->voxelize(Particle::radius * 2.f, p);

    particleNr = static_cast<unsigned>(p.size());
    workGroupSize = 1024;
    workGroupNum = static_cast<unsigned>(ceil(particleNr * 1.f / workGroupSize));
    particles.resize(particleNr);
    for (unsigned i = 0; i < particleNr; i++)
    {
        particles[i].x = p[i] - glm::vec3(0.f, 1.f, 0.f);
        particles[i].m = 0.8f * powf(Particle::radius * 2.f, 3.f) * rho0;
        particles[i].f = particles[i].m * gravity;
        particles[i].w = 1.f / particles[i].m;
        particles[i].corr = glm::vec3(0.f);
        particles[i].phase = 3;
    }

    init_buffers();
}

Fluid::~Fluid()
{
    mesh = nullptr;

    glDeleteBuffers(1, &rhoBuffer);
    glDeleteBuffers(1, &lambdaBuffer);
    glDeleteBuffers(1, &omegaBuffer);
    glDeleteBuffers(1, &positionBuffer);

    glDeleteVertexArrays(1, &VAO);

    glDeleteTextures(1, &depthTexture);
    glDeleteTextures(1, &thicknessTexture);
    glDeleteTextures(2, blurDepthTex);
    glDeleteTextures(2, blurthicknessTex);

    glDeleteFramebuffers(1, &depthFBO);
    glDeleteFramebuffers(1, &thicknessFBO);
    glDeleteFramebuffers(2, blurFBO);
}

void Fluid::init_buffers()
{
    std::vector<glm::vec4> position(particleNr);
    for (unsigned i = 0; i < particleNr; i++)
        position[i] = glm::vec4(particles[i].x, 0.f);

    glGenBuffers(1, &rhoBuffer);
    glGenBuffers(1, &lambdaBuffer);
    glGenBuffers(1, &omegaBuffer);
    glGenBuffers(1, &positionBuffer);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, rhoBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * particleNr, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lambdaBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned) * particleNr, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, omegaBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * particleNr, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * particleNr, position.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, rhoBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &depthTexture);
    glGenTextures(1, &thicknessTexture);
    glGenTextures(2, blurDepthTex);
    glGenTextures(2, blurthicknessTex);

    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1600, 1200, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, thicknessTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1600, 1200, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    for (unsigned i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, blurDepthTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1600, 1200, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindTexture(GL_TEXTURE_2D, blurthicknessTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 1600, 1200, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glGenFramebuffers(1, &depthFBO);
    glGenFramebuffers(1, &thicknessFBO);
    glGenFramebuffers(2, blurFBO);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer error!" << std::endl;
        exit(-1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "Framebuffer error!" << std::endl;
        exit(-1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (unsigned i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, blurDepthTex[i], 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurthicknessTex[i], 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Framebuffer error!" << std::endl;
            exit(-1);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Fluid::bind_buffers()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, rhoBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, lambdaBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, omegaBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, positionBuffer);
}

void Fluid::predict_position()
{
    predictPositionCompShader->use();
    predictPositionCompShader->set_uint("head", head);
    predictPositionCompShader->set_uint("particleNum", particleNr);
    predictPositionCompShader->set_float("deltaTime", deltaTime);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::neighborhood_search()
{
    neighborhoodSearchCompShader->use();
    neighborhoodSearchCompShader->set_uint("head", head);
    neighborhoodSearchCompShader->set_uint("particleNum", particleNr);
    neighborhoodSearchCompShader->set_float("neighborRadius", h);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::calculate_lambda()
{
    calculateLambdaCompShader->use();
    calculateLambdaCompShader->set_uint("head", head);
    calculateLambdaCompShader->set_uint("particleNum", particleNr);
    calculateLambdaCompShader->set_float("h", h);
    calculateLambdaCompShader->set_float("rho0", rho0);
    calculateLambdaCompShader->set_float("solidPressure", solidPressure);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::solve_incompressibility_constraint()
{
    solveIncompressibilityConstraintCompShader->use();
    solveIncompressibilityConstraintCompShader->set_uint("head", head);
    solveIncompressibilityConstraintCompShader->set_uint("particleNum", particleNr);
    solveIncompressibilityConstraintCompShader->set_float("h", h);
    solveIncompressibilityConstraintCompShader->set_float("rho0", rho0);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::solve_environment_collision_constraint()
{
    solveEnvironmentCollisionConstraintCompShader->use();
    solveEnvironmentCollisionConstraintCompShader->set_uint("head", head);
    solveEnvironmentCollisionConstraintCompShader->set_uint("particleNum", particleNr);
    solveEnvironmentCollisionConstraintCompShader->set_float("particleRadius", Particle::radius);
    solveEnvironmentCollisionConstraintCompShader->set_float("mu_s", mu_s);
    solveEnvironmentCollisionConstraintCompShader->set_float("mu_k", mu_k);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::solve_particle_collision_constraint()
{
    solveParticleCollisionConstraintShader->use();
    solveParticleCollisionConstraintShader->set_uint("head", head);
    solveParticleCollisionConstraintShader->set_uint("particleNum", particleNr);
    solveParticleCollisionConstraintShader->set_float("radius", 2.f * Particle::radius);
    solveParticleCollisionConstraintShader->set_float("mu_s", mu_s);
    solveParticleCollisionConstraintShader->set_float("mu_s", mu_k);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    solveParticleCollisionConstraintShader->close();
}

void Fluid::update_predict_position()
{
    updatePredictPositionCompShader->use();
    updatePredictPositionCompShader->set_uint("head", head);
    updatePredictPositionCompShader->set_uint("particleNum", particleNr);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::update_velocity()
{
    updateVelocityCompShader->use();
    updateVelocityCompShader->set_uint("head", head);
    updateVelocityCompShader->set_uint("particleNum", particleNr);
    updateVelocityCompShader->set_float("deltaTime", deltaTime);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::apply_vorticity_confinement()
{
    calculateOmegaCompShader->use();
    calculateOmegaCompShader->set_uint("head", head);
    calculateOmegaCompShader->set_uint("particleNum", particleNr);
    calculateOmegaCompShader->set_float("h", h);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    applyVorticityConfinementCompShader->use();
    applyVorticityConfinementCompShader->set_uint("head", head);
    applyVorticityConfinementCompShader->set_uint("particleNum", particleNr);
    applyVorticityConfinementCompShader->set_float("h", h);
    applyVorticityConfinementCompShader->set_float("vorticity", vorticity);
    applyVorticityConfinementCompShader->set_float("deltaTime", deltaTime);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::apply_XSPHV_viscosity()
{
    applyXSPHVViscosityCompShader->use();
    applyXSPHVViscosityCompShader->set_uint("head", head);
    applyXSPHVViscosityCompShader->set_uint("particleNum", particleNr);
    applyXSPHVViscosityCompShader->set_float("h", h);
    applyXSPHVViscosityCompShader->set_float("viscosity", viscosity);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::update_position()
{
    updatePositionCompShader->use();
    updatePositionCompShader->set_uint("head", head);
    updatePositionCompShader->set_uint("particleNum", particleNr);

    glDispatchCompute(workGroupNum, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Fluid::draw()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, thicknessTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blurDepthTex[0]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blurthicknessTex[0]);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, blurDepthTex[1]);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, blurthicknessTex[1]);
    glActiveTexture(GL_TEXTURE0);

    get_depth();
    get_thickness();
    blur();
    shading();
}

void Fluid::get_depth()
{
    float pointScale = Global::screenHeight / tan(glm::radians(45.f) * 0.5f);

    SSFGetDepthShader->use();
    SSFGetDepthShader->set_mat4("model", model);
    SSFGetDepthShader->set_mat4("view", Global::mainCamera->view);
    SSFGetDepthShader->set_mat4("projection", Global::mainCamera->projection);
    SSFGetDepthShader->set_float("pointScale", pointScale);
    SSFGetDepthShader->set_float("pointSize", Particle::radius);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glClear(GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, particleNr);
    glBindVertexArray(0);

    glDisable(GL_PROGRAM_POINT_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fluid::get_thickness()
{
    float pointScale = Global::screenHeight / tan(glm::radians(45.f) * 0.5f);

    SSFGetThicknessShader->use();
    SSFGetThicknessShader->set_mat4("model", model);
    SSFGetThicknessShader->set_mat4("view", Global::mainCamera->view);
    SSFGetThicknessShader->set_mat4("projection", Global::mainCamera->projection);
    SSFGetThicknessShader->set_float("pointScale", pointScale);
    SSFGetThicknessShader->set_float("pointSize", Particle::radius);

    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, particleNr);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fluid::blur()
{
    for (unsigned i = 0; i < blurIteration; ++i)
    {
        SSFBlurShader->use();
        if (i == 0)
        {
            SSFBlurShader->set_int("depthTexture", 0);
            SSFBlurShader->set_int("thicknessTexture", 1);
        }
        else
        {
            SSFBlurShader->set_int("depthTexture", 4);
            SSFBlurShader->set_int("thicknessTexture", 5);
        }
        SSFBlurShader->set_vec2("blurDir", glm::vec2(1.f / Global::screenWidth, 0.f));
        SSFBlurShader->set_float("filterRadius", static_cast<float>(filterRadius));
        SSFBlurShader->set_float("spatialScale", spatialScale);
        SSFBlurShader->set_float("rangeScale", rangeScale);

        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[0]);
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(Global::screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        SSFBlurShader->set_int("depthTexture", 2);
        SSFBlurShader->set_int("thicknessTexture", 3);
        SSFBlurShader->set_vec2("blurDir", glm::vec2(0.f, 1.f / Global::screenHeight));

        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[1]);
        glClearColor(0.f, 0.f, 0.f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(Global::screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Fluid::shading()
{
    SSFRenderShader->use();
    SSFRenderShader->set_int("depthTexture", 4);
    SSFRenderShader->set_int("thicknessTexture", 5);
    SSFRenderShader->set_int("backgroundDepthTex", 6);
    SSFRenderShader->set_int("backgroundTex", 7);
    SSFRenderShader->set_mat4("projectionInv", glm::inverse(Global::mainCamera->projection));
    SSFRenderShader->set_mat4("view", Global::mainCamera->view);
    SSFRenderShader->set_vec3("material.diffuse", glm::vec3(0.7f, 0.9f, 0.9f));
    SSFRenderShader->set_vec3("material.specular", glm::vec3(1.f, 1.f, 1.f));
    SSFRenderShader->set_float("material.Ns", 400.f);
    Global::dirLights[0]->setup(*SSFRenderShader);

    glClearColor(0.f, 0.f, 0.f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(Global::screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}