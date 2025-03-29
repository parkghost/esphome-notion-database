#include "allocator.h"

namespace esphome {
namespace notion_database {

// Global allocator
RAMAllocator<uint8_t> ALLOCATOR = RAMAllocator<uint8_t>(RAMAllocator<uint8_t>::NONE);

}  // namespace notion_database
}  // namespace esphome
