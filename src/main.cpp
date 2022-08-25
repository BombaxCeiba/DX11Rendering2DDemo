#include <vector>
#include <array>
#include <Windows.h>
#include <wrl/client.h>
#include <dxgi1_3.h>
#include <d3d11_2.h>
#include <DXProgrammableCapture.h>
#include <DirectXMath.h>
#include <dxgitype.h>
#include "CShader.h"
#include "HResultException.h"

using Microsoft::WRL::ComPtr;

/**
 * @brief 析构StaticVariableWrapper包装对象前默认执行的函数，实际上无操作
 *
 * @tparam T
 */
template <class T>
class CDefaultStaticVariableWrapperDtor
{
public:
    void operator()(T*) const {};
};
/**
 * @brief 设计上用于静态变量包装类，用于自定义变量默认初始化后行为和析构前行为
 *
 * @tparam T 要被包装的类型
 * @tparam DTOR 自定义执行析构函数前的行为
 */
template <class T, class DTOR = CDefaultStaticVariableWrapperDtor<T>>
class CStaticVariableWrapper : private DTOR
{
private:
    T m_content;

public:
    /**
     * @brief 构造一个StaticVariableWrapper
     *
     * @tparam CTOR 自定义变量默认初始化后的函数类型
     * @param ctor 自定义变量默认初始化后的行为，传入变量的指针作为参数
     * @param dtor 自定义变量执行析构函数前的行为，传入变量的指针作为参数
     */
    template <class CTOR>
    CStaticVariableWrapper(CTOR ctor, DTOR dtor = {})
        : DTOR{dtor}
    {
        ctor(std::addressof(m_content));
    }
    ~CStaticVariableWrapper()
    {
        (*static_cast<DTOR*>(this))(std::addressof(m_content));
    }
    T& Get() noexcept
    {
        return m_content;
    }
    const T& Get() const noexcept
    {
        return m_content;
    }
};
/**
 * @brief 生成静态变量包装类的函数
 *
 * @tparam T 要被包装的类型
 * @tparam CTOR 自定义变量默认初始化后的函数类型
 * @tparam DTOR 自定义变量执行析构函数前的函数类型
 * @param ctor 自定义变量默认初始化后的行为，传入变量的指针作为参数
 * @param dtor 自定义变量执行析构函数前的行为，传入变量的指针作为参数
 * @return CStaticVariableWrapper<T, DTOR> 包装后的变量，已经初始化
 */
template <class T, class CTOR, class DTOR = CDefaultStaticVariableWrapperDtor<T>>
auto MakeStaticVariableWrapper(CTOR ctor, DTOR dtor = {})
    -> CStaticVariableWrapper<T, DTOR>
{
    return {ctor, dtor};
}

/**
 * @brief 调用指针指向的对象的对应类型的析构函数
 *
 * @tparam T 传入的移除了指针后的类型
 * @param p_memory 指向要执行析构函数的对象的指针
 */
template <class T>
void Destroy(T* p_memory)
{
    p_memory->~T();
}

template <class T, class... Args>
void EmplaceAt(T* p_memory, Args&&... args)
{
    ::new (p_memory) T(std::forward<Args>(args)...);
}

#define CIMAGE2DEFFECT_SHADER_VS_INPUT_DECLARATION \
    "struct VsInput"                               \
    "{"                                            \
    "float3 position : POSITION0;"                 \
    "float2 texture0 : TEXCOORD0;"                 \
    "};"

#define CIMAGE2DEFFECT_SHADER_VS_OUTPUT_DECLARATION \
    "struct VsOutput"                               \
    "{"                                             \
    "float4 position : SV_POSITION;"                \
    "float2 texture0 : TEXCOORD0;"                  \
    "};"

namespace D3DQuadrangle
{
    const std::array<std::uint8_t, 6> VERTEX_INDEX_LIST{
        0, 1, 2,
        2, 3, 0};

    const CShader& GetVsShader()
    {
        static auto result = MakeStaticVariableWrapper<CShader>(
            [](CShader* p_content)
            {
                p_content->SetCode(
                             CIMAGE2DEFFECT_SHADER_VS_INPUT_DECLARATION
                                 CIMAGE2DEFFECT_SHADER_VS_OUTPUT_DECLARATION
                             R"(
VsOutput VS(VsInput input){
   VsOutput result;
   result.position = float4(input.position, 1.0f);
   result.texture0 = input.texture0;
   return result;
}
)")
                    .SetEntryPoint("VS")
                    .SetName("D3DQuadrangleDefaultVS")
                    .SetTarget("vs_4_1")
                    .SetFlags1(D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS);
            });
        return result.Get();
    }

    /**
     * @brief 由三角形012和三角形132组成的四边形，顶点序号见VERTEX_INDEX_LIST
     *
     * @tparam Vertex 顶点类型
     */
    template <class Vertex>
    struct QuadrangleVertexs
    {
        /**
         * @brief 由三角形012和三角形123组成的四边形，下面是顶点排列顺序： \n
         * 1·--------------·2 \n
         *  |              |  \n
         *  |              |  \n
         * 0·--------------·3
         */
        std::array<Vertex, 4> vertexs{};

        Vertex& GetLeftTopVertex() noexcept
        {
            return vertexs[1];
        }
        Vertex& GetRightTopVertex() noexcept
        {
            return vertexs[2];
        }
        Vertex& GetLeftBottomVertex() noexcept
        {
            return vertexs[0];
        }
        Vertex& GetRightBottomVertex() noexcept
        {
            return vertexs[3];
        }
        Vertex* GetData() noexcept
        {
            return vertexs.data();
        }
        static constexpr std::size_t GetSize() noexcept
        {
            return sizeof(vertexs);
        }
    };
}

struct Image2DVertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texcoord;
};

using QuadrangleVertexs = D3DQuadrangle::QuadrangleVertexs<Image2DVertex>;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    default:
        return ::DefWindowProc(hwnd, message, wParam, lParam);
    }
}

constexpr UINT SLOT = 0;
constexpr SIZE WINDOW_SIZE = {350, 100};
constexpr auto PIXEL_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
#define TRAFFICMONITOR_ONE_IN_255 "0.0039215687"

int main()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = ::GetModuleHandle(NULL);
    wc.lpszClassName = CMAKE_PROJECT_NAME;
    ::RegisterClass(&wc);
    HWND hwnd = ::CreateWindow(
        CMAKE_PROJECT_NAME,
        CMAKE_PROJECT_NAME,
        WS_VISIBLE | WS_BORDER,
        0, 0, WINDOW_SIZE.cx, WINDOW_SIZE.cy,
        NULL,
        NULL,
        wc.hInstance,
        NULL);
    ::ShowWindow(hwnd, SW_SHOW);

    ComPtr<ID3D11Device> p_device{};
    ComPtr<IDXGISwapChain> p_swap_chain{};
    ComPtr<ID3D11DeviceContext> p_device_context{};
    ComPtr<ID3D11Device2> p_device2{};
    ComPtr<ID3D11Texture2D> p_back_buffer{};
    ComPtr<ID3D11RenderTargetView> p_back_buffer_rtv{};
    {
        auto feature_levels = D3D_FEATURE_LEVEL_11_1;
        DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
        swap_chain_desc.BufferDesc.Width = WINDOW_SIZE.cx;
        swap_chain_desc.BufferDesc.Height = WINDOW_SIZE.cy;
        swap_chain_desc.BufferDesc.RefreshRate.Numerator = 1;
        swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 1;
        swap_chain_desc.OutputWindow = hwnd;
        swap_chain_desc.Windowed = TRUE;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ThrowIfFailed(D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            NULL,
            D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            &feature_levels,
            1,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &p_swap_chain,
            &p_device,
            NULL,
            &p_device_context));

        ThrowIfFailed(p_device->QueryInterface(IID_PPV_ARGS(&p_device2)));

        ThrowIfFailed(p_swap_chain->GetBuffer(0, IID_PPV_ARGS(&p_back_buffer)));
        ThrowIfFailed(p_device2->CreateRenderTargetView(p_back_buffer.Get(), NULL, &p_back_buffer_rtv));
    }

    // ComPtr<IDXGraphicsAnalysis> p_dxgi_analysis{};
    //{
    //     ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&p_dxgi_analysis)));
    // }
    // p_dxgi_analysis->BeginCapture();

    ComPtr<ID3D11VertexShader> p_vs{};
    ComPtr<ID3DBlob> p_vs_byte_code = D3DQuadrangle::GetVsShader().Compile();
    {
        ThrowIfFailed(p_device2->CreateVertexShader(
            p_vs_byte_code->GetBufferPointer(),
            p_vs_byte_code->GetBufferSize(),
            NULL,
            &p_vs));
    }

    ComPtr<ID3D11InputLayout> p_input_layout{};
    {
        constexpr static std::array<D3D11_INPUT_ELEMENT_DESC, 2> input_elements_desc{
            {{"POSITION",
              0,
              DXGI_FORMAT_R32G32B32_FLOAT,
              SLOT,
              offsetof(Image2DVertex, position),
              D3D11_INPUT_PER_VERTEX_DATA,
              0},
             {"TEXCOORD",
              0,
              DXGI_FORMAT_R32G32_FLOAT,
              SLOT,
              offsetof(Image2DVertex, texcoord),
              D3D11_INPUT_PER_VERTEX_DATA,
              0}}};
        ThrowIfFailed(p_device->CreateInputLayout(
            input_elements_desc.data(),
            static_cast<UINT>(input_elements_desc.size()),
            p_vs_byte_code->GetBufferPointer(),
            p_vs_byte_code->GetBufferSize(),
            &p_input_layout));
    }
    ComPtr<ID3D11Buffer> p_vertex_buffer{};
    ComPtr<ID3D11Buffer> p_index_buffer{};
    QuadrangleVertexs vertexes;

    {
        DirectX::XMFLOAT3* vertex_position;
        DirectX::XMFLOAT2* vertex_texture;
        //左上角
        vertex_position = &vertexes.GetLeftTopVertex().position;
        vertex_position->x = -1.f; // x
        vertex_position->y = 1.f;  // y
        vertex_position->z = .0f;  // z
        vertex_texture = &vertexes.GetLeftTopVertex().texcoord;
        vertex_texture->x = 0.f; // u
        vertex_texture->y = 0.f; // v

        //右上角
        vertex_position = &vertexes.GetRightTopVertex().position;
        vertex_position->x = 1.f;
        vertex_position->y = 1.f;
        vertex_position->z = .0f;
        vertex_texture = &vertexes.GetRightTopVertex().texcoord;
        vertex_texture->x = 1.f;
        vertex_texture->y = 0.f;

        //右下角
        vertex_position = &vertexes.GetRightBottomVertex().position;
        vertex_position->x = 1.f;
        vertex_position->y = -1.f;
        vertex_position->z = .0f;
        vertex_texture = &vertexes.GetRightBottomVertex().texcoord;
        vertex_texture->x = 1.f;
        vertex_texture->y = 1.f;

        //左下角
        vertex_position = &vertexes.GetLeftBottomVertex().position;
        vertex_position->x = -1.f;
        vertex_position->y = -1.f;
        vertex_position->z = .0f;
        vertex_texture = &vertexes.GetLeftBottomVertex().texcoord;
        vertex_texture->x = 0;
        vertex_texture->y = 1;

        D3D11_BUFFER_DESC vertex_buffer_desc{};
        vertex_buffer_desc.ByteWidth = static_cast<UINT>(vertexes.GetSize());
        vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
        vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vertex_data{};
        vertex_data.pSysMem = vertexes.GetData();
        ThrowIfFailed(p_device2->CreateBuffer(
            &vertex_buffer_desc,
            &vertex_data,
            &p_vertex_buffer));

        D3D11_BUFFER_DESC index_buffer_desc{};
        index_buffer_desc.Usage = D3D11_USAGE_DEFAULT;
        index_buffer_desc.ByteWidth = sizeof(D3DQuadrangle::VERTEX_INDEX_LIST);
        index_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        D3D11_SUBRESOURCE_DATA index_data{};
        index_data.pSysMem = &D3DQuadrangle::VERTEX_INDEX_LIST;
        ThrowIfFailed(
            p_device2->CreateBuffer(
                &index_buffer_desc,
                &index_data,
                &p_index_buffer));
    }
    ComPtr<ID3D11RasterizerState> p_rasterizer_state{};
    {
        D3D11_RASTERIZER_DESC rasterizer_desc{};
        rasterizer_desc.FillMode = D3D11_FILL_SOLID;
        rasterizer_desc.CullMode = D3D11_CULL_BACK;
        rasterizer_desc.FrontCounterClockwise = FALSE;
        rasterizer_desc.DepthBias = 0;
        rasterizer_desc.DepthBiasClamp = 0.0f;
        rasterizer_desc.SlopeScaledDepthBias = 0.0f;
        rasterizer_desc.DepthClipEnable = FALSE;
        rasterizer_desc.ScissorEnable = FALSE;
        rasterizer_desc.MultisampleEnable = FALSE;
        rasterizer_desc.AntialiasedLineEnable = FALSE;
        ThrowIfFailed(p_device2->CreateRasterizerState(
            &rasterizer_desc,
            &p_rasterizer_state));
    }
    ComPtr<ID3D11SamplerState> p_ps_tex0_sampler{};
    {
        D3D11_SAMPLER_DESC tex0_sampler_desc{};
        tex0_sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        tex0_sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        tex0_sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        tex0_sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        tex0_sampler_desc.MipLODBias = 0.0f;
        tex0_sampler_desc.MaxAnisotropy = 1;
        tex0_sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        tex0_sampler_desc.MinLOD = -FLT_MAX;
        tex0_sampler_desc.MaxLOD = FLT_MAX;

        ThrowIfFailed(p_device2->CreateSamplerState(
            &tex0_sampler_desc,
            &p_ps_tex0_sampler));
    }
    ComPtr<ID3D11BlendState1> p_blend_state{};
    {
        D3D11_BLEND_DESC1 blend_desc1{};
        blend_desc1.AlphaToCoverageEnable = FALSE;
        blend_desc1.IndependentBlendEnable = FALSE;
        auto& render_target_blend_desc0 = blend_desc1.RenderTarget[0];
        render_target_blend_desc0.BlendEnable = TRUE;
        render_target_blend_desc0.SrcBlend = D3D11_BLEND_SRC_ALPHA;
        render_target_blend_desc0.DestBlend = D3D11_BLEND_INV_DEST_ALPHA;
        render_target_blend_desc0.BlendOp = D3D11_BLEND_OP_ADD;
        render_target_blend_desc0.SrcBlendAlpha = D3D11_BLEND_ONE;
        render_target_blend_desc0.DestBlendAlpha = D3D11_BLEND_ZERO;
        render_target_blend_desc0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
        render_target_blend_desc0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        ThrowIfFailed(p_device2->CreateBlendState1(
            &blend_desc1,
            &p_blend_state));
    }
    ComPtr<ID3D11DepthStencilState> p_depth_stencil_state{};
    {
        D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{};
        depth_stencil_desc.DepthEnable = FALSE;
        depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depth_stencil_desc.DepthFunc = D3D11_COMPARISON_LESS;
        depth_stencil_desc.StencilEnable = FALSE;
        depth_stencil_desc.StencilReadMask = 0xFF;
        depth_stencil_desc.StencilWriteMask = 0xFF;
        depth_stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depth_stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depth_stencil_desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        depth_stencil_desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depth_stencil_desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        ThrowIfFailed(p_device2->CreateDepthStencilState(
            &depth_stencil_desc,
            &p_depth_stencil_state));
    }
    ComPtr<ID3D11Texture2D> p_gdi_initial_texture{};
    ComPtr<ID3D11Texture2D> p_gdi_final_texture{};
    {
        D3D11_TEXTURE2D_DESC description = {};
        description.ArraySize = 1;
        description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        description.Format = PIXEL_FORMAT;
        description.Width = WINDOW_SIZE.cx;
        description.Height = WINDOW_SIZE.cy;
        description.MipLevels = 1;
        description.SampleDesc.Count = 1;
        description.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
        ThrowIfFailed(p_device2->CreateTexture2D(
            &description,
            NULL,
            &p_gdi_initial_texture));

        description.MiscFlags = 0;
        ThrowIfFailed(p_device2->CreateTexture2D(
            &description,
            NULL,
            &p_gdi_final_texture));
    }
    ComPtr<ID3D11ShaderResourceView> p_ps_shader_resource_view{};
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC tex0_shader_resource_view_desc = {};
        tex0_shader_resource_view_desc.Format = PIXEL_FORMAT;
        tex0_shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        tex0_shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
        tex0_shader_resource_view_desc.Texture2D.MipLevels = 1;

        ThrowIfFailed(p_device2->CreateShaderResourceView(
            p_gdi_initial_texture.Get(),
            &tex0_shader_resource_view_desc,
            &p_ps_shader_resource_view));
    }
    ComPtr<ID3D11RenderTargetView> p_render_target_view{};
    {
        ThrowIfFailed(p_device2->CreateRenderTargetView(
            p_gdi_final_texture.Get(),
            NULL,
            &p_render_target_view));
    }
    const static auto ps_alpha_increase_code = MakeStaticVariableWrapper<CShader>(
        [](CShader* p_content)
        {
            p_content->SetCode(
                         CIMAGE2DEFFECT_SHADER_VS_OUTPUT_DECLARATION
                         R"(
SamplerState input_sampler : register(ps_4_1, s0);
Texture2D input_texture : register(ps_4_1, t0);

float4 PS(VsOutput ps_in) : SV_TARGET
{
    float4 color = input_texture.Sample(input_sampler, ps_in.texture0);
    color.w += )" TRAFFICMONITOR_ONE_IN_255 ";"
                         R"(
    color.w = min(1.0, color.w);
    return color;
}
)")
                .SetEntryPoint("PS")
                .SetName("PsGdiTexturePreprocessor")
                .SetTarget("ps_4_1")
                .SetFlags1(D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS);
        });
    auto p_ps_alpha_increase = ps_alpha_increase_code.Get().Compile();
    ComPtr<ID3D11PixelShader> p_ps{};
    p_device2->CreatePixelShader(
        p_ps_alpha_increase->GetBufferPointer(),
        p_ps_alpha_increase->GetBufferSize(),
        NULL,
        &p_ps);

    {
        p_device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        p_device_context->IASetInputLayout(p_input_layout.Get());
        auto raw_p_vertex_buffer = p_vertex_buffer.Get();
        p_device_context->IASetIndexBuffer(
            p_index_buffer.Get(),
            DXGI_FORMAT_R8_UINT,
            0);
        constexpr static std::array<UINT, 1> strides{static_cast<UINT>(QuadrangleVertexs::GetSize())};
        constexpr static std::array<UINT, 1> offsets{0};
        p_device_context->IASetVertexBuffers(
            SLOT,
            1,
            &raw_p_vertex_buffer,
            strides.data(),
            offsets.data());
        p_device_context->VSSetShader(
            p_vs.Get(),
            NULL,
            0);
        p_device_context->GSSetShader(NULL, NULL, 0);
        p_device_context->SOSetTargets(0, NULL, NULL);

        D3D11_VIEWPORT viewport{};
        viewport.Width = WINDOW_SIZE.cx;
        viewport.Height = WINDOW_SIZE.cy;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        p_device_context->RSSetViewports(1, &viewport);
        p_device_context->RSSetState(p_rasterizer_state.Get());

        auto raw_p_tex0_sampler_state = p_ps_tex0_sampler.Get();
        p_device_context->PSSetSamplers(
            SLOT,
            1,
            &raw_p_tex0_sampler_state);
        auto raw_p_ps_shader_resource_view = p_ps_shader_resource_view.Get();
        p_device_context->PSSetShaderResources(
            SLOT,
            1,
            &raw_p_ps_shader_resource_view);
        p_device_context->PSSetShader(
            p_ps.Get(),
            NULL,
            0);

        p_device_context->OMSetBlendState(
            p_blend_state.Get(),
            NULL,
            0);
        p_device_context->OMSetDepthStencilState(
            p_depth_stencil_state.Get(),
            0);
        std::array<ID3D11RenderTargetView*, 2> raw_p_render_target_views = {p_render_target_view.Get(), p_back_buffer_rtv.Get()};
        p_device_context->OMSetRenderTargets(
            raw_p_render_target_views.size(),
            raw_p_render_target_views.data(),
            NULL);
    }

    p_device_context->DrawIndexed(
        static_cast<UINT>(D3DQuadrangle::VERTEX_INDEX_LIST.size()),
        0,
        0);
    p_swap_chain->Present(0, 0);

    // p_dxgi_analysis->EndCapture();

    MSG msg{};
    while (::GetMessage(&msg, NULL, 0, 0) > 0)
    {
        ::TranslateMessage(&msg); //转换
        ::DispatchMessage(&msg);  //分发
        p_device_context->DrawIndexed(
            static_cast<UINT>(D3DQuadrangle::VERTEX_INDEX_LIST.size()),
            0,
            0);
    }
}