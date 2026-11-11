#include "Bunnyhop.hpp"
#include "../../SDK/Offsets/Offsets.hpp"
#include "../../Utilities/Config/Config.hpp"
#include "../../Utilities/Memory/Memory.hpp"
#include <cstdint>

namespace Bunnyhop {

    static uintptr_t s_ClientBase = 0;

    static inline void EnsureBase() {
        if (!s_ClientBase)
            s_ClientBase = Memory::ModuleBase("client.dll");
    }

    void OnCreateMove(void* pCmd, int nSlot) {
        if (nSlot != 0 || !pCmd)
            return;

        if (!g_Config.bunnyhop.load(std::memory_order_relaxed))
            return;

        EnsureBase();
        if (!s_ClientBase)
            return;

        const uintptr_t localPawn = *reinterpret_cast<uintptr_t*>(
            s_ClientBase + offsets::client_dll::dwLocalPlayerPawn);
        if (!localPawn || localPawn < 0x10000)
            return;

        const uint32_t flags = *reinterpret_cast<uint32_t*>(
            localPawn + offsets::C_BaseEntity::m_fFlags);

        uint64_t* buttons = reinterpret_cast<uint64_t*>(
            reinterpret_cast<uintptr_t>(pCmd) + offsets::CUserCmd::m_nButtons);

        if (flags & offsets::flags::FL_ONGROUND)
            *buttons |= offsets::input_flags::IN_JUMP;
        else
            *buttons &= ~offsets::input_flags::IN_JUMP;
    }
}