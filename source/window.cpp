#define UNICODE
#include "rgf/window.h"

namespace rgf {
	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

		if (message == WM_DESTROY) {
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	class Window : public IWindow {
	public:
		Window(WindowDesc* pDesc) {

			WNDCLASSEXW wcex{};
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = wndProc;
			wcex.hInstance = pDesc->mHinstance;
			wcex.hIcon = LoadIconW(pDesc->mHinstance, L"IDI_ICON");
			wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
			wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
			wcex.lpszClassName = L"TheAftermath";
			wcex.hIconSm = LoadIconW(wcex.hInstance, L"IDI_ICON");

			RegisterClassExW(&wcex);
			auto stype = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;
			RECT rc = { 0, 0, static_cast<LONG>(pDesc->mWidth), static_cast<LONG>(pDesc->mHeight) };
			AdjustWindowRect(&rc, stype, FALSE);
			mHwnd = CreateWindowExW(0, L"TheAftermath", pDesc->mTitle.c_str(), stype,
				CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, wcex.hInstance,
				NULL);
			ShowWindow(mHwnd, SW_SHOWDEFAULT);
		}

		HWND getHWND() const {
			return mHwnd;
		}

		HWND mHwnd;
	};

	IWindow* create(WindowDesc* pDesc) {
		return new Window(pDesc);
	}

	void destroy(IWindow* pWindow) {
		auto p = (Window*)pWindow;
		delete p;
	}

	void runMessageLoop(void (*pFunction)()) {
		MSG msg{};
		while (msg.message != WM_QUIT) {
			if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			else {
				pFunction();
			}
		}
	}
}

