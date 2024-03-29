#include "Render.h"

HRESULT fillBMP(GLenum target, std::string name)
{
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int width, height;
    unsigned int imageSize;
    char* data = 0;

    // Open the file
    FILE * file = fopen(name.c_str(), "rb");
    if (!file)
        return E_FAIL;
    if (fread(header, 1, 54, file) != 54)
        return E_FAIL;

    if (header[0] != 'B' || header[1] != 'M')
        return E_FAIL;

    // Read ints from the byte array
    dataPos = *(int*)&(header[0x0A]);
    imageSize = *(int*)&(header[0x22]);
    width = *(int*)&(header[0x12]);
    height = *(int*)&(header[0x16]);

    if (imageSize == 0)    imageSize = width*height * 3;    // 3 : one byte for each Red, Green and Blue component
    if (dataPos == 0)      dataPos = 54;                    // The BMP header is done that way
    data = new char[imageSize];
    if (!data)
        return E_FAIL;
    fread(data, 1, imageSize, file);
    fclose(file);

    glTexImage2D(target, 0, GL_RGB, width, height, 0, GL_BGR, GL_UNSIGNED_BYTE, data);

    delete[] data;

    return S_OK;
}

GLuint aa::render::CreateTexture2D(char const* filename)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (fillBMP(GL_TEXTURE_2D, filename) != S_OK)
        return 0;
    return tex;
}

struct TexturedQuad
{
    GLuint vao, vbo, program;
    GLint uloc_tex, uloc_res, uloc_dim;

    TexturedQuad()
    {
        // init shaders
        program = glCreateProgram();
        {
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            static const char* vs_shdr = GLSLify(330,
                in vec2 iPos;
                uniform vec2 uRes;
                uniform vec4 uDim;
                out vec2 uv;

                void main()
                {
                    vec2 sz = 2.0 * (uDim.zw / uRes);
                    vec2 disp = 2.0 * (uDim.xy / uRes);
                    vec2 p = iPos * 0.5 + vec2(0.5, 0.5);
                    p *= sz;
                    p += disp - vec2(1, 1);
                    gl_Position = vec4(p.x, -p.y, 0.5, 1.0);

                    uv = (iPos + vec2(1.0)) * 0.5;
                }
            );
            glShaderSource(vs, 1, &vs_shdr, NULL);
            glCompileShader(vs);
            glAttachShader(program, vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fs_shdr = GLSLify(330,
                in vec2 uv;
                uniform sampler2D uTex;
                out vec4 fragColor;

                void main()
                {
                    fragColor = texture(uTex, uv);
                    //fragColor = vec4(uv, 0.0, 1.0);
                }
            );
            glShaderSource(fs, 1, &fs_shdr, NULL);
            glCompileShader(fs);
            glAttachShader(program, fs);

            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            {
                printf("Textured quad shader info:\n");
                int infologLen = 0;
                int charsWritten = 0;
                GLchar *infoLog = NULL;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
                if (infologLen > 0)
                {
                    infoLog = (GLchar*)malloc(infologLen);
                    if (infoLog == NULL)
                        return;
                    glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
                }
                printf("%s\n", infoLog);
                delete[] infoLog;
            }
        }
        uloc_res = glGetUniformLocation(program, "uRes");
        uloc_tex = glGetUniformLocation(program, "uTex");
        uloc_dim = glGetUniformLocation(program, "uDim");

        // init fs quad
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        GLfloat verts[12] = { -1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1 };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    void Draw(GLuint texture, unsigned x, unsigned y, unsigned width, unsigned height)
    {
        glUseProgram(program);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(uloc_tex, 0);
        int dims[4];
        glGetIntegerv(GL_VIEWPORT, dims);
        glUniform2f(uloc_res, dims[2], dims[3]);
        glUniform4f(uloc_dim, (float)x, (float)y, (float)width, (float)height);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    ~TexturedQuad()
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;

        glDeleteProgram(program);
        program = 0;
    }
};

void aa::render::DrawTexturedQuad(GLuint texture, unsigned x, unsigned y, unsigned width, unsigned height)
{
    static TexturedQuad qr;
    qr.Draw(texture, x, y, width, height);
}

GLuint aa::render::CreteTextureCubemap(const char* filenames[6])
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    for (GLuint i = 0; i < 6; i++)
    {
        fillBMP(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, filenames[i]);
    }
    return tex;
}

struct CubemapLatlong
{
    GLuint vao, vbo, program;
    GLint uloc_tex, uloc_res, uloc_dim;

    CubemapLatlong()
    {
        // init shaders
        program = glCreateProgram();
        {
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            static const char* vs_shdr = GLSLify(330,
                in vec2 iPos;
                uniform vec2 uRes;
                uniform vec4 uDim;
                out vec2 uv;

                void main()
                {
                    vec2 sz = 2.0 * (uDim.zw / uRes);
                    vec2 disp = 2.0 * (uDim.xy / uRes);
                    vec2 p = iPos * 0.5 + vec2(0.5, 0.5);
                    p *= sz;
                    p += disp - vec2(1, 1);
                    gl_Position = vec4(p.x, -p.y, 0.5, 1.0);

                    uv = (iPos + vec2(1.0)) * 0.5;
                }
            );
            glShaderSource(vs, 1, &vs_shdr, NULL);
            glCompileShader(vs);
            glAttachShader(program, vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fs_shdr = GLSLify(330,
                /* Cubemap as latlong shader by:
                Dario Manesku (https://github.com/dariomanesku) */
                in vec2 uv;
                uniform samplerCube uTex;
                out vec4 fragColor;

                vec3 vecFromLatLong(vec2 _uv)
                {
                    float pi = 3.14159265;
                    float twoPi = 2.0*pi;
                    float phi = _uv.x * twoPi;
                    float theta = _uv.y * pi;

                    vec3 result;
                    result.x = -sin(theta)*sin(phi);
                    result.y = -cos(theta);
                    result.z = -sin(theta)*cos(phi);

                    return result;
                }

                void main()
                {
                    vec3 dir = vecFromLatLong(uv);
                    fragColor = texture(uTex, dir);
                }
            );
            glShaderSource(fs, 1, &fs_shdr, NULL);
            glCompileShader(fs);
            glAttachShader(program, fs);

            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            {
                printf("Latlong cubemap shader info:\n");
                int infologLen = 0;
                int charsWritten = 0;
                GLchar *infoLog = NULL;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
                if (infologLen > 0)
                {
                    infoLog = (GLchar*)malloc(infologLen);
                    if (infoLog == NULL)
                        return;
                    glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
                }
                printf("%s\n", infoLog);
                delete[] infoLog;
            }
        }
        uloc_res = glGetUniformLocation(program, "uRes");
        uloc_tex = glGetUniformLocation(program, "uTex");
        uloc_dim = glGetUniformLocation(program, "uDim");

        // init fs quad
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        GLfloat verts[12] = { -1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1 };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    void Draw(GLuint texture, unsigned x, unsigned y, unsigned width, unsigned height)
    {
        glUseProgram(program);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        glUniform1i(uloc_tex, 0);
        int dims[4];
        glGetIntegerv(GL_VIEWPORT, dims);
        glUniform2f(uloc_res, dims[2], dims[3]);
        glUniform4f(uloc_dim, (float)x, (float)y, (float)width, (float)height);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    ~CubemapLatlong()
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;

        glDeleteProgram(program);
        program = 0;
    }
};

void aa::render::DrawCubemapAsLatlong(GLuint cubemap, unsigned x, unsigned y, unsigned width, unsigned height)
{
    static CubemapLatlong cll;
    cll.Draw(cubemap, x, y, width, height);
}

struct CubemapProbe
{
    GLuint vao, vbo, program;
    GLint uloc_tex, uloc_res, uloc_dim;

    CubemapProbe()
    {
        // init shaders
        program = glCreateProgram();
        {
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            static const char* vs_shdr = GLSLify(330,
                in vec2 iPos;
            uniform vec2 uRes;
            uniform vec4 uDim;
            out vec2 uv;

            void main()
            {
                vec2 sz = 2.0 * (uDim.zw / uRes);
                vec2 disp = 2.0 * (uDim.xy / uRes);
                vec2 p = iPos * 0.5 + vec2(0.5, 0.5);
                p *= sz;
                p += disp - vec2(1, 1);
                gl_Position = vec4(p.x, -p.y, 0.5, 1.0);

                uv = (iPos + vec2(1.0)) * 0.5;
            }
            );
            glShaderSource(vs, 1, &vs_shdr, NULL);
            glCompileShader(vs);
            glAttachShader(program, vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fs_shdr = GLSLify(330,
                /* Cubemap as latlong shader by:
                Dario Manesku (https://github.com/dariomanesku) */
                in vec2 uv;
            uniform samplerCube uTex;
            uniform vec4 uDim;
            out vec4 fragColor;

            void main()
            {
                vec2 p = uv*2.0 - vec2(1.0);
                float aspect = uDim.z / uDim.w;
                p.x *= aspect;
                if (length(p) < 1.0)
                {
                    float theta = acos(p.x);
                    float phi = acos(p.y);
                    float sr = sin(phi);
                    vec3 n;
                    n.x = p.x;
                    n.y = p.y;
                    n.z = sqrt(sr*sr - p.x*p.x);
                    n = normalize(n);

                    vec3 camera = vec3(0.0, 0.0, 30.0);
                    camera.y /= aspect;
                    vec3 ray = n - camera;
                    vec3 rRay = reflect(ray, n);
                    fragColor = texture(uTex, rRay);
                }

                else
                    discard;
                /*vec3 dir = vecFromLatLong(uv);
                fragColor = texture(uTex, dir);*/
            }
            );
            glShaderSource(fs, 1, &fs_shdr, NULL);
            glCompileShader(fs);
            glAttachShader(program, fs);

            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            {
                printf("Probe shader info:\n");
                int infologLen = 0;
                int charsWritten = 0;
                GLchar *infoLog = NULL;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
                if (infologLen > 0)
                {
                    infoLog = (GLchar*)malloc(infologLen);
                    if (infoLog == NULL)
                        return;
                    glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
                }
                printf("%s\n", infoLog);
                delete[] infoLog;
            }
        }
        uloc_res = glGetUniformLocation(program, "uRes");
        uloc_tex = glGetUniformLocation(program, "uTex");
        uloc_dim = glGetUniformLocation(program, "uDim");

        // init fs quad
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        GLfloat verts[12] = { -1, -1, 1, 1, -1, 1, -1, -1, 1, -1, 1, 1 };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    void Draw(GLuint texture, unsigned x, unsigned y, unsigned dim)
    {
        glUseProgram(program);

        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        glUniform1i(uloc_tex, 0);
        int dims[4];
        glGetIntegerv(GL_VIEWPORT, dims);
        glUniform2f(uloc_res, dims[2], dims[3]);
        glUniform4f(uloc_dim, (float)x, (float)y, (float)dim, (float)dim);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    ~CubemapProbe()
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;

        glDeleteProgram(program);
        program = 0;
    }
};

void aa::render::DrawCubemapProbe(GLuint cubemap, unsigned x, unsigned y, unsigned dim)
{
    static CubemapProbe cmp;
    cmp.Draw(cubemap, x, y, dim);
}

GLuint aa::render::CreateCubemapEmpty(glm::ivec2 res)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    for (GLuint i = 0; i < 6; i++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, res.x, res.y, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return tex;
}

struct CubemapFiller
{
    GLuint fbo, rb;
    bool init;
    CubemapFiller()
    {
        // fbo
        glGenFramebuffers(1, &fbo);
        // rb
        glGenRenderbuffers(1, &rb);
        init = false;
    }

    ~CubemapFiller()
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rb);
    }

    void Draw(GLuint cubemap, unsigned res, glm::vec3 position, void(*drawWorldFunc)(glm::mat4 v, glm::mat4 p), void(*callback)(unsigned, unsigned) = 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        if (init == false /*|| (rbres.x!=res.x || rbres.y!=res.y)*/)
        {
            glBindRenderbuffer(GL_RENDERBUFFER, rb);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, res, res);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
            init = true;
        }
        // camera
        glm::mat4 p = glm::perspective(3.14156592f/2.0f, 1.0f, 0.01f, 10.0f);
        /*float n = 0.01f, f = 10.0f;
        p[0][0] = 1.0f;
        p[1][1] = 1.0f;
        p[2][2] = -f / (f - n);
        p[2][3] = -1.0f;
        p[3][2] = -(f*n) / (f - n);*/
        //p = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 v = glm::mat4(1.0f);

        glm::vec3 targets[6] = {
            glm::vec3(+1.0f, 0.0f, 0.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, +1.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, +1.0f),
            glm::vec3(0.0f, 0.0f, -1.0f)
        };
        glm::vec3 ups[6] = {
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        };

        // render
        for (unsigned i = 0; i < 6; i++)
        {
            glViewport(0, 0, res, res);
            // setup target face
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, 0);
            // setup camera
            v = glm::lookAt(position, position + targets[i], ups[i]);
            // draw
            (*drawWorldFunc)(v, p);

            if (callback)
                callback(i, res);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};

void aa::render::FillCubemap(GLuint cubemap, unsigned res, glm::vec3 position, void(*drawWorldFunc)(glm::mat4 v, glm::mat4 p), void(*callback)(unsigned,unsigned))
{
    static CubemapFiller cmf;
    cmf.Draw(cubemap, res, position, drawWorldFunc, callback);
}

struct Skybox
{
    GLuint vao, vbo, program;
    GLint uloc_skybox, uloc_v, uloc_p;

    Skybox()
    {
        // init shaders
        program = glCreateProgram();
        {
            GLuint vs = glCreateShader(GL_VERTEX_SHADER);
            static const char* vs_shdr = GLSLify(330,
                in vec3 iPos;

                uniform mat4 uView;
                uniform mat4 uProj;

                out vec3 coord;

                void main()
                {
                    vec4 pos = uProj * uView * vec4(iPos, 1.0);
                    //pos.xyz = pos.xyz * 0.2;
                    gl_Position = pos.xyww;
                    //gl_Position = vec4(iPos, 1.0);
                    coord = -iPos;
                }
            );
            glShaderSource(vs, 1, &vs_shdr, NULL);
            glCompileShader(vs);
            glAttachShader(program, vs);

            GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
            const char* fs_shdr = GLSLify(330,
                in vec3 coord;
                uniform samplerCube uSkybox;
                out vec4 fragColor;

                void main()
                {
                    fragColor = texture(uSkybox, coord);
                }
            );
            glShaderSource(fs, 1, &fs_shdr, NULL);
            glCompileShader(fs);
            glAttachShader(program, fs);

            glLinkProgram(program);
            glDeleteShader(vs);
            glDeleteShader(fs);
            {
                printf("Skybox shader info:\n");
                int infologLen = 0;
                int charsWritten = 0;
                GLchar *infoLog = NULL;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
                if (infologLen > 0)
                {
                    infoLog = (GLchar*)malloc(infologLen);
                    if (infoLog == NULL)
                        return;
                    glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog);
                }
                printf("%s\n", infoLog);
                delete[] infoLog;
            }
        }
        uloc_skybox = glGetUniformLocation(program, "uSkybox");
        uloc_v = glGetUniformLocation(program, "uView");
        uloc_p = glGetUniformLocation(program, "uProj");

        // init fs quad
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        const GLfloat verts[3 * 3 * 2 * 6] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    ~Skybox()
    {
        glBindVertexArray(0);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        vbo = 0;
        vao = 0;

        glDeleteProgram(program);
        program = 0;
    }

    void Draw(GLuint cubemap, glm::mat4 v, glm::mat4 p)
    {
        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        glUniform1i(uloc_skybox, 0);
        glm::mat4 vnt = v;
        vnt[3][0] = 0.0f;
        vnt[3][1] = 0.0f;
        vnt[3][2] = 0.0f;
        glUniformMatrix4fv(uloc_v, 1, GL_FALSE, glm::value_ptr(vnt));
        glUniformMatrix4fv(uloc_p, 1, GL_FALSE, glm::value_ptr(p));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
};

void aa::render::RenderSkybox(GLuint cubemap, glm::mat4 v, glm::mat4 p)
{
    static Skybox sky;
    sky.Draw(cubemap,v,p);
}

