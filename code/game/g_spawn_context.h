// Restore the native spawn parser across nested/runtime spawns. GPL-2.0-only.
#ifndef JAR_SPAWN_CONTEXT_H
#define JAR_SPAWN_CONTEXT_H
extern qboolean spawning, NPCsPrecached;
extern int numSpawnVars, numSpawnVarChars;
extern char *spawnVars[MAX_SPAWN_VARS][2];
extern char spawnVarChars[MAX_SPAWN_VARS_CHARS];
void G_ResetSubBSPSpawnContext();
class G_SpawnContext
{
    qboolean active, precached;
    int count, chars;
    char *vars[MAX_SPAWN_VARS][2];
    char bytes[MAX_SPAWN_VARS_CHARS];
public:
    G_SpawnContext(): active(spawning), precached(NPCsPrecached), count(numSpawnVars), chars(numSpawnVarChars)
    {
        memcpy(vars, spawnVars, sizeof(vars)); memcpy(bytes, spawnVarChars, sizeof(bytes));
        spawning=qtrue; numSpawnVars=0; numSpawnVarChars=0;
    }
    ~G_SpawnContext()
    {
        memcpy(spawnVars, vars, sizeof(vars)); memcpy(spawnVarChars, bytes, sizeof(bytes));
        spawning=active; NPCsPrecached=precached; numSpawnVars=count; numSpawnVarChars=chars;
    }
    G_SpawnContext(const G_SpawnContext&)=delete;
    G_SpawnContext& operator=(const G_SpawnContext&)=delete;
};
#endif
