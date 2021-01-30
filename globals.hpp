#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <random>
#include <QSettings>
#include <QNetworkAccessManager>
#include "pieceicons.hpp"

struct Globals {
  Globals(const std::uint_fast32_t seed,const QString& fileName,const QSettings::Format format) :
    rndEngine(seed),
    settings(fileName,format) {}
  unsigned int rand(const unsigned int number)
  {
    return std::uniform_int_distribution<unsigned int>(0,number-1)(rndEngine);
  }

  std::mt19937 rndEngine;
  QSettings settings;
  PieceIcons pieceIcons;
  QNetworkAccessManager networkAccessManager;
};

#endif // GLOBALS_HPP
