// CooperationSearchEdit 桩: 真实实现在 gui/win (Windows-only), Linux 编不干净(using namespace 冲突)。
// workspacewidget 引用此类, 这里提供全限定命名空间的最小实现 + 由 AUTOMOC 生成信号 moc。
#include "cooperationsearchedit.h"
#include <QFrame>
#include <QFocusEvent>
namespace cooperation_core {
CooperationSearchEdit::CooperationSearchEdit(QWidget *parent) : QFrame(parent) {}
QString CooperationSearchEdit::text() const { return {}; }
void CooperationSearchEdit::setPlaceholderText(const QString &) {}
void CooperationSearchEdit::setPlaceHolder(const QString &) {}
bool CooperationSearchEdit::eventFilter(QObject *, QEvent *) { return false; }
void CooperationSearchEdit::focusInEvent(QFocusEvent *) {}
} // namespace cooperation_core
