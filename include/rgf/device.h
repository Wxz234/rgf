#pragma once
#include "define.h"

#include <d3d12.h>

namespace rgf {
	struct DeviceDesc {
		HWND mHwnd = NULL;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
	};

	struct IDevice {
		virtual ID3D12Device* getDevice() const = 0;
		virtual ID3D12CommandQueue* getQueue(D3D12_COMMAND_LIST_TYPE type) const = 0;
		virtual uint32 getFrameCount() const = 0;
		virtual uint32 getFrameIndex() const = 0;
		virtual ID3D12Resource* getFrameResource(uint32 index) const = 0;
		virtual void frame() = 0;
	};

	RGF_API IDevice* create(DeviceDesc* pDesc);
	RGF_API void destroy(IDevice* pDevice);
}