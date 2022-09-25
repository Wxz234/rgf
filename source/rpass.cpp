#include "rpass.h"

#include "cmdList.h"

namespace rgf {
	struct GBufferPass : public gbufferPass {
		GBufferPass(gbufferPassDesc* pDesc) {
			pDevice = pDesc->pDevice->getDevice();
		}

		~GBufferPass() {
		
		}

		void release() {
			delete this;
		}

		ID3D12CommandList* getList() const {
			return nullptr;
		}

		void open() {

		}

		void close() {
		
		}

		void updateSkyLightDirection(float x, float y, float z) {

		}

		void updateSkyLightIntensity(float intensity) {
			
		}

		uint64 _getConstantBufferSize(uint64 size) {
			if (size == 0) {
				return 256;
			}
			auto t = size / 256;
			if (size % 256 == 0) {
				return t * 256;
			}
			return (t + 1) * 256;
		}

		ID3D12Device* pDevice;
	};

	gbufferPass* create(gbufferPassDesc* pDesc) {
		return new GBufferPass(pDesc);
	}
}