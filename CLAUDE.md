# zqsfx_ui: agent contract

Inside the ZQ SFX workspace the universal rules in `../CLAUDE.md` apply; `README.md` is the user-facing doc. This file adds only what an agent needs to verify a change here.

## Verification tiers

Policy: `~/Projects/_policy/verification-tiers.md`. Commands run from the repository root.
- Scoped: `cmake --build build-check --target <program> && ctest --test-dir build-check -R '<test name>' --output-on-failure` (the program and test names: `ctest --test-dir build-check -N`)
- Fast (every commit): `tools/check.sh`. It builds the CTest programs in `build-check/` (arm64 Release, nothing installed; the program list comes from `ctest -N`, so a new test needs no edit there) and runs CTest. Measured on an idle Mac: about 4 s after a source edit, 4 s after a core-header edit. Before every `dv commit` that changes this folder, the JUCE repo's pre-commit hook runs it (`.dvhooks/pre-commit` at the JUCE root, through `tools/check.sh --hook`; Diversion stops a hook after 60 s), and zq-commit-gate runs it before Claude's `dv` and `git` commits. Its first run builds from scratch (about 2 minutes), so run it once by hand after a fresh clone or after deleting `build-check/`.
- Full (once per done claim): Universal Release build of every target, including the gallery and snapshot tool; `ctest --test-dir build --output-on-failure` (TokenTests, DialTests); render `snapshots/gallery.png` before and after a visual change and compare. Then, per product that bumps the tag, that product's own full tier.
- CI: GitHub Actions (`.github/workflows/ci.yml`: builds plus ctest) on every push to GitHub. Work in progress is never pushed, so the fast check is the per-commit gate.
