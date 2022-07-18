
#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader();
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = NULL);
    ~Shader();
    // activate the shader
    // ------------------------------------------------------------------------
    void use();
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const;
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const;
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, glm::vec4 tmp) const;
    void setVec3(const std::string& name, glm::vec3 tmp) const;
    void setMat4(const std::string& name, glm::mat4 trans) const;
    /*void setDirLight(const DirLight& light) const
    {
        setVec3(light.name + ".color", light.color);
        setVec3(light.name + ".direction", light.dir);
    }
    void setPointLight(const PointLight& light) const
    {
        setVec3(light.name + ".color", light.color);
        setVec3(light.name + ".position", light.pos);
        setFloat(light.name + ".linear", light.linear);
        setFloat(light.name + ".constant", light.constant);
        setFloat(light.name + ".quadratic", light.quadratic);
    }*/
private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type);
    
};
#endif