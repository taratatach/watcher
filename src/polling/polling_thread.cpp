#include <chrono>
#include <cstdint>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <uv.h>

#include "../log.h"
#include "../message_buffer.h"
#include "../result.h"
#include "../status.h"
#include "../thread.h"
#include "polled_root.h"
#include "polling_thread.h"

using std::endl;
using std::string;

PollingThread::PollingThread(uv_async_t *main_callback) :
  Thread("polling thread", main_callback),
  poll_interval{DEFAULT_POLL_INTERVAL},
  poll_throttle{DEFAULT_POLL_THROTTLE}
{
  //
}

void PollingThread::collect_status(Status &status)
{
  status.polling_thread_state = state_name();
  status.polling_thread_ok = get_error();
  status.polling_in_size = get_in_queue_size();
  status.polling_in_ok = get_in_queue_error();
  status.polling_out_size = get_out_queue_size();
  status.polling_out_ok = get_out_queue_error();
}

Result<> PollingThread::body()
{
  while (true) {
    LOGGER << "Handling commands." << endl;
    Result<size_t> cr = handle_commands();
    if (cr.is_error()) {
      LOGGER << "Unable to process incoming commands: " << cr << endl;
    } else if (is_stopping()) {
      LOGGER << "Polling thread stopping." << endl;
      return ok_result();
    }

    LOGGER << "Polling root directories." << endl;
    cycle();

    if (is_healthy()) {
      LOGGER << "Sleeping for " << poll_interval.count() << "ms." << endl;
      std::this_thread::sleep_for(poll_interval);
      LOGGER << "Waking up." << endl;
    }
  }
}

Result<> PollingThread::cycle()
{
  MessageBuffer buffer;
  size_t remaining = poll_throttle;

  auto it = roots.begin();
  size_t roots_left = roots.size();
  LOGGER << "Polling " << plural(roots_left, "root") << " with " << plural(poll_throttle, "throttle slot") << "."
         << endl;

  while (it != roots.end()) {
    PolledRoot &root = it->second;
    size_t allotment = remaining / roots_left;

    LOGGER << "Polling " << root << " with an allotment of " << plural(allotment, "throttle slot") << "." << endl;

    size_t progress = root.advance(buffer, allotment);
    remaining -= progress;
    LOGGER << root << " consumed " << plural(progress, "throttle slot") << "." << endl;

    roots_left--;
    ++it;
  }

  return emit_all(buffer.begin(), buffer.end());
}

Result<Thread::OfflineCommandOutcome> PollingThread::handle_offline_command(const CommandPayload *command)
{
  Result<OfflineCommandOutcome> r = Thread::handle_offline_command(command);
  if (r.is_error()) return r;

  if (command->get_action() == COMMAND_ADD) {
    return ok_result(TRIGGER_RUN);
  }

  if (command->get_action() == COMMAND_POLLING_INTERVAL) {
    handle_polling_interval_command(command);
  }

  if (command->get_action() == COMMAND_POLLING_THROTTLE) {
    handle_polling_throttle_command(command);
  }

  return ok_result(OFFLINE_ACK);
}

Result<Thread::CommandOutcome> PollingThread::handle_add_command(const CommandPayload *command)
{
  LOGGER << "Adding poll root at path " << command->get_root() << " to channel " << command->get_channel_id() << "."
         << endl;

  roots.emplace(std::piecewise_construct,
    std::forward_as_tuple(command->get_channel_id()),
    std::forward_as_tuple(string(command->get_root()), command->get_id(), command->get_channel_id()));

  return ok_result(NOTHING);
}

Result<Thread::CommandOutcome> PollingThread::handle_remove_command(const CommandPayload *command)
{
  LOGGER << "Removing poll root at channel " << command->get_channel_id() << "." << endl;

  auto it = roots.find(command->get_channel_id());
  if (it != roots.end()) roots.erase(it);

  if (roots.empty()) {
    LOGGER << "Final root removed." << endl;
    return ok_result(TRIGGER_STOP);
  }

  return ok_result(ACK);
}

Result<Thread::CommandOutcome> PollingThread::handle_polling_interval_command(const CommandPayload *command)
{
  poll_interval = std::chrono::milliseconds(command->get_arg());
  return ok_result(ACK);
}

Result<Thread::CommandOutcome> PollingThread::handle_polling_throttle_command(const CommandPayload *command)
{
  poll_throttle = command->get_arg();
  return ok_result(ACK);
}