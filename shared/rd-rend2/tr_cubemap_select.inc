// Native nearest-map fallback and live probe override. GPL-2.0-or-later.
int R_CubemapForPoint( const vec3_t point )
{
	int cubemapIndex = -1;
	int nativeCount = tr.numCubemaps;
	const int live = R_JAR_ProbeForPoint(point);
	if (live) return live;
	if (tr.jarProbes.initialized) nativeCount = tr.jarProbes.nativeCount;

	if (r_cubeMapping->integer && tr.numCubemaps)
	{
		int i;
		float shortest = (float)WORLD_SIZE * (float)WORLD_SIZE;

		for (i = 0; i < nativeCount; i++)
		{
			vec3_t diff;
			float length;

			VectorSubtract(point, tr.cubemaps[i].origin, diff);
			length = DotProduct(diff, diff);

			if (shortest > length)
			{
				shortest = length;
				cubemapIndex = i;
			}
		}
	}

	return cubemapIndex + 1;
}
