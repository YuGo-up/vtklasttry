#pragma once

#include <QString>

namespace vtklasttry
{
struct SceneViewRequest
{
    QString metricKey;
    bool comboEnabled = false;
    bool showBad = false;
    bool isolateSelectedBad = false;
};
} // namespace vtklasttry

