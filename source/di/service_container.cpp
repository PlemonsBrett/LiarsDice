#include <liarsdice/di/service_container.hpp>

namespace liarsdice::di {

ServiceContainer& get_service_container() {
    static ServiceContainer instance;
    return instance;
}

} // namespace liarsdice::di