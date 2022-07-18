#pragma once
#ifndef TEXTURE_H
#define TEXTURE_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<iostream>
using namespace std;
unsigned int loadTexture(char const* path);
class myTexture {
public:
    unsigned int albedo;
    unsigned int normal;
    unsigned int metallic;
    unsigned int roughness;
    myTexture(string texPath);
};
#endif // !TEXTURE_H
