#pragma once
#include "define.h"
#include "robject.h"
#include "rdevice.h"

#include <d3d12.h>
#include <dxgiformat.h>

namespace rgf {

	struct rtextureDesc {
		rdevice* pDevice = nullptr;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
	};

	struct rtexture : public robject {
		virtual DXGI_FORMAT getFormat() const = 0;
		virtual ID3D12Resource* getResource() const = 0;
	};

	RGF_API rtexture* create(rtextureDesc* pDesc);
}