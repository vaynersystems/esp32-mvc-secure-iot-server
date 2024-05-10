#include "System/CORE/esp32_base_service.hpp"

class esp32_status_service : public esp32_base_service{

public:
    /// @brief provide client status of controller
    inline string Execute();

private:
    static DerivedService<esp32_status_service> reg; //register the service
};