#pragma once

namespace rgf {

	struct object {
		virtual void release() = 0;
	};

	inline void removeObject(object* pObj) {
		pObj->release();
	}

}