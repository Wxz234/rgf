#pragma once
#include "define.h"
#include "robject.h"

#include <d3d12.h>
#include <dxgi1_6.h>

namespace rgf {

	struct rpass;

	struct rdeviceDesc {
		HWND mHwnd = NULL;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
	};

	struct rdevice : public robject {
		virtual ID3D12Device* getDevice() const = 0;
		virtual IDXGISwapChain* getSwapChain() const = 0;
		virtual void executePass(rpass* pPass) = 0;
		virtual void frame() = 0;
	};

	RGF_API rdevice* create(rdeviceDesc* pDesc);
}