# zqsfx_ui: the shared ZQ SFX UI module. See README.md for the FetchContent snippet
# every product uses to consume this as zqsfx::zqsfx_ui (aliased also as zqsfx::ui).

cmake_minimum_required (VERSION 3.22)

# Universal macOS builds are mandatory across the workspace (Soundminer et al. need
# arm64 + x86_64). This must run BEFORE project() so the initial compiler checks pick
# it up, and only when this is the top-level build (a consumer that already set these,
# or that isn't building on Apple at all, is left alone).
if (APPLE AND (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR))
    if (NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    endif()
    if (NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set (CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "macOS deployment target")
    endif()
endif()

project (zqsfx_ui VERSION 0.2.1 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

include (FetchContent)

# Only fetch JUCE when nothing already provided it: a consumer that added JUCE first
# (its own FetchContent/submodule, before pulling this module in) keeps using that copy.
if (NOT TARGET juce::juce_gui_basics)
    FetchContent_Declare (JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable (JUCE)
endif()

juce_add_module ("${CMAKE_CURRENT_SOURCE_DIR}/zqsfx_ui" ALIAS_NAMESPACE zqsfx)

# zqsfx::zqsfx_ui (from ALIAS_NAMESPACE above) is the target juce_add_module creates.
# zqsfx::ui is an additional alias to the same real target, so `zqsfx::ui` also works
# as documented in the README; both names refer to the identical library.
add_library (zqsfx::ui ALIAS zqsfx_ui)

# Fonts, embedded as binary data so every host and OS renders the same (style guide
# section 4). PIC is required because plugins link this into shared libraries.
juce_add_binary_data (zqsfx_ui_assets HEADER_NAME ZqsfxUiAssets.h NAMESPACE ZqsfxUiAssets SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-Medium.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/BarlowCondensed-SemiBold.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/VT323-Regular.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/IBMPlexMono-Regular.ttf"
    # the house knobs: CC0 filmstrips, 128 frames each (see assets/knobs/PROVENANCE.md)
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/knobs/strip_a.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/knobs/strip_b.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/knobs/strip_c.png")
set_target_properties (zqsfx_ui_assets PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Consumers get fonts with no extra step: linking zqsfx_ui (by any of its names) pulls
# the binary data in automatically.
target_link_libraries (zqsfx_ui INTERFACE zqsfx_ui_assets)

# Deliberately NO compile definitions or warning flags on the module target itself: it is an
# INTERFACE target, so anything set here is forced onto every product that links it and would
# clash with a product that sets JUCE_WEB_BROWSER / JUCE_USE_CURL differently or has its own
# warning policy. The demo targets below set what they need for themselves.

# The house knobs ship inside this module (CC0). A product that wants its OWN knob art instead
# (Broken keeps its licensed Noisehead strips, which may not be redistributed from here) embeds
# its copy with this helper and hands it to LookAndFeel::setKnobStripsFromMemory():
#
#   zqsfx_ui_add_knob_strips (DIR assets/knobs TARGETS MyPlugin MyTests)
#
# expects DIR/strip_a.png (>= 56 px dials), strip_b.png (>= 42 px), strip_c.png (smaller), and
# generates the header ZqsfxKnobStrips.h in namespace ZqsfxKnobStrips.
function (zqsfx_ui_add_knob_strips)
    cmake_parse_arguments (ZKS "" "DIR" "TARGETS" ${ARGN})
    if (NOT ZKS_DIR OR NOT ZKS_TARGETS)
        message (FATAL_ERROR "zqsfx_ui_add_knob_strips: DIR and TARGETS are required")
    endif()
    get_filename_component (zks_dir "${ZKS_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    foreach (strip strip_a.png strip_b.png strip_c.png)
        if (NOT EXISTS "${zks_dir}/${strip}")
            message (FATAL_ERROR "zqsfx_ui_add_knob_strips: missing ${zks_dir}/${strip}")
        endif()
    endforeach()
    if (NOT TARGET ZqsfxKnobStrips)
        juce_add_binary_data (ZqsfxKnobStrips HEADER_NAME ZqsfxKnobStrips.h NAMESPACE ZqsfxKnobStrips SOURCES
            "${zks_dir}/strip_a.png" "${zks_dir}/strip_b.png" "${zks_dir}/strip_c.png")
        set_target_properties (ZqsfxKnobStrips PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
    foreach (t ${ZKS_TARGETS})
        target_link_libraries (${t} PRIVATE ZqsfxKnobStrips)
    endforeach()
endfunction()

# ON when this is the top-level build (developing the module itself), OFF when a
# product FetchContent's this in as a dependency, so a product's own build does not
# also compile the gallery app, snapshot tool, and tests every time.
option (ZQSFX_UI_BUILD_DEMO "Build the gallery app, snapshot tool and tests" ${PROJECT_IS_TOP_LEVEL})

if (ZQSFX_UI_BUILD_DEMO)
    juce_add_gui_app (ZqsfxUiDemo
        PRODUCT_NAME "ZQ SFX UI Gallery"
        COMPANY_NAME "ZQ SFX"
        BUNDLE_ID com.zqsfx.uigallery
        COMPANY_WEBSITE "https://www.zq-sfx.com"
        COMPANY_EMAIL "connect@zq-sfx.com"
        COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX")
    target_sources (ZqsfxUiDemo PRIVATE demo/Main.cpp)
    target_include_directories (ZqsfxUiDemo PRIVATE demo)
    target_compile_definitions (ZqsfxUiDemo PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries (ZqsfxUiDemo
        PRIVATE zqsfx::zqsfx_ui
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    juce_add_console_app (ZqsfxUiSnapshot PRODUCT_NAME "ZqsfxUiSnapshot" COMPANY_NAME "ZQ SFX"
        BUNDLE_ID com.zqsfx.uisnapshot)
    target_sources (ZqsfxUiSnapshot PRIVATE tools/snapshot/Main.cpp)
    target_include_directories (ZqsfxUiSnapshot PRIVATE demo)
    target_compile_definitions (ZqsfxUiSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries (ZqsfxUiSnapshot
        PRIVATE zqsfx::zqsfx_ui juce::juce_gui_basics
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    juce_add_console_app (ZqsfxUiTokenTests PRODUCT_NAME "ZqsfxUiTokenTests" COMPANY_NAME "ZQ SFX"
        BUNDLE_ID com.zqsfx.uitokentests)
    target_sources (ZqsfxUiTokenTests PRIVATE tests/TokenTests.cpp)
    target_compile_definitions (ZqsfxUiTokenTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries (ZqsfxUiTokenTests
        PRIVATE zqsfx::zqsfx_ui
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

    enable_testing()
    add_test (NAME TokenTests COMMAND ZqsfxUiTokenTests)
endif()
