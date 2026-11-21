// Hexadecimal RGB carried through the existing saber-color userinfo fields.
#pragma once

static inline qboolean JAR_ParseSaberRGB(const char *text, byte color[3])
{
	byte digits[6];
	if (!text || strlen(text) != 7 || text[0] != '#')
		return qfalse;
	for (int i = 0; i < 6; ++i) {
		const unsigned char ch = text[i + 1];
		if (ch >= '0' && ch <= '9')
			digits[i] = ch - '0';
		else if (ch >= 'a' && ch <= 'f')
			digits[i] = ch - 'a' + 10;
		else if (ch >= 'A' && ch <= 'F')
			digits[i] = ch - 'A' + 10;
		else
			return qfalse;
	}
	for (int i = 0; i < 3; ++i)
		color[i] = (digits[2*i] << 4) | digits[2*i + 1];
	return qtrue;
}
