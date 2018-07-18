/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"
#include "hacks/Radar.hpp"

CatVar clean_screenshots(CV_SWITCH, "clean_screenshots", "1",
                         "Clean screenshots",
                         "Don't draw visuals while taking a screenshot");
CatVar disable_visuals(CV_SWITCH, "no_visuals", "0", "Disable ALL drawing",
                       "Completely hides cathook");
CatVar no_zoom(CV_SWITCH, "no_zoom", "0", "Disable scope",
               "Disables black scope overlay");
static CatVar pure_bypass(CV_SWITCH, "pure_bypass", "0", "Pure Bypass",
                          "Bypass sv_pure");
int spamdur = 0;
Timer joinspam{};
CatCommand join_spam("join_spam",
                         "Spam joins server for X seconds",
                         [](const CCommand &args) {
							 if (args.ArgC() < 1)
								 return;
                             int id    = atoi(args.Arg(1));
                             joinspam.update();
                             spamdur = id;
                         });
void *pure_orig  = nullptr;
void **pure_addr = nullptr;

static CatEnum software_cursor_enum({ "KEEP", "ALWAYS", "NEVER", "MENU ON",
                                      "MENU OFF" });
static CatVar
    software_cursor_mode(software_cursor_enum, "software_cursor_mode", "0",
                         "Software cursor",
                         "Try to change this and see what works best for you");
static CatVar no_reportlimit(CV_SWITCH, "no_reportlimit", "0",
                             "no report limit",
                             "Remove playerlist report time limit");
// static CatVar disable_ban_tf(CV_SWITCH, "disable_mm_ban", "0", "Disable MM
// ban", "Disable matchmaking ban");
// static CatVar party_bypass(CV_SWITCH, "party_bypass", "0", "Party bypass",
// "Bypass the have to be friended restrictions on party");
/*void JoinParty(uint32 steamid)
{
    CSteamID id(steamid, EUniverse::k_EUniversePublic,
                EAccountType::k_EAccountTypeIndividual);
}
CatCommand join_party("join_party", "Join this players party (steamid3, no U:1:
and no [])", [](const CCommand &args) {
    if (args.ArgC() < 1)
    {
        g_ICvar->ConsolePrintf("Please give a steamid3, thanks.\n");
        return;
    }

    std::string tofind(args.Arg(1));
    if (tofind.find("[U:1:") || tofind.find("]"))
        g_ICvar->ConsolePrintf("Invalid steamid3. Please remove the [U:1 and the
].\n");
    unsigned int id = atol(args.Arg(1));
    if (!id)
        g_ICvar->ConsolePrintf("Invalid steamid3.\n");
    else
        JoinParty(id);
});*/

bool replaced = false;
namespace hooked_methods
{

DEFINE_HOOKED_METHOD(PaintTraverse, void, vgui::IPanel *this_,
                     unsigned int panel, bool force, bool allow_force)
{
    static bool textures_loaded      = false;
    static unsigned long panel_focus = 0;
    static unsigned long panel_scope = 0;
    static unsigned long panel_top   = 0;
    static bool cur, draw_flag = false;
    static bool call_default       = true;
    static ConVar *software_cursor = g_ICvar->FindVar("cl_software_cursor");
    static const char *name;
    static std::string name_s, name_stripped, reason_stripped;

#if ENABLE_VISUALS
    if (!textures_loaded)
        textures_loaded = true;
#endif
    static bool switcherido = false;
    if (switcherido && spamdur && !joinspam.check(spamdur * 1000))
    {
    	auto gc = re::CTFGCClientSystem::GTFGCClientSystem();
    	if (!gc)
    		goto label1;
    	gc->JoinMMMatch();
    }
    else if (!joinspam.check(spamdur * 1000) && spamdur)
    {
    	INetChannel *ch = (INetChannel *)g_IEngine->GetNetChannelInfo();
    	if (!ch)
    		goto label1;
    	ch->Shutdown("GET GOOD GET CATHOOK");
    }
    label1:
    /*static bool replacedparty = false;
    if (party_bypass && !replacedparty)
    {
        static unsigned char patch[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        static unsigned char patch2[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
        static unsigned char patch3[] = { 0x90, 0x90};
        static unsigned char patch4[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
        uintptr_t addr = gSignatures.GetClientSignature("0F 84 ? ? ? ? 8B 7B ?
    8D 45");
        uintptr_t addr2 = gSignatures.GetClientSignature("0F 8F ? ? ? ? 80 BD ?
    ? ? ? ? 0F 84 ? ? ? ? 80 BD");
        uintptr_t addr3 = gSignatures.GetClientSignature("74 ? E8 ? ? ? ? 89
    F1");
        uintptr_t addr4 = gSignatures.GetClientSignature("0F 84 ? ? ? ? 8B 45 ?
    8B 70 ? 8B 78 ? 8D 45");
        uintptr_t addr5 = gSignatures.GetClientSignature("89 C6 74 ? 8B 43 ? 85
    C0 74 ? 8B 10");
        if (addr && addr2 && addr3 && addr4 && addr5)
        {
            logging::Info("Party bypass: 0x%08X, 0x%08X, 0x%08X, 0x%08X", addr,
    addr2, addr3, addr4);
            Patch((void*) addr, (void *) patch, sizeof(patch));
            Patch((void*) addr2, (void *) patch2, sizeof(patch2));
            Patch((void*) addr3, (void *) patch3, sizeof(patch3));
            Patch((void*) addr4, (void *) patch4, sizeof(patch4));
            replacedparty = true;
        }
        else
            logging::Info("No Party bypass Signature");
    }
    static bool replacedban = false;
    if (disable_ban_tf && !replacedban)
    {
        static unsigned char patch[] = { 0x31, 0xe0 };
        static unsigned char patch2[] = { 0xb0, 0x01, 0x90 };
        uintptr_t addr = gSignatures.GetClientSignature("31 C0 5B 5E 5F 5D C3 8D
    B6 00 00 00 00 BA");
        uintptr_t addr2 = gSignatures.GetClientSignature("0F 92 C0 83 C4 ? 5B 5E
    5F 5D C3 8D B4 26 00 00 00 00 83 C4");
        if (addr && addr2)
        {
            logging::Info("MM Banned: 0x%08x, 0x%08x", addr, addr2);
            Patch((void*) addr, (void *) patch, sizeof(patch));
            Patch((void*) addr2, (void *) patch2, sizeof(patch2));
            replacedban = true;
        }
        else
            logging::Info("No disable ban Signature");

    }*/
    if (no_reportlimit && !replaced)
    {
        static unsigned char patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00 };
        static uintptr_t report_addr = gSignatures.GetClientSignature(
            "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
        if (report_addr)
        {
            uintptr_t topatch = report_addr + 0x75;
            logging::Info("No Report limit: 0x%08x", report_addr);
            Patch((void *) topatch, (void *) patch, sizeof(patch));
            replaced = true;
        }
        else
            report_addr = gSignatures.GetClientSignature(
                "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
    }
    if (pure_bypass)
    {
        if (!pure_addr)
        {
            pure_addr = *reinterpret_cast<void ***>(
                gSignatures.GetEngineSignature(
                    "A1 ? ? ? ? 85 C0 74 ? C7 44 24 ? ? ? ? ? 89 04 24") +
                1);
        }
        if (*pure_addr)
            pure_orig = *pure_addr;
        *pure_addr    = (void *) 0;
    }
    else if (pure_orig)
    {
        *pure_addr = pure_orig;
        pure_orig  = (void *) 0;
    }
    call_default = true;
    if (cathook && panel_scope && no_zoom && panel == panel_scope)
        call_default = false;

    if (software_cursor_mode)
    {
        cur = software_cursor->GetBool();
        switch ((int) software_cursor_mode)
        {
        case 1:
            if (!software_cursor->GetBool())
                software_cursor->SetValue(1);
            break;
        case 2:
            if (software_cursor->GetBool())
                software_cursor->SetValue(0);
            break;
#if ENABLE_GUI
/*
        case 3:
            if (cur != g_pGUI->Visible()) {
                software_cursor->SetValue(g_pGUI->Visible());
            }
            break;
        case 4:
            if (cur == g_pGUI->Visible()) {
                software_cursor->SetValue(!g_pGUI->Visible());
            }
*/
#endif
        }
    }

    if (call_default)
        original::PaintTraverse(this_, panel, force, allow_force);
    // To avoid threading problems.

    if (panel == panel_top)
        draw_flag = true;
    if (!cathook)
        return;

    if (!panel_top)
    {
        name = g_IPanel->GetName(panel);
        if (strlen(name) > 4)
        {
            if (name[0] == 'M' && name[3] == 'S')
            {
                panel_top = panel;
            }
        }
    }
    if (!panel_focus)
    {
        name = g_IPanel->GetName(panel);
        if (strlen(name) > 5)
        {
            if (name[0] == 'F' && name[5] == 'O')
            {
                panel_focus = panel;
            }
        }
    }
    if (!panel_scope)
    {
        name = g_IPanel->GetName(panel);
        if (!strcmp(name, "HudScope"))
        {
            panel_scope = panel;
        }
    }
    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid = true;
    }

    if (panel != panel_focus)
        return;
    g_IPanel->SetTopmostPopup(panel_focus, true);
    if (!draw_flag)
        return;
    draw_flag = false;

    if (disable_visuals)
        return;

    if (clean_screenshots && g_IEngine->IsTakingScreenshot())
        return;
#if ENABLE_GUI
    g_pGUI->Update();
#endif
    draw::UpdateWTS();
}
}
