#include "vm/compiler/like_registers.h"

namespace dart {
namespace compiler {

bool* ThreadLocalInstance() {
  thread_local bool instance = false;
  return &instance;
}

void CreateInstance(int pool_offset) {
  if (pool_offset > 0) {
    return;
  }
  auto* instance = ThreadLocalInstance();
  // ASSERT(*instance == false);
  *instance = true;
}

void DeleteInstance(int pool_offset) {
  if (pool_offset > 0) {
    return;
  }
  auto* instance = ThreadLocalInstance();
  // ASSERT(*instance == true);
  *instance = false;
}

}  // namespace compiler
}  // namespace dart
