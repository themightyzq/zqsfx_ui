// UpdateTests: version maths, Version file parsing, the update decision, the shared settings
// file, the LogoMark badge, and an end-to-end check through UpdateNotice (file:// Version file).
// Exit 0 on pass, 1 on any failure.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <zqsfx_ui/zqsfx_ui.h>

using namespace zqsfx::ui;

namespace
{
int failures = 0;
void check (bool ok, const char* what)
{
    std::cout << (ok ? "PASS " : "FAIL ") << what << "\n";
    if (! ok) ++failures;
}

const char* goodFile = R"({"format": 1, "futureField": [1,2], "products": {
  "worldizer": {"name": "Worldizer", "version": "0.0.12", "extra": true,
    "get": {"mac": "https://example.com/mac", "windows": "https://example.com/win",
            "linux": "https://example.com/linux", "bsd": "x"}},
  "badlink": {"name": "Bad", "version": "1.0.0", "get": {"mac": "http://example.com/x", "windows": "http://example.com/x", "linux": "http://example.com/x"}}
}})";

juce::String expectedLink()
{
   #if JUCE_MAC
    return "https://example.com/mac";
   #elif JUCE_WINDOWS
    return "https://example.com/win";
   #else
    return "https://example.com/linux";
   #endif
}

constexpr juce::int64 day = update::dayMs;
constexpr juce::int64 now = 10 * day;

void pump (int ms) { juce::MessageManager::getInstance()->runDispatchLoopUntil (ms); }

template <typename Pred>
bool waitFor (Pred pred, int timeoutMs = 5000)
{
    const auto deadline = juce::Time::getMillisecondCounter() + (juce::uint32) timeoutMs;
    while (! pred() && juce::Time::getMillisecondCounter() < deadline)
        pump (10);
    return pred();
}

juce::File makeTemp (const juce::String& prefix)
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile (prefix + juce::String (juce::Random::getSystemRandom().nextInt64()));
}

int fileCount (const juce::File& f) { return f.getNumberOfChildFiles (juce::File::findFilesAndDirectories); }
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    // ---- versions
    check (compareVersions ("1.2", "1.2.0") == 0, "1.2 equals 1.2.0");
    check (compareVersions ("1.2.1", "1.2") > 0, "longer with a nonzero tail is newer");
    check (compareVersions ("0.10.1", "0.9.0") > 0, "0.10.1 is newer than 0.9.0 (numeric, not text)");
    check (compareVersions ("0.9.0", "0.10.1") < 0, "0.9.0 is older than 0.10.1");
    check (compareVersions ("v1.0.0", "1.0.0") == 0, "leading v is allowed");
    check (! isValidVersion ("1.2-beta") && ! isValidVersion ("") && ! isValidVersion ("1..2") && ! isValidVersion ("a.b"),
           "non-numeric, empty and gapped versions are invalid");
    check (! isNewerVersion ("banana", "1.0.0") && ! isNewerVersion ("2.0.0", "banana"), "invalid is never newer");
    check (isNewerVersion ("2.0.0", "1.9.9"), "2.0.0 is newer than 1.9.9");

    // ---- Version file
    {
        auto v = parseVersionFile (goodFile, "worldizer");
        check (v.has_value() && v->name == "Worldizer" && v->version == "0.0.12", "good file: name and version read");
        check (v.has_value() && v->getLink == expectedLink(), "good file: this platform's link chosen");
        check (! parseVersionFile (goodFile, "nothere").has_value(), "missing product gives nothing");
        check (! parseVersionFile (goodFile, "badlink").has_value(), "non-https link is rejected");
        check (! parseVersionFile (juce::String (goodFile).replace ("\"format\": 1", "\"format\": 2"), "worldizer").has_value(),
               "wrong format number gives nothing");
        check (! parseVersionFile ("{not json", "worldizer").has_value(), "malformed JSON gives nothing");
        check (! parseVersionFile ("", "worldizer").has_value(), "empty text gives nothing");
        check (isVersionFile (goodFile) && ! isVersionFile ("[]"), "isVersionFile accepts only format-1 files");
    }

    // ---- decision
    {
        auto d = [] (bool en, juce::int64 last, const char* cached, const char* cur, juce::int64 later, const char* skip)
        { return decideUpdate (en, last, now, cached, "worldizer", cur, later, skip); };

        auto off = d (false, 0, goodFile, "0.0.1", 0, "");
        check (! off.shouldFetch && ! off.show.has_value(), "disabled: no fetch, nothing shown");
        check (d (true, 0, "", "0.0.1", 0, "").shouldFetch, "never checked: fetch");
        check (d (true, now - day, "", "0.0.1", 0, "").shouldFetch, "exactly a day ago: fetch");
        check (! d (true, now - day + 1, "", "0.0.1", 0, "").shouldFetch, "under a day ago: no fetch");
        check (d (true, now + 5, "", "0.0.1", 0, "").shouldFetch, "last check in the future: fetch");
        check (d (true, now, goodFile, "0.0.11", 0, "").show.has_value(), "newer version is shown");
        check (! d (true, now, goodFile, "0.0.12", 0, "").show.has_value(), "same version is not shown");
        check (! d (true, now, goodFile, "0.1.0", 0, "").show.has_value(), "older offer is not shown");
        check (! d (true, now, "", "0.0.11", 0, "").show.has_value(), "no cached file shows nothing");
        check (! d (true, now, goodFile, "0.0.11", now + 1000, "").show.has_value(), "Later hides");
        check (! d (true, now, goodFile, "0.0.11", now + day - 1000, "").show.has_value(), "Later 23 h ahead hides");
        check (d (true, now, goodFile, "0.0.11", now + day + 60 * 60 * 1000, "").show.has_value(),
               "Later 25 h ahead counts as expired (clock set back): offer shown");
        check (! d (true, now, goodFile, "0.0.11", now + 60 * 60 * 1000, "").show.has_value(), "Later 1 h ahead hides");
        check (d (true, now, goodFile, "0.0.11", now, "").show.has_value(), "Later expires when its time arrives");
        check (! d (true, now, goodFile, "0.0.11", 0, "0.0.12").show.has_value(), "Skip hides that version");
        check (d (true, now, goodFile, "0.0.11", 0, "0.0.11").show.has_value(), "Skip of an older version still shows a newer one");
    }

    // ---- settings round trip (a folder, one file per setting)
    {
        auto tmp = makeTemp ("zqsfx_update_tests_");
        auto folder = tmp.getChildFile ("sub").getChildFile ("Updates");
        {
            UpdateSettings s (folder);
            check (! tmp.exists(), "settings: nothing created before the first write");
            check (s.isEnabled() && s.getLastCheckMs() == 0 && s.getCachedFile().isEmpty(), "settings: defaults");
            check (! tmp.exists(), "settings: reading creates nothing");
            s.setEnabled (false);
            juce::String text (goodFile);
            s.recordCheck (123456789012345LL, &text);
            s.setLaterUntilMs ("worldizer", 99999999999LL);
            s.setSkippedVersion ("worldizer", "0.0.12");
            check (folder.getChildFile ("enabled").existsAsFile() && folder.getChildFile ("last-check").existsAsFile()
                       && folder.getChildFile ("version-file.json").existsAsFile()
                       && folder.getChildFile ("later.worldizer").existsAsFile()
                       && folder.getChildFile ("skip.worldizer").existsAsFile(),
                   "settings: folder and one file per setting created on write");

            UpdateSettings again (folder);
            check (! again.isEnabled(), "settings: enabled=false persists");
            check (again.getLastCheckMs() == 123456789012345LL, "settings: int64 lastCheckMs round-trips");
            check (again.getCachedFile() == text, "settings: cached file round-trips");
            check (again.getLaterUntilMs ("worldizer") == 99999999999LL && again.getLaterUntilMs ("other") == 0,
                   "settings: later is per product");
            check (again.getSkippedVersion ("worldizer") == "0.0.12", "settings: skip round-trips");

            again.recordCheck (5, nullptr);
            check (again.getLastCheckMs() == 5 && again.getCachedFile() == text, "settings: failure keeps the cached file");
            check (folder.findChildFiles (juce::File::findFiles, false, "*.tmp").isEmpty(), "settings: no temp files left behind");

            // an invalid id is neither written nor read
            const int before = fileCount (folder);
            const auto planted = folder.getChildFile ("x"); // what "later.Bad/../x" would resolve to
            planted.replaceWithText ("999");
            const int beforePlanted = fileCount (folder);
            again.setLaterUntilMs ("Bad/../x", 12345);
            again.setSkippedVersion ("Bad/../x", "9.9.9");
            again.setLaterUntilMs ("", 1);
            again.setLaterUntilMs ("UPPER", 1);
            check (fileCount (folder) == beforePlanted && beforePlanted == before + 1, "settings: invalid id writes no file");
            check (planted.loadFileAsString() == "999", "settings: invalid id leaves a file it would have resolved to alone");
            check (again.getLaterUntilMs ("Bad/../x") == 0 && again.getSkippedVersion ("Bad/../x").isEmpty(),
                   "settings: invalid id reads as unset");
            check (isValidProductId ("worldizer2") && ! isValidProductId ("Bad/../x") && ! isValidProductId ("a.b") && ! isValidProductId (""),
                   "isValidProductId accepts only [a-z0-9]+");
        }
        tmp.deleteRecursively();
    }

    // ---- two settings objects on one folder keep each other's keys
    {
        auto tmp = makeTemp ("zqsfx_update_two_");
        auto folder = tmp.getChildFile ("Updates");
        UpdateSettings a (folder), b (folder);
        juce::String text (goodFile);
        a.setEnabled (false);
        b.recordCheck (777, &text);
        b.setLaterUntilMs ("worldizer", 42);
        a.setSkippedVersion ("worldizer", "0.0.12");
        UpdateSettings c (folder);
        check (! c.isEnabled() && c.getLastCheckMs() == 777 && c.getCachedFile() == text
                   && c.getLaterUntilMs ("worldizer") == 42 && c.getSkippedVersion ("worldizer") == "0.0.12",
               "two writers: different keys both survive");
        tmp.deleteRecursively();
    }

    // ---- garbled files
    {
        auto tmp = makeTemp ("zqsfx_update_garbled_");
        tmp.createDirectory();
        tmp.getChildFile ("enabled").replaceWithText ("banana \x01\x02 !!");
        tmp.getChildFile ("last-check").replaceWithText ("not-a-number \xff\xfe");
        tmp.getChildFile ("later.worldizer").replaceWithText ("???");
        tmp.getChildFile ("version-file.json").replaceWithText ("{ broken");
        UpdateSettings s (tmp);
        check (s.isEnabled(), "garbled: enabled reads as on");
        check (s.getLastCheckMs() == 0, "garbled: last-check reads as 0");
        check (s.getLaterUntilMs ("worldizer") == 0, "garbled: later reads as 0");
        check (! decideUpdate (s.isEnabled(), s.getLastCheckMs(), now, s.getCachedFile(), "worldizer", "0.0.1", 0, "").show.has_value(),
               "garbled: cached file shows nothing");
        tmp.getChildFile ("enabled").replaceWithText ("");
        check (s.isEnabled(), "garbled: empty enabled reads as on");
        tmp.deleteRecursively();
    }

    // ---- logo badge
    {
        LogoMark logo ("Worldizer");
        check (logo.getTitle() == "About Worldizer" && ! logo.isUpdateAvailable(), "logo: plain title by default");
        logo.setUpdateAvailable (true);
        check (logo.isUpdateAvailable() && logo.getTitle() == "About Worldizer (update available)"
                   && logo.getTooltip() == logo.getTitle() && logo.getDescription() == logo.getTitle()
                   && logo.getHelpText() == logo.getTitle(),
               "logo: badge on changes tooltip, title, description, help text");
        logo.setUpdateAvailable (false);
        check (logo.getTitle() == "About Worldizer" && logo.getTooltip() == "About Worldizer", "logo: badge off reverts");
        logo.setSize (24, 24);

        auto render = [&logo] (bool badge)
        {
            logo.setUpdateAvailable (badge);
            juce::Image img (juce::Image::ARGB, 24, 24, true);
            juce::Graphics g (img);
            logo.paintEntireComponent (g, false);
            return img;
        };
        const auto off = render (false).getPixelAt (22, 3);
        const auto on  = render (true).getPixelAt (22, 3);
        const juce::Colour accent = colour::accent;
        check (off != on, "logo: top-right pixel differs with the badge on");
        auto near = [] (juce::uint8 x, juce::uint8 y) { return std::abs ((int) x - (int) y) <= 40; };
        check (on.getAlpha() > 200 && near (on.getRed(), accent.getRed()) && near (on.getGreen(), accent.getGreen())
                   && near (on.getBlue(), accent.getBlue()),
               "logo: badge pixel is close to the accent colour");
    }

    // ---- end to end: file:// Version file -> checker -> notice -> badge
    {
        auto dir = makeTemp ("zqsfx_update_e2e_");
        dir.createDirectory();
        auto versionFile = dir.getChildFile ("versions.json");
        versionFile.replaceWithText (goodFile);
        auto settingsDir = dir.getChildFile ("Updates");

        UpdateChecker::testOverride().url = juce::URL (versionFile).toString (true);
        UpdateChecker::testOverride().settingsFolder = settingsDir;
        UpdateChecker::testFetchCount() = 0;
        UpdateChecker::testHoldFetch() = true;

        {
            LogoMark logo ("Worldizer"), logo2 ("Worldizer");
            UpdateNotice notice (logo, "worldizer", "0.0.11");
            std::unique_ptr<UpdateNotice> notice2;
            check (! logo.isUpdateAvailable(), "e2e: no badge at construction");
            // Wait for the fetch thread rather than a fixed 200 ms: on a loaded machine it can
            // start later than that (seen in the commit gate while a product was building).
            const bool started = waitFor ([] { return UpdateChecker::testFetchCount() == 1; });
            pump (50);
            check (started && ! logo.isUpdateAvailable() && UpdateChecker::testFetchCount() == 1,
                   "e2e: no badge while the fetch is held (and one fetch started)");
            UpdateChecker::testHoldFetch() = false;

            check (waitFor ([&] { return logo.isUpdateAvailable(); }), "e2e: badge comes on after the background check");
            check (notice.available().has_value() && notice.available()->version == "0.0.12", "e2e: offer names 0.0.12");

            // once a day: a second notice after the fetch finished must not fetch again
            notice2 = std::make_unique<UpdateNotice> (logo2, "worldizer", "0.0.11");
            pump (200);
            check (UpdateChecker::testFetchCount() == 1, "once a day: two requestCheck calls make one fetch");
            check (logo2.isUpdateAvailable(), "e2e: a second notice shows the offer from the cache");

            UpdateNoticePanel panel (notice);
            panel.setSize (320, 200);
            const int h320 = panel.getIdealHeight();
            panel.setSize (320, h320);
            check (h320 > geom::minHitTargetPx, "e2e: panel grows to show the offer");

            // narrow panel: buttons stack
            {
                panel.setSize (200, 200);
                const int h200 = panel.getIdealHeight();
                panel.setSize (200, h200);
                check (h200 > h320, "narrow panel: ideal height grows when the buttons stack");
                int buttons = 0;
                bool tall = true, wide = true;
                for (auto* c : panel.getChildren())
                    if (auto* b = dynamic_cast<juce::TextButton*> (c))
                    {
                        ++buttons;
                        tall = tall && b->getHeight() >= geom::minHitTargetPx;
                        const auto font = b->getLookAndFeel().getTextButtonFont (*b, b->getHeight());
                        const int textW = juce::GlyphArrangement::getStringWidthInt (font, b->getButtonText());
                        if (b->getWidth() < textW + 8)
                        {
                            wide = false;
                            std::cout << "  button '" << b->getButtonText() << "' is " << b->getWidth() << " wide, text " << textW << "\n";
                        }
                    }
                check (buttons == 3 && tall, "narrow panel: three buttons, each at least the minimum hit target tall");
                check (wide, "narrow panel: no button narrower than its text plus 8 px");
                panel.setSize (320, h320);
            }

            notice.later();
            check (! logo.isUpdateAvailable() && ! notice.available().has_value(), "e2e: Later hides the badge");
            check (panel.getIdealHeight() < 3 * geom::minHitTargetPx, "e2e: panel shrinks when the offer is hidden");
            pump (200);
            check (! logo2.isUpdateAvailable(), "e2e: Later in one notice hides the other notice's logo");

            UpdateSettings view (settingsDir);
            check (view.getLaterUntilMs ("worldizer") > juce::Time::currentTimeMillis(), "e2e: Later was stored");
            view.setLaterUntilMs ("worldizer", 0);
            notice.refresh();
            notice2->refresh();
            check (logo.isUpdateAvailable() && logo2.isUpdateAvailable(), "e2e: refresh shows it again once Later is cleared");

            notice.skipThisVersion();
            check (! logo.isUpdateAvailable() && view.getSkippedVersion ("worldizer") == "0.0.12", "e2e: Skip hides and stores");
            pump (200);
            check (! logo2.isUpdateAvailable(), "e2e: Skip in one notice hides the other notice's logo");

            view.setSkippedVersion ("worldizer", "");
            notice.refresh();
            notice2->refresh();
            check (logo.isUpdateAvailable() && logo2.isUpdateAvailable(), "e2e: refresh shows it again once Skip is cleared");

            notice.setEnabled (false);
            check (! UpdateSettings (settingsDir).isEnabled() && ! notice.isEnabled(), "e2e: switch off is stored");
            pump (200);
            check (! logo.isUpdateAvailable() && ! logo2.isUpdateAvailable(), "e2e: switch off clears both notices' logos");
        }

        UpdateChecker::testHoldFetch() = false;
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- 64 KB cap
    {
        auto dir = makeTemp ("zqsfx_update_big_");
        dir.createDirectory();
        auto big = dir.getChildFile ("versions.json");
        big.replaceWithText (juce::String (goodFile).replace ("\"format\": 1,", "\"format\": 1, \"pad\": \"" + juce::String::repeatedString ("x", 70 * 1024) + "\","));
        check (big.getSize() > 70 * 1024 && juce::JSON::parse (big.loadFileAsString()).getProperty ("format", {}) == juce::var (1)
               && ! isVersionFile (big.loadFileAsString()), "cap: the 70 KB file is valid JSON but too big to be a Version file");
        auto settingsDir = dir.getChildFile ("Updates");
        UpdateChecker::testOverride().url = juce::URL (big).toString (true);
        UpdateChecker::testOverride().settingsFolder = settingsDir;
        UpdateChecker::testFetchCount() = 0;
        {
            LogoMark logo ("Worldizer");
            UpdateNotice notice (logo, "worldizer", "0.0.11");
            juce::SharedResourcePointer<UpdateChecker> c;
            check (waitFor ([&] { return UpdateChecker::testFetchCount() == 1 && ! c->isFetching(); }), "cap: the fetch finished");
            pump (100);
            check (! settingsDir.getChildFile ("version-file.json").exists(), "cap: an oversize file is not cached");
            check (UpdateSettings (settingsDir).getLastCheckMs() > 0, "cap: the check time is still stamped");
            check (! logo.isUpdateAvailable(), "cap: no badge from an oversize file");
        }
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- switch off during a fetch
    {
        auto dir = makeTemp ("zqsfx_update_cancel_");
        dir.createDirectory();
        auto versionFile = dir.getChildFile ("versions.json");
        versionFile.replaceWithText (goodFile);
        auto settingsDir = dir.getChildFile ("Updates");
        UpdateChecker::testOverride().url = juce::URL (versionFile).toString (true);
        UpdateChecker::testOverride().settingsFolder = settingsDir;
        UpdateChecker::testFetchCount() = 0;
        UpdateChecker::testHoldFetch() = true;
        {
            LogoMark logo ("Worldizer");
            UpdateNotice notice (logo, "worldizer", "0.0.11");
            juce::SharedResourcePointer<UpdateChecker> c;
            check (waitFor ([] { return UpdateChecker::testFetchCount() == 1; }) && c->isFetching(), "cancel: a fetch is under way");
            notice.setEnabled (false);
            UpdateChecker::testHoldFetch() = false;
            check (waitFor ([&] { return ! c->isFetchThreadRunning(); }, 3000), "cancel: the fetch thread ends");
            pump (100);
            check (! settingsDir.getChildFile ("version-file.json").exists(), "cancel: nothing cached after switching off");
            check (UpdateSettings (settingsDir).getLastCheckMs() == 0, "cancel: the day's claim is handed back");
            check (! logo.isUpdateAvailable(), "cancel: no badge after switching off");
        }
        UpdateChecker::testHoldFetch() = false;
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- hung server: teardown must not wait for it
    {
        auto dir = makeTemp ("zqsfx_update_hung_");
        dir.createDirectory();
        juce::StreamingSocket listener; // listens, never accepts or answers
        const bool listening = listener.createListener (0, "127.0.0.1");
        check (listening && listener.getBoundPort() > 0, "hung: listener opened");
        if (listening)
        {
            UpdateChecker::testOverride().url = "http://127.0.0.1:" + juce::String (listener.getBoundPort()) + "/";
            UpdateChecker::testOverride().settingsFolder = dir.getChildFile ("Updates");
            UpdateChecker::testFetchCount() = 0;
            auto logo = std::make_unique<LogoMark> ("Worldizer");
            auto notice = std::make_unique<UpdateNotice> (*logo, "worldizer", "0.0.11");
            check (waitFor ([] { return UpdateChecker::testFetchCount() == 1; }), "hung: a fetch started");
            pump (200);
            {
                juce::SharedResourcePointer<UpdateChecker> peek; // the notice still holds one too
                check (peek->isFetching(), "hung: the fetch is still blocked when teardown starts");
            }
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            notice.reset(); // last holder: the checker is destroyed here
            const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
            std::cout << "hung: teardown took " << ms << " ms\n";
            // The fault this catches is teardown waiting out the 6 s join; 2 s leaves room for a
            // loaded machine (500 ms failed under load) while still telling the two apart.
            check (ms < 2000.0, "hung: destroying the last notice returns in under 2 s");
            logo.reset();
        }
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- deeply nested Version file: refused before the parser, on a thread with a small stack
    {
        const auto deep = juce::String::repeatedString ("[", 60 * 1024);
        const auto deepObj = "{\"format\": 1, \"products\": " + juce::String::repeatedString ("{\"a\":", 5000) + "1"
                             + juce::String::repeatedString ("}", 5000) + "}";
        struct Parser : juce::Thread
        {
            explicit Parser (juce::String t) : juce::Thread ("deep json"), text (std::move (t)) {}
            void run() override { accepted = isVersionFile (text) || parseVersionFile (text, "a").has_value(); }
            juce::String text;
            std::atomic<bool> accepted { true };
        };
        for (auto* t : { &deep, &deepObj })
        {
            Parser p (*t);
            p.startThread();
            check (p.waitForThreadToExit (5000) && ! p.accepted, "deep nesting: refused without crashing a background thread");
        }
        check (detail::isNestingShallow (goodFile) && ! detail::isNestingShallow (juce::String::repeatedString ("[", 33)),
               "deep nesting: a real file passes, 33 levels do not");
        check (detail::isNestingShallow ("{\"s\": \"[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[\\\"\"}"),
               "deep nesting: brackets inside a string do not count");
    }

    // ---- invalid product id: nothing is offered, because Later and Skip could never hide it
    {
        auto dir = makeTemp ("zqsfx_update_badid_");
        dir.createDirectory();
        auto versionFile = dir.getChildFile ("versions.json");
        versionFile.replaceWithText (juce::String (goodFile).replace ("\"worldizer\":", "\"Worldizer\":"));
        UpdateChecker::testOverride().url = juce::URL (versionFile).toString (true);
        UpdateChecker::testOverride().settingsFolder = dir.getChildFile ("Updates");
        UpdateChecker::testFetchCount() = 0;
        {
            LogoMark logo ("Worldizer");
            UpdateNotice notice (logo, "Worldizer", "0.0.11");
            juce::SharedResourcePointer<UpdateChecker> c;
            check (waitFor ([&] { return UpdateChecker::testFetchCount() == 1 && ! c->isFetching(); }), "bad id: the check ran");
            pump (100);
            notice.refresh();
            check (parseVersionFile (versionFile.loadFileAsString(), "Worldizer").has_value(), "bad id: the file does name it");
            check (! logo.isUpdateAvailable() && ! notice.available().has_value(), "bad id: no badge and no offer");
        }
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- two binaries: a second checker (as in another plugin) sees the first one's claim
    {
        auto dir = makeTemp ("zqsfx_update_claim_");
        dir.createDirectory();
        auto versionFile = dir.getChildFile ("versions.json");
        versionFile.replaceWithText (goodFile);
        UpdateChecker::testOverride().url = juce::URL (versionFile).toString (true);
        UpdateChecker::testOverride().settingsFolder = dir.getChildFile ("Updates");
        UpdateChecker::testFetchCount() = 0;
        UpdateChecker::testHoldFetch() = true;
        {
            UpdateChecker a, b; // separate objects, sharing only the settings folder
            a.requestCheck();
            check (waitFor ([] { return UpdateChecker::testFetchCount() == 1; }), "claim: the first checker is fetching");
            b.requestCheck();
            pump (200);
            check (UpdateChecker::testFetchCount() == 1 && ! b.isFetching(), "claim: a second binary skips while the first is fetching");
            UpdateChecker::testHoldFetch() = false;
            check (waitFor ([&] { return ! a.isFetching(); }), "claim: the first fetch finishes");
            UpdateChecker c;
            c.requestCheck();
            pump (200);
            check (UpdateChecker::testFetchCount() == 1, "claim: a checker created later the same day skips");
        }
        UpdateChecker::testHoldFetch() = false;
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    // ---- settings folder that cannot be written: no crash, still at most one fetch
    {
        auto dir = makeTemp ("zqsfx_update_ro_");
        dir.createDirectory();
        auto versionFile = dir.getChildFile ("versions.json");
        versionFile.replaceWithText (goodFile);
        auto blocker = dir.getChildFile ("blocker");
        blocker.replaceWithText ("a file where the folder's parent should be");
        UpdateChecker::testOverride().url = juce::URL (versionFile).toString (true);
        UpdateChecker::testOverride().settingsFolder = blocker.getChildFile ("Updates");
        UpdateChecker::testFetchCount() = 0;
        {
            UpdateChecker c;
            c.requestCheck();
            check (waitFor ([&] { return UpdateChecker::testFetchCount() == 1 && ! c.isFetching(); }), "unwritable: the check ran");
            c.requestCheck();
            pump (200);
            check (UpdateChecker::testFetchCount() == 1, "unwritable: a second request the same day does not fetch again");
            check (! blocker.getChildFile ("Updates").exists(), "unwritable: nothing was written");
        }
        UpdateChecker::testOverride() = {};
        dir.deleteRecursively();
    }

    std::cout << "UpdateTests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
