#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <QSettings>
#include <QNetworkAccessManager>
#include "pieceicons.hpp"

struct Globals {
  Globals(const QString& fileName,const QSettings::Format format) : settings(fileName,format) {}

  QSettings settings;
  PieceIcons pieceIcons;
  QNetworkAccessManager networkAccessManager;
};

#endif // GLOBALS_HPP
