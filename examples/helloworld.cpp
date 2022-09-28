#include "rgf.h"

LRESULT _TEST(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    if (_TEST(hWnd, message, wParam, lParam))
        return true;
    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void draw(rgf::rdevice *pDevice) {
    pDevice->frame();
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd) {
    constexpr unsigned width = 800, height = 600;
    rgf::rwindowDesc rwindow_desc;
    rwindow_desc.mTitle = L"TheAftermath";
    rwindow_desc.mWidth = width;
    rwindow_desc.mHeight = height;
    rwindow_desc.mHinstance = hInstance;
    rwindow_desc.pFunction = wndProc;
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