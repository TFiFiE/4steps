#ifndef ICONENGINE_HPP
#define ICONENGINE_HPP

class QPainter;
class QSvgRenderer;
#include <QIconEngine>

class IconEngine : public QIconEngine {
public:
  IconEngine();
private:
  virtual void paint(QPainter* painter,const QRect& rect,QIcon::Mode,QIcon::State) override;
  virtual QIconEngine* clone() const override;
  virtual QPixmap pixmap(const QSize& size,QIcon::Mode mode,QIcon::State state) override;

  QSvgRenderer subicons[2];
};

#endif // ICONENGINE_HPP
