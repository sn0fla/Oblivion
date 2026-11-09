#include "Weapon.hpp"
#include "../Box/Box.hpp"
#include "../../../../../../Utilities/Maths/Maths.hpp"
#include "../../../../../../Utilities/Config/Config.hpp"
#include "../../../../../../SDK/Schemas/client_dll.hpp"
#include "../../../../../../SDK/Schemas/offsets.hpp"
#include "../../../../../Memory/Memory.hpp"
#include "../../../../../../External/ImGUI/imgui.h"
#include <cmath>
#include <cstdio>
#include <cstring>

static const char* WeaponNameById(int id) {
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

const char* GetWeaponNameFromPawn(uintptr_t pawn, char* outName, size_t outSize) {
    if (!outName || outSize == 0) return nullptr;
    outName[0] = '\0';
    if (!pawn || pawn < 0x10000 || !mem.client || !mem.handle) return nullptr;

    using namespace cs2_dumper::schemas::client_dll;

    const uintptr_t weaponServices = mem.Read<uintptr_t>(
        pawn + C_BasePlayerPawn::m_pWeaponServices);
    if (!weaponServices) return nullptr;

    const uint32_t activeHandle = mem.Read<uint32_t>(
        weaponServices + CPlayer_WeaponServices::m_hActiveWeapon);
    if (!activeHandle || activeHandle == 0xFFFFFFFF) return nullptr;

    const uintptr_t entityList = mem.Read<uintptr_t>(
        mem.client + cs2_dumper::offsets::client_dll::dwEntityList);
    if (!entityList) return nullptr;

    const uintptr_t listEntry = mem.Read<uintptr_t>(
        entityList + 0x8ull * ((activeHandle & 0x7FFF) >> 9) + 0x10);
    if (!listEntry) return nullptr;

    const uintptr_t weapon = mem.Read<uintptr_t>(
        listEntry + 0x70ull * (activeHandle & 0x1FF));
    if (!weapon) return nullptr;

    const uintptr_t attrMgr = weapon + C_EconEntity::m_AttributeManager;
    const uintptr_t econItem = attrMgr + C_AttributeContainer::m_Item;
    const int defIdx = mem.Read<uint16_t>(
        econItem + C_EconItemView::m_iItemDefinitionIndex);

    const char* name = WeaponNameById(defIdx);
    if (!name) return nullptr;

    snprintf(outName, outSize, "%s", name);
    return outName;
}

namespace Features::ESP {

    static inline ImU32 WeaponCol(const float c[4]) {
        return IM_COL32(
            static_cast<int>(c[0] * 255.0f),
            static_cast<int>(c[1] * 255.0f),
            static_cast<int>(c[2] * 255.0f),
            static_cast<int>(c[3] * 255.0f));
    }

    static void RenderWeaponSide(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix, int width, int height, bool isEnemy)
    {
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImFont* font = ImGui::GetFont();
        if (!font) return;

        const float* colArr = isEnemy ? config.esp_enemy_weapon_color
            : config.esp_team_weapon_color;
        const ImU32 textCol = WeaponCol(colArr);
        const ImU32 shadowCol = IM_COL32(0, 0, 0, 200);

        constexpr float kBottomOff = 22.0f;

        for (const auto& ent : frame.entities) {
            const bool enemy = (ent.team != frame.localTeam);
            if (enemy != isEnemy) continue;
            if (!ent.isAlive || ent.isDormant) continue;
            if (ent.weaponName[0] == '\0') continue;

            BoxRect box = ComputeBox(ent, viewMatrix, width, height);
            if (!box.valid) continue;

            const float fontSize = (std::clamp)(box.height() * 0.11f, 10.0f, 18.0f);

            const ImVec2 sz = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ent.weaponName);
            const float x = box.centerX - sz.x * 0.5f;
            const float y = box.bottom + kBottomOff;

            dl->AddText(font, fontSize, ImVec2(x + 1.0f, y + 1.0f), shadowCol, ent.weaponName);
            dl->AddText(font, fontSize, ImVec2(x, y), textCol, ent.weaponName);
        }
    }

    void RenderWeaponESP(const FrameSnapshot& frame,
        const ViewMatrix& viewMatrix,
        ID3D11Device*, ID3D11DeviceContext*, int width, int height)
    {
        if (config.esp_enemy_enable && config.esp_enemy_weapon)
            RenderWeaponSide(frame, viewMatrix, width, height, true);
        if (config.esp_team_enable && config.esp_team_weapon)
            RenderWeaponSide(frame, viewMatrix, width, height, false);
    }
}