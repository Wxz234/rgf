struct VertexInput {
	float4 Position : POSITION;
};

float4 vs_main(VertexInput vIn) : SV_POSITION {
	return vIn.Position;
}