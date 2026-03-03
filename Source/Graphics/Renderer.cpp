/**
 * @file Renderer.cpp
 * @brief 렌더링 서브시스템 구현
 */

#include "Renderer.h"
#include "Device.h"
#include "CommandQueue.h"
#include "CommandListManager.h"
#include "SwapChain.h"
#include "DescriptorHeapManager.h"
#include <Utils/Logger.h>
#include <Core/BuildConfig.h>

namespace DX12GameEngine
{
    Renderer::Renderer()
        : m_vertexBufferView{}
        , m_commandList(nullptr)
        , m_initialized(false)
        , m_width(0)
        , m_height(0)
    {
    }

    Renderer::~Renderer()
    {
        ReleaseRenderTargetViews();
        // unique_ptr이 자동으로 정리
    }

    bool Renderer::Initialize(HWND hwnd, int width, int height, const RendererDesc& desc)
    {
        if (m_initialized)
        {
            LOG_WARNING(LogCategory::Renderer, L"Renderer already initialized");
            return true;
        }

        LOG_INFO(LogCategory::Renderer, L"Initializing renderer...");

        m_width = width;
        m_height = height;

        // Device 생성 및 초기화
        m_device = std::make_unique<Device>();
        if (!m_device->Initialize(desc.enableDebugLayer))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize Device");
            return false;
        }

        // CommandQueue 초기화 (Direct Queue)
        m_commandQueue = std::make_unique<CommandQueue>();
        if (!m_commandQueue->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize CommandQueue");
            return false;
        }

        // CommandQueue 동기화 테스트
        uint64_t fenceValue = m_commandQueue->Signal();
        m_commandQueue->WaitForFenceValue(fenceValue);
        LOG_INFO(LogCategory::Renderer, L"CommandQueue fence synchronization test passed (value: {})", fenceValue);

        // CommandListManager 초기화
        m_commandListManager = std::make_unique<CommandListManager>();
        if (!m_commandListManager->Initialize(m_device->GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize CommandListManager");
            return false;
        }

        // SwapChain 초기화
        SwapChainDesc swapChainDesc;
        swapChainDesc.hwnd = hwnd;
        swapChainDesc.width = static_cast<uint32_t>(width);
        swapChainDesc.height = static_cast<uint32_t>(height);
        swapChainDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.vsync = desc.vsync;
        swapChainDesc.allowTearing = !desc.vsync;  // VSync OFF일 때 Tearing 허용

        m_swapChain = std::make_unique<SwapChain>();
        if (!m_swapChain->Initialize(m_device->GetFactory(), m_commandQueue->GetQueue(), swapChainDesc))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize SwapChain");
            return false;
        }

        // DescriptorHeapManager 초기화
        m_descriptorHeapManager = std::make_unique<DescriptorHeapManager>();
        if (!m_descriptorHeapManager->Initialize(m_device->GetDevice()))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to initialize DescriptorHeapManager");
            return false;
        }

        // RenderTargetView 생성
        if (!CreateRenderTargetViews())
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create RenderTargetViews");
            return false;
        }

        // Root Signature 생성
        if (!CreateRootSignature())
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Root Signature");
            return false;
        }

        // Pipeline State Object 생성
        if (!CreatePipelineState())
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Pipeline State Object");
            return false;
        }

        // 삼각형 Vertex Buffer 생성
        if (!CreateTriangleVertexBuffer())
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create triangle Vertex Buffer");
            return false;
        }

        m_initialized = true;

        LOG_INFO(LogCategory::Renderer, L"Renderer initialized ({}x{})", m_width, m_height);

        return true;
    }

    bool Renderer::CreateRenderTargetViews()
    {
        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            m_rtvHandles[i] = m_descriptorHeapManager->AllocateRtv();
            if (!m_rtvHandles[i].IsValid())
            {
                LOG_ERROR(LogCategory::Renderer, L"Failed to allocate RTV descriptor for back buffer {}", i);
                return false;
            }

            m_device->GetDevice()->CreateRenderTargetView(
                m_swapChain->GetBackBuffer(i), nullptr, m_rtvHandles[i].cpuHandle);
        }

        LOG_INFO(LogCategory::Renderer, L"Created {} RenderTargetViews", kBackBufferCount);
        return true;
    }

    void Renderer::ReleaseRenderTargetViews()
    {
        for (uint32_t i = 0; i < kBackBufferCount; ++i)
        {
            if (m_rtvHandles[i].IsValid())
            {
                m_descriptorHeapManager->FreeRtv(m_rtvHandles[i]);
                m_rtvHandles[i] = DescriptorHandle();
            }
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentRtvHandle() const
    {
        uint32_t index = m_swapChain->GetCurrentBackBufferIndex();
        return m_rtvHandles[index].cpuHandle;
    }

    void Renderer::BeginFrame()
    {
        // CommandListManager 프레임 시작
        m_commandListManager->BeginFrame(
            m_commandQueue->GetFence(),
            m_commandQueue->GetFenceEvent());

        // 커맨드 리스트 획득
        m_commandList = m_commandListManager->GetCommandList();

        // 백 버퍼 상태 전환: PRESENT → RENDER_TARGET
        ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        // 뷰포트 설정
        D3D12_VIEWPORT viewport = {};
        viewport.Width = static_cast<float>(m_width);
        viewport.Height = static_cast<float>(m_height);
        viewport.MaxDepth = 1.0f;
        m_commandList->RSSetViewports(1, &viewport);

        // 시저 렉트 설정
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        m_commandList->RSSetScissorRects(1, &scissorRect);

        // 렌더 타겟 설정
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCurrentRtvHandle();
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    }

    void Renderer::RenderFrame()
    {
        // 렌더 타겟 클리어 (Cornflower Blue)
        const float clearColor[] = { 0.39f, 0.58f, 0.93f, 1.0f };
        m_commandList->ClearRenderTargetView(GetCurrentRtvHandle(), clearColor, 0, nullptr);

        // 삼각형 렌더링
        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->SetPipelineState(m_pipelineState.Get());
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->DrawInstanced(3, 1, 0, 0);
    }

    void Renderer::EndFrame()
    {
        // 백 버퍼 상태 전환: RENDER_TARGET → PRESENT
        ID3D12Resource* backBuffer = m_swapChain->GetCurrentBackBuffer();
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = backBuffer;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        m_commandList->ResourceBarrier(1, &barrier);

        // 커맨드 리스트 닫기
        m_commandList->Close();

        // 커맨드 리스트 실행
        ID3D12CommandList* commandLists[] = { m_commandList };
        m_commandQueue->ExecuteCommandLists(commandLists, 1);

        // 커맨드 리스트 반환
        m_commandListManager->ReturnCommandList(m_commandList);
        m_commandList = nullptr;

        // Present
        m_swapChain->Present();

        // Fence 시그널 및 CommandListManager 프레임 종료
        uint64_t fenceValue = m_commandQueue->Signal();
        m_commandListManager->EndFrame(fenceValue);
    }

    void Renderer::OnResize(int width, int height)
    {
        if (!m_initialized)
        {
            return;
        }

        if (width == m_width && height == m_height)
        {
            return;
        }

        // GPU 작업 완료 대기 (리사이즈 전 필수)
        m_commandQueue->Flush();

        m_width = width;
        m_height = height;

        // RTV 해제 (SwapChain 리사이즈 전 백 버퍼 참조 제거)
        ReleaseRenderTargetViews();

        // SwapChain 리사이즈
        m_swapChain->Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

        // RTV 재생성
        CreateRenderTargetViews();

        LOG_INFO(LogCategory::Renderer, L"Renderer resized ({}x{})", m_width, m_height);
    }

    ComPtr<ID3DBlob> Renderer::CompileShader(const std::wstring& filename,
        const std::string& entryPoint, const std::string& target)
    {
        UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DEBUG)
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompileFromFile(
            filename.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &shaderBlob,
            &errorBlob);

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                // 컴파일 에러 메시지 출력 (narrow string → wide string 변환)
                const char* errorMsg = static_cast<const char*>(errorBlob->GetBufferPointer());
                int len = MultiByteToWideChar(CP_ACP, 0, errorMsg, -1, nullptr, 0);
                std::wstring wErrorMsg(len, L'\0');
                MultiByteToWideChar(CP_ACP, 0, errorMsg, -1, wErrorMsg.data(), len);
                LOG_ERROR(LogCategory::Shader, L"Shader compile error [{}]: {}", filename, wErrorMsg);
            }
            else
            {
                LOG_ERROR(LogCategory::Shader, L"Failed to compile shader [{}], HRESULT: {:#010x}",
                    filename, static_cast<uint32_t>(hr));
            }
            return nullptr;
        }

        return shaderBlob;
    }

    bool Renderer::CreateRootSignature()
    {
        // 삼각형 렌더링에는 외부 리소스(CBV/SRV/UAV)가 필요 없으므로 빈 루트 시그니처 사용
        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = 0;
        rootSigDesc.pParameters = nullptr;
        rootSigDesc.NumStaticSamplers = 0;
        rootSigDesc.pStaticSamplers = nullptr;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSig;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeRootSignature(
            &rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
            &serializedRootSig, &errorBlob);

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                const char* errorMsg = static_cast<const char*>(errorBlob->GetBufferPointer());
                int len = MultiByteToWideChar(CP_ACP, 0, errorMsg, -1, nullptr, 0);
                std::wstring wErrorMsg(len, L'\0');
                MultiByteToWideChar(CP_ACP, 0, errorMsg, -1, wErrorMsg.data(), len);
                LOG_ERROR(LogCategory::Renderer, L"Root Signature serialize error: {}", wErrorMsg);
            }
            return false;
        }

        hr = m_device->GetDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature));

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Root Signature, HRESULT: {:#010x}",
                static_cast<uint32_t>(hr));
            return false;
        }

        LOG_INFO(LogCategory::Renderer, L"Root Signature created");
        return true;
    }

    bool Renderer::CreatePipelineState()
    {
        // 셰이더 컴파일
        ComPtr<ID3DBlob> vertexShader = CompileShader(L"Shaders/Triangle.hlsl", "VSMain", "vs_5_0");
        ComPtr<ID3DBlob> pixelShader  = CompileShader(L"Shaders/Triangle.hlsl", "PSMain", "ps_5_0");
        if (!vertexShader || !pixelShader)
        {
            return false;
        }

        // 입력 레이아웃 (VSInput: float3 POSITION + float4 COLOR)
        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        // 기본 블렌드 상태
        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // 기본 래스터라이저 상태
        D3D12_RASTERIZER_DESC rasterizerDesc = {};
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = FALSE;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.DepthClipEnable = TRUE;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // 뎁스 스텐실 비활성화 (Phase 1: 깊이 버퍼 없음)
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.StencilEnable = FALSE;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
        psoDesc.PS = { pixelShader->GetBufferPointer(),  pixelShader->GetBufferSize() };
        psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
        psoDesc.BlendState = blendDesc;
        psoDesc.RasterizerState = rasterizerDesc;
        psoDesc.DepthStencilState = depthStencilDesc;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_swapChain->GetFormat();
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;

        HRESULT hr = m_device->GetDevice()->CreateGraphicsPipelineState(
            &psoDesc, IID_PPV_ARGS(&m_pipelineState));

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create PSO, HRESULT: {:#010x}",
                static_cast<uint32_t>(hr));
            return false;
        }

        LOG_INFO(LogCategory::Renderer, L"Pipeline State Object created");
        return true;
    }

    bool Renderer::CreateTriangleVertexBuffer()
    {
        // 정점 구조체 (Triangle.hlsl의 VSInput과 일치)
        struct Vertex
        {
            float position[3];
            float color[4];
        };

        // NDC 공간 삼각형 정점 (위쪽 빨강, 왼쪽 아래 초록, 오른쪽 아래 파랑)
        Vertex vertices[] =
        {
            {  0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },
            {  0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f },
            { -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },
        };

        const UINT bufferSize = sizeof(vertices);

        // CPU에서 쓰고 GPU에서 읽는 UPLOAD 힙 사용 (정적 지오메트리지만 Phase 1은 단순화)
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Alignment = 0;
        bufferDesc.Width = bufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = m_device->GetDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer));

        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to create Vertex Buffer, HRESULT: {:#010x}",
                static_cast<uint32_t>(hr));
            return false;
        }

        // CPU에서 정점 데이터 복사
        void* mappedData = nullptr;
        D3D12_RANGE readRange = { 0, 0 };  // 읽기 없음
        hr = m_vertexBuffer->Map(0, &readRange, &mappedData);
        if (FAILED(hr))
        {
            LOG_ERROR(LogCategory::Renderer, L"Failed to map Vertex Buffer");
            return false;
        }
        memcpy(mappedData, vertices, bufferSize);
        m_vertexBuffer->Unmap(0, nullptr);

        // Vertex Buffer View 설정
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = bufferSize;
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);

        LOG_INFO(LogCategory::Renderer, L"Triangle Vertex Buffer created ({} bytes)", bufferSize);
        return true;
    }
}
