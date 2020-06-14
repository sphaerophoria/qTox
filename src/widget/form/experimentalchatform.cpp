
#include "experimentalchatform.h"

#include "src/widget/contentlayout.h"

ExperimentalChatForm::ExperimentalChatForm(QWidget* parent)
    : QQuickWidget(parent)
{
    setSource(QUrl::fromLocalFile("../qml/friendchatform.qml"));
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setClearColor(Qt::transparent);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
}

void ExperimentalChatForm::show(ContentLayout* layout) {
    // Note: We intentionally do not use the mainHead here. This allows us to
    // render more space in the QML window. If we did not do this the popup for
    // incoming calls would be clipped into the area shown by the mainHead.
    //
    // An alternative could be to draw the callConfirm widget oustside of QML
    layout->mainContent->layout()->addWidget(this);
    QQuickWidget::show();
}
