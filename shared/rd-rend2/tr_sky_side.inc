// Build one Rend2 skybox face after clipping the map sky triangles. GPL-2.0-only.

static void BuildSkySide(const int mins[2], const int maxs[2])
{
	int s, t;
	// The projected map triangles are no longer needed. Each face gets
	// its own upload and indexes only the vertices generated here.
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.firstIndex = 0;

	for ( t = mins[1]+HALF_SKY_SUBDIVISIONS;
			t <= maxs[1]+HALF_SKY_SUBDIVISIONS; t++ )
	{
		for ( s = mins[0]+HALF_SKY_SUBDIVISIONS;
				s <= maxs[0]+HALF_SKY_SUBDIVISIONS; s++ )
		{
			tess.xyz[tess.numVertexes][0] = s_skyPoints[t][s][0];
			tess.xyz[tess.numVertexes][1] = s_skyPoints[t][s][1];
			tess.xyz[tess.numVertexes][2] = s_skyPoints[t][s][2];
			tess.xyz[tess.numVertexes][3] = 1.0;

			tess.texCoords[tess.numVertexes][0][0] = s_skyTexCoords[t][s][0];
			tess.texCoords[tess.numVertexes][0][1] = s_skyTexCoords[t][s][1];

			VectorSet4(tess.vertexColors[tess.numVertexes], 1.0f, 1.0f, 1.0f, 1.0f);
			tess.normal[tess.numVertexes] = 0;

			tess.numVertexes++;
		}
	}

	const int ssize = maxs[0] - mins[0];
	const int tsize = maxs[1] - mins[1];
	const int stride = ssize + 1;
	for ( t = 0; t < tsize; t++ )
	{
		for ( s = 0; s < ssize; s++ )
		{
			const int vertex = t * stride + s;
			tess.indexes[tess.numIndexes++] = vertex;
			tess.indexes[tess.numIndexes++] = vertex + stride;
			tess.indexes[tess.numIndexes++] = vertex + 1;
			tess.indexes[tess.numIndexes++] = vertex + 1;
			tess.indexes[tess.numIndexes++] = vertex + stride;
			tess.indexes[tess.numIndexes++] = vertex + stride + 1;
		}
	}

	tess.useInternalVBO = qtrue;
}
