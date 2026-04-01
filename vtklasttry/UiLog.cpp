#include "UiLog.h"

#include <QDateTime>
#include <QTextEdit>

namespace vtklasttry
{
void appendUiLog(QTextEdit* box, const QString& msg)
{
    if (!box)
        return;
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    box->append(QStringLiteral("[%1] %2").arg(ts, msg));
}
} // namespace vtklasttry

