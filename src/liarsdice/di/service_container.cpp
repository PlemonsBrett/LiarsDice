#include "liarsdice/di/service_container.hpp"

namespace liarsdice::di {

// Thread-local static member definition for circular dependency detection
thread_local std::unordered_set<std::type_index> ServiceContainer::resolution_stack_;

} // namespace liarsdice::di