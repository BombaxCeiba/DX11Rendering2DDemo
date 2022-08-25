#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include "HResultException.h"

class CDXShaderException final : public CHResultException
{
private:
    std::string m_error;

public:
    CDXShaderException(HRESULT hr, const char* p_error, Microsoft::WRL::ComPtr<ID3DBlob> p_shader_error);
    ~CDXShaderException() override = default;

    const char* what() const noexcept override;
};

struct ShaderMacro
{
    std::string m_name{};
    std::string m_definition{};
};

class CShader
{
private:
    std::string m_code{};
    std::string m_entry_point{};
    std::string m_name{};
    std::string m_target{};
    std::vector<ShaderMacro> m_macros{};
    mutable std::vector<D3D_SHADER_MACRO> m_shader_macros{0};
    ID3DInclude* m_p_include{};
    UINT m_flags1{};
    UINT m_flags2{};
    mutable bool m_is_macro_changed{true};
    mutable bool m_is_config_changed{true};
    mutable Microsoft::WRL::ComPtr<ID3DBlob> m_p_cached_byte_code{};

public:
    CShader() = default;
    ~CShader() = default;

    auto GetCode() const noexcept
        -> const std::string&;
    auto SetCode(const std::string& code)
        -> CShader&;

    auto GetEntryPoint() const noexcept
        -> const std::string&;
    auto SetEntryPoint(const std::string& entry_point)
        -> CShader&;

    auto GetName() const noexcept
        -> const std::string&;
    auto SetName(const std::string& name)
        -> CShader&;

    auto GetTarget() const noexcept
        -> const std::string&;
    auto SetTarget(const std::string& target)
        -> CShader&;

    auto AddMacro(const ShaderMacro& macro)
        -> CShader&;
    auto DeleteMacro(const std::string& name)
        -> CShader&;
    auto GetMacros() const noexcept
        -> const std::vector<ShaderMacro>&;

    UINT GetFlags1() const noexcept;
    auto SetFlags1(UINT flags1) noexcept
        -> CShader&;

    UINT GetFlags2() const noexcept;
    auto SetFlags2(UINT flags2) noexcept
        -> CShader&;

    auto GetInclude() noexcept
        -> ID3DInclude*;
    auto SetInclude(ID3DInclude* p_indclude) noexcept
        -> CShader&;

    auto Compile() const
        -> Microsoft::WRL::ComPtr<ID3DBlob>;
};