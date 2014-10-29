#include <stdlib.h>
#include <vector>

typedef void (*Callback)(int, int);

class Mediator
{
  int currentSamp;
  std::vector<int> periods;
  std::vector<Callback> callbacks;

  public:
    Mediator ();
    void updateCount(int sampleCount);
    int getCurrentSamp();
    void registerCallback(int period, Callback cb);
};
