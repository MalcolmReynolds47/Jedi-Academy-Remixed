// NPC bodies use the native tokenizer. It strips quotes, so check the source
// character as well as the token before treating a brace as structural.
#pragma once

static const char *JarNpcReadToken(const char *&cursor, qboolean allowLineBreaks)
{
    const char *token = COM_ParseExt(&cursor, allowLineBreaks);
    // COM_ParseExt advances past the terminator of an unterminated quote.
    if (cursor && cursor[-1] == '\0') {
        cursor = nullptr;
        return "";
    }
    return token;
}

static bool JarNpcIsBrace(const char *token, const char *cursor, char brace)
{
    return cursor && token[0] == brace && !token[1] && cursor[-1] == brace;
}

// The caller owns the parse session. Return the start of the first matching
// body, just after its opening brace.
static const char *JarNpcFindDefinition(const char *cursor, const char *name)
{
    while (cursor) {
        const char *token = JarNpcReadToken(cursor, qtrue);
        if (!*token)
            break;
        const bool match = !Q_stricmp(token, name);
        token = JarNpcReadToken(cursor, qtrue);
        if (!JarNpcIsBrace(token, cursor, '{'))
            break;
        if (match)
            return cursor;

        int depth = 1;
        while (cursor && depth) {
            token = JarNpcReadToken(cursor, qtrue);
            if (JarNpcIsBrace(token, cursor, '{'))
                ++depth;
            else if (JarNpcIsBrace(token, cursor, '}'))
                --depth;
        }
    }
    return nullptr;
}

static const char *JarNpcReadValue(const char *&cursor)
{
    const char *value = JarNpcReadToken(cursor, qfalse);
    if (JarNpcIsBrace(value, cursor, '}')) {
        --cursor;
        return nullptr;
    }
    // Empty quoted strings are values. An empty token at a new line isn't.
    return *value || (cursor && cursor[-1] == '"') ? value : nullptr;
}

static bool JarNpcParseValue(const char *&cursor, const char *&value)
{
    value = JarNpcReadValue(cursor);
    return value != nullptr;
}

static bool JarNpcParseValue(const char *&cursor, int &value)
{
    const char *token = JarNpcReadValue(cursor);
    if (!token || !*token)
        return false;
    value = atoi(token);
    return true;
}

static bool JarNpcParseValue(const char *&cursor, float &value)
{
    const char *token = JarNpcReadValue(cursor);
    if (!token || !*token)
        return false;
    value = atof(token);
    return true;
}

static void JarNpcSkipLine(const char *&cursor)
{
    while (JarNpcReadValue(cursor)) {
        // A closing brace stays available to the outer parser.
    }
}

// Keep the existing token-level fingerprint for stock assets while only
// letting real, unquoted braces delimit the definition.
static bool JarNpcBodyFingerprint(const char *cursor, unsigned &hash)
{
    hash = 2166136261u;
    while (cursor) {
        const char *token = JarNpcReadToken(cursor, qtrue);
        if (!cursor || JarNpcIsBrace(token, cursor, '{'))
            break;
        if (JarNpcIsBrace(token, cursor, '}'))
            return true;
        for (const unsigned char *t = reinterpret_cast<const unsigned char *>(token); *t; ++t)
            hash = (hash ^ *t) * 16777619u;
        hash *= 16777619u;
    }
    return false;
}
