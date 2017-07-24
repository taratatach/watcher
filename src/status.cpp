#include <iostream>
#include <iomanip>

#include "status.h"
#include "log.h"

using std::ostream;
using std::endl;

ostream &operator<<(ostream &out, const Status &status)
{
  out << "SFW STATUS SUMMARY\n"
    << "* main thread:\n"
    << "  - " << plural(status.pending_callback_count, "pending callback") << "\n"
    << "  - " << plural(status.channel_callback_count, "channel callback") << "\n"
    << "* worker thread:\n"
    << "  - health: " << status.worker_thread_ok << "\n"
    << "  - in queue health: " << status.worker_in_ok << "\n"
    << "  - " << plural(status.worker_in_size, "in queue message") << "\n"
    << "  - out queue health: " << status.worker_out_ok << "\n"
    << "  - " << plural(status.worker_out_size, "out queue message")
    << endl;
  return out;
}
