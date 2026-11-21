// Native collision identities in absolute model-handle order. GPL-2.0-only.
#include "../../shared/qcommon/jar_bsp_model.h"

int CM_BSPModelAt(int index, char* name, int size, int* first, int* count, unsigned* checksum)
{
	if (!name || size < MAX_QPATH || !first || !count || !checksum || index < -1)
		return -1;
	if (index >= NumSubBSP)
		return 0;
	const int mainModels = cmg.numSubModels;
	if (mainModels < 1 || mainModels > CAPSULE_MODEL_HANDLE)
		return -1;
	if (index == -1)
	{
		name[0] = '\0';
		*first = 0;
		*count = mainModels;
		*checksum = cmg.checksum;
		return 1;
	}

	int offset = mainModels;
	for (int slot = 0; slot <= index; ++slot)
	{
		const int models = SubBSP[slot].numSubModels;
		if (models < 1 || models > CAPSULE_MODEL_HANDLE - offset)
			return -1;
		if (slot < index)
			offset += models;
	}

	const auto& map = SubBSP[index];
	const size_t length = strlen(map.name);
	if (length < 10 || Q_strncmp(map.name, "maps/", 5) || Q_stricmp(map.name + length - 4, ".bsp"))
		return -1;
	char key[MAX_QPATH], path[MAX_QPATH];
	const size_t modelNameLength = length - 9;
	key[0] = '#';
	memcpy(key + 1, map.name + 5, modelNameLength);
	key[modelNameLength + 1] = '\0';
	int child;
	if (!JAR_BSPModelName(key, path, &child) || child || Q_stricmp(path, map.name))
		return -1;

	Q_strncpyz(name, key, size);
	*first = offset;
	*count = map.numSubModels;
	*checksum = map.checksum;
	return 1;
}
