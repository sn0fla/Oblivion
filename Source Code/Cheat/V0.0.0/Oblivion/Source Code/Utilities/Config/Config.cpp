#include "Config.hpp"
#include <fstream>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

Config config;
ConfigBrowser configBrowser;

static constexpr uint32_t kSkelMask = 0x7F;

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

void Config::SetDefaults() {
    show_menu = false;

    aim_enable = false;
    aim_mode = 0;
    aim_key = 0;
    aim_fov = 180.0f;
    aim_hitbox = 1;
    aim_smooth = 50.0f;

    esp_enemy_enable = false;
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
}

void Config::Save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "[Menu]\n";
    file << "show_menu = " << (show_menu ? "true" : "false") << "\n";

    file << "\n[Aimbot]\n";
    file << "enable = " << (aim_enable ? "true" : "false") << "\n";
    file << "mode = " << aim_mode << "\n";
    file << "key = " << aim_key << "\n";
    file << "fov = " << aim_fov << "\n";
    file << "hitbox = " << aim_hitbox << "\n";
    file << "smooth = " << aim_smooth << "\n";

    file << "\n[ESP_Enemy]\n";
    file << "enable = " << (esp_enemy_enable ? "true" : "false") << "\n";
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

    file << "\n[ESP_Team]\n";
    file << "enable = " << (esp_team_enable ? "true" : "false") << "\n";
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

    file << "\n[Display]\n";
    file << "vsync = " << (vsync ? "true" : "false") << "\n";
    file << "allow_tearing = " << (allow_tearing ? "true" : "false") << "\n";
    file << "fullscreen_mode = " << (fullscreen_mode ? "true" : "false") << "\n";

    file << "\n[Misc]\n";
    file << "safe_mode = " << (safe_mode ? "true" : "false") << "\n";
    file << "safe_mode_no_warn = " << (safe_mode_no_warn ? "true" : "false") << "\n";

    file.close();
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
    auto Bool = [&](const char* n) { return key == n; };
    auto Color = [&](const char* n) { return key == n; };

    consumed = true;
    if (Bool("flags"))                             master = (value == "true");
    else if (Color("flags_color"))                 ParseColor(value, masterCol);
    else if (Bool("flags_flashed"))                flashed = (value == "true");
    else if (Color("flags_flashed_color"))         ParseColor(value, flashedCol);
    else if (Bool("flags_scoped"))                 scoped = (value == "true");
    else if (Color("flags_scoped_color"))          ParseColor(value, scopedCol);
    else if (Bool("flags_defusing"))               defusing = (value == "true");
    else if (Color("flags_defusing_color"))        ParseColor(value, defusingCol);
    else if (Bool("flags_planting"))               planting = (value == "true");
    else if (Color("flags_planting_color"))        ParseColor(value, plantingCol);
    else if (Bool("flags_bomb"))                   bomb = (value == "true");
    else if (Color("flags_bomb_color"))            ParseColor(value, bombCol);
    else if (Bool("flags_armor"))                  armor = (value == "true");
    else if (Color("flags_armor_color"))           ParseColor(value, armorCol);
    else if (Bool("flags_reloading"))              reloading = (value == "true");
    else if (Color("flags_reloading_color"))       ParseColor(value, reloadingCol);
    else if (Bool("flags_money"))                  money = (value == "true");
    else if (Color("flags_money_color"))           ParseColor(value, moneyCol);
    else if (Bool("flags_ping"))                   ping = (value == "true");
    else if (Color("flags_ping_color"))            ParseColor(value, pingCol);
    else if (Bool("flags_ammo"))                   ammo = (value == "true");
    else if (Color("flags_ammo_color"))            ParseColor(value, ammoCol);
    else consumed = false;
}

void Config::Load(const std::string& path) {
    if (!fs::exists(path)) {
        SetDefaults();
        return;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        SetDefaults();
        return;
    }

    SetDefaults();

    std::string line;
    std::string currentSection;

    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) continue;
        size_t end = line.find_last_not_of(" \t\n\r");
        line = line.substr(start, end - start + 1);

        if (line.empty() || line[0] == ';' || line[0] == '#') continue;

        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
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
            if (key == "enable") aim_enable = b;
            else if (key == "mode") {
                try { aim_mode = std::stoi(value); }
                catch (...) { aim_mode = 0; }
                if (aim_mode < 0 || aim_mode > 2) aim_mode = 0;
            }
            else if (key == "key") {
                try { aim_key = std::stoi(value); }
                catch (...) { aim_key = 0; }
                if (aim_key < 0 || aim_key > 255) aim_key = 0;
            }
            else if (key == "fov") {
                try { aim_fov = std::stof(value); }
                catch (...) { aim_fov = 180.0f; }
                if (aim_fov < 0.0f) aim_fov = 0.0f;
                if (aim_fov > 180.0f) aim_fov = 180.0f;
            }
            else if (key == "hitbox") {
                try { aim_hitbox = std::stoi(value); }
                catch (...) { aim_hitbox = 1; }
                if (aim_hitbox < 0) aim_hitbox = 0;
                if (aim_hitbox > 127) aim_hitbox = 127;
            }
            else if (key == "smooth") {
                try { aim_smooth = std::stof(value); }
                catch (...) { aim_smooth = 50.0f; }
                if (aim_smooth < 0.0f) aim_smooth = 0.0f;
                if (aim_smooth > 100.0f) aim_smooth = 100.0f;
            }
        }
        else if (currentSection == "ESP_Enemy") {
            if (key == "enable") esp_enemy_enable = b;
            else if (key == "box") esp_enemy_box = b;
            else if (key == "corner") esp_enemy_box_style_corner = b;
            else if (key == "rounded") esp_enemy_box_style_rounded = b;
            else if (key == "fill") esp_enemy_box_style_fill = b;
            else if (key == "outline") esp_enemy_box_outline = b;
            else if (key == "skeleton") esp_enemy_skeleton = b;
            else if (key == "skeleton_joints") {
                try { esp_enemy_skeleton_joints = static_cast<uint32_t>(std::stoul(value)) & kSkelMask; }
                catch (...) { esp_enemy_skeleton_joints = kSkelMask; }
            }
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
            else if (key == "box") esp_team_box = b;
            else if (key == "corner") esp_team_box_style_corner = b;
            else if (key == "rounded") esp_team_box_style_rounded = b;
            else if (key == "fill") esp_team_box_style_fill = b;
            else if (key == "outline") esp_team_box_outline = b;
            else if (key == "skeleton") esp_team_skeleton = b;
            else if (key == "skeleton_joints") {
                try { esp_team_skeleton_joints = static_cast<uint32_t>(std::stoul(value)) & kSkelMask; }
                catch (...) { esp_team_skeleton_joints = kSkelMask; }
            }
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
        }
        else if (currentSection == "Misc") {
            if (key == "safe_mode") safe_mode = b;
            else if (key == "safe_mode_no_warn") safe_mode_no_warn = b;
        }
    }

    file.close();
}