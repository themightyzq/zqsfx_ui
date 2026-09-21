/*******************************************************************************
 The block below describes the properties of this module, and is read by
 the Projucer to automatically generate project code that uses it.

 BEGIN_JUCE_MODULE_DECLARATION

  ID:                 zqsfx_ui
  vendor:             ZQ SFX
  version:            0.1.0
  name:               ZQ SFX shared UI
  description:        Shared design-system UI tokens, LookAndFeel, and controls for
                       every ZQ SFX JUCE product.
  website:            https://www.zq-sfx.com
  license:            GPL-3.0-or-later
  minimumCppStandard: 17

  dependencies:       juce_gui_basics juce_audio_processors

 END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once
#define ZQSFX_UI_H_INCLUDED

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "tokens/Tokens.h"
#include "lookandfeel/LookAndFeel.h"
#include "components/Panel.h"
#include "components/Knob.h"
#include "components/Combo.h"
#include "components/Toggles.h"
#include "components/Led.h"
#include "components/PhosphorScreen.h"
#include "components/PeakMeter.h"
#include "components/LogoMark.h"
