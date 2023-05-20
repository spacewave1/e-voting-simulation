source("workspace/e-voting/simulation/data-analysis/setupFunctions.R")

#install.packages("testthat")

library(testthat)

test_that(desc = "return sync for n=4", code = {
  
  result_1 = didSyncReturnDataPackageSize(1,4)
  packages = 1 + floor(result_1/536)
  # Test that the result is the correct value
  expect_that( object = packages, condition = equals(9) );
})

test_that(desc = "return sync for n=4", code = {
  
  result_1 = didSyncReturnDataPackageSize(1,4)
  packages = 1 + floor(result_1/536)
  # Test that the result is the correct value
  expect_that( object = packages, condition = equals(9) );
})
