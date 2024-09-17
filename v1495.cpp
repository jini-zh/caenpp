#include "v1495.hpp"

namespace caen {

V1495::V1495(const Connection& connection): Device(connection) {
  if (oui() != OUI || id() != 1495) throw WrongDevice(connection, "V1495");
};

};
