#include "triage/priority_queue.hpp"

#include <cassert>
#include <vector>

int main() {
  using namespace triage;
  static_assert(sizeof(PatientId) == 8);

  QueueEntry example{42, 3, 120};
  assert(example.patient_id == 42 && example.priority_key == 3);

  IndexedBinaryMinHeap queue;
  assert(queue.insert({20, 5, 100}) == std::nullopt);
  assert(queue.insert({10, 5, 90}) == std::nullopt);
  assert(queue.insert({5, 5, 90}) == std::nullopt);
  assert(queue.decreaseKey(20, 3) == std::nullopt);

  auto first = queue.extractMin();
  assert(first.ok());
  assert(first.value->patient_id == 20);
  assert(first.value->priority_key == 3);
  assert(first.value->arrival_time == 100);

  assert(queue.deletePatient(5) == std::nullopt);
  assert(queue.decreaseKey(10, 5) == QueueError::key_not_decreased);

  auto second = queue.extractMin();
  assert(second.ok());
  assert(second.value->patient_id == 10);
  assert(queue.extractMin().error == QueueError::empty_queue);
}
