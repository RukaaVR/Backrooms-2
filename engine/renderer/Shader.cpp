#include "engine/renderer/Shader.h"

#include "engine/core/Log.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <sstream>

namespace br::gpu {

ShaderProgram::~ShaderProgram() {
    if (m_id) {
        glDeleteProgram(m_id);
    }
}

GLint ShaderProgram::location(std::string_view name) const {
    const std::string key(name);
    auto it = m_locations.find(key);
    if (it != m_locations.end()) {
        return it->second;
    }
    const GLint loc = glGetUniformLocation(m_id, key.c_str());
    m_locations.emplace(key, loc);
    return loc;
}

void ShaderProgram::set(std::string_view n, int v) const { glProgramUniform1i(m_id, location(n), v); }
void ShaderProgram::set(std::string_view n, unsigned v) const { glProgramUniform1ui(m_id, location(n), v); }
void ShaderProgram::set(std::string_view n, float v) const { glProgramUniform1f(m_id, location(n), v); }
void ShaderProgram::set(std::string_view n, glm::vec2 v) const { glProgramUniform2fv(m_id, location(n), 1, glm::value_ptr(v)); }
void ShaderProgram::set(std::string_view n, glm::vec3 v) const { glProgramUniform3fv(m_id, location(n), 1, glm::value_ptr(v)); }
void ShaderProgram::set(std::string_view n, glm::vec4 v) const { glProgramUniform4fv(m_id, location(n), 1, glm::value_ptr(v)); }
void ShaderProgram::set(std::string_view n, glm::ivec2 v) const { glProgramUniform2iv(m_id, location(n), 1, glm::value_ptr(v)); }
void ShaderProgram::set(std::string_view n, const glm::mat3& m) const {
    glProgramUniformMatrix3fv(m_id, location(n), 1, GL_FALSE, glm::value_ptr(m));
}
void ShaderProgram::set(std::string_view n, const glm::mat4& m) const {
    glProgramUniformMatrix4fv(m_id, location(n), 1, GL_FALSE, glm::value_ptr(m));
}
void ShaderProgram::setArray(std::string_view n, const glm::mat4* m, int count) const {
    glProgramUniformMatrix4fv(m_id, location(n), count, GL_FALSE, glm::value_ptr(*m));
}

ShaderLibrary::ShaderLibrary(fs::path shaderDir) : m_dir(std::move(shaderDir)) {}

bool ShaderLibrary::expandIncludes(const fs::path& file, std::string& out, int depth, std::vector<fs::path>& stack) const {
    if (depth > 16) {
        BR_LOG_ERROR("shader", "#include depth exceeded at '{}'", file.string());
        return false;
    }
    const fs::path full = m_dir / file;
    if (std::find(stack.begin(), stack.end(), full) != stack.end()) {
        return true; // include guard semantics: each file once per program
    }
    const auto text = file::readText(full);
    if (!text) {
        BR_LOG_ERROR("shader", "Cannot read shader '{}'", full.string());
        return false;
    }
    stack.push_back(full);
    std::istringstream in(*text);
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        const auto first = line.find_first_not_of(" \t");
        if (first != std::string::npos && line.compare(first, 8, "#include") == 0) {
            const auto q0 = line.find('"', first);
            const auto q1 = q0 == std::string::npos ? q0 : line.find('"', q0 + 1);
            if (q1 == std::string::npos) {
                BR_LOG_ERROR("shader", "{}:{}: malformed #include", file.string(), lineNo);
                return false;
            }
            const fs::path inc = file.parent_path() / line.substr(q0 + 1, q1 - q0 - 1);
            if (!expandIncludes(inc, out, depth + 1, stack)) {
                return false;
            }
            continue;
        }
        if (first != std::string::npos && line.compare(first, 8, "#version") == 0) {
            continue; // the library emits #version itself
        }
        out += line;
        out += '\n';
    }
    return true;
}

std::optional<std::string> ShaderLibrary::preprocess(const std::string& file, const std::vector<ShaderDefine>& defines) const {
    std::string src = "#version 450 core\n";
    for (const auto& d : defines) {
        src += "#define " + d.name + " " + d.value + "\n";
    }
    std::vector<fs::path> stack;
    if (!expandIncludes(file, src, 0, stack)) {
        return std::nullopt;
    }
    return src;
}

GLuint ShaderLibrary::build(const std::vector<ShaderStageSource>& stages, const std::vector<ShaderDefine>& defines) const {
    std::vector<GLuint> shaders;
    bool ok = true;
    for (const auto& st : stages) {
        std::vector<ShaderDefine> defs = defines;
        switch (st.stage) {
        case GL_VERTEX_SHADER: defs.push_back({"BR_VERTEX_SHADER"}); break;
        case GL_FRAGMENT_SHADER: defs.push_back({"BR_FRAGMENT_SHADER"}); break;
        case GL_COMPUTE_SHADER: defs.push_back({"BR_COMPUTE_SHADER"}); break;
        case GL_GEOMETRY_SHADER: defs.push_back({"BR_GEOMETRY_SHADER"}); break;
        default: break;
        }
        const auto src = preprocess(st.file, defs);
        if (!src) {
            ok = false;
            break;
        }
        const GLuint sh = glCreateShader(st.stage);
        const char* ptr = src->c_str();
        glShaderSource(sh, 1, &ptr, nullptr);
        glCompileShader(sh);
        GLint status = 0;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &status);
        if (!status) {
            GLint len = 0;
            glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
            std::string log(static_cast<usize>(std::max(1, len)), '\0');
            glGetShaderInfoLog(sh, len, nullptr, log.data());
            BR_LOG_ERROR("shader", "Compile error in '{}':\n{}", st.file, log);
            glDeleteShader(sh);
            ok = false;
            break;
        }
        shaders.push_back(sh);
    }
    GLuint program = 0;
    if (ok) {
        program = glCreateProgram();
        for (GLuint sh : shaders) {
            glAttachShader(program, sh);
        }
        glLinkProgram(program);
        GLint status = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (!status) {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::string log(static_cast<usize>(std::max(1, len)), '\0');
            glGetProgramInfoLog(program, len, nullptr, log.data());
            BR_LOG_ERROR("shader", "Link error ({}):\n{}", stages.empty() ? "?" : stages[0].file, log);
            glDeleteProgram(program);
            program = 0;
        }
    }
    for (GLuint sh : shaders) {
        glDeleteShader(sh);
    }
    return program;
}

std::string ShaderLibrary::cacheKey(const std::vector<ShaderStageSource>& stages, const std::vector<ShaderDefine>& defines) {
    std::string key;
    for (const auto& s : stages) {
        key += std::to_string(s.stage) + ":" + s.file + ";";
    }
    for (const auto& d : defines) {
        key += d.name + "=" + d.value + ";";
    }
    return key;
}

std::shared_ptr<ShaderProgram> ShaderLibrary::get(const std::vector<ShaderStageSource>& stages,
                                                  const std::vector<ShaderDefine>& defines) {
    const std::string key = cacheKey(stages, defines);
    if (auto it = m_cache.find(key); it != m_cache.end()) {
        return it->second.program;
    }
    const GLuint id = build(stages, defines);
    if (!id) {
        return nullptr;
    }
    auto program = std::make_shared<ShaderProgram>();
    program->m_id = id;
    m_cache.emplace(key, Entry{stages, defines, program});
    return program;
}

std::shared_ptr<ShaderProgram> ShaderLibrary::graphics(const std::string& vert, const std::string& frag,
                                                       const std::vector<ShaderDefine>& defines) {
    return get({{GL_VERTEX_SHADER, vert}, {GL_FRAGMENT_SHADER, frag}}, defines);
}

std::shared_ptr<ShaderProgram> ShaderLibrary::compute(const std::string& comp, const std::vector<ShaderDefine>& defines) {
    return get({{GL_COMPUTE_SHADER, comp}}, defines);
}

int ShaderLibrary::reloadAll() {
    int reloaded = 0;
    for (auto& [key, entry] : m_cache) {
        const GLuint id = build(entry.stages, entry.defines);
        if (id) {
            glDeleteProgram(entry.program->m_id);
            entry.program->m_id = id;
            entry.program->m_locations.clear();
            ++reloaded;
        }
    }
    BR_LOG_INFO("shader", "Reloaded {}/{} shader programs", reloaded, m_cache.size());
    return reloaded;
}

} // namespace br::gpu
