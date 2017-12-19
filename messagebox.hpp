#ifndef MESSAGEBOX_HPP
#define MESSAGEBOX_HPP

#include <QMessageBox>

template<int minimumWidth_=0>
class MessageBox : public QMessageBox {
public:
  template<typename... Args> explicit MessageBox(Args&&... args) : QMessageBox(args...) {}
protected:
  virtual void showEvent(QShowEvent* event) override
  {
    QMessageBox::showEvent(event);
    QWidget* const textField=findChild<QWidget*>("qt_msgbox_label");
    if (textField!=nullptr)
      textField->setMinimumWidth(std::max(minimumWidth_,textField->width()));
  }
};

#endif // MESSAGEBOX_HPP
