#include "plugin.hpp"


Plugin* pluginInstance;


void init(Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelVCO);
    p->addModel(modelVCA);
    p->addModel(modelSpringMod);
    p->addModel(modelVCF);
    p->addModel(modelLFO);
    p->addModel(modelADSR);
    p->addModel(modelNOISE);
    p->addModel(modelHATS);
    p->addModel(modelKICKS);
    p->addModel(modelSNARES);
    p->addModel(modelWAVECRAFTER);
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
