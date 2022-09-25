#include "rpass.h"

#include "cmdList.h"
#include "descriptor.h"

namespace rgf {
	struct GBufferPass : public gBufferPass {
		GBufferPass(gBufferPassDesc* pDesc) {
			pDevice = pDesc->pDevice->getDevice();
			pList = createCmdList(pDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			pDescriptorManager = (descriptorManager*)pDesc->pDevice->getDescriptorManager();
		}

		~GBufferPass() {
			removeObject(pList);
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
		cmdList* pList;
		descriptorManager* pDescriptorManager;
	};

	gBufferPass* create(gBufferPassDesc* pDesc) {
		return new GBufferPass(pDesc);
	}
}