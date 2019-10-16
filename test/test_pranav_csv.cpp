#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "csv/reader.hpp"
#include "csv/writer.hpp"

#include "CustomNetwork.h"

using namespace std;

TEST(PranavCSV, usability) {
  csv::Reader csv;
  csv.configure_dialect("test_dialect").header(false);
  csv.read("../../lib/csv/tests/inputs/test_08.csv");
  auto rows = csv.rows();
  EXPECT_EQ(rows.size(), 3);
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