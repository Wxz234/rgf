#include "rtexture.h"
#include "descriptor.h"

#include "D3D12MemAlloc.h"
#include "d3dx12.h"

namespace rgf {

	struct Texture : public rtexture {
		Texture(rtextureDesc* pDesc) {

		}

		~Texture() {

		}

		virtual DXGI_FORMAT getFormat() const {
			return DXGI_FORMAT_UNKNOWN;
		}

		void release() {
			delete this;
		}

		ID3D12Device* pDevice;
		descriptorManager* pDescriptorManager;
		D3D12MA::Allocation* pAllocation;
	};

	rtexture* create(rtextureDesc* pDesc) {
		return new Texture(pDesc);
	}
}