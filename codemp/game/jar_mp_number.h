// Decimal input shared by the multiplayer menu and authoritative commands.
#pragma once
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static inline int JAR_NumberDigits(const char **text) {
    const char *start = *text;
    while (**text >= '0' && **text <= '9') ++*text;
    return *text != start;
}

// Permit unfinished prefixes while typing, but require a complete token on Apply.
static inline int JAR_NumberSyntax(const char *text, int decimal, int prefix) {
    if (*text == '+' || *text == '-') ++text;
    int digits = JAR_NumberDigits(&text);
    if (decimal && *text == '.') {
        ++text;
        digits |= JAR_NumberDigits(&text);
    }
    if (decimal && digits && (*text == 'e' || *text == 'E')) {
        ++text;
        if (*text == '+' || *text == '-') ++text;
        digits = JAR_NumberDigits(&text);
    }
    return !*text && (digits || prefix);
}

static inline void JAR_NumberAppend(char *text, size_t size, int character, int decimal) {
    const size_t length = strlen(text);
    if (character <= 0 || character > 127 || length + 1 >= size) return;
    text[length] = (char)character;
    text[length + 1] = '\0';
    if (!JAR_NumberSyntax(text, decimal, 1)) text[length] = '\0';
}

static inline int JAR_ParseInt(const char *text, int *out) {
    if (!JAR_NumberSyntax(text, 0, 0)) return 0;
    errno = 0;
    const long value = strtol(text, NULL, 10);
    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

static inline int JAR_ParseFloat(const char *text, float *out) {
    if (!JAR_NumberSyntax(text, 1, 0)) return 0;
    errno = 0;
    const float value = strtof(text, NULL);
    if (errno == ERANGE || !isfinite(value)) return 0;
    *out = value;
    return 1;
}

static inline int JAR_ParseScaled(const char *text, int scale, int *out) {
    if (!scale) return JAR_ParseInt(text, out);
    if (!JAR_NumberSyntax(text, 1, 0)) return 0;
    errno = 0;
    const double value = round(strtod(text, NULL) * scale);
    if (errno == ERANGE || !isfinite(value) || value < INT_MIN || value > INT_MAX) return 0;
    *out = (int)value;
    return 1;
}

// Engine time and deadlines use signed milliseconds.
static inline int JAR_NumberMillis(double millis) {
    if (millis >= INT_MAX) return INT_MAX;
    if (millis <= INT_MIN) return INT_MIN;
    if (!isfinite(millis)) return 0;
    return (int)round(millis);
}
static inline int JAR_NumberDeadline(int now, double delay) {
    return JAR_NumberMillis((double)now + delay);
}
