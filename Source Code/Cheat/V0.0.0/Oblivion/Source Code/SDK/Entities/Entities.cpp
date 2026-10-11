#include "Entities.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../Core/Memory/Memory.hpp"
#include "../Schemas/offsets.hpp"
#include "../Schemas/client_dll.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <thread>
#include <atomic>
#include <mutex>

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
        uintptr_t boneArray = 0;
        uintptr_t bodyComp = 0;
        char      name[128] = {};
        bool      nameValid = false;
    };

    constexpr int       kMaxPlayers = 64;
    constexpr uintptr_t kSlotStride = 0x70;
    constexpr double    kRescanIntervalSec = 0.5;

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

    void ReadWeaponInfo(uintptr_t pawn, uintptr_t entityList, WeaponInfo& out) {
        if (!pawn) return;

        using namespace cs2_dumper::schemas::client_dll;

        const uintptr_t weaponServices = mem.Read<uintptr_t>(
            pawn + C_BasePlayerPawn::m_pWeaponServices);
        if (!weaponServices) return;

        const uint32_t handle = mem.Read<uint32_t>(
            weaponServices + CPlayer_WeaponServices::m_hActiveWeapon);
        if (!handle || handle == 0xFFFFFFFF) return;

        const uintptr_t weapon = ResolveEntityFromHandle(entityList, handle);
        if (!weapon) return;

        const uintptr_t attrMgr = weapon + C_EconEntity::m_AttributeManager;
        const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;

        const int defIndex = mem.Read<uint16_t>(
            econItem + C_EconItemView::m_iItemDefinitionIndex);
        if (defIndex <= 0) return;

        const char* name = WeaponNameById(defIndex);
        if (name) strncpy_s(out.name, sizeof(out.name), name, _TRUNCATE);
        else snprintf(out.name, sizeof(out.name), "Weapon #%d", defIndex);

        out.isReloading = mem.Read<bool>(weapon + C_CSWeaponBase::m_bInReload);
        out.ammo = mem.Read<int32_t>(weapon + C_BasePlayerWeapon::m_iClip1);
        out.reserve = mem.Read<int32_t>(weapon + C_BasePlayerWeapon::m_pReserveAmmo);
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
                if (defIndex == 49) {
                    c4Out = activeWeapon;
                    return true;
                }
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

            if (defIndex == 49) {
                c4Out = weapon;
                return true;
            }
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
                    dst.boneArray = g_cache[j].boneArray;
                    dst.bodyComp = g_cache[j].bodyComp;
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

    void RefreshPositionsInPlace(FrameSnapshot& frame) {
        using namespace cs2_dumper::schemas::client_dll;

        const size_t kDormantOff = CGameSceneNode::m_bDormant;
        const size_t kOriginOff = CGameSceneNode::m_vecAbsOrigin;
        const size_t kMinOff = (std::min)(kDormantOff, kOriginOff);
        const size_t kMaxEnd = (std::max)(kDormantOff + 1, kOriginOff + 12);
        const size_t kSpan = kMaxEnd - kMinOff;

        constexpr size_t kBoneStride = 32;

        uint8_t sceneBuf[0x200];
        uint8_t boneBuf[kBoneCount * kBoneStride];

        for (auto& ent : frame.entities) {
            if (ent.sceneNode) {
                if (mem.ReadRaw(ent.sceneNode + kMinOff, sceneBuf, kSpan)) {
                    ent.isDormant = (sceneBuf[kDormantOff - kMinOff] != 0);
                    std::memcpy(&ent.origin, sceneBuf + (kOriginOff - kMinOff), sizeof(Vector3));
                }
            }

            bool bonesOk = false;
            if (ent.boneArray) {
                if (mem.ReadRaw(ent.boneArray, boneBuf, sizeof(boneBuf))) {
                    for (int b = 0; b < kBoneCount; ++b)
                        std::memcpy(&ent.bones[b], boneBuf + b * kBoneStride, sizeof(Vector3));
                    ent.hasBones = true;
                    bonesOk = true;
                }
                else {
                    ent.hasBones = false;
                }
            }
            else {
                ent.hasBones = false;
            }

            const Vector3& hb = ent.bones[static_cast<int>(BoneIndex::Head)];
            const bool headValid = bonesOk && (hb.x != 0.0f || hb.y != 0.0f || hb.z != 0.0f);

            if (headValid) {
                const Vector3& lf = ent.bones[static_cast<int>(BoneIndex::LeftFoot)];
                const Vector3& rf = ent.bones[static_cast<int>(BoneIndex::RightFoot)];
                ent.headPos = hb;
                ent.feetPos.x = (lf.x + rf.x) * 0.5f;
                ent.feetPos.y = (lf.y + rf.y) * 0.5f;
                ent.feetPos.z = (lf.z + rf.z) * 0.5f;
            }
            else {
                ent.headPos = ent.origin;
                ent.headPos.z += 72.0f;
                ent.feetPos = ent.origin;
            }
        }
    }
}

void BuildFrameSnapshot(FrameSnapshot& frame) {
    frame.entities.clear();
    frame.localTeam = 0;
    frame.localOrigin = Vector3{};
    frame.viewMatrix = ViewMatrix{};

    if (!mem.client || !mem.pid || !mem.handle) return;

    mem.ReadRaw(mem.client + cs2_dumper::offsets::client_dll::dwViewMatrix,
        &frame.viewMatrix, sizeof(ViewMatrix));

    const uintptr_t localPawn = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (!localPawn || localPawn < 0x10000) return;

    frame.localTeam = mem.Read<uint8_t>(
        localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);

    {
        const uintptr_t localScene = mem.Read<uintptr_t>(
            localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (localScene) {
            mem.ReadRaw(
                localScene + cs2_dumper::schemas::client_dll::CGameSceneNode::m_vecAbsOrigin,
                &frame.localOrigin, sizeof(Vector3));
        }
    }

    const uintptr_t entityList = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return;

    using namespace cs2_dumper::schemas::client_dll;

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
            slot.boneArray = 0;
            slot.bodyComp = 0;
            slot.nameValid = false;
            continue;
        }

        if (slot.pawnHandle != pawnHandle) {
            slot.pawn = ResolvePawn(entityList, pawnHandle);
            slot.pawnHandle = pawnHandle;
            slot.boneArray = 0;
            slot.bodyComp = 0;
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
            std::memcpy(&entity.velocity, pawnBuf + (kVelocityOff - kMinPawnOff), sizeof(Vector3));
            entity.sceneNode = sceneNode;
        }
        else continue;

        {
            float eye[2] = {};
            if (mem.ReadRaw(pawn + C_CSPlayerPawn::m_angEyeAngles,
                eye, sizeof(eye)))
            {
                entity.viewAngles.x = eye[0];
                entity.viewAngles.y = eye[1];
                entity.viewAngles.z = 0.0f;
                entity.hasViewAngles = true;
            }
        }

        {
            const bool isEnemySide = (entity.team != frame.localTeam);
            const bool wantFlags = isEnemySide
                ? config.esp_enemy_flags
                : config.esp_team_flags;

            if (wantFlags) {
                entity.isScoped = mem.Read<bool>(pawn + C_CSPlayerPawn::m_bIsScoped);
                entity.isDefusing = mem.Read<bool>(pawn + C_CSPlayerPawn::m_bIsDefusing);

                const float flashDur = mem.Read<float>(pawn + C_CSPlayerPawnBase::m_flFlashDuration);
                entity.isFlashed = (flashDur > 0.0f);

                const uintptr_t itemServices = mem.Read<uintptr_t>(
                    pawn + C_BasePlayerPawn::m_pItemServices);
                if (itemServices) {
                    entity.hasHelmet = mem.Read<bool>(
                        itemServices + CCSPlayer_ItemServices::m_bHasHelmet);
                }

                const int32_t armorVal = mem.Read<int32_t>(pawn + C_CSPlayerPawn::m_ArmorValue);
                entity.armor = (armorVal > 0 && armorVal <= 100)
                    ? static_cast<uint8_t>(armorVal) : 0;

                const uintptr_t moneyServices = mem.Read<uintptr_t>(
                    slot.controller + CCSPlayerController::m_pInGameMoneyServices);
                if (moneyServices) {
                    entity.money = mem.Read<int32_t>(
                        moneyServices + CCSPlayerController_InGameMoneyServices::m_iAccount);
                }

                entity.ping = mem.Read<int32_t>(slot.controller + CCSPlayerController::m_iPing);
            }
        }

        if (bodyComp) slot.bodyComp = bodyComp;

        const bool lifeOk = entity.isAlive;
        const bool healthOk = (entity.health > 0 && entity.health <= 100);
        const bool teamOk = (entity.team == kTeamT || entity.team == kTeamCT);
        if (!lifeOk || !healthOk || !teamOk) {
            slot.pawn = 0;
            slot.boneArray = 0;
            slot.bodyComp = 0;
            continue;
        }

        if (!slot.nameValid) {
            slot.name[sizeof(slot.name) - 1] = '\0';
            mem.ReadRaw(slot.controller + CBasePlayerController::m_iszPlayerName,
                slot.name, sizeof(slot.name) - 1);
            slot.name[sizeof(slot.name) - 1] = '\0';
            slot.nameValid = true;
        }
        std::memcpy(entity.name, slot.name, sizeof(entity.name));

        {
            WeaponInfo wi;
            ReadWeaponInfo(pawn, entityList, wi);
            std::memcpy(entity.weaponName, wi.name, sizeof(entity.weaponName));
            entity.isReloading = wi.isReloading;
            entity.ammo = wi.ammo;
            entity.reserve = wi.reserve;

            uintptr_t c4 = 0;
            entity.hasBomb = FindC4OnPlayer(pawn, entityList, c4);
            if (c4) {
                const bool startedArming = mem.Read<bool>(c4 + C_C4::m_bStartedArming);
                const bool plantingViaUse = mem.Read<bool>(c4 + C_C4::m_bIsPlantingViaUse);
                const bool placedAnim = mem.Read<bool>(c4 + C_C4::m_bBombPlacedAnimation);
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
        entity.headPos.z += 72.0f;
        entity.feetPos = entity.origin;

        const bool isEnemySide = (entity.team != frame.localTeam);
        const bool espOnForSide = isEnemySide
            ? config.esp_enemy_enable
            : config.esp_team_enable;

        if (espOnForSide) {
            if (!slot.boneArray) {
                if (!slot.bodyComp) slot.bodyComp = mem.Read<uintptr_t>(pawn + kBodyCompOff);
                if (slot.bodyComp) {
                    const uintptr_t skel = mem.Read<uintptr_t>(slot.bodyComp + kSceneNodeOff);
                    if (skel) slot.boneArray = mem.Read<uintptr_t>(
                        skel + kModelStateOff + kBoneArrayOff);
                }
            }
        }

        entity.boneArray = slot.boneArray;

        frame.entities.push_back(entity);
    }
}

static FrameSnapshot    g_snapshots[2];
static std::atomic<int> g_readIdx{ 0 };
static int              g_writeIdx = 1;
static std::mutex       g_snapshotMutex;

static std::thread      g_entityThread;
static std::atomic<bool> g_entityThreadRunning{ false };

static std::atomic<double>   g_entityThreadHz{ 0.0 };
static std::atomic<uint64_t> g_entityThreadIters{ 0 };
static std::atomic<double>   g_entityThreadLastBuildMs{ 0.0 };

static void EntityThreadFunc() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    unsigned cores = std::thread::hardware_concurrency();
    if (cores > 1) {
        DWORD_PTR mask = (DWORD_PTR)1 << ((cores > 2) ? (cores - 2) : 0);
        SetThreadAffinityMask(GetCurrentThread(), mask);
    }

    uint64_t localIters = 0;
    double hzWindowStart = FrameTime();

    while (g_entityThreadRunning.load(std::memory_order_relaxed)) {
        if (!mem.client || !mem.pid || !mem.handle) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const double t0 = FrameTime();

        FrameSnapshot& dst = g_snapshots[g_writeIdx];
        BuildFrameSnapshot(dst);

        const double t1 = FrameTime();
        dst.publishTime = t1;
        dst.buildDuration = (t1 - t0) * 1000.0;
        g_entityThreadLastBuildMs.store(dst.buildDuration, std::memory_order_relaxed);

        {
            std::lock_guard<std::mutex> lock(g_snapshotMutex);
            int prev = g_readIdx.exchange(g_writeIdx, std::memory_order_release);
            g_writeIdx = prev;
        }

        localIters++;
        const double now = FrameTime();
        const double span = now - hzWindowStart;
        if (span >= 0.5) {
            g_entityThreadHz.store(static_cast<double>(localIters) / span, std::memory_order_relaxed);
            g_entityThreadIters.store(localIters, std::memory_order_relaxed);
            localIters = 0;
            hzWindowStart = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void StartEntityThread() {
    if (g_entityThreadRunning.load(std::memory_order_relaxed)) return;
    g_entityThreadRunning.store(true, std::memory_order_relaxed);
    g_entityThread = std::thread(EntityThreadFunc);
}

void StopEntityThread() {
    if (!g_entityThreadRunning.load(std::memory_order_relaxed)) return;
    g_entityThreadRunning.store(false, std::memory_order_relaxed);
    if (g_entityThread.joinable()) g_entityThread.join();
}

void SignalEntityUpdate() {}

const FrameSnapshot* AcquireFrameSnapshot() {
    g_snapshotMutex.lock();
    const int idx = g_readIdx.load(std::memory_order_acquire);
    FrameSnapshot& snap = g_snapshots[idx];
    if (snap.publishTime == 0.0) {
        g_snapshotMutex.unlock();
        return nullptr;
    }
    RefreshPositionsInPlace(snap);
    return &snap;
}

void ReleaseFrameSnapshot() {
    g_snapshotMutex.unlock();
}

double GetEntityThreadHz() {
    return g_entityThreadHz.load(std::memory_order_relaxed);
}

uint64_t GetEntityThreadIterations() {
    return g_entityThreadIters.load(std::memory_order_relaxed);
}

double GetEntityThreadLastBuildMs() {
    return g_entityThreadLastBuildMs.load(std::memory_order_relaxed);
}
