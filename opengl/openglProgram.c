#include "../win32.c"
#include <gl/gl.h>

#include "glFlags.h"
#include "glFunctions.c"

GLuint CompileShader(GLuint shaderEnum, const char* source)
{
    GLuint shader = glCreateShader(shaderEnum);
    glShaderSource(shader, 1, &source, NULL);

    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    char *shaderName = shaderEnum == GL_VERTEX_SHADER ? "Vertex" : "Fragmment";
    if (success)
    {
        OutputDebugStringA(shaderName);
        OutputDebugStringA("Shader Compiled\n");
    }
    else
    {
        OutputDebugStringA(shaderName);
        OutputDebugStringA("Shader Errors\n");

        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        OutputDebugStringA(infoLog);
        OutputDebugStringA("\n");
    }
    return shader;
}


GLuint CreateProgram(char *vertexShaderPath, char *fragmentShaderPath)
{
    FileContent vertexFile   = ReadMyFileImp(vertexShaderPath);
    FileContent fragmentFile = ReadMyFileImp(fragmentShaderPath);

    GLuint vertexShader      = CompileShader(GL_VERTEX_SHADER, vertexFile.content);
    GLuint fragmentShader    = CompileShader(GL_FRAGMENT_SHADER, fragmentFile.content);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success)
        OutputDebugStringA("Program Linked\n");
    else 
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        OutputDebugStringA("Error during linking: \n");
        OutputDebugStringA(infoLog);
        OutputDebugStringA("\n");
    }


    //TODO: add error checking
    VirtualFreeMemory(fragmentFile.content);
    VirtualFreeMemory(vertexFile.content);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}