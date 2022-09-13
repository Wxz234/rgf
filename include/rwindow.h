#pragma once
#include "define.h"
#include "robject.h"
#include <Windows.h>
#include <cstdint>
#include <string>
#include <functional>
#include <utility>
namespace rgf {

	struct rwindowDesc {
		std::wstring mTitle;
		uint32_t mWidth = 800;
		uint32_t mHeight = 600;
		HINSTANCE mHinstance = NULL;
		WNDPROC pFunction = NULL;
	};

	struct rwindow : public robject {
		virtual HWND getHWND() const = 0;

		template <typename Function, typename... Args>
		static void run(Function&& f, Args&&... args) {
			MSG msg{};
			while (msg.message != WM_QUIT) {
				if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
				else {
					std::invoke(std::forward<Function>(f), std::forward<Args>(args)...);
				}
			}
		}
	};

	RGF_API rwindow* create(rwindowDesc* pDesc);
}