#include "CayimeEngine.h"
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>

class CayimeAddonFactory : public fcitx::AddonFactory {
public:
    fcitx::AddonInstance* create(fcitx::AddonManager* manager) override {
        return new CayimeEngine(manager->instance());
    }
};

FCITX_ADDON_FACTORY(CayimeAddonFactory)
