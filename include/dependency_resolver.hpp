/* Things this entity is responsible for:
 *  - creating structure of dependency dir
 *  - resolving the actual dependency (i.e. fetching it from a remote URL)
 *  - building it?
 */

#if !defined(DEPENDENCY_RESOLVER_H)
#include "application_context.hpp"
#include "configuration_io.hpp"
#include "utils.hpp"

bool Resolve(application_context&, dependency*);
vector<dependency*>* FilterUnmodified(application_context&, vector<dependency*>&);

#define DEPENDENCY_RESOLVER_H
#endif
