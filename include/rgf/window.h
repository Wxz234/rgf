#pragma once
#include "define.h"

#include <string>

namespace rgf {
	struct WindowDesc {
		std::wstring mTitle;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		HINSTANCE mHinstance = NULL;
	};

	struct IWindow {
		virtual HWND getHWND() const = 0;
	};

	RGF_API IWindow* create(WindowDesc* pDesc);
	RGF_API void destroy(IWindow* pWindow);
	RGF_API void runMessageLoop(void (*pFunction)());
}