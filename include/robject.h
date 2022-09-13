#pragma once

namespace RGF {

	struct robject {
		virtual void release() = 0;
	};

	inline void removeObject(robject* pObj) {
		if (pObj) {
			pObj->release();
		}
	}

}