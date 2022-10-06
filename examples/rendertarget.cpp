#include "rgf.h"

void draw(rgf::device* pDevice) {
    pDevice->drawText("Hello World!123123", 36, 5, 4, 0xFF0000FF);
    pDevice->drawText("Hello World!", 36, 280, 250, 0xFFFFFFFF);
    pDevice->frame();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    constexpr unsigned width = 800, height = 600;
    rgf::window_desc windowDesc;
    windowDesc.mTitle = L"TheAftermath";
    windowDesc.mWidth = width;
    windowDesc.mHeight = height;
    windowDesc.mHinstance = hInstance;
    auto window = rgf::create(&windowDesc);

    rgf::device_desc deviceDesc;
    deviceDesc.mWidth = width;
    deviceDesc.mHeight = height;
    deviceDesc.mHwnd = window->getHWND();
    auto device = rgf::create(&deviceDesc);

    rgf::buffer_desc buffer_desc{};
    buffer_desc.pDevice = device;
    buffer_desc.mSize = 100;
    auto buffer = rgf::createBuffer(&buffer_desc);

    rgf::window::run(draw, device);

    rgf::removeObject(buffer);
    rgf::removeObject(device);
    rgf::removeObject(window);
    return 0;
}