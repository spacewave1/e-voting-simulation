#devtools::install_github("tidyverse/glue")
#install.packages("glue")
source("workspace/e-voting/simulation/data-analysis/setupFunctions.R")
source("workspace/e-voting/simulation/data-analysis/executionFunctions.R")

library(glue)
library(readr)
library(plyr)
library(ggplot2)
library(tidyr)

n=4

setupDir = glue("workspace/e-voting/simulation/data-analysis/data/e-vote-setup/n{n}")
voteExecutionDir = glue("workspace/e-voting/simulation/data-analysis/data/e-vote-exe/n{n}")
didSetupDir = glue("workspace/e-voting/simulation/data-analysis/data/did-e-vote-setup/n{n}")
didExeuctionDir = glue("workspace/e-voting/simulation/data-analysis/data/did-e-vote-exe/n{n}")

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

std = sd(setup_df$data_pkg_count)

y_lab_text = "Anzahl empfangener Pakete"
x_lab_text = "Host Nr."

std_string = "\u03C3≈{round(std,2)},"
n_string = "n={n}"

caption_text = glue(paste(std_string, n_string))
#caption_text = glue(n_string)

title_text = "Wahlnetzwerkaufbau"

ggplot() + ylab(y_lab_text) + xlab(x_lab_text) +
  geom_ribbon(data=setup_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - sd(expected_data_pkg_count), ymax = expected_data_pkg_count + sd(expected_data_pkg_count)),  fill = "mistyrose") + 
  geom_point(shape = 4, data = setup_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(shape = 4, data=setup_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(setup_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type",title = title_text, caption = caption_text) + theme_bw()


std = sd(exec_df$expected_data_pkg_count)

caption_text = glue(paste(std_string, n_string))
#caption_text = glue(n_string)

title_text = "Wahldurchführung"

ggplot() + ylab(y_lab_text) + xlab(x_lab_text) +
  geom_ribbon(data=exec_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - sd(expected_data_pkg_count), ymax = expected_data_pkg_count + sd(expected_data_pkg_count)),  fill = "mistyrose") + 
  geom_point(shape = 4, data = exec_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(shape = 4,data=exec_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(exec_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type", title = title_text, caption = caption_text) + theme_bw()

std = sd(did_setup_df$expected_data_pkg_count)

caption_text = glue(paste(std_string, n_string))
#caption_text = glue(n_string)

title_text = "DID Wahlnetzwerkaufbau"

ggplot() + ylab(y_lab_text) + xlab(x_lab_text) +
  geom_ribbon(data=did_setup_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - std, ymax = expected_data_pkg_count + std),  fill = "mistyrose") + 
  geom_point(shape = 4, data = did_setup_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(shape = 4, data=did_setup_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(did_setup_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="Type",title = title_text, caption = caption_text) + theme_bw()

std = sd(did_exec_df$expected_data_pkg_count)

caption_text = glue(paste(std_string, n_string))
#caption_text = glue(n_string)

title_text = "DID Wahldurchführung"

ggplot() + ylab(y_lab_text) + xlab(x_lab_text) +
  geom_ribbon(data=did_exec_df, mapping = aes(x = host, y= expected_data_pkg_count, ymin = expected_data_pkg_count - std, ymax = expected_data_pkg_count + std),  fill = "mistyrose") + 
  geom_point(shape = 4, data = did_exec_df, aes(x=host, y = pkg_count, col = "sim all")) + 
  geom_point(shape = 4, data=did_exec_df, aes(x=host, y = data_pkg_count, col="sim data")) +
  geom_line(did_exec_df, mapping=aes(x=host, y = expected_data_pkg_count, col="model data")) +
  labs(colour="type", title = title_text, caption = caption_text) + theme_bw()

