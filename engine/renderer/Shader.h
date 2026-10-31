#pragma once

#include "engine/core/FileSystem.h"
#include "engine/core/Types.h"
#include "engine/renderer/GL.h"

#include <glm/glm.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace br::gpu {

struct ShaderDefine {
    std::string name;
    std::string value = "1";
};

// Linked GLSL program with a uniform-location cache. Programs are created
// through ShaderLibrary, which resolves #include directives and injects
// #defines so permutations share one source file.
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram();
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    bool valid() const { return m_id != 0; }
    GLuint id() const { return m_id; }
    void use() const { glUseProgram(m_id); }

    GLint location(std::string_view name) const;
    void set(std::string_view name, int v) const;
    void set(std::string_view name, unsigned v) const;
    void set(std::string_view name, float v) const;
    void set(std::string_view name, glm::vec2 v) const;
    void set(std::string_view name, glm::vec3 v) const;
    void set(std::string_view name, glm::vec4 v) const;
    void set(std::string_view name, glm::ivec2 v) const;
    void set(std::string_view name, const glm::mat3& m) const;
    void set(std::string_view name, const glm::mat4& m) const;
    void setArray(std::string_view name, const glm::mat4* m, int count) const;

private:
    friend class ShaderLibrary;
    GLuint m_id = 0;
    mutable std::unordered_map<std::string, GLint> m_locations;
};

struct ShaderStageSource {
    GLenum stage;     // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, GL_COMPUTE_SHADER, GL_GEOMETRY_SHADER
    std::string file; // relative to the shader directory
};

class ShaderLibrary {
public:
    explicit ShaderLibrary(fs::path shaderDir);

    // Compiles (or returns the cached) program. Returns nullptr and logs the
    // full compiler output on failure.
    std::shared_ptr<ShaderProgram> get(const std::vector<ShaderStageSource>& stages,
                                       const std::vector<ShaderDefine>& defines = {});
    std::shared_ptr<ShaderProgram> graphics(const std::string& vert, const std::string& frag,
                                            const std::vector<ShaderDefine>& defines = {});
    std::shared_ptr<ShaderProgram> compute(const std::string& comp, const std::vector<ShaderDefine>& defines = {});

    // Preprocessed source (after #include expansion), for diagnostics/tests.
    std::optional<std::string> preprocess(const std::string& file, const std::vector<ShaderDefine>& defines) const;

    // Compiles every program again from disk (developer hot reload). Programs
    // that fail keep their previous binary.
    int reloadAll();
    void clear() { m_cache.clear(); }
    const fs::path& directory() const { return m_dir; }

private:
    struct Entry {
        std::vector<ShaderStageSource> stages;
        std::vector<ShaderDefine> defines;
        std::shared_ptr<ShaderProgram> program;
    };
    bool expandIncludes(const fs::path& file, std::string& out, int depth, std::vector<fs::path>& stack) const;
    GLuint build(const std::vector<ShaderStageSource>& stages, const std::vector<ShaderDefine>& defines) const;
    static std::string cacheKey(const std::vector<ShaderStageSource>& stages, const std::vector<ShaderDefine>& defines);

    fs::path m_dir;
    std::unordered_map<std::string, Entry> m_cache;
};

} // namespace br::gpu
