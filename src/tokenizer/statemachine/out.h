#pragma once

#include <span.h>
#include "fwd.h"

/// 'out' in direct mode is largely ignored
/// 'out' in scratch mode facilitates where found token characters are put, including bounds checking
/// returns the number of bytes searched until we found token delimiters or EOL start, in bytes
typedef unsigned (*token_delim_fnptr)(StateMachine2* sm, ConstSpan in, TokenStateMachineOut* out);

