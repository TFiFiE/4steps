#ifndef POTENTIALMOVE_HPP
#define POTENTIALMOVE_HPP

#include "def.hpp"

class PotentialMove {
public:
  PotentialMove(ExtendedSteps potentialMove_=ExtendedSteps()) :
    potentialMove(std::move(potentialMove_)),
    afterCurrentStep(potentialMove.end()) {}
  ExtendedSteps current() const
  {
    return ExtendedSteps(potentialMove.begin(),afterCurrentStep);
  }
  const ExtendedSteps& all() const
  {
    return potentialMove;
  }
  bool empty() const
  {
    return afterCurrentStep==potentialMove.begin();
  }
  const ExtendedStep& back() const
  {
    return *prev(afterCurrentStep);
  }
  void clear()
  {
    potentialMove.clear();
    afterCurrentStep=potentialMove.end();
  }
  void set(ExtendedSteps potentialMove_,const unsigned int offset)
  {
    potentialMove=std::move(potentialMove_);
    afterCurrentStep=next(potentialMove.begin(),offset);
  }
  void append(const ExtendedSteps& steps)
  {
    if (startsWith(ExtendedSteps(afterCurrentStep,potentialMove.cend()),steps))
      shiftEnd(steps.size());
    else {
      potentialMove.erase(afterCurrentStep,potentialMove.cend());
      ::append(potentialMove,steps);
      afterCurrentStep=potentialMove.end();
    }
  }
  void shiftEnd(const int numSteps)
  {
    assert(numSteps<=distance(afterCurrentStep,potentialMove.cend()));
    assert(-numSteps<=distance(potentialMove.cbegin(),afterCurrentStep));
    advance(afterCurrentStep,numSteps);
  }
  bool shiftEnd(const bool forward,const bool all)
  {
    const auto endPoint=(forward ? potentialMove.end() : potentialMove.begin());
    if (afterCurrentStep==endPoint)
      return false;
    else {
      if (all)
        afterCurrentStep=endPoint;
      else
        advance(afterCurrentStep,forward ? 1 : -1);
      return true;
    }
  }
private:
  ExtendedSteps potentialMove;
  ExtendedSteps::const_iterator afterCurrentStep;
};

#endif // POTENTIALMOVE_HPP
