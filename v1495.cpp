#include "v1495.hpp"

namespace caen {

V1495::V1495(const Connection& connection): Device(connection) {
  // It appears that V1495 OUI and ID registers may be overwritten with user
  // firmware. So, we cannot check that we have connected to the proper board.
  // if (oui() != OUI || id() != 1495) throw WrongDevice(connection, "V1495");
};

};
