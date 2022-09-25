#pragma once
#include "define.h"

namespace rgf {
	class vertex {
		float position[3];
		float normal[3];
		float uv0[2];
		uint32 vertexID;
	};
}