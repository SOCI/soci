#define SOCI_SOURCE

#include "soci/callbacks.h"
#include "soci/soci-platform.h"

using namespace soci;

failover_callback::~failover_callback() = default;

void failover_callback::started() {}

void failover_callback::finished(session& /* sql */) {}

void failover_callback::failed(bool& /* out */ /* retry */, std::string& /* out */ /* newTarget */) {}

void failover_callback::aborted() {}
