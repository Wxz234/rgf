#pragma once
#include "define.h"
#include "object.h"
#include "device.h"
#include "resource.h"

#include <d3d12.h>
#include <dxgiformat.h>

namespace rgf {

	struct rendertarget_desc {
		device* pDevice = nullptr;
		DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
	};

	struct rendertarget : public texture {
		virtual D3D12_CPU_DESCRIPTOR_HANDLE getSRV() const = 0;
		virtual D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const = 0;
	};

	RGF_API rendertarget* create(rendertarget_desc* pDesc);
}