#pragma once
#include "define.h"
#include "robject.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <string_view>

namespace rgf {

	struct device_desc {
		HWND mHwnd = NULL;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
	};

	struct device : public object {
		virtual ID3D12Device* getDevice() const = 0;
		virtual IDXGISwapChain* getSwapChain() const = 0;
		virtual void* getResourceAllocator() const = 0;
		virtual void* getDescriptorManager() const = 0;
		virtual uint32 getFrameIndex() const = 0;
		virtual void drawText(const std::string_view text, float fontSize, float x, float y, uint32 color) = 0;
		virtual void frame() = 0;
	};

	RGF_API device* create(device_desc* pDesc);
}