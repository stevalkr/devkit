#pragma once

#include <fmt/core.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace devkit
{

namespace details
{

struct TaskArg
{
  bool new_process;
  bool search_path;
  const std::string command;

  std::vector<std::string>
  tokens() const
  {
    auto iss = std::istringstream{ command };
    auto tokens =
      std::vector<std::string>{ std::istream_iterator<std::string>{ iss },
                                std::istream_iterator<std::string>{} };
    return tokens;
  }

  static std::vector<char*>
  parse_tokens(std::vector<std::string>& tokens)
  {
    std::vector<char*> args;
    for (auto& t : tokens) {
      args.push_back(t.data());
    }
    args.push_back(nullptr);
    return args;
  }
};

} // namespace details

class Task
{
private:
  details::TaskArg arg;

  void
  execute(std::vector<std::string>& tokens) const
  {
    const auto args = details::TaskArg::parse_tokens(tokens);

    if (arg.search_path) {
      execvp(args[0], args.data());
    }
    else {
      execv(args[0], args.data());
    }

    fmt::println(
      stderr, "Task: Error executing {}.", arg.search_path ? "execvp" : "execv");
    exit(EXIT_FAILURE);
  }

public:
  Task(const details::TaskArg& arg)
    : arg{ arg }
  {}

  int
  run() const
  {
    auto tokens = arg.tokens();
    if (tokens.empty()) {
      fmt::println(stderr, "Task: No command specified.");
      return -1;
    }

    if (arg.new_process) {
      pid_t pid = fork();

      switch (pid) {
        case -1: {
          fmt::println(stderr, "Task: Fork failed.");
          return -1;
        }

        case 0: {
          execute(tokens);
        }

        default: {
          waitpid(pid, nullptr, 0);
        }
      }
    }
    else {
      execute(tokens);
    }
    return 0;
  }
};

} // namespace devkit

#ifndef DOCTEST_CONFIG_DISABLE
#include <doctest/doctest.h>

TEST_CASE("testing task")
{
  SUBCASE("task arg")
  {
    auto tokens = devkit::details::TaskArg{ .new_process = true,
                                            .search_path = true,
                                            .command = "ls -l -a ./dir" }
                    .tokens();
    const auto args = devkit::details::TaskArg::parse_tokens(tokens);

    CHECK(args.size() == 5);
    CHECK(std::string{ args[0] } == "ls");
    CHECK(std::string{ args[1] } == "-l");
    CHECK(std::string{ args[2] } == "-a");
    CHECK(std::string{ args[3] } == "./dir");
    CHECK(args[4] == nullptr);
  }

  SUBCASE("task without command")
  {
    devkit::details::TaskArg arg{ .new_process = true, .search_path = true };
    devkit::Task task(arg);
    CHECK(task.run() == -1);
  }

  SUBCASE("task with new process")
  {
    devkit::details::TaskArg arg{ .new_process = true,
                                  .search_path = true,
                                  .command = "echo 'Hello New process'" };
    devkit::Task task(arg);
    CHECK(task.run() == 0);
  }

  SUBCASE("task without new process")
  {
    devkit::details::TaskArg arg{ .search_path = true,
                                  .command = "echo 'Hello World'" };
    devkit::Task task(arg);
    CHECK(task.run() == 0);
  }
}
#endif
