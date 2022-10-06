#include "rgf.h"

void draw(rgf::device* pDevice) {
    pDevice->drawText("Hello World!123123", 36, 5, 4, 0xFF0000FF);
    pDevice->drawText("Hello World!", 36, 280, 250, 0xFFFFFFFF);
    pDevice->frame();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    constexpr unsigned width = 800, height = 600;
    rgf::rwindowDesc window_desc;
    window_desc.mTitle = L"TheAftermath";
    window_desc.mWidth = width;
    window_desc.mHeight = height;
    window_desc.mHinstance = hInstance;
    auto window = rgf::create(&window_desc);

    rgf::deviceDesc device_desc;
    device_desc.mWidth = width;
    device_desc.mHeight = height;
    device_desc.mHwnd = window->getHWND();
    auto device = rgf::create(&device_desc);

    rgf::bufferDesc buffer_desc{};
    buffer_desc.pDevice = device;
    buffer_desc.mSize = 100;
    auto buffer = rgf::createBuffer(&buffer_desc);

    rgf::rwindow::run(draw, device);

    rgf::removeObject(buffer);
    rgf::removeObject(device);
    rgf::removeObject(window);
    return 0;
}