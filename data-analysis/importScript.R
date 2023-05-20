#devtools::install_github("tidyverse/glue")
#install.packages("glue")
source("workspace/e-voting/simulation/data-analysis/setupFunctions.R")
source("workspace/e-voting/simulation/data-analysis/executionFunctions.R")

library(glue)
library(readr)
library(plyr)
library(ggplot2)
library(tidyr)

n=8

setupDir = glue("workspace/e-voting/simulation/data-analysis/data/e-vote-setup/n{n}gen")
voteExecutionDir = glue("workspace/e-voting/simulation/data-analysis/data/e-vote-exe/n{n}gen")
didSetupDir = glue("workspace/e-voting/simulation/data-analysis/data/did-e-vote-setup/n{n}gen")
didExeuctionDir = glue("workspace/e-voting/simulation/data-analysis/data/did-e-vote-exe/n{n}gen")

setupsCSVs = list.files(path=setupDir, pattern="*.csv", full.names=TRUE)
executionsCSVs = list.files(path=voteExecutionDir, pattern="*.csv", full.names=TRUE)
didSetupsCSVs = list.files(path=didSetupDir, pattern="*.csv", full.names=TRUE)
didExecutionsCSVs = list.files(path=didExeuctionDir, pattern="*.csv", full.names=TRUE)

setup_df <- data.frame(host = 1:n, pkg_count=0, data_pkg_count=0, formula_data_pkg_count=0)
exec_df <- data.frame(host = 1:n, pkg_count=0, data_pkg_count=0, formula_data_pkg_count=0)
did_setup_df <- data.frame(host = 1:n, pkg_count=0, data_pkg_count=0, formula_data_pkg_count=0)
did_exec_df <- data.frame(host = 1:n, pkg_count=0, data_pkg_count=0, formula_data_pkg_count=0)

# TODO: Auf recipient ip achten
for (i in 1:n) {
  calculatedDataPackets <- connectionDataPackets(i,n) + syncDataPackets(i,n)
    
  setupData <- read_delim(setupsCSVs[i], delim = ";",
                          escape_double = FALSE, col_names = FALSE, col_types = cols(X1 = col_skip(), 
                                                                                     X2 = col_integer()), trim_ws = TRUE)
  packageCount = max(setupData$X2)
  packageCountWithData = dplyr::filter(setupData, !grepl("Len=0",X5))
  setup_df$pkg_count[i] = packageCount
  setup_df$data_pkg_count[i] = length(packageCountWithData$X2)
  setup_df$expected_data_pkg_count[i] = calculatedDataPackets
}

for (i in 1:n) {
  expectedDataPackets <- election_updates_data_packets(i,n)
  
  execData <- read_delim(executionsCSVs[i], delim = ";",
                          escape_double = FALSE, col_names = FALSE, col_types = cols(X1 = col_skip(), 
                                                                                     X2 = col_integer()), trim_ws = TRUE)
  packageCount = max(execData$X2)
  packageCountWithData = dplyr::filter(execData, !grepl("Len=0",X5))
  exec_df$pkg_count[i] = packageCount
  exec_df$data_pkg_count[i] = length(packageCountWithData$X2)
  exec_df$expected_data_pkg_count[i] = expectedDataPackets
}

for (i in 1:n) {
  didCalculatedDataPackets <- connectionDataPackets(i,n) + didSyncDataPackets(i,n)
  didSetupData <- read_delim(didSetupsCSVs[i], delim = ";",
                         escape_double = FALSE, col_names = FALSE, col_types = cols(X1 = col_skip(), 
                                                                                    X2 = col_integer(),
                                                                                    X3 = col_double()), trim_ws = TRUE)
  
  packageCount = max(didSetupData$X2)
  packageCountWithData = dplyr::filter(didSetupData, !grepl("Len=0",X5))

  did_setup_df$pkg_count[i] = packageCount
  did_setup_df$data_pkg_count[i] = length(packageCountWithData$X2)
  did_setup_df$expected_data_pkg_count[i] = didCalculatedDataPackets
}

for (i in 1:n) {
  didExpectedDataPackets <- did_election_updates_data_packets(i,n)
  didExecData <- read_delim(didExecutionsCSVs[i], delim = ";",
                             escape_double = FALSE, col_names = FALSE, col_types = cols(X1 = col_skip(), 
                                                                                        X2 = col_integer(),
                                                                                        X3 = col_double()), trim_ws = TRUE)
  
  packageCount = max(didExecData$X2)
  packageCountWithData = dplyr::filter(didExecData, !grepl("Len=0",X5))
  did_exec_df$pkg_count[i] = packageCount
  did_exec_df$data_pkg_count[i] = length(packageCountWithData$X2)
  did_exec_df$expected_data_pkg_count[i] = didExpectedDataPackets
}

caption_text = glue("n={n}")

std = sd(setup_df$expected_data_pkg_count)

#setup_output_df <- gather(setup_df, key = type, value = package_count, c("pkg_count", "data_pkg_count", "expected_data_pkg_count"))
#ggplot(setup_df, aes(x=host, y = pkg_count)) + geom_point()
#ggplot(setup_df, aes(x=host, y = data_pkg_count)) + geom_point()
#ggplot(setup_df, aes(x=host, y = expected_data_pkg_count)) + geom_point()
ggplot() + ylab("Packages Received") + xlab("Host No.") +
  geom_ribbon(data=setup_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - sd(expected_data_pkg_count), ymax = expected_data_pkg_count + sd(expected_data_pkg_count)),  fill = "mistyrose") + 
  geom_point(data = setup_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(data=setup_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(setup_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type",title = "Voting Setup Packages", caption = caption_text)

#exec_output_df <- gather(exec_df, key = type, value = package_count, c("pkg_count", "data_pkg_count", "expected_data_pkg_count"))
#ggplot(exec_df, aes(x=host, y = pkg_count)) + geom_point()
#ggplot(exec_df, aes(x=host, y = data_pkg_count)) + geom_point()
#ggplot(exec_df, aes(x=host, y = expected_data_pkg_count)) + geom_point()
#ggplot(exec_output_df, aes(x=host, y = package_count, group = type, colour = type)) + geom_point()

std = sd(exec_df$expected_data_pkg_count)

ggplot() + ylab("Packages Received") + xlab("Host No.") +
  geom_ribbon(data=exec_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - sd(expected_data_pkg_count), ymax = expected_data_pkg_count + sd(expected_data_pkg_count)),  fill = "mistyrose") + 
  geom_point(data = exec_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(data=exec_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(exec_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type", title = "Voting Execution Packages", caption = caption_text)

#did_setup_output_df <- gather(did_setup_df, key = type, value = package_count, c("pkg_count", "data_pkg_count", "expected_data_pkg_count"))
#ggplot(did_setup_df, aes(x=host, y = pkg_count)) + geom_point()
#ggplot(did_setup_df, aes(x=host, y = data_pkg_count)) + geom_point()
#ggplot(did_setup_df, aes(x=host, y = expected_data_pkg_count)) + geom_point()

std = sd(did_setup_df$expected_data_pkg_count)

ggplot() + ylab("Packages Received") + xlab("Host No.") +
  geom_ribbon(data=did_setup_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - std, ymax = expected_data_pkg_count + std),  fill = "mistyrose") + 
  geom_point(data = did_setup_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(data=did_setup_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(did_setup_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="Type",title = "Did Setup Packages", caption = caption_text)

#ggplot(did_setup_output_df, aes(x=host, y = package_count, group = type, colour = type)) + geom_point()

#did_exec_output_df <- gather(did_exec_df, key = type, value = package_count, c("pkg_count", "data_pkg_count", "expected_data_pkg_count"))
std = sd(did_exec_df$expected_data_pkg_count)

ggplot() + ylab("Packages Received") + xlab("Host No.") +
  geom_ribbon(data=did_exec_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - std, ymax = expected_data_pkg_count + std),  fill = "mistyrose") + 
  geom_point(data = did_exec_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(data=did_exec_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(did_exec_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type",title = "Did Execution Packages", caption = caption_text)

