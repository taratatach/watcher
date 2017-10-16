#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#include "result.h"

enum EntryKind
{
  KIND_FILE = 0,
  KIND_DIRECTORY = 1,
  KIND_UNKNOWN = 2,
  KIND_MIN = KIND_FILE,
  KIND_MAX = KIND_UNKNOWN
};

std::ostream &operator<<(std::ostream &out, EntryKind kind);

bool kinds_are_different(EntryKind a, EntryKind b);

using Entry = std::pair<std::string, EntryKind>;

using ChannelID = uint_fast32_t;

const ChannelID NULL_CHANNEL_ID = 0;

enum FileSystemAction
{
  ACTION_CREATED = 0,
  ACTION_DELETED = 1,
  ACTION_MODIFIED = 2,
  ACTION_RENAMED = 3,
  ACTION_MIN = ACTION_CREATED,
  ACTION_MAX = ACTION_RENAMED
};

std::ostream &operator<<(std::ostream &out, FileSystemAction action);

class FileSystemPayload
{
public:
  static FileSystemPayload created(ChannelID channel_id, std::string &&path, const EntryKind &kind)
  {
    return FileSystemPayload(channel_id, ACTION_CREATED, kind, "", std::move(path));
  }

  static FileSystemPayload modified(ChannelID channel_id, std::string &&path, const EntryKind &kind)
  {
    return FileSystemPayload(channel_id, ACTION_MODIFIED, kind, "", std::move(path));
  }

  static FileSystemPayload deleted(ChannelID channel_id, std::string &&path, const EntryKind &kind)
  {
    return FileSystemPayload(channel_id, ACTION_DELETED, kind, "", std::move(path));
  }

  static FileSystemPayload renamed(ChannelID channel_id,
    std::string &&old_path,
    std::string &path,
    const EntryKind &kind)
  {
    return FileSystemPayload(channel_id, ACTION_RENAMED, kind, std::move(old_path), std::move(path));
  }

  FileSystemPayload(FileSystemPayload &&original) noexcept;

  ~FileSystemPayload() = default;

  const ChannelID &get_channel_id() const { return channel_id; }

  const FileSystemAction &get_filesystem_action() const { return action; }

  const EntryKind &get_entry_kind() const { return entry_kind; }

  const std::string &get_old_path() const { return old_path; }

  const std::string &get_path() const { return path; }

  std::string describe() const;

  FileSystemPayload(const FileSystemPayload &original) = delete;
  FileSystemPayload &operator=(const FileSystemPayload &original) = delete;
  FileSystemPayload &operator=(FileSystemPayload &&original) = delete;

private:
  FileSystemPayload(ChannelID channel_id,
    FileSystemAction action,
    EntryKind entry_kind,
    std::string &&old_path,
    std::string &&path);

  const ChannelID channel_id;
  const FileSystemAction action;
  const EntryKind entry_kind;
  std::string old_path;
  std::string path;
};

enum CommandAction
{
  COMMAND_ADD,
  COMMAND_REMOVE,
  COMMAND_LOG_FILE,
  COMMAND_LOG_STDERR,
  COMMAND_LOG_STDOUT,
  COMMAND_LOG_DISABLE,
  COMMAND_POLLING_INTERVAL,
  COMMAND_POLLING_THROTTLE,
  COMMAND_DRAIN,
  COMMAND_MIN = COMMAND_ADD,
  COMMAND_MAX = COMMAND_DRAIN
};

using CommandID = uint_fast32_t;

const CommandID NULL_COMMAND_ID = 0;

class CommandPayload
{
public:
  CommandPayload(CommandAction action,
    CommandID id = NULL_COMMAND_ID,
    std::string &&root = "",
    uint_fast32_t arg = NULL_CHANNEL_ID,
    size_t split_count = 1);

  CommandPayload(CommandPayload &&original) noexcept;

  ~CommandPayload() = default;

  CommandID get_id() const { return id; }

  const CommandAction &get_action() const { return action; }

  const std::string &get_root() const { return root; }

  const uint_fast32_t &get_arg() const { return arg; }

  const ChannelID &get_channel_id() const { return arg; }

  const size_t &get_split_count() const { return split_count; }

  std::string describe() const;

  CommandPayload(const CommandPayload &original) = delete;
  CommandPayload &operator=(const CommandPayload &original) = delete;
  CommandPayload &operator=(CommandPayload &&original) = delete;

private:
  const CommandID id;
  const CommandAction action;
  std::string root;
  const uint_fast32_t arg;
  const size_t split_count;
};

static_assert(sizeof(CommandPayload) <= sizeof(FileSystemPayload), "CommandPayload is larger than FileSystemPayload");

class AckPayload
{
public:
  AckPayload(CommandID key, ChannelID channel_id, bool success, std::string &&message);

  AckPayload(AckPayload &&original) = default;

  ~AckPayload() = default;

  const CommandID &get_key() const { return key; }

  const ChannelID &get_channel_id() const { return channel_id; }

  const bool &was_successful() const { return success; }

  const std::string &get_message() const { return message; }

  std::string describe() const;

  AckPayload(const AckPayload &original) = delete;
  AckPayload &operator=(const AckPayload &original) = delete;
  AckPayload &operator=(AckPayload &&original) = delete;

private:
  const CommandID key;
  const ChannelID channel_id;
  const bool success;
  std::string message;
};

static_assert(sizeof(AckPayload) <= sizeof(FileSystemPayload), "AckPayload is larger than FileSystemPayload");

enum MessageKind
{
  KIND_FILESYSTEM,
  KIND_COMMAND,
  KIND_ACK
};

class Message
{
public:
  static Message ack(const Message &original, bool success, std::string &&message = "");

  static Message ack(const Message &original, const Result<> &result);

  explicit Message(FileSystemPayload &&payload);

  explicit Message(CommandPayload &&payload);

  explicit Message(AckPayload &&payload);

  Message(Message &&original) noexcept;

  ~Message();

  const FileSystemPayload *as_filesystem() const;

  const CommandPayload *as_command() const;

  const AckPayload *as_ack() const;

  std::string describe() const;

  Message(const Message &) = delete;
  Message &operator=(const Message &) = delete;
  Message &operator=(Message &&) = delete;

private:
  MessageKind kind;
  union
  {
    FileSystemPayload filesystem_payload;
    CommandPayload command_payload;
    AckPayload ack_payload;
    bool pending{false};
  };
};

std::ostream &operator<<(std::ostream &stream, const FileSystemPayload &e);

std::ostream &operator<<(std::ostream &stream, const CommandPayload &e);

std::ostream &operator<<(std::ostream &stream, const AckPayload &e);

std::ostream &operator<<(std::ostream &stream, const Message &e);

#endif
