#pragma once
#include <rack.hpp>


using namespace rack;

// Declare the Plugin, defined in plugin.cpp
extern Plugin* pluginInstance;

// Declare each Model, defined in each module source file
extern Model* modelVCO;
extern Model* modelVCA;
extern Model* modelSpringMod;
extern Model* modelVCF;
extern Model* modelLFO;
extern Model* modelADSR;
extern Model* modelNOISE;
extern Model* modelHATS;
extern Model* modelKICKS;
extern Model* modelSNARES;
extern Model* modelWAVECRAFTER;

// Custom module components
struct GL_SlidePot : app::SvgSlider {
    GL_SlidePot() {
        Vec margin = Vec(4, 4);
        maxHandlePos = Vec(-1.5, -8).plus(margin);
        minHandlePos = Vec(-1.5, 87).plus(margin);
        setBackgroundSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Components/GL_SlidePot.svg")));
        background->wrap();
        background->box.pos = margin;
        box.size = background->box.size.plus(margin.mult(2));
        setHandleSvg(APP->window->loadSvg(asset::plugin(pluginInstance,"res/Components/GL_SlidePotHandle.svg")));
        handle->wrap();
    }
};
