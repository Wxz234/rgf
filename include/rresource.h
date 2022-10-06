#pragma once
#include "define.h"
#include "robject.h"
#include "rdevice.h"

#include <d3d12.h>
#include <dxgiformat.h>

namespace rgf {

	struct rresource : public object {
		virtual ID3D12Resource* getResource() const = 0;
	};

	struct bufferDesc {
		device* pDevice = nullptr;
		uint32 mSize = 0;
		D3D12_HEAP_TYPE mHeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES mStartState = D3D12_RESOURCE_STATE_COMMON;
	};

	struct rbuffer : public rresource {
		virtual uint32 getSize() const = 0;
	};

	RGF_API rbuffer* createBuffer(bufferDesc* pDesc);

	struct textureDesc {
		device* pDevice = nullptr;
		DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		D3D12_HEAP_TYPE mHeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES mStartState = D3D12_RESOURCE_STATE_COMMON;
	};

	struct rtexture : public rresource {
		virtual DXGI_FORMAT getFormat() const = 0;
	};

	RGF_API rtexture* createTexture(textureDesc* pDesc);
}