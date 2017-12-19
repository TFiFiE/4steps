#include <cassert>
#include "timeestimator.hpp"

void TimeEstimator::add(const QDateTime& postTime,const QDateTime& replyTime,const bool instantResponse,const QVariant& timeOnServer)
{
  assert(postTime.isValid());
  if (timeOnServer.isValid()) {
    lastTimeOnServer=QDateTime::fromSecsSinceEpoch(timeOnServer.toLongLong()).addMSecs(500);
    referenceTime=replyTime;
  }
  if (instantResponse) {
    const auto roundTripTime=postTime.msecsTo(replyTime);
    insertSorted(roundTripTimes,roundTripTime);
    if (timeOnServer.isValid())
      estimatedServerToLocalOffsets.push_back(lastTimeOnServer.msecsTo(replyTime.addMSecs(-roundTripTime/2)));
  }
  else if (timeOnServer.isValid())
    estimatedServerToLocalPlusReplyOffsets.push_back(lastTimeOnServer.msecsTo(replyTime));
}

qint64 TimeEstimator::estimatedRoundTripTime() const
{
  if (roundTripTimes.empty())
    return 0;
  else
    return medianSorted(roundTripTimes);
}

qint64 TimeEstimator::estimatedExtraTime() const
{
  const auto numEstimates=estimatedServerToLocalOffsets.size()+estimatedServerToLocalPlusReplyOffsets.size();
  if (numEstimates==0)
    return 0;
  std::vector<qint64> allEstimatedServerToLocal;
  allEstimatedServerToLocal.reserve(numEstimates);
  allEstimatedServerToLocal.insert(allEstimatedServerToLocal.end(),estimatedServerToLocalOffsets.begin(),estimatedServerToLocalOffsets.end());
  const auto estimatedReplyTime=estimatedRoundTripTime()/2;
  for (const auto& estimatedServerToLocalPlusReplyOffset:estimatedServerToLocalPlusReplyOffsets)
    allEstimatedServerToLocal.push_back(estimatedServerToLocalPlusReplyOffset-estimatedReplyTime);
  const auto estimatedTurnTime=std::max(referenceTime,lastTimeOnServer.addMSecs(medianUnsorted(allEstimatedServerToLocal)));
  return estimatedTurnTime.msecsTo(QDateTime::currentDateTimeUtc());
}

qint64 TimeEstimator::medianSorted(const std::vector<qint64>& multiset)
{
  auto iter=next(multiset.begin(),multiset.size()/2);
  if (multiset.size()%2==0) {
    const auto high=*iter--;
    return (high+*iter)/2;
  }
  else
    return *iter;
}

qint64 TimeEstimator::medianUnsorted(std::vector<qint64>& multiset)
{
  assert(!multiset.empty());
  const auto iter=next(multiset.begin(),multiset.size()/2);
  nth_element(multiset.begin(),iter,multiset.end());
  if (multiset.size()%2==0)
    return (*iter+*max_element(multiset.begin(),iter))/2;
  else
    return *iter;
}

void TimeEstimator::insertSorted(std::vector<qint64>& vector,const qint64 element)
{
  vector.insert(upper_bound(vector.begin(),vector.end(),element),element);
}
