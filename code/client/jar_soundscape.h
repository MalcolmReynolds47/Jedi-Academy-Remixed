// Map-local ambient soundscapes. GPL-2.0-only.
#pragma once

void CL_JAR_LoadSoundscape(const char* map);
void CL_JAR_ResetSoundscape();
void CL_JAR_RewindSoundscape();
void CL_JAR_UpdateSoundscape(int time, const float* ear, int player);
