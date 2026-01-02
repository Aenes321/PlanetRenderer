#include <cstdio>
#include <array>

#include "utility.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp> // for matrix calculation

struct Planet
{
    glm::vec3 position;
    glm::vec3 prev_position = glm::vec3(0.0f);
    int parent_index;
    float radius;
    float orbitRadius;
    float orbitSpeed;
    float selfRotationSpeed;
    float current_orbit_angle;
    float current_self_angle;
};

float g_Yaw = -90.0f;
float g_Pitch = 0.0f;
float g_LastX = 640.0f;
float g_LastY = 360.0f;
bool g_FirstMouse = true;
bool g_LeftMouseDown = false;

float g_TimeSpeed = 1.0f;
float g_CurrentTime = 0.0f;
double g_LastFrameTime = 0.0;
// 0 earth 1 moon 2 moons moon 3 fps
int g_CamModeIndex = 0;

std::vector<Planet> g_Planets;

void UpdateCamera(GLState* state, double dt);
void MouseMoveCallback(GLFWwindow* wnd, double x, double y)
{
    GLState* state = static_cast<GLState*> (glfwGetWindowUserPointer(wnd));
    if(g_LeftMouseDown)
    {
        if(g_FirstMouse)
        {
            g_LastX = float(x);
            g_LastY = float(y);
            g_FirstMouse = false;
        }

        float xoffset = float(x) - g_LastX;
        float yoffset = g_LastY - float(y);
        g_LastX = float(x);
        g_LastY = float(y);

        float sensivity = 0.1f;
        xoffset *= sensivity;
        yoffset *= sensivity;

        g_Yaw += xoffset;
        g_Pitch += yoffset;
        
        if(g_Pitch > 89.0f)
        {
            g_Pitch = 89.0f;
        }
        if(g_Pitch < -89.0f)
        {
            g_Pitch = -89.0f;
        }
    }
    else 
    {
        g_FirstMouse = true;
    }
}

void MouseButtonCallback(GLFWwindow* wnd, int button, int action, int)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if(action == GLFW_PRESS)
        {
            g_LeftMouseDown = true;
            double xpos, ypos;
            glfwGetCursorPos(wnd,&xpos,&ypos);
            g_LastX = float(xpos);
            g_LastY = float(ypos);
        }
        else if(action == GLFW_RELEASE)
        {
            g_LeftMouseDown = false;
        }
    }
}

void MouseScrollCallback(GLFWwindow* wnd, double dx, double dy)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    float zoomSpeed = 0.5f;
    state->pos += state->gaze * float(dy) * zoomSpeed;
}

void FramebufferChangeCallback(GLFWwindow* wnd, int w, int h)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    state->width = w;
    state->height = h;
}

void KeyboardCallback(GLFWwindow* wnd, int key, int scancode, int action, int modifier)
{
    GLState* state = static_cast<GLState*>(glfwGetWindowUserPointer(wnd));
    if(action == GLFW_PRESS)
    {
        if(key == GLFW_KEY_P)
        {
            g_CamModeIndex = (g_CamModeIndex + 1) % 4;
        }
        if(key == GLFW_KEY_O)
        {
            g_CamModeIndex = (g_CamModeIndex + 3) % 4;
        }
        float time_step = 0.5f;
        if(key == GLFW_KEY_L)
        {
            g_TimeSpeed += time_step;
        }
        if(key == GLFW_KEY_K)
        {
            g_TimeSpeed -= time_step;
        }
    }
}

void InitPlanets()
{
    g_Planets.clear();
    Planet earth;
    earth.radius = 1.0f;
    earth.orbitRadius = 0.0f;
    earth.orbitSpeed = 0.0f;
    earth.selfRotationSpeed = 10.0f;
    earth.parent_index = -1;
    earth.position = glm::vec3(0,0,0);
    g_Planets.push_back(earth);

    Planet moon;
    moon.radius = 0.27f;
    moon.orbitRadius = 6.0f;
    moon.orbitSpeed = 20.0f;
    moon.selfRotationSpeed = 20.0f;
    moon.parent_index = 0;
    g_Planets.push_back(moon);

    Planet moon_moon;
    moon_moon.radius = 0.1f;
    moon_moon.orbitRadius = 0.8f;
    moon_moon.orbitSpeed = 50.0f;
    moon_moon.selfRotationSpeed = 100.0f;
    moon_moon.parent_index = 1;
    g_Planets.push_back(moon_moon);
}
int main(int argc, const char* argv[])
{
    GLState state = GLState("Planet Renderer", 1280, 720,
                            CallbackPointersGLFW());
    state.pos = glm::vec3(0,0,5);
    state.gaze = glm::vec3(0,0,-1);

    state.mode = 5;

    ShaderGL vShader = ShaderGL(ShaderGL::VERTEX, "shaders/generic.vert");
    ShaderGL fShader = ShaderGL(ShaderGL::FRAGMENT, "shaders/debug.frag");

    MeshGL mesh = MeshGL("meshes/sphere_80k.obj");
    TextureGL texEarth = TextureGL("textures/2k_earth_daymap.jpg",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texEarthSpec = TextureGL("textures/2k_earth_specular_map.png",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texEarthNight = TextureGL("textures/2k_earth_nightmap_alpha.png",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texEarthClouds = TextureGL("textures/2k_earth_clouds_alpha.png",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texMoon = TextureGL("textures/2k_moon.jpg",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texMoon_moon = TextureGL("textures/2k_jupiter.jpg",
                              TextureGL::LINEAR, TextureGL::REPEAT);
    TextureGL texStars = TextureGL("textures/8k_stars_milky_way.jpg",
                              TextureGL::LINEAR, TextureGL::REPEAT);

    InitPlanets();
    // Set unchanged state(s)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);

    static constexpr GLuint U_TRANSFORM_MODEL = 0;
    static constexpr GLuint U_TRANSFORM_VIEW = 1;
    static constexpr GLuint U_TRANSFORM_PROJ = 2;
    static constexpr GLuint U_TRANSFORM_NORMAL = 3;
    static constexpr GLuint U_MODE = 0;
    static constexpr GLuint T_ALBEDO = 0;
    static constexpr GLuint T_SPECULAR = 1;
    static constexpr GLuint T_NIGHT = 2;
    static constexpr GLuint T_SHADOW = 3;
    static constexpr GLuint U_CAM_POS = 4;
    static constexpr GLuint U_LIGHT_DIR = 5;
    static constexpr GLuint U_LIGHT_VP = 6;
    
    
    GLuint shadowFBO;
    glGenFramebuffers(1, &shadowFBO);
    GLuint shadowMapTexture;
    glGenTextures(1, &shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 4096,4096,0, GL_RED, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    GLuint shadowDepth;
    glGenRenderbuffers(1, &shadowDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, shadowDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 4096, 4096);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadowDepth);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // =============== //
    //   RENDER LOOP   //
    // =============== //
    while(!glfwWindowShouldClose(state.window))
    {
        // Poll inputs from the OS via GLFW
        glfwPollEvents();

        double now = glfwGetTime();
        float dt = float(now - g_LastFrameTime);
        g_LastFrameTime = now;

        g_CurrentTime += dt * g_TimeSpeed;
        glm::vec3 cameraMoveDelta = glm::vec3(0.0f);
        for(size_t i = 0; i < g_Planets.size(); i++)
        {
            Planet& p = g_Planets[i];
            p.prev_position = p.position;
            p.current_orbit_angle = p.orbitSpeed * g_CurrentTime;
            p.current_self_angle = p.selfRotationSpeed * g_CurrentTime;

            if(p.parent_index == -1)
            {
                p.position = glm::vec3(0,0,0);
            }
            else
            {
                glm::vec3 parent_pos = g_Planets[p.parent_index].position;
                float rad = glm::radians(p.current_orbit_angle);

                float x = p.orbitRadius * sin(rad);
                float z = p.orbitRadius * cos(rad);
                p.position = parent_pos + glm::vec3(x,0,z);
            }

            if((int)i == g_CamModeIndex)
            {
                cameraMoveDelta = p.position - p.prev_position;
            }
        }

        float sunAngle = g_CurrentTime * 5.0f;
        glm::mat4 sunRot = glm::rotate(glm::mat4(1.0f), glm::radians(sunAngle),glm::vec3(0,1,0));
        glm::vec3 currentSunDir = glm::vec3(sunRot * glm::vec4(0.0,0.0,1.0,0.0));

        state.pos += cameraMoveDelta;
        glm::vec3 mouseFront;
        mouseFront.x = cos(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch));
        mouseFront.y = sin(glm::radians(g_Pitch));
        mouseFront.z = sin(glm::radians(g_Yaw)) * cos(glm::radians(g_Pitch));
        mouseFront = glm::normalize(mouseFront);
        float moveSpeed = 5.0f * dt;
        glm::vec3 right = glm::normalize(glm::cross(state.gaze, state.up));

        if(g_CamModeIndex == 3)
        {
            state.gaze = mouseFront;
            if(glfwGetKey(state.window, GLFW_KEY_W) == GLFW_PRESS)
            {
                state.pos += state.gaze * moveSpeed;
            }
            if(glfwGetKey(state.window, GLFW_KEY_A) == GLFW_PRESS)
            {
                state.pos -= right * moveSpeed;
            }
            if(glfwGetKey(state.window, GLFW_KEY_S) == GLFW_PRESS)
            {
                state.pos -= state.gaze * moveSpeed;
            }
            if(glfwGetKey(state.window, GLFW_KEY_D) == GLFW_PRESS)
            {
                state.pos += right * moveSpeed;
            }
        }

        else if(g_CamModeIndex >= 0 && g_CamModeIndex < (int) g_Planets.size())
        {
            Planet& target_planet = g_Planets[g_CamModeIndex];
            float dist = glm::distance(state.pos,target_planet.position);
            if(dist < target_planet.radius*1.2f)
            {
                dist = target_planet.radius * 1.5f;
            }
            state.pos = target_planet.position - (mouseFront * dist);
            state.gaze = mouseFront;
        }

        // render
        glViewport(0, 0, state.width, state.height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 lightView = glm::lookAt(currentSunDir * 40.0f, glm::vec3(0,0,0),glm::vec3(0,1,0));
        float orthoBox = 15.0f;
        glm::mat4 lightProj = glm::ortho(-orthoBox, orthoBox, -orthoBox, orthoBox, 0.0f,100.0f);
        glm::mat4 light_VP = lightProj * lightView;
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glViewport(0,0,4096,4096);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
        glUniform1ui(U_MODE, 4);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
        glUniformMatrix4fv(U_TRANSFORM_VIEW, 1, false, glm::value_ptr(lightView));
        glUniformMatrix4fv(U_TRANSFORM_PROJ, 1, false, glm::value_ptr(lightProj));
        {
            Planet& p = g_Planets[0];
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
            model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
            model = glm::scale(model,glm::vec3(p.radius));
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        {
            Planet& p = g_Planets[1];
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
            model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
            model = glm::scale(model,glm::vec3(p.radius));
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        {
            Planet& p = g_Planets[2];
            glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
            model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
            model = glm::scale(model,glm::vec3(p.radius));
            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0, state.width, state.height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);

        glUseProgramStages(state.renderPipeline, GL_VERTEX_SHADER_BIT, vShader.shaderId);
        glUseProgramStages(state.renderPipeline, GL_FRAGMENT_SHADER_BIT, fShader.shaderId);
        glActiveShaderProgram(state.renderPipeline, vShader.shaderId);

        {
            glDisable(GL_DEPTH_TEST);
            float orthoSize = 20.0f;
            glm::mat4 projOrtho = glm::ortho(-orthoSize,orthoSize,-orthoSize,orthoSize,-100.0f,100.0f);
            glm::mat4 viewOrtho = glm::lookAt(glm::vec3(0,0,0), state.gaze,state.up);
            glm::mat4 model_stars = glm::scale(glm::mat4(1.0f),glm::vec3(50.0f));
            glUniformMatrix4fv(U_TRANSFORM_MODEL,1,false,glm::value_ptr(model_stars));
            glUniformMatrix4fv(U_TRANSFORM_VIEW,1,false,glm::value_ptr(viewOrtho));
            glUniformMatrix4fv(U_TRANSFORM_PROJ,1,false,glm::value_ptr(projOrtho));

            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
            glBindTexture(GL_TEXTURE_2D, texStars.textureId);
            glUniform1ui(U_MODE, 3);
            glBindVertexArray(mesh.vaoId);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,nullptr);
            glEnable(GL_DEPTH_TEST);
        }
        // Sun
        {
            glDisable(GL_DEPTH_TEST);
            float orthoSize = 20.0f;
            float sunDistance = 40.0f;
            float sunScale = 0.3f;
            glm::mat4 projOrtho = glm::ortho(-orthoSize,orthoSize,-orthoSize,orthoSize,0.0f,100.0f);
            glm::mat4 viewOrtho = glm::lookAt(glm::vec3(0,0,0), state.gaze,state.up);
            
            glm::vec3 sunPos = currentSunDir * sunDistance;
            glm::mat4 model_sun = glm::translate(glm::mat4(1.0f),sunPos);
            model_sun = glm::scale(model_sun,glm::vec3(sunScale));

            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(U_TRANSFORM_MODEL,1,false,glm::value_ptr(model_sun));
            glUniformMatrix4fv(U_TRANSFORM_VIEW,1,false,glm::value_ptr(viewOrtho));
            glUniformMatrix4fv(U_TRANSFORM_PROJ,1,false,glm::value_ptr(projOrtho));
            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            glUniform1ui(U_MODE, 0); 
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT,nullptr);
            glEnable(GL_DEPTH_TEST);
        }

        {
            glm::mat4 proj = glm::perspective(glm::radians(45.0f),float(state.width)/float(state.height),0.1f,1000.0f);
            glm::mat4 view = glm::lookAt(state.pos, state.pos + state.gaze,state.up);
            
            glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
            glUniform3fv(U_CAM_POS,1,glm::value_ptr(state.pos));
            glUniform3fv(U_LIGHT_DIR,1,glm::value_ptr(currentSunDir));
            glUniformMatrix4fv(U_LIGHT_VP, 1, false, glm::value_ptr(light_VP));

            glActiveTexture(GL_TEXTURE0 + T_SHADOW);
            glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
            glUniformMatrix4fv(U_LIGHT_VP, 1, false, glm::value_ptr(light_VP));

            glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
            glUniformMatrix4fv(U_TRANSFORM_VIEW,1,false,glm::value_ptr(view));
            glUniformMatrix4fv(U_TRANSFORM_PROJ,1,false,glm::value_ptr(proj));
            // Earth
            {
                Planet& p = g_Planets[0];
                glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
                model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
                model = glm::scale(model,glm::vec3(p.radius));
                glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));
                glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
                glUniformMatrix4fv(U_TRANSFORM_MODEL,1,false,glm::value_ptr(model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL,1,false,glm::value_ptr(normalMatrix));

                glActiveShaderProgram(state.renderPipeline,fShader.shaderId);

                glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
                glBindTexture(GL_TEXTURE_2D, texEarth.textureId);

                glActiveTexture(GL_TEXTURE0 + T_SPECULAR);
                glBindTexture(GL_TEXTURE_2D, texEarthSpec.textureId);

                glActiveTexture(GL_TEXTURE0 + T_NIGHT);
                glBindTexture(GL_TEXTURE_2D, texEarthNight.textureId);

                glUniform1ui(U_MODE, 6);

                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            }
            // Cloud
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                Planet& p = g_Planets[0];
                glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
                float cloudRotation = p.current_self_angle * 0.18f;
                model = glm::rotate(model, glm::radians(cloudRotation), glm::vec3(0,1,0));
                model = glm::scale(model, glm::vec3(p.radius * 1.02f));
                glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));
                glActiveShaderProgram(state.renderPipeline, vShader.shaderId);
                glUniformMatrix4fv(U_TRANSFORM_MODEL, 1, false, glm::value_ptr(model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL, 1, false, glm::value_ptr(normalMatrix));
                glActiveShaderProgram(state.renderPipeline, fShader.shaderId);
                glActiveTexture(GL_TEXTURE0 + T_ALBEDO);
                glBindTexture(GL_TEXTURE_2D, texEarthClouds.textureId);
                glUniform1ui(U_MODE, 7);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glDisable(GL_BLEND);
            }
            // Moon
            {
                Planet& p = g_Planets[1];
                glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
                model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
                model = glm::scale(model,glm::vec3(p.radius));
                glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));
                glUniform1ui(U_MODE, 5);
                glActiveShaderProgram(state.renderPipeline,vShader.shaderId);
                glUniformMatrix4fv(U_TRANSFORM_MODEL,1,false,glm::value_ptr(model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL,1,false,glm::value_ptr(normalMatrix));

                glActiveShaderProgram(state.renderPipeline,fShader.shaderId);
                glBindTexture(GL_TEXTURE_2D, texMoon.textureId);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            }

            {
                Planet& p = g_Planets[2];
                glm::mat4 model = glm::translate(glm::mat4(1.0f), p.position);
                model = glm::rotate(model, glm::radians(p.current_self_angle), glm::vec3(0,1,0));
                model = glm::scale(model,glm::vec3(p.radius));
                glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(model));
                glUniform1ui(U_MODE, state.mode);
                glActiveShaderProgram(state.renderPipeline,vShader.shaderId);
                glUniformMatrix4fv(U_TRANSFORM_MODEL,1,false,glm::value_ptr(model));
                glUniformMatrix3fv(U_TRANSFORM_NORMAL,1,false,glm::value_ptr(normalMatrix));

                glActiveShaderProgram(state.renderPipeline,fShader.shaderId);
                glBindTexture(GL_TEXTURE_2D, texMoon_moon.textureId);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            }
            
        }
        glfwSwapBuffers(state.window);
    }

}
