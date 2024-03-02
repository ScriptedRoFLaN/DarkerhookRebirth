#include "config.h"
#include "common.hpp"
#include "libnullnexus/nullnexus.hpp"
#include "nullnexus.hpp"
#include "netadr.h"
#if ENABLE_VISUALS
#include "colors.hpp"
#include "MiscTemporary.hpp"
#endif

namespace nullnexus
{
static settings::Boolean enabled("nullnexus.enabled", "true");
#if ENABLE_VISUALS
static settings::Boolean debug("nullnexus.debug", "false");
#endif
static settings::Boolean anon("nullnexus.user.anon", "false"); // Closet cheater
static settings::String address("nullnexus.host", "45.45.238.81"); // nullnexus.cathook.club is an ip logger, this server is not.
static settings::String port("nullnexus.port", "3000");
static settings::String endpoint("nullnexus.endpoint", "/api/v1/client");
#if ENABLE_TEXTMODE
static settings::Boolean proxyenabled("nullnexus.proxy.enabled", "true");
#else
static settings::Boolean proxyenabled("nullnexus.proxy.enabled", "false");
#endif
static settings::String proxysocket("nullnexus.relay.socket", "/tmp/nullnexus.sock");
static settings::Boolean authenticate("nullnexus.auth", "true");
#if ENABLE_VISUALS
static settings::Rgba colour("nullnexus.user.colour");
#endif

static NullNexus nexus;

void printmsg(std::string &usr, std::string &msg, int colour = 0xff9340)
{
#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
    if (msg.size() > 128 || usr.size() > 32)
    {
        logging::Info("Nullnexus: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        g_ICvar->ConsoleColorPrintf(MENU_COLOR, "[Nullnexus] %s: %s\n", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[\x07%06XNullnexus\x07%06X] \x07%06X%s\x01: %s", 0x5e3252, 0xba3d9a, 0x5e3252, colour, usr.c_str(), msg.c_str());
#endif
}
void printmsgcopy(std::string usr, std::string msg)
{
    printmsg(usr, msg);
}

namespace handlers
{
void message(std::string usr, std::string msg, int colour)
{
    printmsg(usr, msg, colour);
}
void authedplayers(std::vector<std::string> steamids)
{
    // Check if we are in a game
    if (g_Settings.bInvalid)
        return;
    // Yo are we identifing the world?
    for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
    {
        player_info_s pinfo{};
        if (GetPlayerInfo(i, &pinfo))
        {
            if (!pinfo.friendsID || !pinfo.name)
                continue;
            MD5Value_t result;
            std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
            MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
            std::stringstream ss;
            ss << std::hex;
            for (auto i : result.bits)
                ss << std::setw(2) << std::setfill('0') << (int) i;
            steamidhash = ss.str();
            std::remove_if(steamids.begin(), steamids.end(),
                           [&steamidhash, &pinfo](std::string &steamid)
                           {
#if ENABLE_VISUALS
                               if (*debug)
                                   g_ICvar->ConsoleColorPrintf(MENU_COLOR, "Got Data from nullnexus Server:\n\nSteamID %s\nSteamIDhash %s\n", steamid, steamidhash);
#endif
                               if (steamid == steamidhash)
                               {
                                   // Use actual steamid to set cat status
                                   if (playerlist::ChangeState(pinfo.friendsID, playerlist::k_EState::CAT))
                                       PrintChat("\x07%06X%s\x01 Marked as CAT (Nullnexus user)", 0xe05938, pinfo.name);
                                   return true;
                               }
                               return false;
                           });
        }
    }
}
} // namespace handlers

// Update info about the current server we are on.
void updateServer(NullNexus::UserSettings &settings)
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch && *authenticate)
    {
        static int *gHostSpawnCount = *reinterpret_cast<int **>(CSignature::GetEngineSignature("A3 ? ? ? ? A1 ? ? ? ? 8B 10 89 04 24 FF 52 ? 83 C4 2C") + sizeof(char));
        player_info_s pinfo{};
        if (GetPlayerInfo(g_pLocalPlayer->entity_idx, &pinfo) && pinfo.friendsID && pinfo.name)
        {
            MD5Value_t result;
            std::string steamidhash = std::to_string(pinfo.friendsID) + pinfo.name;
            MD5_ProcessSingleBuffer(steamidhash.c_str(), strlen(steamidhash.c_str()), result);
            std::stringstream ss;
            ss << std::hex;
            for (auto i : result.bits)
                ss << std::setw(2) << std::setfill('0') << (int) i;
            steamidhash        = ss.str();
#if ENABLE_VISUALS
            if (*debug)
                g_ICvar->ConsoleColorPrintf(MENU_COLOR, "Data sent to nullnexus Server:\n\nSteamIDhash %s\n gHostSpawnCount %i\n", steamidhash, *gHostSpawnCount);
#endif
            // IP, Port and Server SteamID are broken (IP/Port is broken because of SDR and Server steamid is broken due to detourhook not working)
            // Soooooooo we can only use gHostSpawnCount for now ={
            settings.tf2server = { true, "nullnexus", "nullnexus", steamidhash, *gHostSpawnCount };
            return;
        }
    }
    // Not connected
    settings.tf2server = { false };
}

// Update info about the current server we are on.
void updateServer()
{
    NullNexus::UserSettings settings;
    updateServer(settings);
    nexus.changeData(settings);
}

void updateData()
{
    std::optional<std::string> username = std::nullopt;
    std::optional<int> newcolour        = std::nullopt;
    username                            = *anon ? "anon" : g_ISteamFriends->GetPersonaName();
#if ENABLE_VISUALS
    if ((*colour).r || (*colour).g || (*colour).b)
    {
        int r     = (*colour).r * 255;
        int g     = (*colour).g * 255;
        int b     = (*colour).b * 255;
        newcolour = (r << 16) + (g << 8) + b;
    }
#endif
    NullNexus::UserSettings settings;
    settings.username = *username;
    settings.colour   = newcolour;
    // Tell nullnexus about the current server we are connected to.
    updateServer(settings);

    nexus.changeData(settings);
}

bool sendmsg(std::string &msg)
{
    if (!enabled)
    {
        printmsgcopy("Cathook", "Error! Nullnexus is disabled!");
        return false;
    }
    if (nexus.sendChat(msg))
        return true;
    printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

template <typename T> void rvarCallback(settings::VariableBase<T> &, T)
{
#if ENABLE_VISUALS
    std::thread reload(
        []()
        {
            std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
            updateData();
            if (*enabled)
            {
                if (*proxyenabled)
                    nexus.connectunix(*proxysocket, *endpoint, true);
                else
                    nexus.connect(*address, *port, *endpoint, true);
            }
            else if (!*proxyenabled)
                nexus.disconnect();
        });
    reload.detach();
#endif
}

template <typename T> void rvarDataCallback(settings::VariableBase<T> &, T)
{
    std::thread reload(
        []()
        {
            std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
            updateData();
        });
    reload.detach();
}

void Shutdown()
{
#if ENABLE_VISUALS
    nexus.disconnect();
#endif
}

static InitRoutine init(
    []()
    {
        updateData();
        enabled.installChangeCallback(rvarCallback<bool>);
        address.installChangeCallback(rvarCallback<std::string>);
        port.installChangeCallback(rvarCallback<std::string>);
        endpoint.installChangeCallback(rvarCallback<std::string>);

        proxyenabled.installChangeCallback(rvarCallback<bool>);
        proxysocket.installChangeCallback(rvarCallback<std::string>);

#if ENABLE_VISUALS
        colour.installChangeCallback(rvarDataCallback<rgba_t>);
#endif
        anon.installChangeCallback(rvarDataCallback<bool>);
        authenticate.installChangeCallback(rvarDataCallback<bool>);

#if ENABLE_VISUALS
        nexus.setHandlerChat(handlers::message);
#endif
        nexus.setHandlerAuthedplayers(handlers::authedplayers);
        // DISCONNECTING/RECONNECTING WHEN USING PROXY WILL CAUSE A CRASH DUE TO HOW SHIT WEBSOCKETS ARE!!!
        if (*enabled)
        {
            if (*proxyenabled)
                nexus.connectunix(*proxysocket, *endpoint, true);
            else
                nexus.connect(*address, *port, *endpoint, true);
        }
        EC::Register(
            EC::Shutdown, []() { Shutdown(); }, "shutdown_nullnexus");
        EC::Register(
            EC::FirstCM, []() { updateServer(); }, "firstcm_nullnexus");
        EC::Register(
            EC::LevelShutdown, []() { updateServer(); }, "firstcm_nullnexus");
    });
static CatCommand nullnexus_send("nullnexus_send", "Send message to IRC",
                                 [](const CCommand &args)
                                 {
                                     std::string msg(args.ArgS());
                                     sendmsg(msg);
                                 });
} // namespace nullnexus
