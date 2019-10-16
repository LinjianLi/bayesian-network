#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csv/reader.hpp"
#include "csv/writer.hpp"

TEST(PranavCSV, usability) {
  csv::Reader csv;
  csv.configure_dialect("test_dialect").header(false);
  csv.read("../../lib/csv/tests/inputs/test_08.csv");
  auto rows = csv.rows();
  auto cols = csv.cols();
  std::vector<std::string> expect_cols {"0", "1", "2"};
  EXPECT_EQ(rows.size(), 3);
  EXPECT_EQ(cols, expect_cols);
  EXPECT_EQ(rows[0]["0"], "1");
  EXPECT_EQ(rows[0]["1"], "2");
  EXPECT_EQ(rows[0]["2"], "3");
  EXPECT_EQ(rows[1]["0"], "4");
  EXPECT_EQ(rows[1]["1"], "5");
  EXPECT_EQ(rows[1]["2"], "6");
  EXPECT_EQ(rows[2]["0"], "7");
  EXPECT_EQ(rows[2]["1"], "8");
  EXPECT_EQ(rows[2]["2"], "9");
}

TEST(PranavCSV, correctness) {
  csv::Reader csv;
  csv.read("../../data/dataset/20k_rows_data.csv");
  auto rows = csv.rows();
  auto cols = csv.cols();
  std::vector<std::string> expect_cols {"a", "b", "c", "d", "e"};
  EXPECT_EQ(rows.size(), 20000);
  EXPECT_EQ(cols, expect_cols);
  int r = 0;
  for (auto row : rows) {
    std::string s = std::to_string(++r);
    for (auto col : cols) {
      EXPECT_EQ(1, row[col].size());
      s = s + ' ' + (std::string)(row[col]);
    }
    std::cout << s << std::endl;
  }
}