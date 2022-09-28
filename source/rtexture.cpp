#include "rtexture.h"

#include "D3D12MemAlloc.h"

#include <d3d12.h>

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


	};

	rtexture* create(rtextureDesc* pDesc) {
		return new Texture(pDesc);
	}
}