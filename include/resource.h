#pragma once
#include "define.h"
#include "object.h"
#include "device.h"

#include <d3d12.h>
#include <dxgiformat.h>

namespace rgf {

	struct resource : public object {
		virtual ID3D12Resource* getResource() const = 0;
	};

	struct buffer_desc {
		device* pDevice = nullptr;
		uint32 mSize = 0;
		D3D12_HEAP_TYPE mHeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES mStartState = D3D12_RESOURCE_STATE_COMMON;
	};

	struct buffer : public resource {
		virtual uint32 getSize() const = 0;
	};

	RGF_API buffer* createBuffer(buffer_desc* pDesc);

	struct texture_desc {
		device* pDevice = nullptr;
		DXGI_FORMAT mFormat = DXGI_FORMAT_UNKNOWN;
		uint32 mWidth = 800;
		uint32 mHeight = 600;
		D3D12_HEAP_TYPE mHeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES mStartState = D3D12_RESOURCE_STATE_COMMON;
	};

	struct texture : public resource {
		virtual DXGI_FORMAT getFormat() const = 0;
	};

	RGF_API texture* createTexture(texture_desc* pDesc);
}