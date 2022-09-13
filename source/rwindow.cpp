#include "rwindow.h"
namespace rgf {
	class RWindow : public rwindow {
	public:
		RWindow(rwindowDesc* pDesc) {

			WNDCLASSEXW wcex{};
			wcex.cbSize = sizeof(WNDCLASSEXW);
			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = pDesc->pFunction;
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

	rwindow* create(rwindowDesc* pDesc) {
		return new RWindow(pDesc);
	}
}