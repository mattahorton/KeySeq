#include "Mediator.h"
#include "globals.h"
#include <iostream>
using namespace std;

Mediator::Mediator( )
{
    currentSamp = 0;
    std::vector<int> periods;
    std::vector<Callback> callbacks;
}


void Mediator::updateCount(int sampleCount){
  currentSamp = sampleCount;
  int per;
  Callback cb;
  int prev, next;

  for (int i = 0; i < periods.size(); i++) {
    per = periods.at(i);
    cb = callbacks.at(i);

    if ((sampleCount % per) == 0) {
      prev = Globals::playingStep;
      next = Globals::playingStep + 1;

      cb(prev,(next % Globals::numSteps));
      Globals::playingStep = (next % Globals::numSteps);
    }

  }
}

void Mediator::registerCallback(int period, Callback cb) {
  periods.push_back(period);
  callbacks.push_back(cb);
}

int Mediator::getCurrentSamp() {
  return currentSamp;
}
