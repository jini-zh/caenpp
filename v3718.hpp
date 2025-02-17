#include "vme.hpp"

class V3718: public Bridge {
  public:
    V3718(const Connection& connection): Bridge(connection) {};

    V3718(
        Connection::ConetType conet = Connection::ConetType::None,
        uint32_t              link  = 0,
        short                 node  = 0,
        bool                  local = false
    ):
      Bridge(Connection::BridgeType::V3718, conet, link, node, local)
    {};

    V3718(uint32_t link, short node = 0, bool local = false):
      Bridge(Connection::BridgeType::V3718, link, node, local)
    {};

    V3718(int32_t handle, bool own): Bridge(handle, own) {};
};
