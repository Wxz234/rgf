#include "rgf/rgf.h"

#include "nvrhi/nvrhi.h"
#include "nvrhi/d3d12.h"
#include "nvrhi/utils.h"
#include "nvrhi/validation.h"

#include <d3d12.h>
#include <dxgi1_6.h>

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
    }

    static void destroy() {
        nvrhiDevice->waitForIdle();
        nvrhiDevice->runGarbageCollection();
        rgf::destroy(pDevice);
    }

    static void render() {
        pDevice->frame();
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