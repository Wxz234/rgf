#include "rgf/rgf.h"

#include "nvrhi/nvrhi.h"
#include "nvrhi/d3d12.h"
#include "nvrhi/utils.h"
#include "nvrhi/validation.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

struct Context {
    static void create(uint32_t width, uint32_t height, HWND hwnd) {
        rgf::DeviceDesc deviceDesc;
        deviceDesc.mWidth = width;
        deviceDesc.mHeight = height;
        deviceDesc.mHwnd = hwnd;
        pDevice = rgf::create(&deviceDesc);

        nvrhi::d3d12::DeviceDesc nvrhiDeviceDesc;
        nvrhiDeviceDesc.errorCB = &DefaultMessageCallback::GetInstance();
        nvrhiDeviceDesc.pDevice = pDevice->getDevice();
        nvrhiDeviceDesc.pGraphicsCommandQueue = pDevice->getQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        nvrhiDeviceDesc.pCopyCommandQueue = pDevice->getQueue(D3D12_COMMAND_LIST_TYPE_COPY);
        nvrhiDeviceDesc.pComputeCommandQueue = pDevice->getQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
        nvrhiDevice = nvrhi::d3d12::createDevice(nvrhiDeviceDesc);

        nvrhi::TextureDesc textureDesc;
        textureDesc.width = width;
        textureDesc.height = height;
        textureDesc.format = nvrhi::Format::RGBA8_UNORM;
        textureDesc.debugName = "SwapChainBuffer";
        textureDesc.isRenderTarget = true;
        textureDesc.isUAV = false;
        textureDesc.initialState = nvrhi::ResourceStates::Present;
        textureDesc.keepInitialState = true;
        for (uint32_t i = 0;i < pDevice->getFrameCount(); ++i) {
            swapchainTextures.push_back(nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(pDevice->getFrameResource(i)), textureDesc));
            swapchainFramebuffers.push_back(nvrhiDevice->createFramebuffer(nvrhi::FramebufferDesc().addColorAttachment(swapchainTextures[i])));
        }

        commandList = nvrhiDevice->createCommandList();
    }

    static void destroy() {
        nvrhiDevice->waitForIdle();
        nvrhiDevice->runGarbageCollection();
        rgf::destroy(pDevice);
    }

    static void render() {
        
        commandList->open();
        auto frameIndex = pDevice->getFrameIndex();
        nvrhi::utils::ClearColorAttachment(commandList, swapchainFramebuffers[frameIndex], 0, nvrhi::Color(0.5f));
        commandList->close();

        nvrhiDevice->executeCommandList(commandList);

        pDevice->frame();
        nvrhiDevice->runGarbageCollection();
    }
private:
    struct DefaultMessageCallback : public nvrhi::IMessageCallback
    {
        static DefaultMessageCallback& GetInstance() {
            static DefaultMessageCallback Instance;
            return Instance;
        }

        void message(nvrhi::MessageSeverity severity, const char* messageText) override {
            MessageBoxA(NULL, messageText, 0, 0);
        }
    };

    inline static rgf::IDevice* pDevice = nullptr;
    inline static nvrhi::DeviceHandle nvrhiDevice;
    inline static std::vector<nvrhi::TextureHandle> swapchainTextures;
    inline static std::vector<nvrhi::FramebufferHandle> swapchainFramebuffers;
    inline static nvrhi::CommandListHandle commandList;
    inline static nvrhi::GraphicsPipelineHandle pipeline;
};

void render() {
    Context::render();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    constexpr uint32_t width = 800, height = 600;
    rgf::WindowDesc windowDesc;
    windowDesc.mTitle = L"rgf";
    windowDesc.mWidth = width;
    windowDesc.mHeight = height;
    windowDesc.mHinstance = hInstance;
    auto pWindow = rgf::create(&windowDesc);
    Context::create(width, height, pWindow->getHWND());

    rgf::runMessageLoop(render);

    Context::destroy();
    rgf::destroy(pWindow);
    return 0;
}