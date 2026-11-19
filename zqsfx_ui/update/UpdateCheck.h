#pragma once
// Update notice, logic half: version maths, the Version file reader, the shared settings file,
// the once-a-day decision, and the background checker. Nothing here downloads or installs a
// product; it only learns whether a newer version exists and keeps the user's answers
// (Later, Skip, off). components/UpdateNotice.h is the UI half.
//
// Origin: new to this module. The Version file is a small JSON file on the web that the ZQ SFX
// site publishes; this header only ever reads it. A failure of any kind (offline, timeout,
// blocked network, bad file) is silent: the caller sees "no information", never an error.

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>
#include <optional>
#include <vector>

namespace zqsfx::ui
{
namespace update
{
    constexpr juce::int64 dayMs         = 24LL * 60 * 60 * 1000;
    constexpr int         maxFileBytes  = 64 * 1024;   // a bigger answer is treated as no answer
    constexpr int         connectTimeoutMs = 5000;
    inline const char*    defaultUrl = "https://themightyzq.github.io/audio-tools/versions.json";
} // namespace update

// ---- versions -------------------------------------------------------------------------------

namespace detail
{
    // "v1.2.3" / "1.2" -> {1,2,3}. Only digits and single dots are allowed, so "1.2-beta",
    // "1..2" and "" are invalid. Nine digits a part keeps the maths inside int64.
    inline bool parseVersionParts (const juce::String& text, std::vector<juce::int64>& out)
    {
        auto t = text.trim();
        if (t.startsWithChar ('v') || t.startsWithChar ('V'))
            t = t.substring (1);

        out.clear();
        juce::int64 current = 0;
        int digits = 0;

        for (auto c : t)
        {
            if (c == '.')
            {
                if (digits == 0)
                    return false;
                out.push_back (current);
                current = 0;
                digits = 0;
            }
            else if (c >= '0' && c <= '9')
            {
                if (++digits > 9)
                    return false;
                current = current * 10 + (juce::int64) (c - '0');
            }
            else
                return false;
        }

        if (digits == 0)
            return false;
        out.push_back (current);
        return true;
    }
} // namespace detail

inline bool isValidVersion (const juce::String& v)
{
    std::vector<juce::int64> parts;
    return detail::parseVersionParts (v, parts);
}

// -1, 0 or 1 like strcmp. Missing trailing parts count as zero ("1.2" == "1.2.0"). If either
// side is invalid the answer is 0, so an invalid string is never ordered above anything.
inline int compareVersions (const juce::String& a, const juce::String& b)
{
    std::vector<juce::int64> pa, pb;
    if (! detail::parseVersionParts (a, pa) || ! detail::parseVersionParts (b, pb))
        return 0;

    const auto n = juce::jmax (pa.size(), pb.size());
    for (size_t i = 0; i < n; ++i)
    {
        const auto x = i < pa.size() ? pa[i] : 0;
        const auto y = i < pb.size() ? pb[i] : 0;
        if (x != y)
            return x > y ? 1 : -1;
    }
    return 0;
}

inline bool isNewerVersion (const juce::String& offered, const juce::String& current)
{
    return isValidVersion (offered) && isValidVersion (current) && compareVersions (offered, current) > 0;
}

// ---- the Version file -----------------------------------------------------------------------

struct VersionInfo
{
    juce::String name, version, getLink;
};

namespace detail
{
    inline const char* platformKey()
    {
       #if JUCE_MAC
        return "mac";
       #elif JUCE_WINDOWS
        return "windows";
       #else
        return "linux";
       #endif
    }

    // JUCE's JSON parser recurses once per nesting level with no limit, so a file like "[[[[..."
    // (3 KB is enough on a 512 KB thread stack) would crash the host. A real Version file is
    // four levels deep; anything past maxDepth is refused before the parser sees it.
    inline bool isNestingShallow (const juce::String& json, int maxDepth = 32)
    {
        int depth = 0;
        bool inString = false, escaped = false;
        for (auto c : json)
        {
            if (inString)
            {
                if (escaped)          escaped = false;
                else if (c == '\\')   escaped = true;
                else if (c == '"')    inString = false;
            }
            else if (c == '"')                  inString = true;
            else if (c == '{' || c == '[')    { if (++depth > maxDepth) return false; }
            else if (c == '}' || c == ']')      --depth;
        }
        return true;
    }

    // Parses only text that is safe to parse; anything else gives a void var.
    inline juce::var parseJsonSafely (const juce::String& json)
    {
        juce::var root;
        if (json.isEmpty() || json.length() > update::maxFileBytes || ! isNestingShallow (json)
            || juce::JSON::parse (json, root).failed())
            return {};
        return root;
    }

    // The "products" object of a well-formed format-1 file, else null.
    inline juce::DynamicObject* findProducts (const juce::var& root)
    {
        if (root.getDynamicObject() == nullptr)
            return nullptr;
        const auto format = root.getProperty ("format", juce::var());
        if (! (format.isInt() || format.isInt64() || format.isDouble()) || (double) format != 1.0)
            return nullptr;
        return root.getProperty ("products", juce::var()).getDynamicObject();
    }
} // namespace detail

// True when the text is a format-1 Version file (so it is safe to cache).
inline bool isVersionFile (const juce::String& json)
{
    return detail::findProducts (detail::parseJsonSafely (json)) != nullptr;
}

// The entry for productId with this platform's link, or nothing. Unknown fields are ignored so a
// newer file can add some. A link that is not https:// makes the entry unusable (no information).
inline std::optional<VersionInfo> parseVersionFile (const juce::String& json, const juce::String& productId)
{
    if (json.isEmpty() || productId.isEmpty())
        return std::nullopt;

    const auto root = detail::parseJsonSafely (json); // keeps the objects below alive
    auto* products = detail::findProducts (root);
    if (products == nullptr)
        return std::nullopt;

    auto* product = products->getProperty (juce::Identifier (productId)).getDynamicObject();
    if (product == nullptr)
        return std::nullopt;

    const auto version = product->getProperty ("version");
    const auto get     = product->getProperty ("get").getDynamicObject();
    if (! version.isString() || get == nullptr)
        return std::nullopt;

    const auto link = get->getProperty (juce::Identifier (detail::platformKey()));
    if (! link.isString() || ! link.toString().startsWith ("https://") || link.toString().length() <= 8)
        return std::nullopt;

    const auto versionText = version.toString().trim();
    if (! isValidVersion (versionText))
        return std::nullopt;

    const auto name = product->getProperty ("name");
    return VersionInfo { name.isString() && name.toString().isNotEmpty() ? name.toString() : productId,
                         versionText, link.toString() };
}

// ---- the decision ---------------------------------------------------------------------------

struct UpdateDecision
{
    bool shouldFetch = false;
    std::optional<VersionInfo> show;
};

// Due when never checked (lastCheckMs <= 0), a day has passed, or the clock reads earlier than
// the last check (a clock set back must not freeze the check forever).
inline bool isFetchDue (juce::int64 lastCheckMs, juce::int64 nowMs)
{
    return lastCheckMs <= 0 || lastCheckMs > nowMs || nowMs - lastCheckMs >= update::dayMs;
}

// Pure: no network, no disk, no clock. UpdateChecker feeds it the real values.
inline UpdateDecision decideUpdate (bool enabled, juce::int64 lastCheckMs, juce::int64 nowMs,
                                    const juce::String& cachedFile, const juce::String& productId,
                                    const juce::String& currentVersion, juce::int64 laterUntilMs,
                                    const juce::String& skippedVersion)
{
    UpdateDecision d;
    if (! enabled)
        return d;

    d.shouldFetch = isFetchDue (lastCheckMs, nowMs);

    auto info = parseVersionFile (cachedFile, productId);
    // A Later more than a day ahead can only come from a clock that was set back, so it counts as expired.
    const bool laterActive = nowMs < laterUntilMs && laterUntilMs - nowMs <= update::dayMs;
    if (! info || ! isNewerVersion (info->version, currentVersion) || laterActive)
        return d;

    // An unreadable skip entry is ignored rather than hiding updates for good.
    if (skippedVersion.isNotEmpty() && isValidVersion (skippedVersion)
        && ! isNewerVersion (info->version, skippedVersion))
        return d;

    d.show = std::move (info);
    return d;
}

// ---- the settings folder --------------------------------------------------------------------

// A product id is used in file names, so only lower-case letters and digits are allowed.
inline bool isValidProductId (const juce::String& id)
{
    if (id.isEmpty())
        return false;
    for (auto c : id)
        if (! ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')))
            return false;
    return true;
}

// The user's answers, shared by every ZQ SFX product on the computer. One tiny file per setting
// (enabled, last-check, version-file.json, later.<id>, skip.<id>) in one folder. A write goes
// to a temp file and is renamed over the target (an atomic replace on macOS and Linux), so a
// reader never sees half a file, two settings can never overwrite each other, and the same
// setting is last-writer-wins. There are no locks, so nothing here can block a host's UI.
// A missing or garbled file reads as unset. The folder is created on the first write only.
class UpdateSettings
{
public:
    UpdateSettings() : UpdateSettings (defaultFolder()) {}
    explicit UpdateSettings (const juce::File& settingsFolder) : folder (settingsFolder) {}

    static juce::File defaultFolder()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
       #if JUCE_MAC
        dir = dir.getChildFile ("Application Support"); // JUCE returns ~/Library here
       #endif
        return dir.getChildFile ("ZQ SFX").getChildFile ("Updates");
    }

    const juce::File& getFolder() const { return folder; }

    bool isEnabled() const                 { return read ("enabled").trim() != "0"; }
    void setEnabled (bool on)              { write ("enabled", on ? "1" : "0"); }

    juce::int64 getLastCheckMs() const     { return read ("last-check").trim().getLargeIntValue(); }
    juce::String getCachedFile() const     { return read ("version-file.json"); }

    // Stamps the check time; goodFileText (when the check succeeded) replaces the cached file.
    // The file goes first so a reader that sees the new stamp also sees the new file.
    void recordCheck (juce::int64 nowMs, const juce::String* goodFileText)
    {
        if (goodFileText != nullptr)
            write ("version-file.json", *goodFileText);
        write ("last-check", juce::String (nowMs));
    }

    juce::int64 getLaterUntilMs (const juce::String& id) const
    {
        return isValidProductId (id) ? read ("later." + id).trim().getLargeIntValue() : 0;
    }
    void setLaterUntilMs (const juce::String& id, juce::int64 ms)
    {
        if (isValidProductId (id))
            write ("later." + id, juce::String (ms));
    }

    juce::String getSkippedVersion (const juce::String& id) const
    {
        return isValidProductId (id) ? read ("skip." + id).trim() : juce::String();
    }
    void setSkippedVersion (const juce::String& id, const juce::String& version)
    {
        if (isValidProductId (id))
            write ("skip." + id, version);
    }

private:
    juce::String read (const juce::String& name) const
    {
        const auto f = folder.getChildFile (name);
        return f.existsAsFile() && f.getSize() <= update::maxFileBytes ? f.loadFileAsString() : juce::String();
    }

    void write (const juce::String& name, const juce::String& text) const
    {
        if (! folder.createDirectory().wasOk())
            return;
        // Uuid, not the shared juce::Random, which is not safe to use from two threads.
        const auto temp = folder.getChildFile (name + "." + juce::Uuid().toString() + ".tmp");
        if (! temp.replaceWithText (text, false, false, "\n"))
            return;
        // On Windows the replace fails while another program (often a virus scanner) holds the
        // target open, so try a few times before giving up. macOS and Linux rename atomically.
        for (int attempt = 0; attempt < 3; ++attempt)
        {
            if (temp.replaceFileIn (folder.getChildFile (name)))
                return;
            juce::Thread::sleep (15);
        }
        temp.deleteFile();
    }

    juce::File folder;
};

// ---- the checker ----------------------------------------------------------------------------

// One per plugin binary (juce::SharedResourcePointer<UpdateChecker>), so the editors of one
// binary share a fetch. Each binary may fetch at most once a day when an editor opens; the daily
// gate lives in the shared settings, so once any binary has stamped last-check the others skip.
// Listeners are told on the message thread when a check finishes or a user answer changes.
class UpdateChecker : public juce::ChangeBroadcaster
{
public:
    UpdateChecker() : settings (initialFolder()) {}
    ~UpdateChecker() override { retireFetcher (6000); }

    UpdateSettings& getSettings() { return settings; }

    // Message thread. Starts at most one background fetch, and only when the decision says one is due.
    void requestCheck()
    {
        if (isFetching() || ! settings.isEnabled())
            return;
        // The in-memory stamp keeps the once-a-day limit even when the settings folder cannot be written.
        const auto last = juce::jmax (settings.getLastCheckMs(), lastAttemptMs);
        if (! decideUpdate (true, last, juce::Time::currentTimeMillis(), {}, {}, {}, 0, {}).shouldFetch)
            return;

        retireFetcher (1000); // reap a finished one
        lastAttemptMs = juce::Time::currentTimeMillis();
        fetcher = std::make_unique<Fetcher> (settings, resolveUrl(), juce::WeakReference<UpdateChecker> (this));
        fetcher->startThread (juce::Thread::Priority::background);
    }

    // Stops a running fetch without writing anything (the "Check for updates" switch went off).
    void cancelFetch()
    {
        if (fetcher != nullptr)
            fetcher->cancel();
    }

    bool isFetching() const         { return fetcher != nullptr && ! fetcher->finished.load(); }
    bool isFetchThreadRunning() const { return fetcher != nullptr && fetcher->isThreadRunning(); }

    UpdateDecision evaluate (const juce::String& productId, const juce::String& currentVersion)
    {
        // Later and Skip can only be stored for a valid id, so an offer for any other id could
        // never be hidden. Show none.
        if (! isValidProductId (productId))
            return {};
        return decideUpdate (settings.isEnabled(), settings.getLastCheckMs(), juce::Time::currentTimeMillis(),
                             settings.getCachedFile(), productId, currentVersion,
                             settings.getLaterUntilMs (productId), settings.getSkippedVersion (productId));
    }

    // The address to read. ZQSFX_UPDATE_URL may replace the default, but only with an https:// one.
    static juce::String resolveUrl()
    {
       #if ZQSFX_UI_TESTING
        if (testOverride().url.isNotEmpty())
            return testOverride().url;
       #endif
        const auto env = juce::SystemStats::getEnvironmentVariable ("ZQSFX_UPDATE_URL", {}).trim();
        return env.startsWith ("https://") ? env : juce::String (update::defaultUrl);
    }

   #if ZQSFX_UI_TESTING
    // Test hooks, compiled only into the module's own tests. Set before the checker is created.
    struct TestOverride
    {
        juce::File settingsFolder;
        juce::String url; // any scheme, including http:// and file://
    };
    static TestOverride& testOverride() { static TestOverride o; return o; }
    static std::atomic<bool>& testHoldFetch() { static std::atomic<bool> h { false }; return h; }
    static std::atomic<int>&  testFetchCount() { static std::atomic<int> n { 0 }; return n; }
   #endif

private:
    static juce::File initialFolder()
    {
       #if ZQSFX_UI_TESTING
        if (testOverride().settingsFolder != juce::File())
            return testOverride().settingsFolder;
       #endif
        return UpdateSettings::defaultFolder();
    }

    // Holds copies only (never the checker) so a thread that outlives the checker is harmless.
    struct Fetcher : public juce::Thread
    {
        Fetcher (UpdateSettings s, juce::String u, juce::WeakReference<UpdateChecker> o)
            : juce::Thread ("ZQ SFX update check"), settings (std::move (s)), url (std::move (u)), owner (std::move (o)) {}

        // Wakes a read that is blocked on the network, and stops any later connect.
        void cancel()
        {
            signalThreadShouldExit();
            const juce::ScopedLock sl (streamLock);
            if (web != nullptr)
                web->cancel();
        }

        void run() override
        {
            // Re-checked here: another binary may have stamped last-check since requestCheck.
            const auto previousStamp = settings.getLastCheckMs();
            if (settings.isEnabled()
                && decideUpdate (true, previousStamp, juce::Time::currentTimeMillis(), {}, {}, {}, 0, {}).shouldFetch
                && ! threadShouldExit())
            {
                // Claim today's check before fetching, so plugins whose windows open at the same
                // moment (32 HyperPrism bundles, a session reopening) see it and skip.
                settings.recordCheck (juce::Time::currentTimeMillis(), nullptr);
               #if ZQSFX_UI_TESTING
                ++testFetchCount();
                while (testHoldFetch().load() && ! threadShouldExit())
                    juce::Thread::sleep (5);
               #endif
                juce::String text;
                const bool ok = ! threadShouldExit() && download (text) && isVersionFile (text);

                // A cancelled fetch writes no file and hands the day back, so the next window
                // that opens checks again. A failure keeps the claim, so an offline machine
                // retries tomorrow rather than at every window open.
                if (threadShouldExit())
                    settings.recordCheck (previousStamp, nullptr);
                else if (ok)
                    settings.recordCheck (juce::Time::currentTimeMillis(), &text);
            }

            finished = true;
            // After a cancel the checker (and perhaps the whole plugin) is going away: touch no
            // JUCE singletons from here on.
            if (threadShouldExit())
                return;
            auto weak = owner;
            juce::MessageManager::callAsync ([weak] { if (auto* c = weak.get()) c->sendChangeMessage(); });
        }

        bool download (juce::String& out)
        {
            // An ordinary GET: no query parameters and no custom headers. The server still sees
            // the computer's internet address and the host app's standard user agent.
            const juce::URL u (url);
            std::unique_ptr<juce::InputStream> plain;
            std::unique_ptr<juce::WebInputStream> webStream;
            juce::InputStream* in = nullptr;

            if (u.isLocalFile()) // only reachable through the test hook
            {
                plain = u.getLocalFile().createInputStream();
                in = plain.get();
            }
            else
            {
                webStream = std::make_unique<juce::WebInputStream> (u, false);
                webStream->withConnectionTimeout (update::connectTimeoutMs).withNumRedirectsToFollow (2);
                {
                    const juce::ScopedLock sl (streamLock);
                    web = webStream.get();
                }
                const bool connected = ! threadShouldExit() && webStream->connect (nullptr);
                const bool good = connected && ! webStream->isError() && webStream->getStatusCode() == 200;
                in = good ? webStream.get() : nullptr;
            }

            bool ok = in != nullptr;
            juce::MemoryBlock data;
            if (ok)
            {
                char buffer[4096];
                while (! threadShouldExit())
                {
                    const int n = in->read (buffer, (int) sizeof (buffer));
                    if (n <= 0)
                        break;
                    data.append (buffer, (size_t) n);
                    if (data.getSize() > (size_t) update::maxFileBytes)
                    {
                        ok = false;
                        break;
                    }
                }
                ok = ok && ! threadShouldExit();
            }

            {
                const juce::ScopedLock sl (streamLock); // cancel() can no longer reach a dying stream
                web = nullptr;
            }
            if (ok)
                out = juce::String::fromUTF8 (static_cast<const char*> (data.getData()), (int) data.getSize());
            return ok;
        }

        UpdateSettings settings;
        juce::String url;
        juce::WeakReference<UpdateChecker> owner;
        std::atomic<bool> finished { false };
        juce::CriticalSection streamLock;
        juce::WebInputStream* web = nullptr;
    };

    // Cancels and waits. Never kills: if the thread has not ended in time it is left to finish
    // on its own (it holds no pointer to this object), which costs one parked thread at worst.
    void retireFetcher (int waitMs)
    {
        if (fetcher == nullptr)
            return;
        fetcher->cancel();
        if (fetcher->waitForThreadToExit (waitMs))
            fetcher.reset();
        else
            (void) fetcher.release();
    }

    UpdateSettings settings;
    std::unique_ptr<Fetcher> fetcher;
    juce::int64 lastAttemptMs = 0; // message thread only

    JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateChecker)
};
} // namespace zqsfx::ui
