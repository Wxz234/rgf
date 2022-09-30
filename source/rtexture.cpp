#include "rtexture.h"
#include "descriptor.h"

#include "D3D12MemAlloc.h"
#include "d3dx12.h"

namespace rgf {

	struct Texture : public rtexture {
		Texture(rtextureDesc* pDesc) {
			pDevice = pDesc->pDevice->getDevice();
			pDescriptorManager = (descriptorManager*)pDesc->pDevice->getDescriptorManager();
			D3D12MA::Allocator* pAllocator = (D3D12MA::Allocator*)pDesc->pDevice->getResourceAllocator();

			D3D12MA::ALLOCATION_DESC allocDesc{};
			allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
			CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(pDesc->mFormat, pDesc->mWidth, pDesc->mHeight);
			pAllocation = nullptr;
			pAllocator->CreateResource(&allocDesc, &texDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, &pAllocation, IID_NULL, nullptr);

			mSRVIndex = 0;
			pDescriptorManager->allocStatic(mSRVIndex);
			pDescriptorManager->bindSRV(mSRVIndex, pAllocation->GetResource(), nullptr);

			mFormat = pDesc->mFormat;
		}

		~Texture() {
			pAllocation->Release();
			pDescriptorManager->remove(mSRVIndex);
		}

		virtual DXGI_FORMAT getFormat() const {
			return mFormat;
		}

		ID3D12Resource* getResource() const {
			ID3D12Resource* res = pAllocation->GetResource();
			return res;
		}

		void release() {
			delete this;
		}

		ID3D12Device* pDevice;
		descriptorManager* pDescriptorManager;
		D3D12MA::Allocation* pAllocation;
		DXGI_FORMAT mFormat;
		uint32 mSRVIndex;
	};

	rtexture* create(rtextureDesc* pDesc) {
		return new Texture(pDesc);
	}
}