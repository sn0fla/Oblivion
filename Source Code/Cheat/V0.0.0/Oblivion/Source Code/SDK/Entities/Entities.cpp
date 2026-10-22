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
    };

    constexpr int       kMaxPlayers = 64;
    constexpr uintptr_t kSlotStride = 0x70;
    constexpr double    kRescanIntervalSec = 0.5;
    constexpr double    kC4CacheSec = 0.5;

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

    struct WeaponInfo {
        char     name[32] = {};
        bool     isReloading = false;
        int32_t  ammo = 0;
        int32_t  reserve = 0;
    };

    struct WeaponReadNeeds {
        bool name = false;
        bool reload = false;
        bool ammo = false;
    };

    void ReadWeaponInfo(uintptr_t pawn, uintptr_t entityList, CachedPlayer& slot,
        const WeaponReadNeeds& needs, WeaponInfo& out) {
        if (!pawn) return;
        if (!needs.name && !needs.reload && !needs.ammo) return;
        using namespace cs2_dumper::schemas::client_dll;

        const uintptr_t weaponServices = mem.Read<uintptr_t>(
            pawn + C_BasePlayerPawn::m_pWeaponServices);
        if (!weaponServices) return;

        const uint32_t handle = mem.Read<uint32_t>(
            weaponServices + CPlayer_WeaponServices::m_hActiveWeapon);
        if (!handle || handle == 0xFFFFFFFF) return;

        const uintptr_t weapon = ResolveEntityFromHandle(entityList, handle);
        if (!weapon) return;

        if (needs.name) {
            const uintptr_t attrMgr = weapon + C_EconEntity::m_AttributeManager;
            const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
            const int defIndex = mem.Read<uint16_t>(econItem + C_EconItemView::m_iItemDefinitionIndex);
            if (defIndex <= 0) return;

            if (static_cast<uint16_t>(defIndex) != slot.lastDefIndex) {
                const char* wname = WeaponNameById(defIndex);
                if (wname) strncpy_s(slot.cachedWeaponName, sizeof(slot.cachedWeaponName), wname, _TRUNCATE);
                else snprintf(slot.cachedWeaponName, sizeof(slot.cachedWeaponName), "Weapon #%d", defIndex);
                slot.lastDefIndex = static_cast<uint16_t>(defIndex);
            }
            strncpy_s(out.name, sizeof(out.name), slot.cachedWeaponName, _TRUNCATE);
        }

        if (needs.reload)
            out.isReloading = mem.Read<bool>(weapon + C_CSWeaponBase::m_bInReload);
        if (needs.ammo) {
            out.ammo = mem.Read<int32_t>(weapon + C_BasePlayerWeapon::m_iClip1);
            out.reserve = mem.Read<int32_t>(weapon + C_BasePlayerWeapon::m_pReserveAmmo);
        }
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
                    break;
                }
            }
        }

        std::memcpy(g_cache, newCache, sizeof(CachedPlayer) * newCount);
        g_cacheCount = newCount;
        g_lastRescan = FrameTime();
    }

    uintptr_t ResolvePawn(uintptr_t entityList, uint32_t handle) {
        return ResolveEntityFromHandle(entityList, handle);
    }
}

void BuildFrameSnapshot(FrameSnapshot& frame) {
    frame.entities.clear();
    frame.localTeam = 0;
    frame.localOrigin = Vector3{};
    frame.localEyePos = Vector3{};
    frame.localVelocity = Vector3{};
    frame.viewMatrix = ViewMatrix{};

    if (!mem.client || !mem.pid || !mem.handle) return;

    using namespace cs2_dumper::schemas::client_dll;

    const uintptr_t localPawn = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);

    if (!localPawn || localPawn < 0x10000) return;

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

    mem.ReadRaw(mem.client + cs2_dumper::offsets::client_dll::dwViewMatrix,
        &frame.viewMatrix, sizeof(ViewMatrix));

    const size_t kTeamOff = C_BaseEntity::m_iTeamNum;
    const size_t kLifeOff = C_BaseEntity::m_lifeState;
    const size_t kSceneOff = C_BaseEntity::m_pGameSceneNode;
    const size_t kHealthOff = C_BaseEntity::m_iHealth;
    const size_t kBodyCompOff = C_BaseEntity::m_CBodyComponent;
    const size_t kVelocityOff = C_BaseEntity::m_vecAbsVelocity;
    const size_t kSceneNodeOff = CBodyComponent::m_pSceneNode;
    const size_t kModelStateOff = CSkeletonInstance::m_modelState;
    const size_t kCtrlHandleOff = CCSPlayerController::m_hPlayerPawn;

    constexpr size_t kBoneArrayOff = 0x80;
    constexpr size_t kBoneStride = 32;

    const size_t kMinPawnOff = (std::min)({ kTeamOff, kLifeOff, kSceneOff, kHealthOff, kBodyCompOff, kVelocityOff });
    const size_t kMaxPawnEnd = (std::max)({ kTeamOff + 1, kLifeOff + 1,
                                            kSceneOff + 8, kHealthOff + 4, kBodyCompOff + 8, kVelocityOff + 12 });
    const size_t kPawnSpan = kMaxPawnEnd - kMinPawnOff;

    const size_t kDormantOff = CGameSceneNode::m_bDormant;
    const size_t kOriginOff = CGameSceneNode::m_vecAbsOrigin;
    const size_t kMinSceneOff = (std::min)(kDormantOff, kOriginOff);
    const size_t kMaxSceneEnd = (std::max)(kDormantOff + 1, kOriginOff + 12);
    const size_t kSceneSpan = kMaxSceneEnd - kMinSceneOff;

    constexpr size_t kBufSize = 0x2000;
    uint8_t pawnBuf[kBufSize];
    uint8_t sceneBuf[kBufSize];
    alignas(16) uint8_t boneBuf[kBoneCount * kBoneStride];

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
            slot.pawn = 0;
            slot.pawnHandle = 0;
            slot.bodyComp = 0;
            slot.cachedC4 = 0;
            slot.lastC4Check = 0.0;
            slot.lastDefIndex = 0;
            slot.cachedWeaponName[0] = '\0';
            slot.nameValid = false;
            continue;
        }

        if (slot.pawnHandle != pawnHandle) {
            slot.pawn = ResolvePawn(entityList, pawnHandle);
            slot.pawnHandle = pawnHandle;
            slot.bodyComp = 0;
            slot.cachedC4 = 0;
            slot.lastC4Check = 0.0;
            slot.lastDefIndex = 0;
            slot.cachedWeaponName[0] = '\0';
            slot.nameValid = false;
            if (!slot.pawn) continue;
        }
        else if (slot.pawn == 0) {
            slot.pawn = ResolvePawn(entityList, pawnHandle);
            if (!slot.pawn) continue;
        }

        const uintptr_t pawn = slot.pawn;
        if (!pawn || pawn == localPawn) continue;

        FrameEntity entity{};
        entity.address = pawn;
        uintptr_t sceneNode = 0;
        uintptr_t bodyComp = 0;

        if (mem.ReadRaw(pawn + kMinPawnOff, pawnBuf, kPawnSpan)) {
            entity.team = pawnBuf[kTeamOff - kMinPawnOff];
            entity.isAlive = (pawnBuf[kLifeOff - kMinPawnOff] == 0);
            int32_t hp = 0;
            std::memcpy(&hp, pawnBuf + (kHealthOff - kMinPawnOff), sizeof(hp));
            entity.health = (hp > 0 && hp <= 100) ? static_cast<uint8_t>(hp) : 0;
            std::memcpy(&sceneNode, pawnBuf + (kSceneOff - kMinPawnOff), sizeof(uintptr_t));
            std::memcpy(&bodyComp, pawnBuf + (kBodyCompOff - kMinPawnOff), sizeof(uintptr_t));

            Vector3 rawVel{};
            std::memcpy(&rawVel, pawnBuf + (kVelocityOff - kMinPawnOff), sizeof(Vector3));
            entity.velocity = rawVel;
            entity.sceneNode = sceneNode;
        }
        else continue;

        const bool teamOk = (entity.team == kTeamT || entity.team == kTeamCT);
        const bool lifeOk = entity.isAlive;
        const bool healthOk = (entity.health > 0 && entity.health <= 100);
        if (!lifeOk || !healthOk || !teamOk) {
            slot.pawn = 0;
            slot.bodyComp = 0;
            slot.cachedC4 = 0;
            slot.lastC4Check = 0.0;
            continue;
        }

        const bool isEnemySide = (entity.team != frame.localTeam);
        const bool espEnabled = isEnemySide ? config.esp_enemy_enable : config.esp_team_enable;
        const bool aimNeedsBones = config.aim_enable && isEnemySide;
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
            float eye[2] = {};
            if (mem.ReadRaw(pawn + C_CSPlayerPawn::m_angEyeAngles, eye, sizeof(eye))) {
                entity.viewAngles.x = eye[0];
                entity.viewAngles.y = eye[1];
                entity.viewAngles.z = 0.0f;
                entity.hasViewAngles = true;
            }
        }

        if (needScoped)
            entity.isScoped = mem.Read<bool>(pawn + C_CSPlayerPawn::m_bIsScoped);
        if (needDefusing)
            entity.isDefusing = mem.Read<bool>(pawn + C_CSPlayerPawn::m_bIsDefusing);
        if (needFlashed) {
            const float flashDur = mem.Read<float>(pawn + C_CSPlayerPawnBase::m_flFlashDuration);
            entity.isFlashed = (flashDur > 0.0f);
        }
        if (needArmor) {
            const uintptr_t itemServices = mem.Read<uintptr_t>(
                pawn + C_BasePlayerPawn::m_pItemServices);
            if (itemServices) {
                entity.hasHelmet = mem.Read<bool>(
                    itemServices + CCSPlayer_ItemServices::m_bHasHelmet);
            }
            const int32_t armorVal = mem.Read<int32_t>(pawn + C_CSPlayerPawn::m_ArmorValue);
            entity.armor = (armorVal > 0 && armorVal <= 100)
                ? static_cast<uint8_t>(armorVal) : 0;
        }
        if (needMoney) {
            const uintptr_t moneyServices = mem.Read<uintptr_t>(
                slot.controller + CCSPlayerController::m_pInGameMoneyServices);
            if (moneyServices) {
                entity.money = mem.Read<int32_t>(
                    moneyServices + CCSPlayerController_InGameMoneyServices::m_iAccount);
            }
        }
        if (needPing) {
            entity.ping = mem.Read<int32_t>(slot.controller + CCSPlayerController::m_iPing);
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
            WeaponReadNeeds needs;
            needs.name = needWeapon;
            needs.reload = needReload;
            needs.ammo = needAmmo;
            WeaponInfo wi;
            ReadWeaponInfo(pawn, entityList, slot, needs, wi);
            if (needWeapon) std::memcpy(entity.weaponName, wi.name, sizeof(entity.weaponName));
            if (needReload) entity.isReloading = wi.isReloading;
            if (needAmmo) { entity.ammo = wi.ammo; entity.reserve = wi.reserve; }
        }

        if (needBomb || needPlanting) {
            const bool refresh = (!slot.cachedC4) || ((now - slot.lastC4Check) > kC4CacheSec);
            if (refresh) {
                FindC4OnPlayer(pawn, entityList, slot.cachedC4);
                slot.lastC4Check = now;
            }
            entity.hasBomb = (slot.cachedC4 != 0);
            if (needPlanting && slot.cachedC4) {
                const bool startedArming = mem.Read<bool>(slot.cachedC4 + C_C4::m_bStartedArming);
                const bool plantingViaUse = mem.Read<bool>(slot.cachedC4 + C_C4::m_bIsPlantingViaUse);
                const bool placedAnim = mem.Read<bool>(slot.cachedC4 + C_C4::m_bBombPlacedAnimation);
                entity.isPlanting = startedArming || plantingViaUse || placedAnim;
            }
        }

        if (!sceneNode) { entity.isDormant = true; frame.entities.push_back(entity); continue; }

        if (mem.ReadRaw(sceneNode + kMinSceneOff, sceneBuf, kSceneSpan)) {
            entity.isDormant = (sceneBuf[kDormantOff - kMinSceneOff] != 0);
            std::memcpy(&entity.origin, sceneBuf + (kOriginOff - kMinSceneOff), sizeof(Vector3));
        }
        else continue;

        entity.headPos = entity.origin;
        entity.feetPos = entity.origin;

        uintptr_t boneArray = 0;
        if (needBones && bodyComp) {
            const uintptr_t skel = mem.Read<uintptr_t>(bodyComp + kSceneNodeOff);
            if (skel)
                boneArray = mem.Read<uintptr_t>(skel + kModelStateOff + kBoneArrayOff);
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