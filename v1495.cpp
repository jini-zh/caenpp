#include "v1495.hpp"

namespace caen {

bool V1495::check() const {
  // It appears that V1495 OUI and ID registers may be overwritten with user
  // firmware. So, we cannot check that we have connected to the proper board.
  // return oui() == OUI && id() == 1495;
  return true;
};

};
