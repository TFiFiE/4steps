#include <QSvgRenderer>
#include <QPainter>
#include "iconengine.hpp"

IconEngine::IconEngine()
{
  subicons[0].load(QString(":/piece_icons/vector/white-Rabbit.svg"));
  subicons[1].load(QString(":/piece_icons/vector/black-Rabbit.svg"));
}

void IconEngine::paint(QPainter* painter,const QRect& rect,QIcon::Mode,QIcon::State)
{
  QRect half(0,0,rect.width()/2,rect.height());
  subicons[0].render(painter,half);
  half.moveRight(rect.height()-1);
  subicons[1].render(painter,half);
}

QIconEngine* IconEngine::clone() const
{
  return new IconEngine();
}

QPixmap IconEngine::pixmap(const QSize& size,QIcon::Mode mode,QIcon::State state)
{
  QImage image(size,QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  QPixmap pixmap=QPixmap::fromImage(image,Qt::NoFormatConversion);
  QPainter painter(&pixmap);
  paint(&painter,QRect(QPoint(0,0),size),mode,state);
  return pixmap;
}
