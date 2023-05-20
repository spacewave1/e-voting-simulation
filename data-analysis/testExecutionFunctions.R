source("workspace/e-voting/simulation/data-analysis/executionFunctions.R")

#install.packages("testthat")

library(testthat)

test_that(desc = "request rebound test positions for n=8", code = {
  
  result_1 = request_rebound_data_packets(1,8)
  result_2 = request_rebound_data_packets(2,8)
  result_3 = request_rebound_data_packets(3,8)
  result_4 = request_rebound_data_packets(4,8)
  result_5 = request_rebound_data_packets(5,8)
  result_6 = request_rebound_data_packets(6,8)
  result_7 = request_rebound_data_packets(7,8)
  result_8 = request_rebound_data_packets(8,8)
  
  # Test that the result is the correct value
  expect_that( object = result_1, condition = equals(7) );
  expect_that( object = result_2, condition = equals(10) );
  expect_that( object = result_3, condition = equals(9) );
  expect_that( object = result_4, condition = equals(8) );
  expect_that( object = result_5, condition = equals(8) );
  expect_that( object = result_6, condition = equals(9) );
  expect_that( object = result_7, condition = equals(10) );
  expect_that( object = result_8, condition = equals(7) );
})

test_that(desc = "reply rebound test positions for n=8", code = {
  
  result_1 = reply_rebound_data_packets(1,8)
  result_2 = reply_rebound_data_packets(2,8)
  result_3 = reply_rebound_data_packets(3,8)
  result_4 = reply_rebound_data_packets(4,8)
  result_5 = reply_rebound_data_packets(5,8)
  result_6 = reply_rebound_data_packets(6,8)
  result_7 = reply_rebound_data_packets(7,8)
  result_8 = reply_rebound_data_packets(8,8)
  
  # Test that the result is the correct value
  expect_that( object = result_1, condition = equals(4) );
  expect_that( object = result_2, condition = equals(11) );
  expect_that( object = result_3, condition = equals(10) );
  expect_that( object = result_4, condition = equals(9) );
  expect_that( object = result_5, condition = equals(9) );
  expect_that( object = result_6, condition = equals(10) );
  expect_that( object = result_7, condition = equals(11) );
  expect_that( object = result_8, condition = equals(4) );
})

test_that(desc = "votes map by time n=4", code = {
  result_1 = votes_map_by_time(4,1,7)
  result_2 = votes_map_by_time(4,2,7)
  result_3 = votes_map_by_time(4,3,7)
  result_4 = votes_map_by_time(4,4,7)
  
  # Test that the result is the correct value
  expect_that(object = result_1, condition = equals(98));
  expect_that(object = result_2, condition = equals(100));
  expect_that(object = result_3, condition = equals(102));
  expect_that(object = result_4, condition = equals(104));
})

test_that(desc = "groups by time n=4", code = {
  result_0 = election_groups(0,4,10)
  result_1 = election_groups(1,4,10)
  result_2 = election_groups(2,4,10)
  result_3 = election_groups(3,4,10)
  result_4 = election_groups(4,4,10)
  
  # Test that the result is the correct value
  expect_that(object = result_0, condition = equals(2));
  expect_that(object = result_1, condition = equals(14));
  expect_that(object = result_2, condition = equals(25));
  expect_that(object = result_3, condition = equals(36));
  expect_that(object = result_4, condition = equals(48));
})

test_that(desc = "election data for n=4", code = {
  result_1 = election_data_size(4,1,0,4,10)
  result_2 = election_data_size(4,2,0,4,10)
  result_3 = election_data_size(4,3,0,4,10)
  result_4 = election_data_size(4,4,0,4,10)
  election_data_size
  # Test that the result is the correct value
  expect_that(object = result_1, condition = equals(147));
  expect_that(object = result_2, condition = equals(149));
  expect_that(object = result_3, condition = equals(151));
  expect_that(object = result_4, condition = equals(153));
})
