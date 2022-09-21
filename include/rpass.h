#pragma once
#include "define.h"
#include "robject.h"

#include <d3d12.h>

namespace rgf {
	struct rpass : public robject {
		virtual ID3D12CommandList* getList() const = 0;
		virtual void open() = 0;
		virtual void close() = 0;
	};
}