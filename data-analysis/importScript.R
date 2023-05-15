#devtools::install_github("tidyverse/glue")
#install.packages("glue")

library(glue)
library(readr)
library(plyr)
name <- "Fred"
glue('My name is {n}.')

n=8

evoteSetupDir  = glue("workspace/e-voting/simulation/data-analysis/data/e-vote-setup/n{n}gen")
evoteSetupFiles = list.files()
  
ethHost<- list()
bla = bind_cols(1)
i=1
read_delim(glue('workspace/e-voting/simulation/data-analysis/data/e-vote-setup/n{n}gen/ethHost{i}.csv'), delim = ";", escape_double = FALSE, col_types = cols(`1` = col_integer(), ...1 = col_skip()), trim_ws = TRUE)
for (i in n) {
  ethHost[i] <- bind_cols(ethHost, read_delim(glue('workspace/e-voting/simulation/data-analysis/data/e-vote-setup/n{n}gen/ethHost{i}.csv'), delim = ";", escape_double = FALSE, col_types = cols(`1` = col_integer(), ...1 = col_skip()), trim_ws = TRUE))
}

View(ethHost2)
