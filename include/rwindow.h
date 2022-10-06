#pragma once
#include "define.h"
#include "robject.h"

#include <string>
#include <functional>
#include <utility>

namespace rgf {

	struct window_desc {
		std::wstring mTitle;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		HINSTANCE mHinstance = NULL;
	};

	struct window : public object {
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

	RGF_API window* create(window_desc* pDesc);
}