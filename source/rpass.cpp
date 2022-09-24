#include "rpass.h"

#include "pass.h"

namespace rgf {
	struct GBufferPass : public gbufferPass {
		GBufferPass(gbufferPassDesc* pDesc) {
			pDevice = pDesc->pDevice->getDevice();
			pList = new commandList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
		}

		~GBufferPass() {
			delete pList;
		}

		void release() {
			delete this;
		}

		ID3D12CommandList* getList() const {
			return pList->getList();
		}

		void open() {
			pList->open(nullptr);
		}

		void close() {
			pList->close();
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

		commandList* pList;
	};

	gbufferPass* create(gbufferPassDesc* pDesc) {
		return new GBufferPass(pDesc);
	}
}