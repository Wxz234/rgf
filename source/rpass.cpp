#include "rpass.h"
#include "cmdList.h"
#include "descriptor.h"

namespace rgf {
	struct GBufferPass : public gBufferPass {
		GBufferPass(gBufferPassDesc* pDesc) {

		}

		~GBufferPass() {
			//removeObject(pList);
		}

		void release() {
			delete this;
		}

		ID3D12CommandList* getList() const {
			return nullptr;
		}

		void open() {
			//pList->open(nullptr);
		}

		void close() {
			//pList->close();
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

	};

	gBufferPass* create(gBufferPassDesc* pDesc) {
		return new GBufferPass(pDesc);
	}
}