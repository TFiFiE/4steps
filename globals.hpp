#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <QSettings>
#include <QSvgRenderer>
#include <QNetworkAccessManager>
#include "def.hpp"

struct Globals {
  Globals(const QString& fileName,const QSettings::Format format) : settings(fileName,format) {}

  QSettings settings;
  QSvgRenderer pieceIcons[NUM_PIECE_SIDE_COMBINATIONS];
  QNetworkAccessManager networkAccessManager;
};

#endif // GLOBALS_HPP
