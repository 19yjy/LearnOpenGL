#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<string.h>
#include"global.h"
using namespace std;
using namespace glm;
class Light {
public:
	vec3 color;
	string name;
	vec3 pos;
	void render(Shader &shader) {
		shader.use();
		mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, pos);
		model = glm::scale(model, glm::vec3(0.5f));
		shader.setMat4("model", model);
		renderSphere();
	}
	Light() {}
	Light(vec3 P, vec3 C) {
		pos = P;
		color = C;
	}

};
class DirLight :public Light {
public:
	vec3 dir;
	DirLight(string Name,vec3 COL,vec3 DIR) {
		name = Name;
		color = COL;
		dir = DIR;
		pos = -1000.0f * dir;
	}
};
class PointLight :public Light {
public:
	float constant;
	float linear;
	float quadratic;
	PointLight(string Name, vec3 COL, vec3 Pos, float c, float l, float q) {
		name = Name;
		color = COL;
		pos = Pos;
		constant = c;
		linear = l;
		quadratic = q;
	}
};
#endif // LIGHT_H
