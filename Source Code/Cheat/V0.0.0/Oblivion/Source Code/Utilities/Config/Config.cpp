#include "Config.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <Windows.h>
#include <cstdio>
#include <cstring>

namespace fs = std::filesystem;

Config config;
ConfigBrowser configBrowser;

static constexpr uint32_t kSkelMask = 0x7F;

static bool s_labelsInit = false;

static void InitLabels();

const char* GetKeyName(int vk) {
    static char buf[32];
    if (vk <= 0) return "None";
    switch (vk) {
    case VK_LBUTTON:  return "M1";
    case VK_RBUTTON:  return "M2";
    case VK_MBUTTON:  return "M3";
    case VK_XBUTTON1: return "M4";
    case VK_XBUTTON2: return "M5";
    case VK_LSHIFT:   return "LShift";
    case VK_RSHIFT:   return "RShift";
    case VK_LCONTROL: return "LCtrl";
    case VK_RCONTROL: return "RCtrl";
    case VK_LMENU:    return "LAlt";
    case VK_RMENU:    return "RAlt";
    case VK_SPACE:    return "Space";
    case VK_TAB:      return "Tab";
    case VK_RETURN:   return "Enter";
    case VK_ESCAPE:   return "Esc";
    case VK_BACK:     return "Back";
    case VK_INSERT:   return "Ins";
    case VK_DELETE:   return "Del";
    case VK_HOME:     return "Home";
    case VK_END:      return "End";
    case VK_PRIOR:    return "PgUp";
    case VK_NEXT:     return "PgDn";
    case VK_CAPITAL:  return "Caps";
    case VK_NUMLOCK:  return "Num";
    case VK_SCROLL:   return "Scrl";
    }
    if (vk >= 'A' && vk <= 'Z') { buf[0] = (char)vk; buf[1] = 0; return buf; }
    if (vk >= '0' && vk <= '9') { buf[0] = (char)vk; buf[1] = 0; return buf; }
    if (vk >= VK_F1 && vk <= VK_F12) {
        int n = vk - VK_F1 + 1;
        if (n < 10) { buf[0] = 'F'; buf[1] = (char)('0' + n); buf[2] = 0; }
        else { buf[0] = 'F'; buf[1] = '1'; buf[2] = (char)('0' + (n - 10)); buf[3] = 0; }
        return buf;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        buf[0] = 'N'; buf[1] = (char)('0' + (vk - VK_NUMPAD0)); buf[2] = 0; return buf;
    }
    snprintf(buf, sizeof(buf), "VK%02X", vk);
    return buf;
}

static void ParseColor(const std::string& v, float out[4]) {
    std::stringstream ss(v);
    std::string tok;
    int i = 0;
    while (i < 4 && std::getline(ss, tok, ',')) {
        try { out[i++] = std::stof(tok); }
        catch (...) { break; }
    }
}

static void WriteColor(std::ofstream& f, const char* key, const float c[4]) {
    f << key << " = " << c[0] << "," << c[1] << "," << c[2] << "," << c[3] << "\n";
}

static void WriteAimSettings(std::ofstream& f, const AimSettings& a) {
    f << "enable = " << (a.enable ? "true" : "false") << "\n";
    f << "mode = " << a.mode << "\n";
    f << "key = " << a.key << "\n";
    f << "fov = " << a.fov << "\n";
    f << "hitbox = " << a.hitbox << "\n";
    f << "hitbox_priority = " << a.hitbox_priority << "\n";
    f << "smooth = " << a.smooth << "\n";
    f << "visible_check = " << (a.visible_check ? "true" : "false") << "\n";
    f << "team_check = " << (a.team_check ? "true" : "false") << "\n";
    f << "fov_visualize = " << (a.fov_visualize ? "true" : "false") << "\n";
    WriteColor(f, "fov_visualize_color", a.fov_visualize_color);
    f << "humanize_enable = " << (a.humanize_enable ? "true" : "false") << "\n";
    f << "humanize_rand_smooth = " << (a.humanize_rand_smooth ? "true" : "false") << "\n";
    f << "humanize_smooth_min = " << a.humanize_smooth_min << "\n";
    f << "humanize_smooth_max = " << a.humanize_smooth_max << "\n";
    f << "humanize_jitter = " << (a.humanize_jitter ? "true" : "false") << "\n";
    f << "humanize_jitter_amount = " << a.humanize_jitter_amount << "\n";
    f << "humanize_overshoot = " << (a.humanize_overshoot ? "true" : "false") << "\n";
    f << "humanize_overshoot_min = " << a.humanize_overshoot_min << "\n";
    f << "humanize_overshoot_max = " << a.humanize_overshoot_max << "\n";
    f << "humanize_undershoot = " << (a.humanize_undershoot ? "true" : "false") << "\n";
    f << "humanize_undershoot_min = " << a.humanize_undershoot_min << "\n";
    f << "humanize_undershoot_max = " << a.humanize_undershoot_max << "\n";
    f << "humanize_curve = " << (a.humanize_curve ? "true" : "false") << "\n";
    f << "humanize_curve_amount = " << a.humanize_curve_amount << "\n";
    f << "humanize_reaction_enable = " << (a.humanize_reaction_enable ? "true" : "false") << "\n";
    f << "humanize_reaction_min = " << a.humanize_reaction_min << "\n";
    f << "humanize_reaction_max = " << a.humanize_reaction_max << "\n";
}

static void LoadAimSetting(const std::string& key, const std::string& value, AimSettings& a) {
    bool b = (value == "true");
    if (key == "enable") a.enable = b;
    else if (key == "mode") { try { a.mode = std::stoi(value); } catch (...) { a.mode = 0; } if (a.mode < 0 || a.mode > 2) a.mode = 0; }
    else if (key == "key") { try { a.key = std::stoi(value); } catch (...) { a.key = 0; } if (a.key < 0 || a.key > 255) a.key = 0; }
    else if (key == "fov") { try { a.fov = std::stof(value); } catch (...) { a.fov = 180.0f; } if (a.fov < 0.0f) a.fov = 0.0f; if (a.fov > 180.0f) a.fov = 180.0f; }
    else if (key == "hitbox") { try { a.hitbox = std::stoi(value); } catch (...) { a.hitbox = 1; } if (a.hitbox < 0) a.hitbox = 0; if (a.hitbox > 127) a.hitbox = 127; }
    else if (key == "hitbox_priority") { try { a.hitbox_priority = std::stoi(value); } catch (...) { a.hitbox_priority = 0; } if (a.hitbox_priority < 0 || a.hitbox_priority > 2) a.hitbox_priority = 0; }
    else if (key == "smooth") { try { a.smooth = std::stof(value); } catch (...) { a.smooth = 50.0f; } if (a.smooth < 0.0f) a.smooth = 0.0f; if (a.smooth > 100.0f) a.smooth = 100.0f; }
    else if (key == "visible_check") a.visible_check = b;
    else if (key == "team_check") a.team_check = b;
    else if (key == "fov_visualize") a.fov_visualize = b;
    else if (key == "fov_visualize_color") ParseColor(value, a.fov_visualize_color);
    else if (key == "humanize_enable") a.humanize_enable = b;
    else if (key == "humanize_rand_smooth") a.humanize_rand_smooth = b;
    else if (key == "humanize_smooth_min") { try { a.humanize_smooth_min = std::stof(value); } catch (...) {} if (a.humanize_smooth_min < 0.0f) a.humanize_smooth_min = 0.0f; if (a.humanize_smooth_min > 100.0f) a.humanize_smooth_min = 100.0f; }
    else if (key == "humanize_smooth_max") { try { a.humanize_smooth_max = std::stof(value); } catch (...) {} if (a.humanize_smooth_max < 0.0f) a.humanize_smooth_max = 0.0f; if (a.humanize_smooth_max > 100.0f) a.humanize_smooth_max = 100.0f; }
    else if (key == "humanize_jitter") a.humanize_jitter = b;
    else if (key == "humanize_jitter_amount") { try { a.humanize_jitter_amount = std::stof(value); } catch (...) {} if (a.humanize_jitter_amount < 0.0f) a.humanize_jitter_amount = 0.0f; if (a.humanize_jitter_amount > 20.0f) a.humanize_jitter_amount = 20.0f; }
    else if (key == "humanize_overshoot") a.humanize_overshoot = b;
    else if (key == "humanize_overshoot_min") { try { a.humanize_overshoot_min = std::stof(value); } catch (...) {} if (a.humanize_overshoot_min < 0.0f) a.humanize_overshoot_min = 0.0f; if (a.humanize_overshoot_min > 50.0f) a.humanize_overshoot_min = 50.0f; }
    else if (key == "humanize_overshoot_max") { try { a.humanize_overshoot_max = std::stof(value); } catch (...) {} if (a.humanize_overshoot_max < 0.0f) a.humanize_overshoot_max = 0.0f; if (a.humanize_overshoot_max > 50.0f) a.humanize_overshoot_max = 50.0f; }
    else if (key == "humanize_undershoot") a.humanize_undershoot = b;
    else if (key == "humanize_undershoot_min") { try { a.humanize_undershoot_min = std::stof(value); } catch (...) {} if (a.humanize_undershoot_min < 0.0f) a.humanize_undershoot_min = 0.0f; if (a.humanize_undershoot_min > 50.0f) a.humanize_undershoot_min = 50.0f; }
    else if (key == "humanize_undershoot_max") { try { a.humanize_undershoot_max = std::stof(value); } catch (...) {} if (a.humanize_undershoot_max < 0.0f) a.humanize_undershoot_max = 0.0f; if (a.humanize_undershoot_max > 50.0f) a.humanize_undershoot_max = 50.0f; }
    else if (key == "humanize_curve") a.humanize_curve = b;
    else if (key == "humanize_curve_amount") { try { a.humanize_curve_amount = std::stof(value); } catch (...) {} if (a.humanize_curve_amount < 0.0f) a.humanize_curve_amount = 0.0f; if (a.humanize_curve_amount > 1.0f) a.humanize_curve_amount = 1.0f; }
    else if (key == "humanize_reaction_enable") a.humanize_reaction_enable = b;
    else if (key == "humanize_reaction_min") { try { a.humanize_reaction_min = std::stof(value); } catch (...) {} if (a.humanize_reaction_min < 0.0f) a.humanize_reaction_min = 0.0f; if (a.humanize_reaction_min > 1000.0f) a.humanize_reaction_min = 1000.0f; }
    else if (key == "humanize_reaction_max") { try { a.humanize_reaction_max = std::stof(value); } catch (...) {} if (a.humanize_reaction_max < 0.0f) a.humanize_reaction_max = 0.0f; if (a.humanize_reaction_max > 1000.0f) a.humanize_reaction_max = 1000.0f; }
}

static void WriteBind(std::ofstream& f, const char* key, const Bind& b) {
    f << key << " = " << (b.enabled ? 1 : 0) << "," << b.key << ","
        << b.mode << "," << b.slot << "\n";
}

static void WriteSliderBinds(std::ofstream& f, const char* key, const SliderBinds& s) {
    f << key << " = ";
    for (int i = 0; i < 3; ++i) {
        if (i) f << "|";
        const ValueBind& b = s.slots[i];
        f << (b.enabled ? 1 : 0) << "," << b.key << "," << b.mode << ","
            << b.slot << "," << b.value;
    }
    f << "\n";
}

static void WriteAimBinds(std::ofstream& f, const AimSettings& a) {
    WriteBind(f, "bind_enable", a.bind_enable);
    WriteBind(f, "bind_visible_check", a.bind_visible_check);
    WriteBind(f, "bind_team_check", a.bind_team_check);
    WriteBind(f, "bind_fov_visualize", a.bind_fov_visualize);
    WriteBind(f, "bind_humanize_enable", a.bind_humanize_enable);
    WriteBind(f, "bind_humanize_rand_smooth", a.bind_humanize_rand_smooth);
    WriteBind(f, "bind_humanize_jitter", a.bind_humanize_jitter);
    WriteBind(f, "bind_humanize_overshoot", a.bind_humanize_overshoot);
    WriteBind(f, "bind_humanize_undershoot", a.bind_humanize_undershoot);
    WriteBind(f, "bind_humanize_curve", a.bind_humanize_curve);
    WriteBind(f, "bind_humanize_reaction_enable", a.bind_humanize_reaction_enable);

    WriteSliderBinds(f, "bind_fov", a.bind_fov);
    WriteSliderBinds(f, "bind_smooth", a.bind_smooth);
    WriteSliderBinds(f, "bind_hum_smooth_min", a.bind_hum_smooth_min);
    WriteSliderBinds(f, "bind_hum_smooth_max", a.bind_hum_smooth_max);
    WriteSliderBinds(f, "bind_hum_jitter_amount", a.bind_hum_jitter_amount);
    WriteSliderBinds(f, "bind_hum_overshoot_min", a.bind_hum_overshoot_min);
    WriteSliderBinds(f, "bind_hum_overshoot_max", a.bind_hum_overshoot_max);
    WriteSliderBinds(f, "bind_hum_undershoot_min", a.bind_hum_undershoot_min);
    WriteSliderBinds(f, "bind_hum_undershoot_max", a.bind_hum_undershoot_max);
    WriteSliderBinds(f, "bind_hum_curve_amount", a.bind_hum_curve_amount);
    WriteSliderBinds(f, "bind_hum_reaction_min", a.bind_hum_reaction_min);
    WriteSliderBinds(f, "bind_hum_reaction_max", a.bind_hum_reaction_max);
}

static void ParseBind(const std::string& v, Bind& out) {
    std::stringstream ss(v);
    std::string tok;
    int idx = 0;
    while (std::getline(ss, tok, ',')) {
        try {
            int n = std::stoi(tok);
            if (idx == 0) out.enabled = (n != 0);
            else if (idx == 1) { if (n < 0 || n > 255) n = 0; out.key = n; }
            else if (idx == 2) { if (n < 0 || n > 1) n = 0; out.mode = n; }
            else if (idx == 3) { if (n < 1 || n > 9) n = 1; out.slot = n; }
        }
        catch (...) {}
        ++idx;
    }
}

static void ParseSliderBinds(const std::string& v, SliderBinds& out) {
    std::stringstream ss(v);
    std::string chunk;
    int slotIdx = 0;
    while (slotIdx < 3 && std::getline(ss, chunk, '|')) {
        ValueBind& b = out.slots[slotIdx];
        std::stringstream cs(chunk);
        std::string tok;
        int idx = 0;
        while (std::getline(cs, tok, ',')) {
            try {
                if (idx == 0) b.enabled = (std::stoi(tok) != 0);
                else if (idx == 1) { int n = std::stoi(tok); if (n < 0 || n > 255) n = 0; b.key = n; }
                else if (idx == 2) { int n = std::stoi(tok); if (n < 0 || n > 1) n = 0; b.mode = n; }
                else if (idx == 3) { int n = std::stoi(tok); if (n < 1 || n > 9) n = 1; b.slot = n; }
                else if (idx == 4) b.value = std::stof(tok);
            }
            catch (...) {}
            ++idx;
        }
        ++slotIdx;
    }
}

static void LoadAimBind(const std::string& key, const std::string& value, AimSettings& a, bool& consumed) {
    consumed = true;
    if (key == "bind_enable") ParseBind(value, a.bind_enable);
    else if (key == "bind_visible_check") ParseBind(value, a.bind_visible_check);
    else if (key == "bind_team_check") ParseBind(value, a.bind_team_check);
    else if (key == "bind_fov_visualize") ParseBind(value, a.bind_fov_visualize);
    else if (key == "bind_humanize_enable") ParseBind(value, a.bind_humanize_enable);
    else if (key == "bind_humanize_rand_smooth") ParseBind(value, a.bind_humanize_rand_smooth);
    else if (key == "bind_humanize_jitter") ParseBind(value, a.bind_humanize_jitter);
    else if (key == "bind_humanize_overshoot") ParseBind(value, a.bind_humanize_overshoot);
    else if (key == "bind_humanize_undershoot") ParseBind(value, a.bind_humanize_undershoot);
    else if (key == "bind_humanize_curve") ParseBind(value, a.bind_humanize_curve);
    else if (key == "bind_humanize_reaction_enable") ParseBind(value, a.bind_humanize_reaction_enable);
    else if (key == "bind_fov") ParseSliderBinds(value, a.bind_fov);
    else if (key == "bind_smooth") ParseSliderBinds(value, a.bind_smooth);
    else if (key == "bind_hum_smooth_min") ParseSliderBinds(value, a.bind_hum_smooth_min);
    else if (key == "bind_hum_smooth_max") ParseSliderBinds(value, a.bind_hum_smooth_max);
    else if (key == "bind_hum_jitter_amount") ParseSliderBinds(value, a.bind_hum_jitter_amount);
    else if (key == "bind_hum_overshoot_min") ParseSliderBinds(value, a.bind_hum_overshoot_min);
    else if (key == "bind_hum_overshoot_max") ParseSliderBinds(value, a.bind_hum_overshoot_max);
    else if (key == "bind_hum_undershoot_min") ParseSliderBinds(value, a.bind_hum_undershoot_min);
    else if (key == "bind_hum_undershoot_max") ParseSliderBinds(value, a.bind_hum_undershoot_max);
    else if (key == "bind_hum_curve_amount") ParseSliderBinds(value, a.bind_hum_curve_amount);
    else if (key == "bind_hum_reaction_min") ParseSliderBinds(value, a.bind_hum_reaction_min);
    else if (key == "bind_hum_reaction_max") ParseSliderBinds(value, a.bind_hum_reaction_max);
    else consumed = false;
}

static void WriteFlagsBlock(std::ofstream& f, const char* prefix,
    bool master, const float masterCol[4],
    bool flashed, const float flashedCol[4],
    bool scoped, const float scopedCol[4],
    bool defusing, const float defusingCol[4],
    bool planting, const float plantingCol[4],
    bool bomb, const float bombCol[4],
    bool armor, const float armorCol[4],
    bool reloading, const float reloadingCol[4],
    bool money, const float moneyCol[4],
    bool ping, const float pingCol[4],
    bool ammo, const float ammoCol[4])
{
    f << prefix << " = " << (master ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_color").c_str(), masterCol);
    f << prefix << "_flashed = " << (flashed ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_flashed_color").c_str(), flashedCol);
    f << prefix << "_scoped = " << (scoped ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_scoped_color").c_str(), scopedCol);
    f << prefix << "_defusing = " << (defusing ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_defusing_color").c_str(), defusingCol);
    f << prefix << "_planting = " << (planting ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_planting_color").c_str(), plantingCol);
    f << prefix << "_bomb = " << (bomb ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_bomb_color").c_str(), bombCol);
    f << prefix << "_armor = " << (armor ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_armor_color").c_str(), armorCol);
    f << prefix << "_reloading = " << (reloading ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_reloading_color").c_str(), reloadingCol);
    f << prefix << "_money = " << (money ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_money_color").c_str(), moneyCol);
    f << prefix << "_ping = " << (ping ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_ping_color").c_str(), pingCol);
    f << prefix << "_ammo = " << (ammo ? "true" : "false") << "\n";
    WriteColor(f, (std::string(prefix) + "_ammo_color").c_str(), ammoCol);
}

static void LoadFlagsBlock(const std::string& key, const std::string& value,
    bool& master, float masterCol[4],
    bool& flashed, float flashedCol[4],
    bool& scoped, float scopedCol[4],
    bool& defusing, float defusingCol[4],
    bool& planting, float plantingCol[4],
    bool& bomb, float bombCol[4],
    bool& armor, float armorCol[4],
    bool& reloading, float reloadingCol[4],
    bool& money, float moneyCol[4],
    bool& ping, float pingCol[4],
    bool& ammo, float ammoCol[4],
    bool& consumed)
{
    consumed = true;
    if (key == "flags") master = (value == "true");
    else if (key == "flags_color") ParseColor(value, masterCol);
    else if (key == "flags_flashed") flashed = (value == "true");
    else if (key == "flags_flashed_color") ParseColor(value, flashedCol);
    else if (key == "flags_scoped") scoped = (value == "true");
    else if (key == "flags_scoped_color") ParseColor(value, scopedCol);
    else if (key == "flags_defusing") defusing = (value == "true");
    else if (key == "flags_defusing_color") ParseColor(value, defusingCol);
    else if (key == "flags_planting") planting = (value == "true");
    else if (key == "flags_planting_color") ParseColor(value, plantingCol);
    else if (key == "flags_bomb") bomb = (value == "true");
    else if (key == "flags_bomb_color") ParseColor(value, bombCol);
    else if (key == "flags_armor") armor = (value == "true");
    else if (key == "flags_armor_color") ParseColor(value, armorCol);
    else if (key == "flags_reloading") reloading = (value == "true");
    else if (key == "flags_reloading_color") ParseColor(value, reloadingCol);
    else if (key == "flags_money") money = (value == "true");
    else if (key == "flags_money_color") ParseColor(value, moneyCol);
    else if (key == "flags_ping") ping = (value == "true");
    else if (key == "flags_ping_color") ParseColor(value, pingCol);
    else if (key == "flags_ammo") ammo = (value == "true");
    else if (key == "flags_ammo_color") ParseColor(value, ammoCol);
    else consumed = false;
}

static const char* s_espLabels[Config::kBindEspItemCount] = {
    "Enable ESP", "Box", "Corner", "Rounded", "Fill", "Outline", "Skeleton",
    "Head", "Torso", "Left Arm", "Right Arm", "Left Leg", "Right Leg", "Feet",
    "Draw Joints", "Name", "Healthbar", "Distance", "Headcircle", "Snapline",
    "Viewdirection", "Weapon", "Flags", "Flashed", "Scoped", "Defusing",
    "Planting", "Bomb", "Armor", "Reloading", "Money", "Ammo", "Offscreen",
    "Distance", "Size"
};

static void InitLabels() {
    static std::string s_espCombined[2][Config::kBindEspItemCount];
    for (int side = 0; side < 2; ++side) {
        const char* sideLabel = (side == 0) ? "Enemy" : "Team";
        for (int i = 0; i < Config::kBindEspItemCount; ++i) {
            s_espCombined[side][i] = std::string(sideLabel) + " " + s_espLabels[i];
            config.bind_esp[side][i].label = s_espCombined[side][i];
        }
    }

    static std::string s_distLbl[2], s_sizeLbl[2];
    for (int side = 0; side < 2; ++side) {
        const char* sideLabel = (side == 0) ? "Enemy" : "Team";
        s_distLbl[side] = std::string(sideLabel) + " Offscreen Distance";
        s_sizeLbl[side] = std::string(sideLabel) + " Offscreen Size";
        for (int s = 0; s < 3; ++s) {
            config.bind_esp_offscreen_dist[side].slots[s].label = s_distLbl[side];
            config.bind_esp_offscreen_size[side].slots[s].label = s_sizeLbl[side];
        }
    }

    AimSettings* aims[64];
    int nAim = 0;
    aims[nAim++] = &config.aim_shared;
    for (auto& g : config.aim_groups) if (nAim < 64) aims[nAim++] = &g.settings;

    for (int i = 0; i < nAim; ++i) {
        AimSettings& a = *aims[i];
        a.bind_enable.label = "Aimbot Enable";
        a.bind_visible_check.label = "Visible Check";
        a.bind_team_check.label = "Team Check";
        a.bind_fov_visualize.label = "Show FOV";
        a.bind_humanize_enable.label = "Humanize Enable";
        a.bind_humanize_rand_smooth.label = "Randomize Smooth";
        a.bind_humanize_jitter.label = "Microjitter";
        a.bind_humanize_overshoot.label = "Overshoot";
        a.bind_humanize_undershoot.label = "Undershoot";
        a.bind_humanize_curve.label = "Aim Curve";
        a.bind_humanize_reaction_enable.label = "Reaction Delay";

        a.bind_fov.slots[0].label = "Aim FOV";
        a.bind_smooth.slots[0].label = "Aim Smooth";
        a.bind_hum_smooth_min.slots[0].label = "Smooth Min";
        a.bind_hum_smooth_max.slots[0].label = "Smooth Max";
        a.bind_hum_jitter_amount.slots[0].label = "Jitter Amount";
        a.bind_hum_overshoot_min.slots[0].label = "Overshoot Min";
        a.bind_hum_overshoot_max.slots[0].label = "Overshoot Max";
        a.bind_hum_undershoot_min.slots[0].label = "Undershoot Min";
        a.bind_hum_undershoot_max.slots[0].label = "Undershoot Max";
        a.bind_hum_curve_amount.slots[0].label = "Curve Amount";
        a.bind_hum_reaction_min.slots[0].label = "Reaction Min";
        a.bind_hum_reaction_max.slots[0].label = "Reaction Max";

        for (int s = 1; s < 3; ++s) {
            a.bind_fov.slots[s].label = a.bind_fov.slots[0].label;
            a.bind_smooth.slots[s].label = a.bind_smooth.slots[0].label;
            a.bind_hum_smooth_min.slots[s].label = a.bind_hum_smooth_min.slots[0].label;
            a.bind_hum_smooth_max.slots[s].label = a.bind_hum_smooth_max.slots[0].label;
            a.bind_hum_jitter_amount.slots[s].label = a.bind_hum_jitter_amount.slots[0].label;
            a.bind_hum_overshoot_min.slots[s].label = a.bind_hum_overshoot_min.slots[0].label;
            a.bind_hum_overshoot_max.slots[s].label = a.bind_hum_overshoot_max.slots[0].label;
            a.bind_hum_undershoot_min.slots[s].label = a.bind_hum_undershoot_min.slots[0].label;
            a.bind_hum_undershoot_max.slots[s].label = a.bind_hum_undershoot_max.slots[0].label;
            a.bind_hum_curve_amount.slots[s].label = a.bind_hum_curve_amount.slots[0].label;
            a.bind_hum_reaction_min.slots[s].label = a.bind_hum_reaction_min.slots[0].label;
            a.bind_hum_reaction_max.slots[s].label = a.bind_hum_reaction_max.slots[0].label;
        }
    }

    config.bind_vsync.label = "VSync";
    config.bind_allow_tearing.label = "Allow Tearing";
    config.bind_fullscreen_mode.label = "Fullscreen Mode";
    config.bind_streamproof.label = "Streamproof";
    config.bind_safe_mode.label = "Safemode";
}

void Config::SetDefaults() {
    show_menu = false;

    aim_shared = AimSettings{};
    aim_groups.clear();
    aim_current_edit = 0;

    esp_enemy_enable = false;
    esp_enemy_hotkey = 0;
    esp_enemy_hotkey_mode = 0;
    esp_enemy_active = false;
    esp_enemy_box = false;
    esp_enemy_box_color[0] = 1.0f; esp_enemy_box_color[1] = 0.2f; esp_enemy_box_color[2] = 0.2f; esp_enemy_box_color[3] = 1.0f;
    esp_enemy_box_fill_color[0] = 1.0f; esp_enemy_box_fill_color[1] = 0.2f; esp_enemy_box_fill_color[2] = 0.2f; esp_enemy_box_fill_color[3] = 0.15f;
    esp_enemy_box_style_corner = false;
    esp_enemy_box_style_rounded = false;
    esp_enemy_box_style_fill = false;
    esp_enemy_box_outline = false;
    esp_enemy_box_outline_color[0] = 0.0f; esp_enemy_box_outline_color[1] = 0.0f; esp_enemy_box_outline_color[2] = 0.0f; esp_enemy_box_outline_color[3] = 0.85f;
    esp_enemy_skeleton = false;
    esp_enemy_skeleton_color[0] = 1.0f; esp_enemy_skeleton_color[1] = 1.0f; esp_enemy_skeleton_color[2] = 1.0f; esp_enemy_skeleton_color[3] = 1.0f;
    esp_enemy_skeleton_joints = kSkelMask;
    esp_enemy_skeleton_draw_joints = false;
    esp_enemy_name = false;
    esp_enemy_name_color[0] = 1.0f; esp_enemy_name_color[1] = 1.0f; esp_enemy_name_color[2] = 1.0f; esp_enemy_name_color[3] = 1.0f;
    esp_enemy_healthbar = false;
    esp_enemy_healthbar_color[0] = 0.3f; esp_enemy_healthbar_color[1] = 1.0f; esp_enemy_healthbar_color[2] = 0.3f; esp_enemy_healthbar_color[3] = 1.0f;
    esp_enemy_distance = false;
    esp_enemy_distance_color[0] = 1.0f; esp_enemy_distance_color[1] = 1.0f; esp_enemy_distance_color[2] = 1.0f; esp_enemy_distance_color[3] = 1.0f;
    esp_enemy_headcircle = false;
    esp_enemy_headcircle_color[0] = 1.0f; esp_enemy_headcircle_color[1] = 0.2f; esp_enemy_headcircle_color[2] = 0.2f; esp_enemy_headcircle_color[3] = 1.0f;
    esp_enemy_snapline = false;
    esp_enemy_snapline_color[0] = 1.0f; esp_enemy_snapline_color[1] = 1.0f; esp_enemy_snapline_color[2] = 1.0f; esp_enemy_snapline_color[3] = 0.5f;
    esp_enemy_viewdirection = false;
    esp_enemy_viewdirection_color[0] = 1.0f; esp_enemy_viewdirection_color[1] = 1.0f; esp_enemy_viewdirection_color[2] = 1.0f; esp_enemy_viewdirection_color[3] = 1.0f;
    esp_enemy_weapon = false;
    esp_enemy_weapon_color[0] = 1.0f; esp_enemy_weapon_color[1] = 1.0f; esp_enemy_weapon_color[2] = 1.0f; esp_enemy_weapon_color[3] = 1.0f;

    esp_enemy_flags = false;
    esp_enemy_flags_color[0] = 1.0f; esp_enemy_flags_color[1] = 1.0f; esp_enemy_flags_color[2] = 1.0f; esp_enemy_flags_color[3] = 1.0f;
    esp_enemy_flags_flashed = false;
    esp_enemy_flags_flashed_color[0] = 1.0f; esp_enemy_flags_flashed_color[1] = 1.0f; esp_enemy_flags_flashed_color[2] = 0.4f; esp_enemy_flags_flashed_color[3] = 1.0f;
    esp_enemy_flags_scoped = false;
    esp_enemy_flags_scoped_color[0] = 0.4f; esp_enemy_flags_scoped_color[1] = 0.8f; esp_enemy_flags_scoped_color[2] = 1.0f; esp_enemy_flags_scoped_color[3] = 1.0f;
    esp_enemy_flags_defusing = false;
    esp_enemy_flags_defusing_color[0] = 1.0f; esp_enemy_flags_defusing_color[1] = 0.6f; esp_enemy_flags_defusing_color[2] = 0.2f; esp_enemy_flags_defusing_color[3] = 1.0f;
    esp_enemy_flags_planting = false;
    esp_enemy_flags_planting_color[0] = 1.0f; esp_enemy_flags_planting_color[1] = 0.3f; esp_enemy_flags_planting_color[2] = 0.3f; esp_enemy_flags_planting_color[3] = 1.0f;
    esp_enemy_flags_bomb = false;
    esp_enemy_flags_bomb_color[0] = 0.8f; esp_enemy_flags_bomb_color[1] = 0.4f; esp_enemy_flags_bomb_color[2] = 1.0f; esp_enemy_flags_bomb_color[3] = 1.0f;
    esp_enemy_flags_armor = false;
    esp_enemy_flags_armor_color[0] = 0.4f; esp_enemy_flags_armor_color[1] = 0.6f; esp_enemy_flags_armor_color[2] = 1.0f; esp_enemy_flags_armor_color[3] = 1.0f;
    esp_enemy_flags_reloading = false;
    esp_enemy_flags_reloading_color[0] = 1.0f; esp_enemy_flags_reloading_color[1] = 0.85f; esp_enemy_flags_reloading_color[2] = 0.4f; esp_enemy_flags_reloading_color[3] = 1.0f;
    esp_enemy_flags_money = false;
    esp_enemy_flags_money_color[0] = 0.4f; esp_enemy_flags_money_color[1] = 1.0f; esp_enemy_flags_money_color[2] = 0.4f; esp_enemy_flags_money_color[3] = 1.0f;
    esp_enemy_flags_ping = false;
    esp_enemy_flags_ping_color[0] = 1.0f; esp_enemy_flags_ping_color[1] = 1.0f; esp_enemy_flags_ping_color[2] = 1.0f; esp_enemy_flags_ping_color[3] = 1.0f;
    esp_enemy_flags_ammo = false;
    esp_enemy_flags_ammo_color[0] = 1.0f; esp_enemy_flags_ammo_color[1] = 1.0f; esp_enemy_flags_ammo_color[2] = 1.0f; esp_enemy_flags_ammo_color[3] = 1.0f;

    esp_enemy_offscreen = false;
    esp_enemy_offscreen_color[0] = 1.0f; esp_enemy_offscreen_color[1] = 0.3f; esp_enemy_offscreen_color[2] = 0.3f; esp_enemy_offscreen_color[3] = 1.0f;
    esp_enemy_offscreen_distance = 0.35f;
    esp_enemy_offscreen_size = 22.0f;

    esp_team_enable = false;
    esp_team_hotkey = 0;
    esp_team_hotkey_mode = 0;
    esp_team_active = false;
    esp_team_box = false;
    esp_team_box_color[0] = 0.2f; esp_team_box_color[1] = 1.0f; esp_team_box_color[2] = 0.2f; esp_team_box_color[3] = 1.0f;
    esp_team_box_fill_color[0] = 0.2f; esp_team_box_fill_color[1] = 1.0f; esp_team_box_fill_color[2] = 0.2f; esp_team_box_fill_color[3] = 0.15f;
    esp_team_box_style_corner = false;
    esp_team_box_style_rounded = false;
    esp_team_box_style_fill = false;
    esp_team_box_outline = false;
    esp_team_box_outline_color[0] = 0.0f; esp_team_box_outline_color[1] = 0.0f; esp_team_box_outline_color[2] = 0.0f; esp_team_box_outline_color[3] = 0.85f;
    esp_team_skeleton = false;
    esp_team_skeleton_color[0] = 1.0f; esp_team_skeleton_color[1] = 1.0f; esp_team_skeleton_color[2] = 1.0f; esp_team_skeleton_color[3] = 1.0f;
    esp_team_skeleton_joints = kSkelMask;
    esp_team_skeleton_draw_joints = false;
    esp_team_name = false;
    esp_team_name_color[0] = 1.0f; esp_team_name_color[1] = 1.0f; esp_team_name_color[2] = 1.0f; esp_team_name_color[3] = 1.0f;
    esp_team_healthbar = false;
    esp_team_healthbar_color[0] = 0.3f; esp_team_healthbar_color[1] = 1.0f; esp_team_healthbar_color[2] = 0.3f; esp_team_healthbar_color[3] = 1.0f;
    esp_team_distance = false;
    esp_team_distance_color[0] = 1.0f; esp_team_distance_color[1] = 1.0f; esp_team_distance_color[2] = 1.0f; esp_team_distance_color[3] = 1.0f;
    esp_team_headcircle = false;
    esp_team_headcircle_color[0] = 0.2f; esp_team_headcircle_color[1] = 1.0f; esp_team_headcircle_color[2] = 0.2f; esp_team_headcircle_color[3] = 1.0f;
    esp_team_snapline = false;
    esp_team_snapline_color[0] = 1.0f; esp_team_snapline_color[1] = 1.0f; esp_team_snapline_color[2] = 1.0f; esp_team_snapline_color[3] = 0.5f;
    esp_team_viewdirection = false;
    esp_team_viewdirection_color[0] = 1.0f; esp_team_viewdirection_color[1] = 1.0f; esp_team_viewdirection_color[2] = 1.0f; esp_team_viewdirection_color[3] = 1.0f;
    esp_team_weapon = false;
    esp_team_weapon_color[0] = 1.0f; esp_team_weapon_color[1] = 1.0f; esp_team_weapon_color[2] = 1.0f; esp_team_weapon_color[3] = 1.0f;

    esp_team_flags = false;
    esp_team_flags_color[0] = 1.0f; esp_team_flags_color[1] = 1.0f; esp_team_flags_color[2] = 1.0f; esp_team_flags_color[3] = 1.0f;
    esp_team_flags_flashed = false;
    esp_team_flags_flashed_color[0] = 1.0f; esp_team_flags_flashed_color[1] = 1.0f; esp_team_flags_flashed_color[2] = 0.4f; esp_team_flags_flashed_color[3] = 1.0f;
    esp_team_flags_scoped = false;
    esp_team_flags_scoped_color[0] = 0.4f; esp_team_flags_scoped_color[1] = 0.8f; esp_team_flags_scoped_color[2] = 1.0f; esp_team_flags_scoped_color[3] = 1.0f;
    esp_team_flags_defusing = false;
    esp_team_flags_defusing_color[0] = 1.0f; esp_team_flags_defusing_color[1] = 0.6f; esp_team_flags_defusing_color[2] = 0.2f; esp_team_flags_defusing_color[3] = 1.0f;
    esp_team_flags_planting = false;
    esp_team_flags_planting_color[0] = 1.0f; esp_team_flags_planting_color[1] = 0.3f; esp_team_flags_planting_color[2] = 0.3f; esp_team_flags_planting_color[3] = 1.0f;
    esp_team_flags_bomb = false;
    esp_team_flags_bomb_color[0] = 0.8f; esp_team_flags_bomb_color[1] = 0.4f; esp_team_flags_bomb_color[2] = 1.0f; esp_team_flags_bomb_color[3] = 1.0f;
    esp_team_flags_armor = false;
    esp_team_flags_armor_color[0] = 0.4f; esp_team_flags_armor_color[1] = 0.6f; esp_team_flags_armor_color[2] = 1.0f; esp_team_flags_armor_color[3] = 1.0f;
    esp_team_flags_reloading = false;
    esp_team_flags_reloading_color[0] = 1.0f; esp_team_flags_reloading_color[1] = 0.85f; esp_team_flags_reloading_color[2] = 0.4f; esp_team_flags_reloading_color[3] = 1.0f;
    esp_team_flags_money = false;
    esp_team_flags_money_color[0] = 0.4f; esp_team_flags_money_color[1] = 1.0f; esp_team_flags_money_color[2] = 0.4f; esp_team_flags_money_color[3] = 1.0f;
    esp_team_flags_ping = false;
    esp_team_flags_ping_color[0] = 1.0f; esp_team_flags_ping_color[1] = 1.0f; esp_team_flags_ping_color[2] = 1.0f; esp_team_flags_ping_color[3] = 1.0f;
    esp_team_flags_ammo = false;
    esp_team_flags_ammo_color[0] = 1.0f; esp_team_flags_ammo_color[1] = 1.0f; esp_team_flags_ammo_color[2] = 1.0f; esp_team_flags_ammo_color[3] = 1.0f;

    esp_team_offscreen = false;
    esp_team_offscreen_color[0] = 0.3f; esp_team_offscreen_color[1] = 1.0f; esp_team_offscreen_color[2] = 0.3f; esp_team_offscreen_color[3] = 1.0f;
    esp_team_offscreen_distance = 0.35f;
    esp_team_offscreen_size = 22.0f;

    vsync = false;
    allow_tearing = false;
    fullscreen_mode = false;

    safe_mode = true;
    safe_mode_no_warn = false;
    streamproof = false;

    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < kBindEspItemCount; ++i) {
            bind_esp[side][i] = Bind{};
            if (!s_espLabels[i]) continue;
            bind_esp[side][i].label = std::string((side == 0 ? "Enemy " : "Team ")) + s_espLabels[i];
        }
        for (int s = 0; s < 3; ++s) {
            bind_esp_offscreen_dist[side].slots[s] = ValueBind{};
            bind_esp_offscreen_size[side].slots[s] = ValueBind{};
        }
    }

    bind_vsync = Bind{};
    bind_allow_tearing = Bind{};
    bind_fullscreen_mode = Bind{};
    bind_streamproof = Bind{};
    bind_safe_mode = Bind{};
}

static void ApplyBoolBind(Bind& b, bool* target) {
    if (!b.enabled || b.key == 0) { b.prev = false; return; }
    const bool down = (GetAsyncKeyState(b.key) & 0x8000) != 0;
    if (b.mode == 0) {
        if (down && !b.prev) *target = !(*target);
        b.state = *target;
    }
    else {
        if (down) *target = true;
        else if (b.prev) *target = false;
        b.state = down;
    }
    b.prev = down;
}

static void ApplyValueBind(ValueBind& b, float* target) {
    if (!b.enabled || b.key == 0) { b.prev = false; return; }
    const bool down = (GetAsyncKeyState(b.key) & 0x8000) != 0;
    if (b.mode == 0) {
        if (down && !b.prev) {
            *target = b.value;
            b.activatedAt = GetTickCount64();
        }
    }
    else {
        if (down && !b.prev) { b.saved = *target; *target = b.value; b.activatedAt = GetTickCount64(); }
        else if (!down && b.prev) *target = b.saved;
    }
    b.prev = down;
}

static void ApplySliderBinds(SliderBinds& s, float* target) {
    for (int i = 0; i < 3; ++i) ApplyValueBind(s.slots[i], target);
}

static void ApplyAimBinds(AimSettings& a) {
    ApplyBoolBind(a.bind_enable, &a.enable);
    ApplyBoolBind(a.bind_visible_check, &a.visible_check);
    ApplyBoolBind(a.bind_team_check, &a.team_check);
    ApplyBoolBind(a.bind_fov_visualize, &a.fov_visualize);
    ApplyBoolBind(a.bind_humanize_enable, &a.humanize_enable);
    ApplyBoolBind(a.bind_humanize_rand_smooth, &a.humanize_rand_smooth);
    ApplyBoolBind(a.bind_humanize_jitter, &a.humanize_jitter);
    ApplyBoolBind(a.bind_humanize_overshoot, &a.humanize_overshoot);
    ApplyBoolBind(a.bind_humanize_undershoot, &a.humanize_undershoot);
    ApplyBoolBind(a.bind_humanize_curve, &a.humanize_curve);
    ApplyBoolBind(a.bind_humanize_reaction_enable, &a.humanize_reaction_enable);

    ApplySliderBinds(a.bind_fov, &a.fov);
    ApplySliderBinds(a.bind_smooth, &a.smooth);
    ApplySliderBinds(a.bind_hum_smooth_min, &a.humanize_smooth_min);
    ApplySliderBinds(a.bind_hum_smooth_max, &a.humanize_smooth_max);
    ApplySliderBinds(a.bind_hum_jitter_amount, &a.humanize_jitter_amount);
    ApplySliderBinds(a.bind_hum_overshoot_min, &a.humanize_overshoot_min);
    ApplySliderBinds(a.bind_hum_overshoot_max, &a.humanize_overshoot_max);
    ApplySliderBinds(a.bind_hum_undershoot_min, &a.humanize_undershoot_min);
    ApplySliderBinds(a.bind_hum_undershoot_max, &a.humanize_undershoot_max);
    ApplySliderBinds(a.bind_hum_curve_amount, &a.humanize_curve_amount);
    ApplySliderBinds(a.bind_hum_reaction_min, &a.humanize_reaction_min);
    ApplySliderBinds(a.bind_hum_reaction_max, &a.humanize_reaction_max);
}

static bool* EspTargetBool(int side, int idx) {
    if (side == 0) {
        switch (idx) {
        case 0: return &config.esp_enemy_enable;
        case 1: return &config.esp_enemy_box;
        case 2: return &config.esp_enemy_box_style_corner;
        case 3: return &config.esp_enemy_box_style_rounded;
        case 4: return &config.esp_enemy_box_style_fill;
        case 5: return &config.esp_enemy_box_outline;
        case 6: return &config.esp_enemy_skeleton;
        case 7: case 8: case 9: case 10: case 11: case 12: case 13: return nullptr;
        case 14: return &config.esp_enemy_skeleton_draw_joints;
        case 15: return &config.esp_enemy_name;
        case 16: return &config.esp_enemy_healthbar;
        case 17: return &config.esp_enemy_distance;
        case 18: return &config.esp_enemy_headcircle;
        case 19: return &config.esp_enemy_snapline;
        case 20: return &config.esp_enemy_viewdirection;
        case 21: return &config.esp_enemy_weapon;
        case 22: return &config.esp_enemy_flags;
        case 23: return &config.esp_enemy_flags_flashed;
        case 24: return &config.esp_enemy_flags_scoped;
        case 25: return &config.esp_enemy_flags_defusing;
        case 26: return &config.esp_enemy_flags_planting;
        case 27: return &config.esp_enemy_flags_bomb;
        case 28: return &config.esp_enemy_flags_armor;
        case 29: return &config.esp_enemy_flags_reloading;
        case 30: return &config.esp_enemy_flags_money;
        case 31: return &config.esp_enemy_flags_ammo;
        case 32: return &config.esp_enemy_offscreen;
        default: return nullptr;
        }
    }
    else {
        switch (idx) {
        case 0: return &config.esp_team_enable;
        case 1: return &config.esp_team_box;
        case 2: return &config.esp_team_box_style_corner;
        case 3: return &config.esp_team_box_style_rounded;
        case 4: return &config.esp_team_box_style_fill;
        case 5: return &config.esp_team_box_outline;
        case 6: return &config.esp_team_skeleton;
        case 7: case 8: case 9: case 10: case 11: case 12: case 13: return nullptr;
        case 14: return &config.esp_team_skeleton_draw_joints;
        case 15: return &config.esp_team_name;
        case 16: return &config.esp_team_healthbar;
        case 17: return &config.esp_team_distance;
        case 18: return &config.esp_team_headcircle;
        case 19: return &config.esp_team_snapline;
        case 20: return &config.esp_team_viewdirection;
        case 21: return &config.esp_team_weapon;
        case 22: return &config.esp_team_flags;
        case 23: return &config.esp_team_flags_flashed;
        case 24: return &config.esp_team_flags_scoped;
        case 25: return &config.esp_team_flags_defusing;
        case 26: return &config.esp_team_flags_planting;
        case 27: return &config.esp_team_flags_bomb;
        case 28: return &config.esp_team_flags_armor;
        case 29: return &config.esp_team_flags_reloading;
        case 30: return &config.esp_team_flags_money;
        case 31: return &config.esp_team_flags_ammo;
        case 32: return &config.esp_team_offscreen;
        default: return nullptr;
        }
    }
}

void Config::TickBinds() {
    if (!s_labelsInit) { s_labelsInit = true; InitLabels(); }

    ApplyAimBinds(aim_shared);
    for (auto& g : aim_groups) ApplyAimBinds(g.settings);

    for (int side = 0; side < 2; ++side) {
        for (int i = 0; i < kBindEspItemCount; ++i) {
            if (i == 33 || i == 34) continue;
            bool* t = EspTargetBool(side, i);
            if (t) ApplyBoolBind(bind_esp[side][i], t);
        }
        ApplySliderBinds(bind_esp_offscreen_dist[side],
            (side == 0) ? &esp_enemy_offscreen_distance : &esp_team_offscreen_distance);
        ApplySliderBinds(bind_esp_offscreen_size[side],
            (side == 0) ? &esp_enemy_offscreen_size : &esp_team_offscreen_size);
    }

    ApplyBoolBind(bind_vsync, &vsync);
    ApplyBoolBind(bind_allow_tearing, &allow_tearing);
    ApplyBoolBind(bind_fullscreen_mode, &fullscreen_mode);
    ApplyBoolBind(bind_streamproof, &streamproof);
    ApplyBoolBind(bind_safe_mode, &safe_mode);

    esp_enemy_active = esp_enemy_enable;
    esp_team_active = esp_team_enable;
}

void Config::TickEspHotkeys() {
    TickBinds();
}

void Config::Save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "[Menu]\n";
    file << "show_menu = " << (show_menu ? "true" : "false") << "\n";

    file << "\n[Aimbot]\n";
    WriteAimSettings(file, aim_shared);
    WriteAimBinds(file, aim_shared);
    file << "current_edit = " << aim_current_edit << "\n";

    for (size_t i = 0; i < aim_groups.size(); ++i) {
        file << "\n[Aimbot_Group_" << i << "]\n";
        file << "name = " << aim_groups[i].name << "\n";
        std::string weapons;
        for (size_t j = 0; j < aim_groups[i].weapons.size(); ++j) {
            if (j > 0) weapons += "|";
            weapons += aim_groups[i].weapons[j];
        }
        file << "weapons = " << weapons << "\n";
        WriteAimSettings(file, aim_groups[i].settings);
        WriteAimBinds(file, aim_groups[i].settings);
    }

    file << "\n[ESP_Enemy]\n";
    file << "enable = " << (esp_enemy_enable ? "true" : "false") << "\n";
    file << "hotkey = " << esp_enemy_hotkey << "\n";
    file << "hotkey_mode = " << esp_enemy_hotkey_mode << "\n";
    file << "box = " << (esp_enemy_box ? "true" : "false") << "\n";
    file << "corner = " << (esp_enemy_box_style_corner ? "true" : "false") << "\n";
    file << "rounded = " << (esp_enemy_box_style_rounded ? "true" : "false") << "\n";
    file << "fill = " << (esp_enemy_box_style_fill ? "true" : "false") << "\n";
    file << "outline = " << (esp_enemy_box_outline ? "true" : "false") << "\n";
    file << "skeleton = " << (esp_enemy_skeleton ? "true" : "false") << "\n";
    file << "skeleton_joints = " << (esp_enemy_skeleton_joints & kSkelMask) << "\n";
    file << "skeleton_draw_joints = " << (esp_enemy_skeleton_draw_joints ? "true" : "false") << "\n";
    file << "name = " << (esp_enemy_name ? "true" : "false") << "\n";
    file << "healthbar = " << (esp_enemy_healthbar ? "true" : "false") << "\n";
    file << "distance = " << (esp_enemy_distance ? "true" : "false") << "\n";
    file << "headcircle = " << (esp_enemy_headcircle ? "true" : "false") << "\n";
    file << "snapline = " << (esp_enemy_snapline ? "true" : "false") << "\n";
    file << "viewdirection = " << (esp_enemy_viewdirection ? "true" : "false") << "\n";
    file << "weapon = " << (esp_enemy_weapon ? "true" : "false") << "\n";
    file << "offscreen = " << (esp_enemy_offscreen ? "true" : "false") << "\n";
    file << "offscreen_distance = " << esp_enemy_offscreen_distance << "\n";
    file << "offscreen_size = " << esp_enemy_offscreen_size << "\n";
    WriteColor(file, "box_color", esp_enemy_box_color);
    WriteColor(file, "fill_color", esp_enemy_box_fill_color);
    WriteColor(file, "outline_color", esp_enemy_box_outline_color);
    WriteColor(file, "skeleton_color", esp_enemy_skeleton_color);
    WriteColor(file, "name_color", esp_enemy_name_color);
    WriteColor(file, "healthbar_color", esp_enemy_healthbar_color);
    WriteColor(file, "distance_color", esp_enemy_distance_color);
    WriteColor(file, "headcircle_color", esp_enemy_headcircle_color);
    WriteColor(file, "snapline_color", esp_enemy_snapline_color);
    WriteColor(file, "viewdirection_color", esp_enemy_viewdirection_color);
    WriteColor(file, "weapon_color", esp_enemy_weapon_color);
    WriteColor(file, "offscreen_color", esp_enemy_offscreen_color);

    WriteFlagsBlock(file, "flags",
        esp_enemy_flags, esp_enemy_flags_color,
        esp_enemy_flags_flashed, esp_enemy_flags_flashed_color,
        esp_enemy_flags_scoped, esp_enemy_flags_scoped_color,
        esp_enemy_flags_defusing, esp_enemy_flags_defusing_color,
        esp_enemy_flags_planting, esp_enemy_flags_planting_color,
        esp_enemy_flags_bomb, esp_enemy_flags_bomb_color,
        esp_enemy_flags_armor, esp_enemy_flags_armor_color,
        esp_enemy_flags_reloading, esp_enemy_flags_reloading_color,
        esp_enemy_flags_money, esp_enemy_flags_money_color,
        esp_enemy_flags_ping, esp_enemy_flags_ping_color,
        esp_enemy_flags_ammo, esp_enemy_flags_ammo_color);

    for (int i = 0; i < kBindEspItemCount; ++i) {
        char k[64]; snprintf(k, sizeof(k), "bind_%d", i);
        WriteBind(file, k, bind_esp[0][i]);
    }
    WriteSliderBinds(file, "bind_offscreen_dist", bind_esp_offscreen_dist[0]);
    WriteSliderBinds(file, "bind_offscreen_size", bind_esp_offscreen_size[0]);

    file << "\n[ESP_Team]\n";
    file << "enable = " << (esp_team_enable ? "true" : "false") << "\n";
    file << "hotkey = " << esp_team_hotkey << "\n";
    file << "hotkey_mode = " << esp_team_hotkey_mode << "\n";
    file << "box = " << (esp_team_box ? "true" : "false") << "\n";
    file << "corner = " << (esp_team_box_style_corner ? "true" : "false") << "\n";
    file << "rounded = " << (esp_team_box_style_rounded ? "true" : "false") << "\n";
    file << "fill = " << (esp_team_box_style_fill ? "true" : "false") << "\n";
    file << "outline = " << (esp_team_box_outline ? "true" : "false") << "\n";
    file << "skeleton = " << (esp_team_skeleton ? "true" : "false") << "\n";
    file << "skeleton_joints = " << (esp_team_skeleton_joints & kSkelMask) << "\n";
    file << "skeleton_draw_joints = " << (esp_team_skeleton_draw_joints ? "true" : "false") << "\n";
    file << "name = " << (esp_team_name ? "true" : "false") << "\n";
    file << "healthbar = " << (esp_team_healthbar ? "true" : "false") << "\n";
    file << "distance = " << (esp_team_distance ? "true" : "false") << "\n";
    file << "headcircle = " << (esp_team_headcircle ? "true" : "false") << "\n";
    file << "snapline = " << (esp_team_snapline ? "true" : "false") << "\n";
    file << "viewdirection = " << (esp_team_viewdirection ? "true" : "false") << "\n";
    file << "weapon = " << (esp_team_weapon ? "true" : "false") << "\n";
    file << "offscreen = " << (esp_team_offscreen ? "true" : "false") << "\n";
    file << "offscreen_distance = " << esp_team_offscreen_distance << "\n";
    file << "offscreen_size = " << esp_team_offscreen_size << "\n";
    WriteColor(file, "box_color", esp_team_box_color);
    WriteColor(file, "fill_color", esp_team_box_fill_color);
    WriteColor(file, "outline_color", esp_team_box_outline_color);
    WriteColor(file, "skeleton_color", esp_team_skeleton_color);
    WriteColor(file, "name_color", esp_team_name_color);
    WriteColor(file, "healthbar_color", esp_team_healthbar_color);
    WriteColor(file, "distance_color", esp_team_distance_color);
    WriteColor(file, "headcircle_color", esp_team_headcircle_color);
    WriteColor(file, "snapline_color", esp_team_snapline_color);
    WriteColor(file, "viewdirection_color", esp_team_viewdirection_color);
    WriteColor(file, "weapon_color", esp_team_weapon_color);
    WriteColor(file, "offscreen_color", esp_team_offscreen_color);

    WriteFlagsBlock(file, "flags",
        esp_team_flags, esp_team_flags_color,
        esp_team_flags_flashed, esp_team_flags_flashed_color,
        esp_team_flags_scoped, esp_team_flags_scoped_color,
        esp_team_flags_defusing, esp_team_flags_defusing_color,
        esp_team_flags_planting, esp_team_flags_planting_color,
        esp_team_flags_bomb, esp_team_flags_bomb_color,
        esp_team_flags_armor, esp_team_flags_armor_color,
        esp_team_flags_reloading, esp_team_flags_reloading_color,
        esp_team_flags_money, esp_team_flags_money_color,
        esp_team_flags_ping, esp_team_flags_ping_color,
        esp_team_flags_ammo, esp_team_flags_ammo_color);

    for (int i = 0; i < kBindEspItemCount; ++i) {
        char k[64]; snprintf(k, sizeof(k), "bind_%d", i);
        WriteBind(file, k, bind_esp[1][i]);
    }
    WriteSliderBinds(file, "bind_offscreen_dist", bind_esp_offscreen_dist[1]);
    WriteSliderBinds(file, "bind_offscreen_size", bind_esp_offscreen_size[1]);

    file << "\n[Display]\n";
    file << "vsync = " << (vsync ? "true" : "false") << "\n";
    file << "allow_tearing = " << (allow_tearing ? "true" : "false") << "\n";
    file << "fullscreen_mode = " << (fullscreen_mode ? "true" : "false") << "\n";
    WriteBind(file, "bind_vsync", bind_vsync);
    WriteBind(file, "bind_allow_tearing", bind_allow_tearing);
    WriteBind(file, "bind_fullscreen_mode", bind_fullscreen_mode);

    file << "\n[Misc]\n";
    file << "safe_mode = " << (safe_mode ? "true" : "false") << "\n";
    file << "safe_mode_no_warn = " << (safe_mode_no_warn ? "true" : "false") << "\n";
    file << "streamproof = " << (streamproof ? "true" : "false") << "\n";
    WriteBind(file, "bind_streamproof", bind_streamproof);
    WriteBind(file, "bind_safe_mode", bind_safe_mode);

    file.close();
}

void Config::Load(const std::string& path) {
    if (!fs::exists(path)) {
        SetDefaults();
        if (!s_labelsInit) { s_labelsInit = true; InitLabels(); }
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        SetDefaults();
        if (!s_labelsInit) { s_labelsInit = true; InitLabels(); }
        return;
    }

    SetDefaults();

    std::string line;
    std::string currentSection;
    int groupSectionIdx = -1;

    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\n\r");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            groupSectionIdx = -1;
            const std::string prefix = "Aimbot_Group_";
            if (currentSection.compare(0, prefix.size(), prefix) == 0) {
                try { groupSectionIdx = std::stoi(currentSection.substr(prefix.size())); }
                catch (...) { groupSectionIdx = -1; }
                if (groupSectionIdx >= 0) {
                    while ((int)aim_groups.size() <= groupSectionIdx)
                        aim_groups.emplace_back();
                }
            }
            continue;
        }

        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;

        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        bool b = (value == "true");

        if (currentSection == "Menu") {
            if (key == "show_menu") show_menu = b;
        }
        else if (currentSection == "Aimbot") {
            if (key == "current_edit") {
                try { aim_current_edit = std::stoi(value); }
                catch (...) { aim_current_edit = 0; }
            }
            else {
                bool consumed = false;
                LoadAimBind(key, value, aim_shared, consumed);
                if (!consumed) LoadAimSetting(key, value, aim_shared);
            }
        }
        else if (groupSectionIdx >= 0 && groupSectionIdx < (int)aim_groups.size()) {
            auto& grp = aim_groups[groupSectionIdx];
            if (key == "name") grp.name = value;
            else if (key == "weapons") {
                grp.weapons.clear();
                std::stringstream ss(value);
                std::string tok;
                while (std::getline(ss, tok, '|')) {
                    size_t s2 = tok.find_first_not_of(" \t");
                    if (s2 == std::string::npos) continue;
                    size_t e2 = tok.find_last_not_of(" \t");
                    tok = tok.substr(s2, e2 - s2 + 1);
                    if (!tok.empty()) grp.weapons.push_back(tok);
                }
            }
            else {
                bool consumed = false;
                LoadAimBind(key, value, grp.settings, consumed);
                if (!consumed) LoadAimSetting(key, value, grp.settings);
            }
        }
        else if (currentSection == "ESP_Enemy") {
            if (key == "enable") esp_enemy_enable = b;
            else if (key == "hotkey") { try { esp_enemy_hotkey = std::stoi(value); } catch (...) { esp_enemy_hotkey = 0; } if (esp_enemy_hotkey < 0 || esp_enemy_hotkey > 255) esp_enemy_hotkey = 0; }
            else if (key == "hotkey_mode") { try { esp_enemy_hotkey_mode = std::stoi(value); } catch (...) { esp_enemy_hotkey_mode = 0; } if (esp_enemy_hotkey_mode < 0 || esp_enemy_hotkey_mode > 1) esp_enemy_hotkey_mode = 0; }
            else if (key == "box") esp_enemy_box = b;
            else if (key == "corner") esp_enemy_box_style_corner = b;
            else if (key == "rounded") esp_enemy_box_style_rounded = b;
            else if (key == "fill") esp_enemy_box_style_fill = b;
            else if (key == "outline") esp_enemy_box_outline = b;
            else if (key == "skeleton") esp_enemy_skeleton = b;
            else if (key == "skeleton_joints") { try { esp_enemy_skeleton_joints = static_cast<uint32_t>(std::stoul(value)) & kSkelMask; } catch (...) { esp_enemy_skeleton_joints = kSkelMask; } }
            else if (key == "skeleton_draw_joints") esp_enemy_skeleton_draw_joints = b;
            else if (key == "name") esp_enemy_name = b;
            else if (key == "healthbar") esp_enemy_healthbar = b;
            else if (key == "distance") esp_enemy_distance = b;
            else if (key == "headcircle") esp_enemy_headcircle = b;
            else if (key == "snapline") esp_enemy_snapline = b;
            else if (key == "viewdirection") esp_enemy_viewdirection = b;
            else if (key == "weapon") esp_enemy_weapon = b;
            else if (key == "offscreen") esp_enemy_offscreen = b;
            else if (key == "offscreen_distance") { try { esp_enemy_offscreen_distance = std::stof(value); } catch (...) {} }
            else if (key == "offscreen_size") { try { esp_enemy_offscreen_size = std::stof(value); } catch (...) {} }
            else if (key == "box_color") ParseColor(value, esp_enemy_box_color);
            else if (key == "fill_color") ParseColor(value, esp_enemy_box_fill_color);
            else if (key == "outline_color") ParseColor(value, esp_enemy_box_outline_color);
            else if (key == "skeleton_color") ParseColor(value, esp_enemy_skeleton_color);
            else if (key == "name_color") ParseColor(value, esp_enemy_name_color);
            else if (key == "healthbar_color") ParseColor(value, esp_enemy_healthbar_color);
            else if (key == "distance_color") ParseColor(value, esp_enemy_distance_color);
            else if (key == "headcircle_color") ParseColor(value, esp_enemy_headcircle_color);
            else if (key == "snapline_color") ParseColor(value, esp_enemy_snapline_color);
            else if (key == "viewdirection_color") ParseColor(value, esp_enemy_viewdirection_color);
            else if (key == "weapon_color") ParseColor(value, esp_enemy_weapon_color);
            else if (key == "offscreen_color") ParseColor(value, esp_enemy_offscreen_color);
            else if (key == "bind_offscreen_dist") ParseSliderBinds(value, bind_esp_offscreen_dist[0]);
            else if (key == "bind_offscreen_size") ParseSliderBinds(value, bind_esp_offscreen_size[0]);
            else if (key.size() > 5 && key.compare(0, 5, "bind_") == 0) {
                try {
                    int idx = std::stoi(key.substr(5));
                    if (idx >= 0 && idx < kBindEspItemCount) ParseBind(value, bind_esp[0][idx]);
                }
                catch (...) {}
            }
            else {
                bool consumed = false;
                LoadFlagsBlock(key, value,
                    esp_enemy_flags, esp_enemy_flags_color,
                    esp_enemy_flags_flashed, esp_enemy_flags_flashed_color,
                    esp_enemy_flags_scoped, esp_enemy_flags_scoped_color,
                    esp_enemy_flags_defusing, esp_enemy_flags_defusing_color,
                    esp_enemy_flags_planting, esp_enemy_flags_planting_color,
                    esp_enemy_flags_bomb, esp_enemy_flags_bomb_color,
                    esp_enemy_flags_armor, esp_enemy_flags_armor_color,
                    esp_enemy_flags_reloading, esp_enemy_flags_reloading_color,
                    esp_enemy_flags_money, esp_enemy_flags_money_color,
                    esp_enemy_flags_ping, esp_enemy_flags_ping_color,
                    esp_enemy_flags_ammo, esp_enemy_flags_ammo_color,
                    consumed);
            }
        }
        else if (currentSection == "ESP_Team") {
            if (key == "enable") esp_team_enable = b;
            else if (key == "hotkey") { try { esp_team_hotkey = std::stoi(value); } catch (...) { esp_team_hotkey = 0; } if (esp_team_hotkey < 0 || esp_team_hotkey > 255) esp_team_hotkey = 0; }
            else if (key == "hotkey_mode") { try { esp_team_hotkey_mode = std::stoi(value); } catch (...) { esp_team_hotkey_mode = 0; } if (esp_team_hotkey_mode < 0 || esp_team_hotkey_mode > 1) esp_team_hotkey_mode = 0; }
            else if (key == "box") esp_team_box = b;
            else if (key == "corner") esp_team_box_style_corner = b;
            else if (key == "rounded") esp_team_box_style_rounded = b;
            else if (key == "fill") esp_team_box_style_fill = b;
            else if (key == "outline") esp_team_box_outline = b;
            else if (key == "skeleton") esp_team_skeleton = b;
            else if (key == "skeleton_joints") { try { esp_team_skeleton_joints = static_cast<uint32_t>(std::stoul(value)) & kSkelMask; } catch (...) { esp_team_skeleton_joints = kSkelMask; } }
            else if (key == "skeleton_draw_joints") esp_team_skeleton_draw_joints = b;
            else if (key == "name") esp_team_name = b;
            else if (key == "healthbar") esp_team_healthbar = b;
            else if (key == "distance") esp_team_distance = b;
            else if (key == "headcircle") esp_team_headcircle = b;
            else if (key == "snapline") esp_team_snapline = b;
            else if (key == "viewdirection") esp_team_viewdirection = b;
            else if (key == "weapon") esp_team_weapon = b;
            else if (key == "offscreen") esp_team_offscreen = b;
            else if (key == "offscreen_distance") { try { esp_team_offscreen_distance = std::stof(value); } catch (...) {} }
            else if (key == "offscreen_size") { try { esp_team_offscreen_size = std::stof(value); } catch (...) {} }
            else if (key == "box_color") ParseColor(value, esp_team_box_color);
            else if (key == "fill_color") ParseColor(value, esp_team_box_fill_color);
            else if (key == "outline_color") ParseColor(value, esp_team_box_outline_color);
            else if (key == "skeleton_color") ParseColor(value, esp_team_skeleton_color);
            else if (key == "name_color") ParseColor(value, esp_team_name_color);
            else if (key == "healthbar_color") ParseColor(value, esp_team_healthbar_color);
            else if (key == "distance_color") ParseColor(value, esp_team_distance_color);
            else if (key == "headcircle_color") ParseColor(value, esp_team_headcircle_color);
            else if (key == "snapline_color") ParseColor(value, esp_team_snapline_color);
            else if (key == "viewdirection_color") ParseColor(value, esp_team_viewdirection_color);
            else if (key == "weapon_color") ParseColor(value, esp_team_weapon_color);
            else if (key == "offscreen_color") ParseColor(value, esp_team_offscreen_color);
            else if (key == "bind_offscreen_dist") ParseSliderBinds(value, bind_esp_offscreen_dist[1]);
            else if (key == "bind_offscreen_size") ParseSliderBinds(value, bind_esp_offscreen_size[1]);
            else if (key.size() > 5 && key.compare(0, 5, "bind_") == 0) {
                try {
                    int idx = std::stoi(key.substr(5));
                    if (idx >= 0 && idx < kBindEspItemCount) ParseBind(value, bind_esp[1][idx]);
                }
                catch (...) {}
            }
            else {
                bool consumed = false;
                LoadFlagsBlock(key, value,
                    esp_team_flags, esp_team_flags_color,
                    esp_team_flags_flashed, esp_team_flags_flashed_color,
                    esp_team_flags_scoped, esp_team_flags_scoped_color,
                    esp_team_flags_defusing, esp_team_flags_defusing_color,
                    esp_team_flags_planting, esp_team_flags_planting_color,
                    esp_team_flags_bomb, esp_team_flags_bomb_color,
                    esp_team_flags_armor, esp_team_flags_armor_color,
                    esp_team_flags_reloading, esp_team_flags_reloading_color,
                    esp_team_flags_money, esp_team_flags_money_color,
                    esp_team_flags_ping, esp_team_flags_ping_color,
                    esp_team_flags_ammo, esp_team_flags_ammo_color,
                    consumed);
            }
        }
        else if (currentSection == "Display") {
            if (key == "vsync") vsync = b;
            else if (key == "allow_tearing") allow_tearing = b;
            else if (key == "fullscreen_mode") fullscreen_mode = b;
            else if (key == "bind_vsync") ParseBind(value, bind_vsync);
            else if (key == "bind_allow_tearing") ParseBind(value, bind_allow_tearing);
            else if (key == "bind_fullscreen_mode") ParseBind(value, bind_fullscreen_mode);
        }
        else if (currentSection == "Misc") {
            if (key == "safe_mode") safe_mode = b;
            else if (key == "safe_mode_no_warn") safe_mode_no_warn = b;
            else if (key == "streamproof") streamproof = b;
            else if (key == "bind_streamproof") ParseBind(value, bind_streamproof);
            else if (key == "bind_safe_mode") ParseBind(value, bind_safe_mode);
        }
    }

    file.close();

    if (aim_current_edit < 0 || aim_current_edit >(int)aim_groups.size())
        aim_current_edit = 0;

    if (!s_labelsInit) { s_labelsInit = true; InitLabels(); }
}