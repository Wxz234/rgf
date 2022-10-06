#include "window.h"

#include "third/imgui/imgui.h"
#include "third/imgui/imgui_impl_win32.h"
#include "third/imgui/imgui_impl_dx12.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace rgf {

	LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
			return true;
		}

		if (message == WM_DESTROY) {
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	class RWindow : public window {
	public:
		RWindow(window_desc* pDesc) {

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

		void release() {
			delete this;
		}

		HWND getHWND() const {
			return mHwnd;
		}

		HWND mHwnd;
	};

	window* create(window_desc* pDesc) {
		return new RWindow(pDesc);
	}
}