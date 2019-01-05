#ifndef MESSAGEBOX_HPP
#define MESSAGEBOX_HPP

#include <QMessageBox>
#include <QLabel>
#include <QApplication>

class MessageBox : public QMessageBox {
public:
  template<typename... Args> explicit MessageBox(Args&&... args) : QMessageBox(args...) {}
protected:
  virtual void showEvent(QShowEvent* event) override
  {
    QLabel* const textField=findChild<QLabel*>("qt_msgbox_label");
    if (textField!=nullptr) {
      auto font=QApplication::font("QMdiSubWindowTitleBar");
      font.setPointSize(font.pointSize()+4);
      textField->setMinimumWidth(std::max(textField->width(),QFontMetrics(font).boundingRect(windowTitle()).width()+53));
      textField->setAlignment(Qt::AlignCenter);
    }
    QMessageBox::showEvent(event);
  }
};

#endif // MESSAGEBOX_HPP
