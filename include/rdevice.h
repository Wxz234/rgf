#pragma once
#include "define.h"
#include "robject.h"
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cstdint>

namespace rgf {

	struct rdeviceDesc {
		HWND mHwnd = NULL;
		uint32_t mWidth = 800;
		uint32_t mHeight = 600;
	};

	struct rdevice : public robject {
		virtual ID3D12Device* getDevice() const = 0;
		virtual IDXGISwapChain* getSwapChain() const = 0;
		virtual void frame() = 0;
	};

	RGF_API rdevice* create(rdeviceDesc* pDesc);
}