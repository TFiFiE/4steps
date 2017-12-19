#ifndef TIMEESTIMATOR_HPP
#define TIMEESTIMATOR_HPP

#include <QDateTime>
#include <QVariant>

class TimeEstimator {
public:
  void add(const QDateTime& postTime,const QDateTime& replyTime,const bool instantResponse,const QVariant& timeOnServer);
  qint64 estimatedRoundTripTime() const;
  qint64 estimatedExtraTime() const;
private:
  static qint64 medianSorted(const std::vector<qint64>& multiset);
  static qint64 medianUnsorted(std::vector<qint64>& multiset);
  static void insertSorted(std::vector<qint64>& vector,const qint64 element);
  std::vector<qint64> roundTripTimes,estimatedServerToLocalOffsets,estimatedServerToLocalPlusReplyOffsets;
  QDateTime lastTimeOnServer,referenceTime;
};

#endif // TIMEESTIMATOR_HPP
