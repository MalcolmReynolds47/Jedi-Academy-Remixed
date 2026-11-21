// Native shooter firing, including Sandbox links restored by Duplicate. GPL-2.0-only.
#include "g_sandbox_shooter.h"
void misc_weapon_shooter_fire(gentity_t* self)
{
    // Paste restores the target name before the first shot, while the native
    // aim callback may not have run. Resolve each shot so moved or deleted
    // targets are also handled during repeat fire.
    if (G_JAR_WorkshopOwns(self)) {
        if (self->svFlags & SVF_INACTIVE) {
            G_SandboxShooterSchedule(*self,0,0);
            return;
        }
        self->enemy = G_Find(nullptr, FOFS(targetname), self->target);
        if (!self->enemy) VectorClear(self->pos1);
    }

    auto& render = self->client->renderInfo;
    VectorCopy(self->currentOrigin, render.muzzlePoint);
    if (self->enemy && self->enemy->inuse) {
        VectorSubtract(self->enemy->currentOrigin, self->currentOrigin, render.muzzleDir);
        if (!VectorNormalize(render.muzzleDir))
            AngleVectors(self->currentAngles, render.muzzleDir, nullptr, nullptr);
    } else {
        AngleVectors(self->currentAngles, render.muzzleDir, nullptr, nullptr);
    }
    vectoangles(render.muzzleDir, self->client->ps.viewangles);
    FireWeapon(self, (self->spawnflags & jar::shooter::altFireFlag) ? qtrue : qfalse);
    if (self->spawnflags & jar::shooter::repeatFlag) {
        self->e_ThinkFunc = thinkF_misc_weapon_shooter_fire;
        self->nextthink = level.time + self->wait;
        if (self->random) self->nextthink += int(Q_flrand(0.0f, 1.0f) * self->random);
    }
}
