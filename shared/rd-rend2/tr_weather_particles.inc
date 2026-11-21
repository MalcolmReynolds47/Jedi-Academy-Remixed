// Native GPU particle initialization, also exercised without GL. GPL-2.0-only.
// Initialize both buffers for editable counts: untouched tails must remain valid.
	void GenerateRainModel( weatherObject_t& ws, const int maxParticleCount, bool initializeBoth=false )
	{
		std::vector<rainVertex_t> rainVertices(maxParticleCount * CHUNK_COUNT);

		for ( size_t i = 0; i < rainVertices.size(); ++i )
		{
			rainVertex_t& vertex = rainVertices[i];
			vertex.position[0] = Q_flrand(-HALF_CHUNK_EXTENDS, HALF_CHUNK_EXTENDS);
			vertex.position[1] = Q_flrand(-HALF_CHUNK_EXTENDS, HALF_CHUNK_EXTENDS);
			vertex.position[2] = Q_flrand(tr.world->bmodels[0].bounds[0][2], tr.world->bmodels[0].bounds[1][2]);
			vertex.velocity[0] = 0.0f; //Q_flrand(0.0f, 0.0f);
			vertex.velocity[1] = 0.0f; //Q_flrand(0.0f, 0.0f);
			vertex.velocity[2] = 0.0f; //Q_flrand(-1.0f, 0.0f);
		}

		ws.lastVBO = R_CreateVBO(
			initializeBoth ? (byte *)rainVertices.data() : nullptr,
			sizeof(rainVertex_t) * rainVertices.size(),
			VBO_USAGE_XFB, "Weather_ping");
		ws.vbo = R_CreateVBO(
			(byte *)rainVertices.data(),
			sizeof(rainVertex_t) * rainVertices.size(),
			VBO_USAGE_XFB, "Weather_pong");
		ws.vboLastUpdateFrame = 0;

		ws.attribsTemplate[0].index = ATTR_INDEX_POSITION;
		ws.attribsTemplate[0].numComponents = 3;
		ws.attribsTemplate[0].offset = offsetof(rainVertex_t, position);
		ws.attribsTemplate[0].stride = sizeof(rainVertex_t);
		ws.attribsTemplate[0].type = GL_FLOAT;
		ws.attribsTemplate[0].vbo = nullptr;

		ws.attribsTemplate[1].index = ATTR_INDEX_COLOR;
		ws.attribsTemplate[1].numComponents = 3;
		ws.attribsTemplate[1].offset = offsetof(rainVertex_t, velocity);
		ws.attribsTemplate[1].stride = sizeof(rainVertex_t);
		ws.attribsTemplate[1].type = GL_FLOAT;
		ws.attribsTemplate[1].vbo = nullptr;
	}

