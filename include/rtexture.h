#pragma once
#include "define.h"
#include "robject.h"
#include "rdevice.h"

#include <dxgiformat.h>

namespace rgf {

	struct rtextureDesc {
		rdevice* pDevice = nullptr;

	};

	struct rtexture : public robject {

	};

	RGF_API rtexture* create(rtextureDesc* pDesc);
}