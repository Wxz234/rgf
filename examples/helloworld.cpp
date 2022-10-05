#include "rgf.h"

void draw(rgf::rdevice *pDevice) {
    pDevice->drawText("Hello World!", 36, 280, 250, 0xFFFFFFFF);
    pDevice->frame();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    constexpr unsigned width = 800, height = 600;
    rgf::rwindowDesc rwindow_desc;
    rwindow_desc.mTitle = L"TheAftermath";
    rwindow_desc.mWidth = width;
    rwindow_desc.mHeight = height;
    rwindow_desc.mHinstance = hInstance;
    auto rwindow = rgf::create(&rwindow_desc);

    rgf::rdeviceDesc rdevice_desc;
    rdevice_desc.mWidth = width;
    rdevice_desc.mHeight = height;
    rdevice_desc.mHwnd = rwindow->getHWND();
    auto rdevice = rgf::create(&rdevice_desc);

    rgf::rwindow::run(draw, rdevice);

    rgf::removeObject(rdevice);
    rgf::removeObject(rwindow);
    return 0;
}