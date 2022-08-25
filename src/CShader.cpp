#include "CShader.h"
#include <algorithm>

using Microsoft::WRL::ComPtr;

CDXShaderException::CDXShaderException(HRESULT hr, const char* p_error, Microsoft::WRL::ComPtr<ID3DBlob> p_shader_error)
    : CHResultException{hr, p_error}, m_error{p_error}
{
    if (p_shader_error != nullptr)
    {
        m_error += static_cast<char*>(p_shader_error->GetBufferPointer());
    }
}

const char* CDXShaderException::what() const noexcept
{
    return m_error.c_str();
}

auto CShader::Compile() const
    -> Microsoft::WRL::ComPtr<ID3DBlob>
{
    if (m_is_macro_changed && !m_macros.empty())
    {
        m_shader_macros.resize(m_macros.size() + 1);
        std::transform(m_macros.begin(), m_macros.end(), m_shader_macros.begin(),
                       [](const ShaderMacro& current) -> D3D_SHADER_MACRO
                       {
                           return {current.m_name.c_str(), current.m_definition.c_str()};
                       });
        m_shader_macros.back() = {NULL, NULL};
    }
    m_is_macro_changed = false;

    if (m_is_config_changed)
    {
        ComPtr<ID3DBlob> p_error_message{};
        ThrowIfFailed<CDXShaderException>(
            D3DCompile(
                m_code.c_str(),
                m_code.size(),
                m_name.c_str(),
                m_macros.empty() ? NULL : m_shader_macros.data(),
                m_p_include == nullptr ? D3D_COMPILE_STANDARD_FILE_INCLUDE : m_p_include,
                m_entry_point.c_str(),
                m_target.c_str(),
                m_flags1,
                m_flags2,
                &m_p_cached_byte_code,
                &p_error_message),
            "Compile DX shader failed.",
            p_error_message);
        m_is_config_changed = false;
    }

    return m_p_cached_byte_code;
}

auto CShader::GetCode() const noexcept
    -> const std::string&
{
    return m_code;
}

auto CShader::SetCode(const std::string& code)
    -> CShader&
{
    m_is_config_changed = true;
    m_code = code;
    return *this;
}

auto CShader::GetEntryPoint() const noexcept
    -> const std::string&
{
    return m_entry_point;
}

auto CShader::SetEntryPoint(const std::string& entry_point)
    -> CShader&
{
    m_is_config_changed = true;
    m_entry_point = entry_point;
    return *this;
}

auto CShader::GetName() const noexcept
    -> const std::string&
{
    return m_name;
}

auto CShader::SetName(const std::string& name)
    -> CShader&
{
    m_is_config_changed = true;
    m_name = name;
    return *this;
}

auto CShader::GetTarget() const noexcept
    -> const std::string&
{
    return m_target;
}

auto CShader::SetTarget(const std::string& target)
    -> CShader&
{
    m_is_config_changed = true;
    m_target = target;
    return *this;
}

auto CShader::AddMacro(const ShaderMacro& macro)
    -> CShader&
{
    m_is_config_changed = true;
    m_macros.push_back(macro);
    return *this;
}

auto CShader::DeleteMacro(const std::string& name)
    -> CShader&
{
    m_is_config_changed = true;
    std::ignore = std::remove_if(m_macros.begin(), m_macros.end(), [&name](const ShaderMacro& x)
                                 { return x.m_name == name; });
    return *this;
}

auto CShader::GetMacros() const noexcept
    -> const std::vector<ShaderMacro>&
{
    return m_macros;
}

UINT CShader::GetFlags1() const noexcept
{
    return m_flags1;
}

auto CShader::SetFlags1(UINT flags1) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_flags1 = flags1;
    return *this;
}

UINT CShader::GetFlags2() const noexcept
{
    return m_flags2;
}

auto CShader::SetFlags2(UINT flags2) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_flags2 = flags2;
    return *this;
}

auto CShader::GetInclude() noexcept
    -> ID3DInclude*
{
    m_is_config_changed = true;
    return m_p_include;
}

auto CShader::SetInclude(ID3DInclude* p_indclude) noexcept
    -> CShader&
{
    m_is_config_changed = true;
    m_p_include = p_indclude;
    return *this;
}
