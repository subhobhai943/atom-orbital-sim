#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    GLuint id = 0;

    Shader() = default;

    bool loadFromStrings(const std::string& vertexSrc, const std::string& fragmentSrc) {
        GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vertex || !fragment) return false;

        id = glCreateProgram();
        glAttachShader(id, vertex);
        glAttachShader(id, fragment);
        glLinkProgram(id);

        GLint success;
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(id, 1024, nullptr, infoLog);
            std::cerr << "[Shader Link Error]:\n" << infoLog << "\n";
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            glDeleteProgram(id);
            id = 0;
            return false;
        }

        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return true;
    }

    bool loadFromFiles(const std::string& vertPath, const std::string& fragPath) {
        std::string vertSrc = readFile(vertPath);
        std::string fragSrc = readFile(fragPath);
        if (vertSrc.empty() || fragSrc.empty()) {
            std::cerr << "[Shader File Error]: Failed to read " << vertPath << " or " << fragPath << "\n";
            return false;
        }
        return loadFromStrings(vertSrc, fragSrc);
    }

    void use() const {
        if (id) glUseProgram(id);
    }

    void setBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
    }
    void setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(id, name.c_str()), value);
    }
    void setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(id, name.c_str()), value);
    }
    void setVec2(const std::string& name, const glm::vec2& value) const {
        glUniform2fv(glGetUniformLocation(id, name.c_str()), 1, glm::value_ptr(value));
    }
    void setVec3(const std::string& name, const glm::vec3& value) const {
        glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, glm::value_ptr(value));
    }
    void setVec4(const std::string& name, const glm::vec4& value) const {
        glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, glm::value_ptr(value));
    }
    void setMat4(const std::string& name, const glm::mat4& mat) const {
        glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }

    ~Shader() {
        if (id) glDeleteProgram(id);
    }

private:
    static GLuint compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            std::cerr << "[Shader Compile Error (" << (type == GL_VERTEX_SHADER ? "VERT" : "FRAG") << ")]:\n"
                      << infoLog << "\n";
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    static std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
};

#endif // SHADER_H
