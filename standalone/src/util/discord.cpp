// Discord Webhook implementation

#ifdef WITH_DISCORD

#include "util/discord.h"
#include "util/logging.h"

#include <nlohmann/json.hpp>
#include <spdlog/fmt/fmt.h>
#include <curl/curl.h>
#include <string>
#include <thread>

using json = nlohmann::json;

static std::string GWebhookURL;
static bool bInitialized = false;

namespace Discord
{

static void SendWebhook(const json& Payload)
{
    if (!bInitialized || GWebhookURL.empty()) return;

    // Send in background thread to avoid blocking game loop
    std::string PayloadStr = Payload.dump();
    std::string URL = GWebhookURL;

    std::thread([URL, PayloadStr]() {
        CURL* Curl = curl_easy_init();
        if (!Curl) return;

        struct curl_slist* Headers = nullptr;
        Headers = curl_slist_append(Headers, "Content-Type: application/json");

        curl_easy_setopt(Curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(Curl, CURLOPT_HTTPHEADER, Headers);
        curl_easy_setopt(Curl, CURLOPT_POSTFIELDS, PayloadStr.c_str());
        curl_easy_setopt(Curl, CURLOPT_TIMEOUT, 10L);

        CURLcode Res = curl_easy_perform(Curl);
        if (Res != CURLE_OK)
        {
            // Can't use LOG_ERROR here safely from a detached thread
            // fprintf(stderr, "Discord webhook failed: %s\n", curl_easy_strerror(Res));
        }

        curl_slist_free_all(Headers);
        curl_easy_cleanup(Curl);
    }).detach();
}

bool Initialize(const std::string& WebhookURL)
{
    if (WebhookURL.empty()) return false;

    CURLcode Res = curl_global_init(CURL_GLOBAL_ALL);
    if (Res != CURLE_OK)
    {
        LOG_ERROR(LogDev, "Failed to initialize curl for Discord");
        return false;
    }

    GWebhookURL = WebhookURL;
    bInitialized = true;
    return true;
}

void Shutdown()
{
    if (bInitialized)
    {
        curl_global_cleanup();
        bInitialized = false;
    }
}

void SendMatchStarted(int32 PlayerCount)
{
    SendEmbed("🎮 Match Started",
        fmt::format("**{}** players in the match", PlayerCount),
        0x00FF00);
}

void SendMatchEnded(const std::string& WinnerName, int32 Kills)
{
    SendEmbed("🏆 Victory Royale!",
        fmt::format("**{}** won with **{}** eliminations", WinnerName, Kills),
        0xFFD700);
}

void SendPlayerJoined(const std::string& PlayerName, int32 CurrentCount, int32 MaxCount)
{
    json Payload;
    Payload["content"] = fmt::format("📥 **{}** joined ({}/{})", PlayerName, CurrentCount, MaxCount);
    SendWebhook(Payload);
}

void SendPlayerKill(const std::string& Killer, const std::string& Victim, const std::string& Weapon)
{
    json Payload;
    Payload["content"] = fmt::format("💀 **{}** eliminated **{}** ({})", Killer, Victim, Weapon);
    SendWebhook(Payload);
}

void SendServerStatus(const std::string& Status)
{
    SendEmbed("🖥️ Server Status", Status, 0x3498DB);
}

void SendEmbed(const std::string& Title, const std::string& Description, int32 Color)
{
    json Embed;
    Embed["title"] = Title;
    Embed["description"] = Description;
    Embed["color"] = Color;

    json Payload;
    Payload["embeds"] = json::array({Embed});
    SendWebhook(Payload);
}

} // namespace Discord

#endif // WITH_DISCORD
