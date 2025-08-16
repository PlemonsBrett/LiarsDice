#include <liarsdice/database/connection_manager.hpp>

namespace liarsdice::database {

// Static member definitions
std::unique_ptr<ConnectionManager> ConnectionManager::instance_;
boost::once_flag ConnectionManager::init_flag_ = BOOST_ONCE_INIT;

} // namespace liarsdice::database