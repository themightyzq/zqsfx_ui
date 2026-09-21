// Console test for zqsfx_ui's tokens: WCAG contrast, colour-blind separation (Machado
// 2009 simulation + CIE Lab deltaE76), token parity against Broken's Theme.h, and the
// logo path's shape. Prints each check; returns 0 if every check passes, 1 otherwise.

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#include <zqsfx_ui/zqsfx_ui.h>

using namespace zqsfx::ui;

namespace
{
int totalChecks = 0;
int totalFailures = 0;

void report (const juce::String& name, bool ok, const juce::String& detail)
{
    ++totalChecks;
    if (! ok)
        ++totalFailures;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << " - " << detail << std::endl;
}

// ---- colour maths -----------------------------------------------------------------------
struct Rgb { double r, g, b; };
struct Lab { double l, a, b; };

double srgbToLinear (double v)
{
    return v <= 0.04045 ? v / 12.92 : std::pow ((v + 0.055) / 1.055, 2.4);
}

Rgb toLinear (juce::Colour c)
{
    return { srgbToLinear ((double) c.getFloatRed()),
             srgbToLinear ((double) c.getFloatGreen()),
             srgbToLinear ((double) c.getFloatBlue()) };
}

double relativeLuminance (juce::Colour c)
{
    auto lin = toLinear (c);
    return 0.2126 * lin.r + 0.7152 * lin.g + 0.0722 * lin.b;
}

double contrastRatio (juce::Colour a, juce::Colour b)
{
    const double la = relativeLuminance (a) + 0.05;
    const double lb = relativeLuminance (b) + 0.05;
    return la > lb ? la / lb : lb / la;
}

using Matrix3 = double[3][3];

Rgb applyMatrix (const Rgb& in, const Matrix3& m)
{
    Rgb out {
        m[0][0] * in.r + m[0][1] * in.g + m[0][2] * in.b,
        m[1][0] * in.r + m[1][1] * in.g + m[1][2] * in.b,
        m[2][0] * in.r + m[2][1] * in.g + m[2][2] * in.b
    };
    out.r = juce::jlimit (0.0, 1.0, out.r);
    out.g = juce::jlimit (0.0, 1.0, out.g);
    out.b = juce::jlimit (0.0, 1.0, out.b);
    return out;
}

Lab linearToLab (const Rgb& lin)
{
    // sRGB primaries -> XYZ, D65
    const double x = 0.4124564 * lin.r + 0.3575761 * lin.g + 0.1804375 * lin.b;
    const double y = 0.2126729 * lin.r + 0.7151522 * lin.g + 0.0721750 * lin.b;
    const double z = 0.0193339 * lin.r + 0.1191920 * lin.g + 0.9503041 * lin.b;

    const double xn = 0.95047, yn = 1.0, zn = 1.08883;
    auto f = [] (double t)
    {
        const double d = 6.0 / 29.0;
        return t > d * d * d ? std::cbrt (t) : (t / (3.0 * d * d) + 4.0 / 29.0);
    };
    const double fx = f (x / xn), fy = f (y / yn), fz = f (z / zn);
    return { 116.0 * fy - 16.0, 500.0 * (fx - fy), 200.0 * (fy - fz) };
}

double deltaE76 (const Lab& a, const Lab& b)
{
    const double dl = a.l - b.l, da = a.a - b.a, db = a.b - b.b;
    return std::sqrt (dl * dl + da * da + db * db);
}

Lab labFor (juce::Colour c, const Matrix3* m)
{
    auto lin = toLinear (c);
    if (m != nullptr)
        lin = applyMatrix (lin, *m);
    return linearToLab (lin);
}

// Machado 2009 severity-1.0 simulation matrices, applied to LINEAR RGB.
const Matrix3 kProtan { { 0.152286, 1.052583, -0.204868 },
                        { 0.114503, 0.786281, 0.099216 },
                        { -0.003882, -0.048116, 1.051998 } };
const Matrix3 kDeutan { { 0.367322, 0.860646, -0.227968 },
                        { 0.280085, 0.672501, 0.047413 },
                        { -0.011820, 0.042940, 0.968881 } };
const Matrix3 kTritan { { 1.255528, -0.076749, -0.178779 },
                        { -0.078411, 0.930809, 0.147602 },
                        { 0.004733, 0.691367, 0.303900 } };

// ---- test 1: WCAG contrast ----------------------------------------------------------------
void testContrast()
{
    struct Check { const char* name; juce::Colour fg; juce::Colour bg; };
    const std::vector<Check> checks {
        { "silkTitle vs panelTop", colour::silkTitle, colour::panelTop },
        { "silkLabel vs panelTop", colour::silkLabel, colour::panelTop },
        { "silkCaption vs panelTop", colour::silkCaption, colour::panelTop },
        { "btnText vs panelTop", colour::btnText, colour::panelTop },
        { "accent vs panelTop", colour::accent, colour::panelTop },
        { "comp::sky vs panelTop", comp::sky, colour::panelTop },
        { "comp::yellow vs panelTop", comp::yellow, colour::panelTop },
        { "comp::purple vs panelTop", comp::purple, colour::panelTop },
        { "comp::green vs panelTop", comp::green, colour::panelTop },
        { "comp::white vs panelTop", comp::white, colour::panelTop },
        { "lcdText vs lcdBg", colour::lcdText, colour::lcdBg },
        { "comp::sky vs lcdBg", comp::sky, colour::lcdBg },
        { "comp::yellow vs lcdBg", comp::yellow, colour::lcdBg },
        { "comp::purple vs lcdBg", comp::purple, colour::lcdBg },
        { "comp::green vs lcdBg", comp::green, colour::lcdBg },
        { "comp::white vs lcdBg", comp::white, colour::lcdBg },
    };

    for (auto& c : checks)
    {
        const double ratio = contrastRatio (c.fg, c.bg);
        report (juce::String ("WCAG contrast: ") + c.name, ratio >= 4.5,
                "ratio = " + juce::String (ratio, 2) + " (need >= 4.5)");
    }
}

// ---- test 2: colour-blind separation -------------------------------------------------------
void testColourBlindSeparation()
{
    struct Vision { const char* name; const Matrix3* matrix; };
    const std::array<Vision, 4> visions { {
        { "normal", nullptr }, { "protan", &kProtan }, { "deutan", &kDeutan }, { "tritan", &kTritan }
    } };

    struct Named { const char* name; juce::Colour c; };
    const std::array<Named, 5> channels { {
        { "sky", comp::sky }, { "yellow", comp::yellow }, { "purple", comp::purple },
        { "green", comp::green }, { "white", comp::white }
    } };

    double overallMin = std::numeric_limits<double>::max();
    juce::String overallMinDesc;
    double accentMin = std::numeric_limits<double>::max();
    juce::String accentMinDesc;

    bool overallOk = true;
    bool accentOk = true;

    for (auto& v : visions)
    {
        std::array<Lab, 5> labs;
        for (size_t i = 0; i < channels.size(); ++i)
            labs[i] = labFor (channels[i].c, v.matrix);
        const Lab accentLab = labFor (colour::accent, v.matrix);

        for (size_t i = 0; i < channels.size(); ++i)
        {
            for (size_t j = i + 1; j < channels.size(); ++j)
            {
                const double dE = deltaE76 (labs[i], labs[j]);
                if (dE < overallMin)
                {
                    overallMin = dE;
                    overallMinDesc = juce::String (channels[i].name) + " vs " + channels[j].name
                                    + " under " + v.name;
                }
                if (dE < 18.0)
                    overallOk = false;
            }

            const double dEAccent = deltaE76 (accentLab, labs[i]);
            if (dEAccent < accentMin)
            {
                accentMin = dEAccent;
                accentMinDesc = juce::String ("accent vs ") + channels[i].name + " under " + v.name;
            }
            if (dEAccent < 30.0)
                accentOk = false;
        }
    }

    report ("Colour-blind separation: all pairs >= 18.0 deltaE (normal + protan/deutan/tritan)",
            overallOk, "minimum = " + juce::String (overallMin, 1) + " (" + overallMinDesc + ")");
    report ("Colour-blind separation: accent >= 30.0 deltaE from every channel",
            accentOk, "minimum = " + juce::String (accentMin, 1) + " (" + accentMinDesc + ")");

    std::cout << "Minimum deltaE overall (channel vs channel): " << overallMin << " (" << overallMinDesc << ")" << std::endl;
    std::cout << "Minimum deltaE for accent (accent vs channel): " << accentMin << " (" << accentMinDesc << ")" << std::endl;
}

// ---- test 3: token parity with Broken's Theme.h ---------------------------------------------
void testTokenParity()
{
    struct Expected { const char* name; juce::Colour actual; uint32_t expectedArgb; };
    const std::vector<Expected> table {
        { "chassisTop", colour::chassisTop, 0xff0e1011u },
        { "chassisMid", colour::chassisMid, 0xff0a0b0cu },
        { "chassisBot", colour::chassisBot, 0xff0d0f10u },
        { "panelTop", colour::panelTop, 0xff1d2022u },
        { "panelBot", colour::panelBot, 0xff121416u },
        { "panelBorder", colour::panelBorder, 0xff060707u },
        { "silkTitle", colour::silkTitle, 0xff66b7aeu },
        { "silkLabel", colour::silkLabel, 0xff8fb3aeu },
        { "silkCaption", colour::silkCaption, 0xff7a9a94u },
        { "ruleTitle", colour::ruleTitle, 0xff22272au },
        { "ruleInner", colour::ruleInner, 0xff1b1f21u },
        { "lcdBg", colour::lcdBg, 0xff0c150eu },
        { "lcdBorder", colour::lcdBorder, 0xff1f2b21u },
        { "lcdText", colour::lcdText, 0xff8fe89au },
        { "lcdDim", colour::lcdDim, 0xff63b871u },
        { "lcdFaint", colour::lcdFaint, 0xff4e8a5au },
        { "lcdFaint2", colour::lcdFaint2, 0xff3f7a4au },
        { "lcdScreenDark", colour::lcdScreenDark, 0xff0a120cu },
        { "btnTop", colour::btnTop, 0xff212326u },
        { "btnBot", colour::btnBot, 0xff141618u },
        { "btnBorder", colour::btnBorder, 0xff0a0b0cu },
        { "btnText", colour::btnText, 0xffc9d4d2u },
        { "btnText2", colour::btnText2, 0xffb9c7c4u },
        { "btnDisTop", colour::btnDisTop, 0xff17191bu },
        { "btnDisBot", colour::btnDisBot, 0xff101214u },
        { "accent", colour::accent, 0xffe8622au },
        { "accentInk", colour::accentInk, 0xff140d07u },
        { "accentDim", colour::accentDim, 0xff8a4a28u },
        { "ledOffFill", colour::ledOffFill, 0xff151719u },
        { "ledOffRim", colour::ledOffRim, 0xff3d4448u },
        { "pointer", colour::pointer, 0xffded6c2u },
        { "meterLo", colour::meterLo, 0xff2f8f45u },
        { "meterMid", colour::meterMid, 0xff6fd57eu },
        { "meterHi", colour::meterHi, 0xff8fe89au },
        { "meterHot", colour::meterHot, 0xffd9a441u },
        { "meterClip", colour::meterClip, 0xffdd4433u },
        { "logoBright", colour::logoBright, 0xffe2e5e8u },
        { "footer", colour::footer, 0xff45524fu },
        { "warn", colour::warn, 0xffdd4433u },
    };

    for (auto& e : table)
    {
        const auto expected = juce::Colour (e.expectedArgb);
        report (juce::String ("Token parity: ") + e.name, e.actual == expected,
                "expected 0x" + juce::String::toHexString ((int) e.expectedArgb)
                    + ", got 0x" + juce::String::toHexString ((int) e.actual.getARGB()));
    }
}

// ---- test 4: logo path ------------------------------------------------------------------
void testLogoPath()
{
    auto path = LogoMark::createPath();
    report ("Logo path: non-empty", ! path.isEmpty(), "isEmpty() = " + juce::String (path.isEmpty() ? "true" : "false"));

    auto bounds = path.getBounds();
    const double aspect = bounds.getHeight() > 0.0f ? (double) bounds.getWidth() / (double) bounds.getHeight() : 0.0;
    const double expected = 933.66 / 802.79;
    const double relErr = std::abs (aspect - expected) / expected;
    report ("Logo path: bounds aspect ratio within 1%", relErr <= 0.01,
            "aspect = " + juce::String (aspect, 4) + ", expected = " + juce::String (expected, 4)
                + ", relative error = " + juce::String (relErr * 100.0, 3) + "%");

    report ("Logo path: even-odd fill rule", ! path.isUsingNonZeroWinding(),
            "isUsingNonZeroWinding() = " + juce::String (path.isUsingNonZeroWinding() ? "true" : "false"));
}
} // namespace

int main()
{
    std::cout << "zqsfx_ui TokenTests" << std::endl;
    testContrast();
    testColourBlindSeparation();
    testTokenParity();
    testLogoPath();

    std::cout << totalChecks << " checks, " << totalFailures << " failures" << std::endl;
    return totalFailures == 0 ? 0 : 1;
}
