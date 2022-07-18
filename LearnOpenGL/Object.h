#pragma once
#ifndef OBJECT_H
#define OBJECT_H
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include"Texture.h"
using namespace glm;
using namespace std;
class Object {
public:
	mat4 model;
    unsigned int irradianceMap;
    unsigned int prefiltermap;
    unsigned int brdfMap;
    shared_ptr<myTexture> texture;
    virtual void render(Shader& shader);
    Object(shared_ptr<myTexture> Texture, mat4 M);
};
class Cube :public Object {
public:
    Cube(shared_ptr<myTexture> texture, mat4 M);
    static unsigned int cubeVAO ;
    static unsigned int cubeVBO ;
    void render(Shader& shader);
};
class Sphere :public Object {
public:
    Sphere(shared_ptr<myTexture> texture, mat4 M);
    static unsigned int sphereVAO;
    static unsigned int sphereVBO;
    static unsigned int indexCount;
    void render(Shader& shader);
};
class Quad :public Object {
public:
    Quad(shared_ptr<myTexture> texture, mat4 M);
    static unsigned int quadVAO; 
    static unsigned int quadVBO;
    void render(Shader& shader);
};
#endif // !OBJECT_H
