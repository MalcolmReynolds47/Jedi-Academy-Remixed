// Validate the UDP port shared by the MP host menu and network command.
#pragma once

#define JAR_DEFAULT_HOST_PORT 29070

static inline int JAR_HostPort(const char *text)
{
	int port = 0;
	for (int digits = 0; *text; ++text, ++digits) {
		if (digits == 5 || *text < '0' || *text > '9')
			return 0;
		port = port * 10 + (*text - '0');
	}
	return port <= 65535 ? port : 0;
}
