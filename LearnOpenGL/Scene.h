#pragma once
#ifndef SCENE_H
#define SCENE_H
#include<iostream>
#include<vector>
#include"Object.h"
#include"Light.h"
using namespace std;

class Scene {
public:
	unsigned int SCR_WIDTH;
	unsigned int SCR_HEIGHT;
	const unsigned int SHADOW_WIDTH=1024;
	const unsigned int SHADOW_HEIGHT=1024;

	unsigned int envCubemap;
	unsigned int irradianceMap;
	unsigned int prefiltermap;
	unsigned int brdfMap;

	unsigned int shadowMap;

	unsigned int gPosition;
	unsigned int gNormal;
	unsigned int gAlbedo;
	unsigned int gRoughness;
	unsigned int gLightpos;
	unsigned int rboDepth;
	unsigned int rboDepth1;
	unsigned int colorBuffers[2];
	unsigned int pingpongBuffer[2];

	Shader *equirectangularToCubemapShader;
	Shader *backgroundShader;
	Shader *Precompute;
	Shader *prefilterShader;
	Shader *brdfShader;
	Shader* shadowShader;
	Shader* blur;
	Shader* ShowBack;
	vector<shared_ptr<Object>>Objects;
	vector<shared_ptr<Light>>Lights;
	Scene(const char* path, unsigned int WIDTH = 1280, unsigned int HEIGHT = 720);
	void render(Shader& shader, mat4 view);
	void renderLight(Shader& shader, mat4 view);
	void setLight(Shader& shader);
	unsigned int HDR2Cube(Shader* equirectangularToCubemapShader, const char* dir);
	unsigned int precompute(Shader* precompute);
	unsigned int prefilterMap(Shader* prefilterShader);
	unsigned int prebrdf(Shader* brdfShader);
	void add(shared_ptr<Object>obj);
	void addLight(shared_ptr<Light>obj);
	void DrawSky(mat4 view);
	unsigned int CreateShadow();
	void RenderShadow();
	void CreateGbuffer();
	void GetGbuffer(Shader& shader, mat4 view);
	void renderGbuffer(Shader& shader, mat4 view);
	void renderforword(Shader& shader, Shader& Lightshader, mat4 view);
	void CreateHDR();
	void AddBloom();
	void showBack(unsigned int a, unsigned int b);
	
};
#endif // !SCENE_H
