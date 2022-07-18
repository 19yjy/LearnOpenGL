#include"Scene.h"
#include"Texture.h"
#include"global.h"
unsigned int captureFBO;
unsigned int captureRBO;
unsigned int hdrFBO;
unsigned int shadowFBO;
unsigned int Gbuffer;
unsigned int pingpongFBO[2];

glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
glm::mat4 captureViews[] =
{
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};
Scene::Scene(const char* path, unsigned int WIDTH , unsigned int HEIGHT ) {
		SCR_WIDTH = WIDTH;
		SCR_HEIGHT = HEIGHT;
		equirectangularToCubemapShader = new Shader("Shader/HDR2Cube/vertexShader.txt", "Shader/HDR2Cube/fragmentShader.txt");
		backgroundShader = new Shader("Shader/SkyBox/vertexShader.txt", "Shader/SkyBox/fragmentShader.txt");
		Precompute = new Shader("Shader/SkyBox/vertexShader.txt", "Shader/precompute/fragmentShader.txt");
		prefilterShader = new Shader("Shader/prefilterShader/vertexShader.txt", "Shader/prefilterShader/fragmentShader.txt");
		brdfShader = new Shader("Shader/brdffilterShader/vertexShader.txt", "Shader/brdffilterShader/fragmentShader.txt");
		shadowShader = new Shader("Shader/ShadowMapping/vertexShader.txt", "Shader/ShadowMapping/fragmentShader.txt");
		blur = new Shader("Shader/Gaussian_blur/vertexShader.txt", "Shader/Gaussian_blur/fragmentShader.txt");
		ShowBack = new Shader("Shader/ShowBack/vertexShader.txt", "Shader/ShowBack/fragmentShader.txt");

		envCubemap = HDR2Cube(equirectangularToCubemapShader, path);
		irradianceMap = precompute(Precompute);
		prefiltermap = prefilterMap(prefilterShader);
		brdfMap = prebrdf(brdfShader);
		shadowMap = CreateShadow();
		CreateGbuffer();
		CreateHDR();
		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

		backgroundShader->use();
		backgroundShader->setInt("environmentMap", 0);
		backgroundShader->setMat4("projection", projection);


	}
	void Scene::render(Shader& shader, mat4 view) {
		shader.use();
		shader.setMat4("view", view);
		for (int i = 0;i < Objects.size();i++) {
			Objects[i]->render(shader);
		}
	}
	void Scene::renderLight(Shader& shader, mat4 view) {
		shader.use();

		shader.setMat4("view", view);
		for (int i = 0;i < Lights.size();i++) {
			Lights[i]->render(shader);
		}
	}
	void Scene::setLight(Shader& shader) {
		shader.use();
		for (int i = 0;i < Lights.size();i++) {
			shader.setVec3("lightPositions[" + std::to_string(i) + "]", Lights[i]->pos);
			shader.setVec3("lightColors[" + std::to_string(i) + "]", Lights[i]->color);
		}
	}
	unsigned int Scene::HDR2Cube(Shader* equirectangularToCubemapShader, const char* dir) {
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
		unsigned int hdrTexture=loadTexture(dir);
		unsigned int envCubemap;
		glGenTextures(1, &envCubemap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		// ----------------------------------------------------------------------------------------------

		equirectangularToCubemapShader->use();
		equirectangularToCubemapShader->setInt("equirectangularMap", 0);
		equirectangularToCubemapShader->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, hdrTexture);

		glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			equirectangularToCubemapShader->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return envCubemap;
	}
	unsigned int Scene::precompute(Shader* precompute) {
		unsigned int irradianceMap;
		glGenTextures(1, &irradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);


		// pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
		// ----------------------------------------------------------------------------------------------

		precompute->use();
		precompute->setInt("environmentMap", 0);
		precompute->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			precompute->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderCube();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return irradianceMap;
	}
	unsigned int Scene::prefilterMap(Shader* prefilterShader) {
		unsigned int prefilterMap;
		glGenTextures(1, &prefilterMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		prefilterShader->use();
		prefilterShader->setInt("environmentMap", 0);
		prefilterShader->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
		{
			// reisze framebuffer according to mip-level size.
			unsigned int mipWidth = 128 * std::pow(0.5, mip);
			unsigned int mipHeight = 128 * std::pow(0.5, mip);
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
			glViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			prefilterShader->setFloat("roughness", roughness);
			for (unsigned int i = 0; i < 6; ++i)
			{
				prefilterShader->setMat4("view", captureViews[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				renderCube();
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return prefilterMap;
	}
	unsigned int Scene::prebrdf(Shader* brdfShader) {
		unsigned int brdfLUTTexture;
		glGenTextures(1, &brdfLUTTexture);

		// pre-allocate enough memory for the LUT texture.
		glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
		// be sure to set wrapping mode to GL_CLAMP_TO_EDGE
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

		glViewport(0, 0, 512, 512);
		brdfShader->use();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		renderQuad();

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return brdfLUTTexture;
	}
	void Scene::add(shared_ptr<Object>obj) {
		obj->irradianceMap = irradianceMap;
		obj->prefiltermap = prefiltermap;
		obj->brdfMap = brdfMap;

		Objects.push_back(obj);
	}
	void Scene::addLight(shared_ptr<Light>obj) {

		Lights.push_back(obj);
	}
	void Scene::DrawSky(mat4 view) {
		backgroundShader->use();
		backgroundShader->setMat4("view", view);
		backgroundShader->setInt("environmentMap", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
		renderCube();
	}
	unsigned int Scene::CreateShadow() {

		glGenFramebuffers(1, &shadowFBO);
		glGenTextures(1, &shadowMap);
		glBindTexture(GL_TEXTURE_2D, shadowMap);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		//glDrawBuffer(GL_FRONT);
		//glReadBuffer(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return shadowMap;
	}
	void Scene::RenderShadow() {
		glCullFace(GL_FRONT);
		mat4 View = glm::lookAt(Lights[0]->pos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));

		mat4 lightProjection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 1.0f, 600.0f);

		shadowShader->use();
		shadowShader->setMat4("projection", lightProjection);
		shadowShader->setMat4("view", View);
		//shadowShader->setMat4("model", model);
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		render(*shadowShader, View);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);

	}
	void Scene::CreateGbuffer() {
		glGenFramebuffers(1, &Gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer);

		glGenTextures(1, &gPosition);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

		glGenTextures(1, &gNormal);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

		glGenTextures(1, &gAlbedo);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedo, 0);

		glGenTextures(1, &gRoughness);
		glBindTexture(GL_TEXTURE_2D, gRoughness);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gRoughness, 0);


		unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 };
		glDrawBuffers(4, attachments);

		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
		// - Finally check if framebuffer is complete
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}
	void Scene::GetGbuffer(Shader& shader, mat4 view) {
		glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		render(shader, view);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void Scene::renderGbuffer(Shader& shader, mat4 view) {

		RenderShadow();
		shader.use();
		shader.setInt("albedoMap", 0);
		shader.setInt("normalMap", 1);
		shader.setInt("gPos", 2);
		shader.setInt("roughnessMap", 3);
		shader.setInt("irradianceMap", 4);
		shader.setInt("prefilterMap", 5);
		shader.setInt("brdfLUT", 6);
		shader.setInt("shadowMap", 7);
		shader.setInt("gLightPos", 8);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gAlbedo);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gRoughness);/**/
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefiltermap);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, brdfMap);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, gLightpos);


		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader.use();
		renderQuad();
		glBindFramebuffer(GL_READ_FRAMEBUFFER, Gbuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, hdrFBO); // Ð´Èëµ½Ä¬ÈÏÖ¡»º³å
		glBlitFramebuffer(
			0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST
		);
		DrawSky(view);
		AddBloom();
		showBack(colorBuffers[0], pingpongBuffer[0]);


	}
	void Scene::renderforword(Shader& shader, Shader& Lightshader, mat4 view) {
		RenderShadow();
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		render(shader, view);
		renderLight(Lightshader, view);
		DrawSky(view);
	}
	void Scene::CreateHDR() {
		glGenFramebuffers(1, &hdrFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
		glGenTextures(2, colorBuffers);
		for (GLuint i = 0; i < 2; i++)
		{
			glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// attach texture to framebuffer
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
			);
		}

		glGenRenderbuffers(1, &rboDepth1);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth1);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth1);
		GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		glGenFramebuffers(2, pingpongFBO);
		glGenTextures(2, pingpongBuffer);
		for (GLuint i = 0; i < 2; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
			glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
			glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongBuffer[i], 0
			);
			glGenerateMipmap(GL_TEXTURE_2D);
		}/**/

	}
	void Scene::AddBloom() {
		bool horizontal = true;
		int amount = 8;
		blur->use();
		blur->setInt("image", 0);
		for (int i = 0; i < amount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
			glActiveTexture(GL_TEXTURE0);
			blur->setBool("horizontal", horizontal);
			glBindTexture(GL_TEXTURE_2D, i == 0 ? colorBuffers[1] : pingpongBuffer[!horizontal]);
			renderQuad();
			horizontal = !horizontal;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void Scene::showBack(unsigned int a, unsigned int b) {
		ShowBack->use();
		ShowBack->setInt("image", 0);
		ShowBack->setInt("image2", 1);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, a);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, b);
		renderQuad();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
