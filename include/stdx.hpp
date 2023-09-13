#pragma once

#if __cplusplus >= 201703L
#define USE_STD17
#endif

/**
 * std::optional to stdx::optional fallback
*/
#if __has_include(<optional>) && USE_STD17

#   include <optional>
namespace stdx {
  using namespace ::std;
}
#elif __has_include(<experimental/optional>)
#   include <experimental/optional>

namespace stdx {
  using namespace ::std;
  using namespace ::std::experimental;
}

#else
#   error <experimental/optional> and <optional> not found
#endif

