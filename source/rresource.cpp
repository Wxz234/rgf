#include "rresource.h"

#include "D3D12MemAlloc.h"
#include "d3dx12.h"
namespace rgf {

	struct Buffer : public buffer {
		Buffer(buffer_desc* pDesc) {
			pDevice = pDesc->pDevice;
			mSize = pDesc->mSize;

			D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(pDesc->mSize);
			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = pDesc->mHeapType;
			auto pAllocator = (D3D12MA::Allocator*)pDevice->getResourceAllocator();

			pAllocator->CreateResource(&allocDesc, &resourceDesc, pDesc->mStartState, NULL, &pAllocation, IID_PPV_ARGS(&pResource));
		}

		~Buffer(){
			pAllocation->Release();
			pResource->Release();
		}

		void release() {
			delete this;
		}

		ID3D12Resource* getResource() const {
			return pResource;
		}

		uint32 getSize() const {
			return mSize;
		}

		uint32 mSize;

		device* pDevice;

		D3D12MA::Allocation* pAllocation;
		ID3D12Resource* pResource;
	};

	buffer* createBuffer(buffer_desc* pDesc) {
		return new Buffer(pDesc);
	}

	struct Texture : public rtexture {
		Texture(textureDesc* pDesc) {
			pDevice = pDesc->pDevice;
			mFormat = pDesc->mFormat;

			D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mFormat, pDesc->mWidth, pDesc->mHeight);
			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = pDesc->mHeapType;
			auto pAllocator = (D3D12MA::Allocator*)pDevice->getResourceAllocator();
			pAllocator->CreateResource(&allocDesc, &resourceDesc, pDesc->mStartState, NULL, &pAllocation, IID_PPV_ARGS(&pResource));
		}

		~Texture() {
			pAllocation->Release();
			pResource->Release();
		}

		void release() {
			delete this;
		}

		ID3D12Resource* getResource() const {
			return pResource;
		}

		DXGI_FORMAT getFormat() const {
			return mFormat;
		}

		DXGI_FORMAT mFormat;

		device* pDevice;

		D3D12MA::Allocation* pAllocation;
		ID3D12Resource* pResource;
	};

	rtexture* createTexture(textureDesc* pDesc) {
		return new Texture(pDesc);
	}
}