#include "Entities.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../Utilities/Logging/Logging.hpp"
#include "../../Core/Memory/Memory.hpp"
#include "../Schemas/offsets.hpp"
#include "../Schemas/client_dll.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <intrin.h>

double FrameTime() {
    static const LARGE_INTEGER freq = [] {
        LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f;
        }();
    LARGE_INTEGER t; QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) / static_cast<double>(freq.QuadPart);
}

namespace {

    struct CachedPlayer {
        int       slotIndex = -1;
        uintptr_t controller = 0;
        uintptr_t pawn = 0;
        uint32_t  pawnHandle = 0;
        uintptr_t bodyComp = 0;
        uintptr_t cachedC4 = 0;
        double    lastC4Check = 0.0;
        uint16_t  lastDefIndex = 0;
        char      cachedWeaponName[32] = {};
        char      name[128] = {};
        bool      nameValid = false;

        uintptr_t weaponEnt = 0;
        uint32_t  weaponHandle = 0;
        double    lastWeaponPoll = 0.0;
        bool      weaponValid = false;
        bool      weaponIsReloading = false;
        int32_t   weaponAmmo = 0;
        int32_t   weaponReserve = 0;

        int32_t   lastMoney = 0;
        int32_t   lastPing = 0;
        uint8_t   lastArmor = 0;
        bool      lastHelmet = false;
        double    lastSlowPoll = 0.0;
    };

    constexpr int       kMaxPlayers = 64;
    constexpr uintptr_t kSlotStride = 0x70;
    constexpr double    kRescanIntervalSec = 0.5;
    constexpr double    kC4CacheSec = 1.0;
    constexpr double    kWeaponPollSec = 0.033;
    constexpr double    kSlowPollSec = 0.05;

    constexpr uint8_t kTeamT = 2;
    constexpr uint8_t kTeamCT = 3;

    CachedPlayer g_cache[kMaxPlayers];
    int          g_cacheCount = 0;

    uintptr_t g_cachedEntityList = 0;
    uintptr_t g_cachedControllerListEntry = 0;
    double    g_lastRescan = 0.0;

    const char* WeaponNameById(int id) {
        switch (id) {
        case 42:  return "Knife (T)";
        case 59:  return "Knife (CT)";
        case 500: return "Bayonet";
        case 503: return "Classic Knife";
        case 505: return "Flip Knife";
        case 506: return "Gut Knife";
        case 507: return "Karambit";
        case 508: return "M9 Bayonet";
        case 509: return "Huntsman Knife";
        case 512: return "Falchion Knife";
        case 514: return "Bowie Knife";
        case 515: return "Butterfly Knife";
        case 516: return "Shadow Daggers";
        case 517: return "Paracord Knife";
        case 518: return "Survival Knife";
        case 519: return "Ursus Knife";
        case 520: return "Navaja Knife";
        case 521: return "Nomad Knife";
        case 522: return "Stiletto Knife";
        case 523: return "Talon Knife";
        case 525: return "Skeleton Knife";
        case 526: return "Kukri Knife";
        case 1:  return "Deagle";
        case 2:  return "Dual Berettas";
        case 3:  return "Five-SeveN";
        case 4:  return "Glock-18";
        case 32: return "P2000";
        case 61: return "USP-S";
        case 36: return "P250";
        case 63: return "CZ75-Auto";
        case 30: return "Tec-9";
        case 64: return "R8 Revolver";
        case 7:  return "AK-47";
        case 16: return "M4A4";
        case 60: return "M4A1-S";
        case 8:  return "AUG";
        case 10: return "FAMAS";
        case 13: return "Galil AR";
        case 39: return "SG 553";
        case 9:  return "AWP";
        case 40: return "SSG 08";
        case 11: return "G3SG1";
        case 38: return "SCAR-20";
        case 17: return "MAC-10";
        case 23: return "MP5-SD";
        case 33: return "MP7";
        case 34: return "MP9";
        case 26: return "PP-Bizon";
        case 19: return "P90";
        case 24: return "UMP-45";
        case 35: return "Nova";
        case 29: return "Sawed-Off";
        case 25: return "XM1014";
        case 27: return "MAG-7";
        case 14: return "M249";
        case 28: return "Negev";
        case 43: return "Flashbang";
        case 44: return "HE Grenade";
        case 45: return "Smoke";
        case 46: return "Molotov";
        case 47: return "Decoy";
        case 48: return "Incendiary";
        case 31: return "Zeus x27";
        case 49: return "C4";
        case 55: return "Defuser";
        case 57: return "Healthshot";
        default: return nullptr;
        }
    }

    uintptr_t ResolveEntityFromHandle(uintptr_t entityList, uint32_t handle) {
        if (!entityList || !handle || handle == 0xFFFFFFFF) return 0;
        const uintptr_t listEntry = mem.Read<uintptr_t>(
            entityList + 0x8ull * ((handle & 0x7FFF) >> 9) + 0x10);
        if (!listEntry) return 0;
        return mem.Read<uintptr_t>(
            listEntry + kSlotStride * static_cast<uintptr_t>(handle & 0x1FF));
    }

    bool FindC4OnPlayer(uintptr_t pawn, uintptr_t entityList, uintptr_t& c4Out) {
        c4Out = 0;
        if (!pawn) return false;
        using namespace cs2_dumper::schemas::client_dll;

        const uintptr_t weaponServices = mem.Read<uintptr_t>(
            pawn + C_BasePlayerPawn::m_pWeaponServices);
        if (!weaponServices) return false;

        const uint32_t activeHandle = mem.Read<uint32_t>(
            weaponServices + CPlayer_WeaponServices::m_hActiveWeapon);
        if (activeHandle && activeHandle != 0xFFFFFFFF) {
            const uintptr_t activeWeapon = ResolveEntityFromHandle(entityList, activeHandle);
            if (activeWeapon) {
                const uintptr_t attrMgr = activeWeapon + C_EconEntity::m_AttributeManager;
                const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
                const int defIndex = mem.Read<uint16_t>(
                    econItem + C_EconItemView::m_iItemDefinitionIndex);
                if (defIndex == 49) { c4Out = activeWeapon; return true; }
            }
        }

        const uintptr_t vecBase = weaponServices + CPlayer_WeaponServices::m_hMyWeapons;
        const uint32_t  size = mem.Read<uint32_t>(vecBase);
        if (size == 0 || size > 64) return false;

        const uintptr_t dataPtr = mem.Read<uintptr_t>(vecBase + 0x8);
        if (!dataPtr) return false;

        for (uint32_t i = 0; i < size; ++i) {
            const uint32_t handle = mem.Read<uint32_t>(dataPtr + i * 4);
            if (!handle || handle == 0xFFFFFFFF) continue;
            const uintptr_t weapon = ResolveEntityFromHandle(entityList, handle);
            if (!weapon) continue;

            const uintptr_t attrMgr = weapon + C_EconEntity::m_AttributeManager;
            const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
            const int defIndex = mem.Read<uint16_t>(
                econItem + C_EconItemView::m_iItemDefinitionIndex);

            if (defIndex == 49) { c4Out = weapon; return true; }
        }
        return false;
    }

    void RescanControllers(uintptr_t entityList) {
        g_cachedControllerListEntry = mem.Read<uintptr_t>(entityList + 0x10);
        if (!g_cachedControllerListEntry) {
            g_cacheCount = 0; g_lastRescan = FrameTime(); return;
        }

        constexpr size_t chunkWindowSize = kMaxPlayers * kSlotStride;
        alignas(16) uint8_t listChunk[chunkWindowSize];

        if (!mem.ReadRaw(g_cachedControllerListEntry, listChunk, chunkWindowSize)) {
            g_cacheCount = 0; g_lastRescan = FrameTime(); return;
        }

        CachedPlayer newCache[kMaxPlayers];
        int newCount = 0;

        for (int i = 1; i < kMaxPlayers && newCount < kMaxPlayers; ++i) {
            uintptr_t c = 0;
            std::memcpy(&c, listChunk + kSlotStride * static_cast<uintptr_t>(i), sizeof(uintptr_t));
            if (!c) continue;

            CachedPlayer& dst = newCache[newCount++];
            dst.slotIndex = i;
            dst.controller = c;

            for (int j = 0; j < g_cacheCount; ++j) {
                if (g_cache[j].controller == c) {
                    dst.pawn = g_cache[j].pawn;
                    dst.pawnHandle = g_cache[j].pawnHandle;
                    dst.bodyComp = g_cache[j].bodyComp;
                    dst.cachedC4 = g_cache[j].cachedC4;
                    dst.lastC4Check = g_cache[j].lastC4Check;
                    dst.lastDefIndex = g_cache[j].lastDefIndex;
                    std::memcpy(dst.cachedWeaponName, g_cache[j].cachedWeaponName, sizeof(dst.cachedWeaponName));
                    if (g_cache[j].nameValid) {
                        std::memcpy(dst.name, g_cache[j].name, sizeof(dst.name));
                        dst.nameValid = true;
                    }
                    dst.weaponEnt = g_cache[j].weaponEnt;
                    dst.weaponHandle = g_cache[j].weaponHandle;
                    dst.lastWeaponPoll = g_cache[j].lastWeaponPoll;
                    dst.weaponValid = g_cache[j].weaponValid;
                    dst.weaponIsReloading = g_cache[j].weaponIsReloading;
                    dst.weaponAmmo = g_cache[j].weaponAmmo;
                    dst.weaponReserve = g_cache[j].weaponReserve;
                    dst.lastMoney = g_cache[j].lastMoney;
                    dst.lastPing = g_cache[j].lastPing;
                    dst.lastArmor = g_cache[j].lastArmor;
                    dst.lastHelmet = g_cache[j].lastHelmet;
                    dst.lastSlowPoll = g_cache[j].lastSlowPoll;
                    break;
                }
            }
        }

        std::memcpy(g_cache, newCache, sizeof(CachedPlayer) * newCount);
        g_cacheCount = newCount;
        g_lastRescan = FrameTime();
    }
}

void BuildFrameSnapshot(FrameSnapshot& frame) {
    frame.entities.clear();
    frame.localTeam = 0;
    frame.localOrigin = Vector3{};
    frame.localEyePos = Vector3{};
    frame.localVelocity = Vector3{};
    frame.viewMatrix = ViewMatrix{};
    frame.localIndex = 0xFFFFFFFFu;
    frame.localPawn = 0;
    frame.localWeaponName[0] = '\0';
    frame.localDefIndex = 0;

    if (!mem.client || !mem.pid || !mem.handle) return;

    using namespace cs2_dumper::schemas::client_dll;

    const uintptr_t localPawn = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);

    if (!localPawn || localPawn < 0x10000) return;

    frame.localPawn = localPawn;

    frame.localTeam = mem.Read<uint8_t>(localPawn + C_BaseEntity::m_iTeamNum);

    {
        const uintptr_t localScene = mem.Read<uintptr_t>(
            localPawn + C_BaseEntity::m_pGameSceneNode);
        if (localScene) {
            mem.ReadRaw(localScene + CGameSceneNode::m_vecAbsOrigin,
                &frame.localOrigin, sizeof(Vector3));
        }
        mem.ReadRaw(localPawn + C_BaseEntity::m_vecAbsVelocity,
            &frame.localVelocity, sizeof(Vector3));

        float duckAmount = 0.0f;
        const uintptr_t moveSvc = mem.Read<uintptr_t>(
            localPawn + C_BasePlayerPawn::m_pMovementServices);
        if (moveSvc) {
            duckAmount = mem.Read<float>(
                moveSvc + CCSPlayer_MovementServices::m_flDuckAmount);
            if (!std::isfinite(duckAmount)) duckAmount = 0.0f;
            duckAmount = (std::clamp)(duckAmount, 0.0f, 1.0f);
        }
        const float eyeHeight = 64.0f - duckAmount * 10.0f;
        frame.localEyePos = frame.localOrigin + Vector3(0.0f, 0.0f, eyeHeight);
    }

    const uintptr_t entityList = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    {
        const uintptr_t weaponSvc = mem.Read<uintptr_t>(
            localPawn + C_BasePlayerPawn::m_pWeaponServices);
        if (weaponSvc) {
            const uint32_t wh = mem.Read<uint32_t>(
                weaponSvc + CPlayer_WeaponServices::m_hActiveWeapon);
            if (wh && wh != 0xFFFFFFFF) {
                const uintptr_t wep = ResolveEntityFromHandle(entityList, wh);
                if (wep) {
                    const uintptr_t attrMgr = wep + C_EconEntity::m_AttributeManager;
                    const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
                    const int defIdx = mem.Read<uint16_t>(
                        econItem + C_EconItemView::m_iItemDefinitionIndex);
                    frame.localDefIndex = defIdx;
                    const char* wn = WeaponNameById(defIdx);
                    if (wn) {
                        strncpy_s(frame.localWeaponName, sizeof(frame.localWeaponName), wn, _TRUNCATE);
                    }
                    else if (defIdx > 0) {
                        snprintf(frame.localWeaponName, sizeof(frame.localWeaponName), "Weapon #%d", defIdx);
                    }
                }
            }
        }
    }

    mem.ReadRaw(mem.client + cs2_dumper::offsets::client_dll::dwViewMatrix,
        &frame.viewMatrix, sizeof(ViewMatrix));

    size_t bMin = SIZE_MAX, bMax = 0;
    auto add = [&](size_t o, size_t s) {
        if (o < bMin) bMin = o;
        if (o + s > bMax) bMax = o + s;
        };
    add(C_BaseEntity::m_CBodyComponent, 8);
    add(C_BaseEntity::m_iTeamNum, 1);
    add(C_BaseEntity::m_lifeState, 1);
    add(C_BaseEntity::m_pGameSceneNode, 8);
    add(C_BaseEntity::m_iHealth, 4);
    add(C_BaseEntity::m_vecAbsVelocity, 12);
    add(C_BasePlayerPawn::m_pWeaponServices, 8);
    add(C_BasePlayerPawn::m_pItemServices, 8);
    add(C_CSPlayerPawn::m_angEyeAngles, 8);
    add(C_CSPlayerPawn::m_bIsScoped, 1);
    add(C_CSPlayerPawn::m_bIsDefusing, 1);
    add(C_CSPlayerPawnBase::m_flFlashDuration, 4);
    add(C_CSPlayerPawn::m_ArmorValue, 4);

    const size_t span = bMax - bMin;
    constexpr size_t kBuf = 0x4000;
    uint8_t pawnBuf[kBuf];

    const size_t sDormantOff = CGameSceneNode::m_bDormant;
    const size_t sOriginOff = CGameSceneNode::m_vecAbsOrigin;
    const size_t sMin = (std::min)(sDormantOff, sOriginOff);
    const size_t sMax = (std::max)(sDormantOff + 1, sOriginOff + 12);
    const size_t sSpan = sMax - sMin;
    uint8_t sceneBuf[0x100];

    constexpr size_t kBoneStride = 32;
    alignas(16) uint8_t boneBuf[kBoneCount * kBoneStride];

    const size_t kCtrlHandleOff = CCSPlayerController::m_hPlayerPawn;

    const double now = FrameTime();
    if (entityList != g_cachedEntityList || g_cacheCount == 0 ||
        !g_cachedControllerListEntry || (now - g_lastRescan) > kRescanIntervalSec) {
        g_cachedEntityList = entityList;
        RescanControllers(entityList);
    }

    for (int i = 0; i < g_cacheCount; ++i) {
        CachedPlayer& slot = g_cache[i];
        if (!slot.controller) continue;

        const uint32_t pawnHandle = mem.Read<uint32_t>(slot.controller + kCtrlHandleOff);
        if (!pawnHandle) {
            slot.pawn = 0; slot.pawnHandle = 0; slot.bodyComp = 0;
            slot.cachedC4 = 0; slot.lastC4Check = 0.0;
            slot.lastDefIndex = 0; slot.cachedWeaponName[0] = '\0';
            slot.nameValid = false;
            slot.weaponEnt = 0; slot.weaponHandle = 0; slot.weaponValid = false;
            continue;
        }

        if (slot.pawnHandle != pawnHandle) {
            slot.pawn = ResolveEntityFromHandle(entityList, pawnHandle);
            slot.pawnHandle = pawnHandle;
            slot.bodyComp = 0; slot.cachedC4 = 0; slot.lastC4Check = 0.0;
            slot.lastDefIndex = 0; slot.cachedWeaponName[0] = '\0';
            slot.nameValid = false;
            slot.weaponEnt = 0; slot.weaponHandle = 0; slot.weaponValid = false;
            if (!slot.pawn) continue;
        }
        else if (slot.pawn == 0) {
            slot.pawn = ResolveEntityFromHandle(entityList, pawnHandle);
            if (!slot.pawn) continue;
        }

        const uintptr_t pawn = slot.pawn;
        if (!pawn) continue;

        if (pawn == localPawn) {
            frame.localIndex = (slot.slotIndex > 0)
                ? static_cast<uint32_t>(slot.slotIndex - 1)
                : 0u;
            continue;
        }

        const bool batchOk = (span <= kBuf) &&
            mem.ReadRaw(pawn + bMin, pawnBuf, span);

        auto rd8 = [&](size_t o) -> uint8_t {
            uint8_t v = 0;
            if (batchOk && o >= bMin && o + 1 <= bMax) v = pawnBuf[o - bMin];
            else mem.ReadRaw(pawn + o, &v, 1);
            return v;
            };
        auto rd16 = [&](size_t o) -> uint16_t {
            uint16_t v = 0;
            if (batchOk && o >= bMin && o + 2 <= bMax) memcpy(&v, pawnBuf + (o - bMin), 2);
            else mem.ReadRaw(pawn + o, &v, 2);
            return v;
            };
        auto rd32 = [&](size_t o) -> int32_t {
            int32_t v = 0;
            if (batchOk && o >= bMin && o + 4 <= bMax) memcpy(&v, pawnBuf + (o - bMin), 4);
            else mem.ReadRaw(pawn + o, &v, 4);
            return v;
            };
        auto rd64 = [&](size_t o) -> uint64_t {
            uint64_t v = 0;
            if (batchOk && o >= bMin && o + 8 <= bMax) memcpy(&v, pawnBuf + (o - bMin), 8);
            else mem.ReadRaw(pawn + o, &v, 8);
            return v;
            };
        auto rdf = [&](size_t o) -> float {
            float v = 0.0f;
            if (batchOk && o >= bMin && o + 4 <= bMax) memcpy(&v, pawnBuf + (o - bMin), 4);
            else mem.ReadRaw(pawn + o, &v, 4);
            return v;
            };
        auto rdVec3 = [&](size_t o, Vector3& out) {
            if (batchOk && o >= bMin && o + 12 <= bMax) memcpy(&out, pawnBuf + (o - bMin), 12);
            else mem.ReadRaw(pawn + o, &out, 12);
            };

        FrameEntity entity{};
        entity.address = pawn;

        const uint8_t team = rd8(C_BaseEntity::m_iTeamNum);
        const uint8_t lifeState = rd8(C_BaseEntity::m_lifeState);
        const int32_t hp = rd32(C_BaseEntity::m_iHealth);
        const uintptr_t sceneNode = static_cast<uintptr_t>(rd64(C_BaseEntity::m_pGameSceneNode));
        const uintptr_t bodyComp = static_cast<uintptr_t>(rd64(C_BaseEntity::m_CBodyComponent));

        entity.team = team;
        entity.isAlive = (lifeState == 0);
        entity.health = (hp > 0 && hp <= 100) ? static_cast<uint8_t>(hp) : 0;
        entity.sceneNode = sceneNode;
        rdVec3(C_BaseEntity::m_vecAbsVelocity, entity.velocity);

        const bool teamOk = (team == kTeamT || team == kTeamCT);
        if (!entity.isAlive || entity.health == 0 || !teamOk) {
            slot.pawn = 0; slot.bodyComp = 0;
            slot.cachedC4 = 0; slot.lastC4Check = 0.0;
            slot.weaponEnt = 0; slot.weaponHandle = 0; slot.weaponValid = false;
            continue;
        }

        const bool isEnemySide = (team != frame.localTeam);
        const bool espEnabled = isEnemySide ? config.esp_enemy_enable : config.esp_team_enable;
        const bool aimNeedsBones = config.AimbotNeedsBones() && isEnemySide;
        const bool needBones = espEnabled || aimNeedsBones;
        const bool flagsEnabled = espEnabled && (isEnemySide ? config.esp_enemy_flags : config.esp_team_flags);

        const bool needWeapon = espEnabled && (isEnemySide ? config.esp_enemy_weapon : config.esp_team_weapon);
        const bool needAmmo = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_ammo : config.esp_team_flags_ammo);
        const bool needReload = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_reloading : config.esp_team_flags_reloading);
        const bool needBomb = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_bomb : config.esp_team_flags_bomb);
        const bool needPlanting = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_planting : config.esp_team_flags_planting);
        const bool needScoped = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_scoped : config.esp_team_flags_scoped);
        const bool needDefusing = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_defusing : config.esp_team_flags_defusing);
        const bool needFlashed = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_flashed : config.esp_team_flags_flashed);
        const bool needArmor = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_armor : config.esp_team_flags_armor);
        const bool needMoney = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_money : config.esp_team_flags_money);
        const bool needPing = flagsEnabled && (isEnemySide ? config.esp_enemy_flags_ping : config.esp_team_flags_ping);

        {
            const size_t eyeOff = C_CSPlayerPawn::m_angEyeAngles;
            if (batchOk && eyeOff >= bMin && eyeOff + 8 <= bMax) {
                float eye[2] = {};
                memcpy(eye, pawnBuf + (eyeOff - bMin), 8);
                entity.viewAngles.x = eye[0];
                entity.viewAngles.y = eye[1];
                entity.viewAngles.z = 0.0f;
                entity.hasViewAngles = true;
            }
            else {
                float eye[2] = {};
                if (mem.ReadRaw(pawn + eyeOff, eye, sizeof(eye))) {
                    entity.viewAngles.x = eye[0];
                    entity.viewAngles.y = eye[1];
                    entity.viewAngles.z = 0.0f;
                    entity.hasViewAngles = true;
                }
            }
        }

        if (needScoped)    entity.isScoped = (rd8(C_CSPlayerPawn::m_bIsScoped) != 0);
        if (needDefusing)  entity.isDefusing = (rd8(C_CSPlayerPawn::m_bIsDefusing) != 0);
        if (needFlashed)   entity.isFlashed = (rdf(C_CSPlayerPawnBase::m_flFlashDuration) > 0.0f);

        if (needArmor || needMoney || needPing) {
            if (now - slot.lastSlowPoll > kSlowPollSec) {
                slot.lastSlowPoll = now;
                if (needArmor) {
                    const uintptr_t itemSvc = static_cast<uintptr_t>(rd64(C_BasePlayerPawn::m_pItemServices));
                    slot.lastArmor = static_cast<uint8_t>(
                        (std::clamp)(rd32(C_CSPlayerPawn::m_ArmorValue), 0, 100));
                    slot.lastHelmet = itemSvc ?
                        mem.Read<bool>(itemSvc + CCSPlayer_ItemServices::m_bHasHelmet) : false;
                }
                if (needMoney) {
                    const uintptr_t svc = mem.Read<uintptr_t>(
                        slot.controller + CCSPlayerController::m_pInGameMoneyServices);
                    slot.lastMoney = svc ? mem.Read<int32_t>(
                        svc + CCSPlayerController_InGameMoneyServices::m_iAccount) : 0;
                }
                if (needPing) {
                    slot.lastPing = mem.Read<int32_t>(slot.controller + CCSPlayerController::m_iPing);
                }
            }
            if (needArmor) { entity.armor = slot.lastArmor; entity.hasHelmet = slot.lastHelmet; }
            if (needMoney) entity.money = slot.lastMoney;
            if (needPing)  entity.ping = slot.lastPing;
        }

        if (bodyComp) slot.bodyComp = bodyComp;

        if (!slot.nameValid) {
            slot.name[sizeof(slot.name) - 1] = '\0';
            mem.ReadRaw(slot.controller + CBasePlayerController::m_iszPlayerName,
                slot.name, sizeof(slot.name) - 1);
            slot.name[sizeof(slot.name) - 1] = '\0';
            slot.nameValid = true;
        }
        std::memcpy(entity.name, slot.name, sizeof(entity.name));

        if (needWeapon || needAmmo || needReload) {
            const uintptr_t weaponSvc = static_cast<uintptr_t>(rd64(C_BasePlayerPawn::m_pWeaponServices));
            if (weaponSvc && (now - slot.lastWeaponPoll) > kWeaponPollSec) {
                slot.lastWeaponPoll = now;
                const uint32_t handle = mem.Read<uint32_t>(
                    weaponSvc + CPlayer_WeaponServices::m_hActiveWeapon);
                if (handle != slot.weaponHandle) {
                    slot.weaponHandle = handle;
                    slot.weaponEnt = handle ? ResolveEntityFromHandle(entityList, handle) : 0;
                    slot.lastDefIndex = 0;
                    slot.cachedWeaponName[0] = '\0';
                    slot.weaponValid = false;
                }
                if (slot.weaponEnt) {
                    const uintptr_t w = slot.weaponEnt;
                    const uintptr_t attrMgr = w + C_EconEntity::m_AttributeManager;
                    const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
                    const int defIdx = mem.Read<uint16_t>(
                        econItem + C_EconItemView::m_iItemDefinitionIndex);
                    if (defIdx > 0 && static_cast<uint16_t>(defIdx) != slot.lastDefIndex) {
                        const char* wn = WeaponNameById(defIdx);
                        if (wn) strncpy_s(slot.cachedWeaponName, sizeof(slot.cachedWeaponName), wn, _TRUNCATE);
                        else snprintf(slot.cachedWeaponName, sizeof(slot.cachedWeaponName), "Weapon #%d", defIdx);
                        slot.lastDefIndex = static_cast<uint16_t>(defIdx);
                    }
                    slot.weaponIsReloading = mem.Read<bool>(w + C_CSWeaponBase::m_bInReload);
                    slot.weaponAmmo = mem.Read<int32_t>(w + C_BasePlayerWeapon::m_iClip1);
                    slot.weaponReserve = mem.Read<int32_t>(w + C_BasePlayerWeapon::m_pReserveAmmo);
                    slot.weaponValid = true;
                }
            }
            if (slot.weaponValid) {
                if (needWeapon) std::memcpy(entity.weaponName, slot.cachedWeaponName, sizeof(entity.weaponName));
                if (needReload) entity.isReloading = slot.weaponIsReloading;
                if (needAmmo) { entity.ammo = slot.weaponAmmo; entity.reserve = slot.weaponReserve; }
            }
        }

        if (needBomb || needPlanting) {
            if (!slot.cachedC4 || (now - slot.lastC4Check) > kC4CacheSec) {
                FindC4OnPlayer(pawn, entityList, slot.cachedC4);
                slot.lastC4Check = now;
            }
            entity.hasBomb = (slot.cachedC4 != 0);
            if (needPlanting && slot.cachedC4) {
                const bool a = mem.Read<bool>(slot.cachedC4 + C_C4::m_bStartedArming);
                const bool b = mem.Read<bool>(slot.cachedC4 + C_C4::m_bIsPlantingViaUse);
                const bool c = mem.Read<bool>(slot.cachedC4 + C_C4::m_bBombPlacedAnimation);
                entity.isPlanting = a || b || c;
            }
        }

        if (!sceneNode) { entity.isDormant = true; frame.entities.push_back(entity); continue; }

        if (sSpan <= sizeof(sceneBuf) && mem.ReadRaw(sceneNode + sMin, sceneBuf, sSpan)) {
            entity.isDormant = (sceneBuf[sDormantOff - sMin] != 0);
            std::memcpy(&entity.origin, sceneBuf + (sOriginOff - sMin), sizeof(Vector3));
        }
        else {
            entity.isDormant = mem.Read<bool>(sceneNode + sDormantOff);
            mem.ReadRaw(sceneNode + sOriginOff, &entity.origin, sizeof(Vector3));
        }

        entity.headPos = entity.origin;
        entity.feetPos = entity.origin;

        uintptr_t boneArray = 0;
        if (needBones && bodyComp) {
            const uintptr_t skel = mem.Read<uintptr_t>(bodyComp + CBodyComponent::m_pSceneNode);
            if (skel) boneArray = mem.Read<uintptr_t>(
                skel + CSkeletonInstance::m_modelState + 0x80);
        }
        entity.boneArray = boneArray;

        if (needBones && boneArray && !entity.isDormant) {
            if (mem.ReadRaw(boneArray, boneBuf, sizeof(boneBuf))) {
                for (int b = 0; b < kBoneCount; ++b)
                    std::memcpy(&entity.bones[b], boneBuf + b * kBoneStride, sizeof(Vector3));
                entity.hasBones = true;

                const Vector3& hb = entity.bones[static_cast<int>(BoneIndex::Head)];
                if (hb.x != 0.0f || hb.y != 0.0f || hb.z != 0.0f) {
                    const Vector3& lf = entity.bones[static_cast<int>(BoneIndex::LeftFoot)];
                    const Vector3& rf = entity.bones[static_cast<int>(BoneIndex::RightFoot)];
                    entity.headPos = hb;
                    entity.feetPos.x = (lf.x + rf.x) * 0.5f;
                    entity.feetPos.y = (lf.y + rf.y) * 0.5f;
                    entity.feetPos.z = (lf.z + rf.z) * 0.5f;
                }
            }
            else {
                entity.hasBones = false;
            }
        }
        else {
            entity.hasBones = false;
        }

        frame.entities.push_back(entity);
    }
}

void AcquireRefreshedFrameSnapshot(FrameSnapshot& out) {
    const double t0 = FrameTime();
    BuildFrameSnapshot(out);
    const double t1 = FrameTime();
    out.publishTime = t1;
    out.buildDuration = (t1 - t0) * 1000.0;
    Logging::Sample("entity_build_ms", out.buildDuration);
}