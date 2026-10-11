#pragma once
#include <cstdint>
#include <vector>
#include <array>
#include <atomic>
#include "../../Utilities/Maths/Maths.hpp"

enum class BoneIndex : int {
    Pelvis = 1, Spine_0 = 2, Spine_1 = 3, Spine_2 = 4,
    Neck = 6, Head = 7,
    LeftClavicle = 8, LeftUpperArm = 9, LeftForearm = 10, LeftHand = 11,
    RightClavicle = 12, RightUpperArm = 13, RightForearm = 14, RightHand = 15,
    LeftHip = 17, LeftKnee = 18, LeftFoot = 19,
    RightHip = 20, RightKnee = 21, RightFoot = 22,
    Count = 23
};
static constexpr int kBoneCount = static_cast<int>(BoneIndex::Count);

struct FrameEntity {
    Vector3 origin{};
    Vector3 headPos{};
    Vector3 feetPos{};
    Vector3 velocity{};
    Vector3 viewAngles{};
    std::array<Vector3, kBoneCount> bones{};
    uintptr_t address = 0;
    uintptr_t sceneNode = 0;
    uintptr_t boneArray = 0;
    uint8_t   team = 0;
    uint8_t   health = 0;
    bool isAlive = false;
    bool isDormant = false;
    bool hasBones = false;
    bool hasViewAngles = false;
    char name[128] = {};
    char weaponName[32] = {};

    bool  isFlashed = false;
    bool  isScoped = false;
    bool  isDefusing = false;
    bool  isPlanting = false;
    bool  hasBomb = false;
    bool  hasHelmet = false;
    bool  isReloading = false;
    uint8_t armor = 0;
    int32_t money = 0;
    int32_t ping = 0;
    int32_t ammo = 0;
    int32_t reserve = 0;
};

struct FrameSnapshot {
    std::vector<FrameEntity> entities;
    ViewMatrix viewMatrix{};
    Vector3 localOrigin{};
    uint8_t localTeam = 0;
    double publishTime = 0.0;
    double buildDuration = 0.0;
};

void BuildFrameSnapshot(FrameSnapshot& frame);

void StartEntityThread();
void StopEntityThread();
void SignalEntityUpdate();

const FrameSnapshot* AcquireFrameSnapshot();
void ReleaseFrameSnapshot();

double FrameTime();

double GetEntityThreadHz();
uint64_t GetEntityThreadIterations();
double GetEntityThreadLastBuildMs();