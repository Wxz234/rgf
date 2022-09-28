#pragma once
#include "define.h"
#include "robject.h"
#include "rdevice.h"

#include <dxgiformat.h>

namespace rgf {

	struct rtextureDesc {
		rdevice* pDevice = nullptr;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
		bool bHasUAV = false;
		bool bHasRTV = false;
	};

	struct rtexture : public robject {
		virtual DXGI_FORMAT getFormat() const = 0;
	};

	RGF_API rtexture* create(rtextureDesc* pDesc);
}