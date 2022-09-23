#pragma once
#include "define.h"
#include "robject.h"
#include "rdevice.h"

#include <d3d12.h>

namespace rgf {
	struct rpass : public robject {
		virtual ID3D12CommandList* getList() const = 0;
		virtual void open() = 0;
		virtual void close() = 0;
	};

	struct gbufferPassDesc {
		rdevice* pDevice = nullptr;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
	};

	struct gbufferPass : public rpass {
		virtual void updateSkyLightDirection(float x, float y, float z) = 0;
		virtual void updateSkyLightIntensity(float intensity) = 0;
	};

	RGF_API gbufferPass* create(gbufferPassDesc* pDesc);
}