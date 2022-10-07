// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cdc_rsync_cli/params.h"

#include "absl/strings/match.h"
#include "common/log.h"
#include "common/path.h"
#include "common/test_main.h"
#include "gtest/gtest.h"

namespace cdc_ft {
namespace params {
namespace {

class TestLog : public Log {
 public:
  explicit TestLog() : Log(LogLevel::kInfo) {}

 protected:
  void WriteLogMessage(LogLevel level, const char* file, int line,
                       const char* func, const char* message) override {
    errors_ += message;
  }

 private:
  std::string errors_;
};

std::string NeedsValueError(const char* option_name) {
  return absl::StrFormat("Option '%s' needs a value", option_name);
}

class ParamsTest : public ::testing::Test {
 public:
  void SetUp() override { prev_stderr_ = std::cerr.rdbuf(errors_.rdbuf()); }

  void TearDown() override { std::cerr.rdbuf(prev_stderr_); }

 protected:
  void ExpectNoError() const {
    EXPECT_TRUE(errors_.str().empty())
        << "Expected empty stderr but got\n'" << errors_.str() << "'";
  }

  void ExpectError(const std::string& expected) const {
    EXPECT_TRUE(absl::StrContains(errors_.str(), expected))
        << "Expected stderr to contain '" << expected << "' but got\n'"
        << errors_.str() << "'";
  }

  void ClearErrors() { errors_.str(std::string()); }

  std::string base_dir_ = GetTestDataDir("params");
  std::string sources_file_ = path::Join(base_dir_, "source_files.txt");
  std::string empty_sources_file_ =
      path::Join(base_dir_, "empty_source_files.txt");

  Parameters parameters_;
  std::stringstream errors_;
  std::streambuf* prev_stderr_;
};

TEST_F(ParamsTest, ParseSucceedsDefaults) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234",
                        "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_STREQ("1.2.3.4", parameters_.options.ip);
  EXPECT_EQ(1234, parameters_.options.port);
  EXPECT_FALSE(parameters_.options.delete_);
  EXPECT_FALSE(parameters_.options.recursive);
  EXPECT_EQ(0, parameters_.options.verbosity);
  EXPECT_FALSE(parameters_.options.quiet);
  EXPECT_FALSE(parameters_.options.whole_file);
  EXPECT_FALSE(parameters_.options.compress);
  EXPECT_FALSE(parameters_.options.checksum);
  EXPECT_FALSE(parameters_.options.dry_run);
  EXPECT_EQ(parameters_.options.copy_dest, nullptr);
  EXPECT_EQ(6, parameters_.options.compress_level);
  EXPECT_EQ(10, parameters_.options.connection_timeout_sec);
  EXPECT_EQ(1, parameters_.sources.size());
  EXPECT_EQ(parameters_.sources[0], "source");
  EXPECT_EQ(parameters_.destination, "destination");
  ExpectNoError();
}

TEST_F(ParamsTest, ParseSucceedsWithOptionFromTwoArguments) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--compress-level", "2",
      "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_EQ(parameters_.options.compress_level, 2);
  ExpectNoError();
}

TEST_F(ParamsTest,
       ParseSucceedsWithOptionFromOneArgumentWithEqualityWithValue) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--compress-level=2",
      "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ASSERT_EQ(parameters_.sources.size(), 1);
  EXPECT_EQ(parameters_.options.compress_level, 2);
  EXPECT_EQ(parameters_.sources[0], "source");
  EXPECT_EQ(parameters_.destination, "destination");
  ExpectNoError();
}

TEST_F(ParamsTest, ParseFailsOnCompressLevelEqualsNoValue) {
  const char* argv[] = {"cdc_rsync.exe", "--compress-level=", "source",
                        "destination", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("compress-level"));
}

TEST_F(ParamsTest, ParseFailsOnPortEqualsNoValue) {
  const char* argv[] = {"cdc_rsync.exe", "--port=", "source", "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("port"));
}

TEST_F(ParamsTest, ParseFailsOnContimeoutEqualsNoValue) {
  const char* argv[] = {"cdc_rsync.exe", "--contimeout=", "source",
                        "destination", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("contimeout"));
}

TEST_F(ParamsTest, ParseFailsOnIpEqualsNoValue) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=", "source", "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("ip"));
}

TEST_F(ParamsTest, ParseWithoutParametersFailsOnMissingSourceAndDestination) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("Missing source");
}

TEST_F(ParamsTest, ParseWithSingleParameterFailsOnMissingDestination) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234",
                        "source", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("Missing destination");
}

TEST_F(ParamsTest, ParseSuccessedsWithMultipleLetterKeyConsumed) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "-rvqWRzcn",
      "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_TRUE(parameters_.options.recursive);
  EXPECT_EQ(parameters_.options.verbosity, 1);
  EXPECT_TRUE(parameters_.options.quiet);
  EXPECT_TRUE(parameters_.options.whole_file);
  EXPECT_TRUE(parameters_.options.relative);
  EXPECT_TRUE(parameters_.options.compress);
  EXPECT_TRUE(parameters_.options.checksum);
  EXPECT_TRUE(parameters_.options.dry_run);
  ExpectNoError();
}

TEST_F(ParamsTest,
       ParseFailsOnMultipleLetterKeyConsumedOptionsWithUnsupportedOne) {
  const char* argv[] = {"cdc_rsync.exe", "-rvqaWRzcn", "source", "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("Unknown option: 'a'");
}

TEST_F(ParamsTest, ParseSuccessedsWithMultipleLongKeyConsumedOptions) {
  const char* argv[] = {"cdc_rsync.exe",
                        "--ip=1.2.3.4",
                        "--port=1234",
                        "--recursive",
                        "--verbosity",
                        "--quiet",
                        "--whole-file",
                        "--compress",
                        "--relative",
                        "--delete",
                        "--checksum",
                        "--dry-run",
                        "--existing",
                        "--json",
                        "source",
                        "destination",
                        NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_TRUE(parameters_.options.recursive);
  EXPECT_EQ(parameters_.options.verbosity, 1);
  EXPECT_TRUE(parameters_.options.quiet);
  EXPECT_TRUE(parameters_.options.whole_file);
  EXPECT_TRUE(parameters_.options.relative);
  EXPECT_TRUE(parameters_.options.compress);
  EXPECT_TRUE(parameters_.options.delete_);
  EXPECT_TRUE(parameters_.options.checksum);
  EXPECT_TRUE(parameters_.options.dry_run);
  EXPECT_TRUE(parameters_.options.existing);
  EXPECT_TRUE(parameters_.options.json);
  ExpectNoError();
}

TEST_F(ParamsTest, ParseFailsOnUnknownKey) {
  const char* argv[] = {"cdc_rsync.exe", "-unknownKey", "source", "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("Unknown option: 'u'");
}

TEST_F(ParamsTest, ParseSuccessedsWithSupportedKeyValue) {
  const char* argv[] = {
      "cdc_rsync.exe",  "--compress-level", "11", "--port=4086",
      "--ip=127.0.0.1", "--contimeout",     "99", "--copy-dest=dest",
      "source",         "destination",      NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_EQ(parameters_.options.compress_level, 11);
  EXPECT_EQ(parameters_.options.connection_timeout_sec, 99);
  EXPECT_EQ(parameters_.options.port, 4086);
  EXPECT_STREQ(parameters_.options.ip, "127.0.0.1");
  EXPECT_STREQ(parameters_.options.copy_dest, "dest");
  ExpectNoError();
}

TEST_F(ParamsTest,
       ParseSuccessedsWithSupportedKeyValueWithoutEqualityForChars) {
  const char* argv[] = {"cdc_rsync.exe", "--port",      "4086", "--ip",
                        "127.0.0.1",     "--copy-dest", "dest", "source",
                        "destination",   NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_EQ(parameters_.options.port, 4086);
  EXPECT_STREQ(parameters_.options.ip, "127.0.0.1");
  EXPECT_STREQ(parameters_.options.copy_dest, "dest");
  ExpectNoError();
}

TEST_F(ParamsTest, ParseFailsOnGameletIpNeedsPort) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=127.0.0.1", "source",
                        "destination", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("--port must specify a valid port");
}

TEST_F(ParamsTest, ParseFailsOnDeleteNeedsRecursive) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--delete",
      "source",        "destination",  NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("--delete does not work without --recursive (-r)");
}

TEST_F(ParamsTest, ParseChecksCompressLevel) {
  int minLevel = Options::kMinCompressLevel;
  int maxLevel = Options::kMaxCompressLevel;
  int levels[] = {minLevel - 1, minLevel, 0, maxLevel, maxLevel + 1};
  bool valid[] = {false, true, false, true, false};

  for (int n = 0; n < std::size(levels); ++n) {
    std::string level = "--compress-level=" + std::to_string(levels[n]);
    const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234",
                          level.c_str(),   "source",       "destination"};
    EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv,
                      &parameters_) == valid[n]);
    if (valid[n]) {
      ExpectNoError();
    } else {
      ExpectError("--compress_level must be between");
    }
    ClearErrors();
  }
}

TEST_F(ParamsTest, ParseFailsOnUnknownKeyValue) {
  const char* argv[] = {"cdc_rsync.exe", "--unknownKey=5", "source",
                        "destination", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("unknownKey");
}

TEST_F(ParamsTest, ParseFailsWithHelpOption) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234",
                        "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  const char* argv2[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "source",
      "destination",   "--help",       NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv2)) - 1, argv2, &parameters_));
  ExpectNoError();

  const char* argv3[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "source",
      "destination",   "-h",           NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv3)) - 1, argv3, &parameters_));
  ExpectNoError();
}

TEST_F(ParamsTest, ParseSucceedsWithIncludeExclude) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--include=*.txt",
      "--exclude",     "*.dat",        "--include",   "*.exe",
      "source",        "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ASSERT_EQ(parameters_.filter_rules.size(), 3);
  ASSERT_EQ(parameters_.filter_rules[0].type, FilterRule::Type::kInclude);
  ASSERT_EQ(parameters_.filter_rules[0].pattern, "*.txt");
  ASSERT_EQ(parameters_.filter_rules[1].type, FilterRule::Type::kExclude);
  ASSERT_EQ(parameters_.filter_rules[1].pattern, "*.dat");
  ASSERT_EQ(parameters_.filter_rules[2].type, FilterRule::Type::kInclude);
  ASSERT_EQ(parameters_.filter_rules[2].pattern, "*.exe");
  ExpectNoError();
}

TEST_F(ParamsTest, FilesFrom_NoFile) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "source",
      "destination",   "--files-from", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("files-from"));
}

TEST_F(ParamsTest, FilesFrom_ImpliesRelative) {
  const char* argv[] = {
      "cdc_rsync.exe",       "--ip=1.2.3.4",    "--port=1234", "--files-from",
      sources_file_.c_str(), base_dir_.c_str(), "destination", NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_TRUE(parameters_.options.relative);
  ExpectNoError();
}

TEST_F(ParamsTest, FilesFrom_WithoutSourceArg) {
  const char* argv[] = {
      "cdc_rsync.exe",       "--ip=1.2.3.4", "--port=1234", "--files-from",
      sources_file_.c_str(), "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  EXPECT_TRUE(parameters_.sources_dir.empty());
  EXPECT_EQ(parameters_.destination, "destination");
  ExpectNoError();
}

TEST_F(ParamsTest, FilesFrom_WithSourceArg) {
  const char* argv[] = {
      "cdc_rsync.exe",       "--ip=1.2.3.4",    "--port=1234", "--files-from",
      sources_file_.c_str(), base_dir_.c_str(), "destination", NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  std::string expected_sources_dir = base_dir_;
  path::EnsureEndsWithPathSeparator(&expected_sources_dir);
  EXPECT_EQ(parameters_.sources_dir, expected_sources_dir);
  EXPECT_EQ(parameters_.destination, "destination");
  ExpectNoError();
}

TEST_F(ParamsTest, FilesFrom_ParsesFile) {
  const char* argv[] = {
      "cdc_rsync.exe",       "--ip=1.2.3.4", "--port=1234", "--files-from",
      sources_file_.c_str(), "destination",  NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  std::vector<const char*> expected = {"file1", "file2", "file3"};
  ASSERT_EQ(parameters_.sources.size(), expected.size());
  for (size_t n = 0; n < expected.size(); ++n) {
    EXPECT_EQ(parameters_.sources[n], expected[n]);
  }
  ExpectNoError();
}

TEST_F(ParamsTest, FilesFrom_EmptyFile_WithoutSourceArg) {
  const char* argv[] = {"cdc_rsync.exe",
                        "--ip=1.2.3.4",
                        "--port=1234",
                        "--files-from",
                        empty_sources_file_.c_str(),
                        "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(empty_sources_file_);
  ExpectError("--files-from option is empty");
}

TEST_F(ParamsTest, FilesFrom_EmptyFile_WithSourceArg) {
  const char* argv[] = {"cdc_rsync.exe",
                        "--ip=1.2.3.4",
                        "--port=1234",
                        "--files-from",
                        empty_sources_file_.c_str(),
                        base_dir_.c_str(),
                        "destination",
                        NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(empty_sources_file_);
  ExpectError("--files-from option is empty");
}

TEST_F(ParamsTest, FilesFrom_NoDestination) {
  const char* argv[] = {"cdc_rsync.exe", "--ip=1.2.3.4",        "--port=1234",
                        "--files-from",  sources_file_.c_str(), NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError("Missing destination");
}

TEST_F(ParamsTest, IncludeFrom_NoFile) {
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4",   "--port=1234", "source",
      "destination",   "--include-from", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("include-from"));
}

TEST_F(ParamsTest, IncludeFrom_ParsesFile) {
  std::string file = path::Join(base_dir_, "include_files.txt");
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--include-from",
      file.c_str(),    "source",       "destination", NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  ASSERT_EQ(parameters_.filter_rules.size(), 1);
  ASSERT_EQ(parameters_.filter_rules[0].type, FilterRule::Type::kInclude);
  ASSERT_EQ(parameters_.filter_rules[0].pattern, "file3");
  ExpectNoError();
}

TEST_F(ParamsTest, ExcludeFrom_NoFile) {
  const char* argv[] = {"cdc_rsync.exe", "source", "destination",
                        "--exclude-from", NULL};
  EXPECT_FALSE(
      Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));
  ExpectError(NeedsValueError("exclude-from"));
}

TEST_F(ParamsTest, ExcludeFrom_ParsesFile) {
  std::string file = path::Join(base_dir_, "exclude_files.txt");
  const char* argv[] = {
      "cdc_rsync.exe", "--ip=1.2.3.4", "--port=1234", "--exclude-from",
      file.c_str(),    "source",       "destination", NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  ASSERT_EQ(parameters_.filter_rules.size(), 2);
  EXPECT_EQ(parameters_.filter_rules[0].type, FilterRule::Type::kExclude);
  EXPECT_EQ(parameters_.filter_rules[0].pattern, "file1");
  EXPECT_EQ(parameters_.filter_rules[1].type, FilterRule::Type::kExclude);
  EXPECT_EQ(parameters_.filter_rules[1].pattern, "file2");
  ExpectNoError();
}

TEST_F(ParamsTest, IncludeExcludeMixed_ProperOrder) {
  std::string exclude_file = path::Join(base_dir_, "exclude_files.txt");
  std::string include_file = path::Join(base_dir_, "include_files.txt");
  const char* argv[] = {"cdc_rsync.exe",
                        "--ip=1.2.3.4",
                        "--port=1234",
                        "--include-from",
                        include_file.c_str(),
                        "--exclude=excl1",
                        "source",
                        "--exclude-from",
                        exclude_file.c_str(),
                        "destination",
                        "--include",
                        "incl1",
                        NULL};
  EXPECT_TRUE(Parse(static_cast<int>(std::size(argv)) - 1, argv, &parameters_));

  ASSERT_EQ(parameters_.filter_rules.size(), 5);
  EXPECT_EQ(parameters_.filter_rules[0].type, FilterRule::Type::kInclude);
  EXPECT_EQ(parameters_.filter_rules[0].pattern, "file3");
  EXPECT_EQ(parameters_.filter_rules[1].type, FilterRule::Type::kExclude);
  EXPECT_EQ(parameters_.filter_rules[1].pattern, "excl1");
  EXPECT_EQ(parameters_.filter_rules[2].type, FilterRule::Type::kExclude);
  EXPECT_EQ(parameters_.filter_rules[2].pattern, "file1");
  EXPECT_EQ(parameters_.filter_rules[3].type, FilterRule::Type::kExclude);
  EXPECT_EQ(parameters_.filter_rules[3].pattern, "file2");
  EXPECT_EQ(parameters_.filter_rules[4].type, FilterRule::Type::kInclude);
  EXPECT_EQ(parameters_.filter_rules[4].pattern, "incl1");
  ExpectNoError();
}

}  // namespace
}  // namespace params
}  // namespace cdc_ft