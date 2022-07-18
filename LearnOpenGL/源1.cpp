
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include"Object.h"
#include"Texture.h"
#include"Scene.h"
#include"global.h"
#include <iostream>

void Debuger(Shader& Debug, unsigned int test);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int quadVAO = 0;
unsigned int quadVBO = 0;

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

unsigned int sphereVAO = 0;
unsigned int sphereVBO = 0;
unsigned int indexCount;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Initialize GLEW to setup the OpenGL Function pointers
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    shared_ptr<myTexture>light_gold_bl = make_shared<myTexture>(string("light-gold-bl"));
    shared_ptr<myTexture>rustediron1 = make_shared<myTexture>(string("rustediron1"));
    //unsigned int ao = loadTexture("resources/textures/pbr/rusted_iron/ao.png");

    // build and compile shaders
    // -------------------------
    Shader shader("Shader/Microfacets/vertexShader.txt", "Shader/Microfacets/fragmentShader.txt");
    Shader Lightshader("Shader/Light/vertexShader.txt", "Shader/Light/fragmentShader.txt");
    Shader Debug("Shader/Debug/vertexShader.txt", "Shader/Debug/fragmentShader.txt");
    Shader Gbuffer("Shader/GBuffer/vertexShader.txt", "Shader/GBuffer/fragmentShader.txt");
    Shader DeferredShading("Shader/DeferredShading--/vertexShader.txt", "Shader/DeferredShading--/fragmentShader.txt");

    shader.use();
    shader.setInt("albedoMap", 0);
    shader.setInt("normalMap", 1);
    shader.setInt("metallicMap", 2);
    shader.setInt("roughnessMap", 3);
    shader.setInt("irradianceMap", 4);
    shader.setInt("prefilterMap", 5);
    shader.setInt("brdfLUT", 6);
    shader.setInt("shadowMap", 7);

    // lights
    // ------
    glm::vec3 lightPositions[] = {
        glm::vec3(-10.0f,  10.0f, 10.0f),
        glm::vec3(10.0f,  10.0f, 10.0f),
        glm::vec3(-10.0f, -10.0f, 10.0f),
        glm::vec3(10.0f, -10.0f, 10.0f),
    };
    glm::vec3 lightColors[] = {
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f),
        glm::vec3(300.0f, 300.0f, 300.0f)
    };
    int nrRows = 9;
    int nrColumns = 9;
    float spacing = 2.5;

    Scene scene("Milkyway/Milkyway_small.hdr");
    scene.addLight(make_shared<Light>(lightPositions[0], lightColors[0]));
    //scene.addLight(make_shared<Light>(lightPositions[1], lightColors[1]));
   // scene.addLight(make_shared<Light>(lightPositions[2], lightColors[2]));
    //scene.addLight(make_shared<Light>(lightPositions[3], lightColors[3]));
    scene.setLight(shader);
    scene.setLight(DeferredShading);

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    mat4 model = mat4(1.0f);
    model = glm::translate(model, vec3(0.0f, -31.0f, 0.0f));
    //model = rotate(model, radians(90.0f), vec3(1.0f, 0.0f, 0.0f));
    model = scale(model, vec3(20.0f));
    scene.add(make_shared<Quad>(rustediron1, model));
    for (int row = 0; row <9; ++row)
    {
        for (int col = 0; col < 10; ++col)
        {

            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(
                (col - (nrColumns / 2)) * spacing,
                (row - (nrRows / 2)) * spacing,
                0.0f
            ));
            if(col>=5)scene.add(make_shared<Sphere>(light_gold_bl, model));
            else scene.add(make_shared<Cube>(light_gold_bl, model));
        }
    }
    // initialize static shader uniforms before rendering
    // --------------------------------------------------

    shader.use();
    shader.setMat4("projection", projection);
    mat4 lightView = glm::lookAt(lightPositions[0], glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 1.0f, 600.0f);
    mat4 inProjection = glm::inverse(projection);
    shader.setMat4("lightpvm", lightProjection * lightView);

    //setVec3("camPos", camera.Position);
    model = mat4(1.0);
    Gbuffer.use();
    Gbuffer.setMat4("projection", projection);


    DeferredShading.use();
    DeferredShading.setMat4("Lpv", lightProjection * lightView);
    DeferredShading.setMat4("inprojection", projection);
    Lightshader.use();
    Lightshader.setMat4("projection", projection);
    Lightshader.setMat4("model", model);


    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glm::mat4 view = camera.GetViewMatrix();
        shader.use();
        shader.setVec3("camPos", camera.Position);
        //scene.renderforword(shader, Lightshader, view);

        DeferredShading.use();
        DeferredShading.setVec3("camPos", camera.Position);
        DeferredShading.setMat4("vWorldToScreen", projection * view);
        DeferredShading.setMat4("view",  view);
        scene.GetGbuffer(Gbuffer, view);
        //
        scene.renderGbuffer(DeferredShading,view);
        //Debuger(Debug, scene.colorBuffers[0]);
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void Debuger(Shader& Debug,unsigned int test) {
    Debug.use();
    Debug.setInt("test", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, test);
    renderQuad();
}
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
/*unsigned int CreateShadowMap(Shader &shadowMapShaderm,Light light) {

}*/