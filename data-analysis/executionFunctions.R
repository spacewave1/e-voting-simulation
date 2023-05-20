ports_request_data_packets <- function(n){
  return(n-1)
}

ports_reply_data_packets <- function(n,h) {
  packets <- 0
  if(h == 1 || h == n) {
    packets = 1
  } else {
    packets = n + 1
  }
  return(packets)
}

request_rebound_data_packets <- function(h,n){
  packets <- 0
  if(h == 1 || h == n){
    packets = n-1
  } else if(h <= (n/2)) {
    packets = n + (n/2 - h)
  } else if(h > (n/2)) {
    packets = h-1 + n/2
  }
  return(packets)
}

reply_rebound_data_packets <- function(h,n){
  packets <- 0
  if(h == 1 || h == n){
    packets = n/2
  } else if(h <= (n/2)) {
    packets = n + (n/2 - h) + 1
  } else if(h > (n/2)) {
    packets = h + n/2
  }
  return(packets)
}

votes_map_by_time <- function(n, t, voter_id_size) {
  if(t > n){
    stop("variable t must be smaller that n")
  }
  size <- 0
  empty_vote_size <- 4 * (n-t)
  all_vote_ids_size <- (voter_id_size + 2) * n
  place_vote_size <- 6 * t
  entries_head_size = 2 + n * 10
  vote_map_head_size <- 2
  size = place_vote_size + empty_vote_size + vote_map_head_size + entries_head_size + all_vote_ids_size
  return(size)
}

election_groups <- function(r,g, id_size) {
  size <- 0
  tally_ready_ids_size = 0
  if(r > 0){
    for(i in 1:r){
      if(i > 3){
        tally_ready_ids_size = tally_ready_ids_size + id_size + 1
      } else if(i > 30){
        tally_ready_ids_size = tally_ready_ids_size + id_size + 2
      } else {
        tally_ready_ids_size = tally_ready_ids_size + id_size
      }
    }
  }
  commas_in_groups <- r - ceiling(r/g)
  commas_between_groups <- 0
  if(r > 0){
    commas_between_groups = floor((r-1)/g)
  }
  square_brackets <- ceiling(r/g) * 2
  size = tally_ready_ids_size + commas_in_groups + commas_between_groups + square_brackets + 2
  return(size)
}

election_data_size <- function(n, t, r, g, voter_id_size){
  size_in_bytes <- 0
  id_bytes = 1
  sequence_id_bytes = 1
  timestamp_in_bytes = 10
  delimiter_byte = 1
  votes_map = votes_map_by_time(n, t, voter_id_size)
  election_groups = election_groups(r,g, voter_id_size)
  vote_options <- 3
  vote_option_length <- 1
  vote_options_bytes = (vote_options * (vote_option_length + 2)) + (vote_options - 1) + 2
  vote_entry_head <- 2
  result_size <- vote_options * 3 # kann man eigentlich auch abhängig von n machen
  
  size_in_bytes = id_bytes + sequence_id_bytes + timestamp_in_bytes + vote_options_bytes + votes_map + election_groups + vote_options + delimiter_byte *7
  
  return(size_in_bytes)
}

keys_exchange_packets <- function(g) {
  return((g - 1) * 2)
}

election_updates_data_packets <- function(h, n) {
  if(h > n){
    stop("variable h can not be bigger than n")
  }
  packets <- 0
  MSS <- 536
  id_size = 9
  tally_group_size = 4
  
  packets = 3 * (ports_request_data_packets(n) + ports_reply_data_packets(n,h) 
                 + 2 *reply_rebound_data_packets(h,n) + 2* request_rebound_data_packets(h,n) 
                 + request_rebound_data_packets(h,n)) + 
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, 0, 4, id_size)/MSS)) +
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, h, 4, id_size)/MSS)) +
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, h, 4, id_size)/MSS)) +
    keys_exchange_packets(tally_group_size)
  return(packets)
}

did_election_updates_data_packets <- function(h, n) {
  if(h > n){
    stop("variable h can not be bigger than n")
  }
  packets <- 0
  MSS <- 536
  id_size = 159
  tally_group_size = 4
   
  packets = 3 * (ports_request_data_packets(n) + ports_reply_data_packets(n,h) 
                 + 2 *reply_rebound_data_packets(h,n) + 2* request_rebound_data_packets(h,n) 
                 + request_rebound_data_packets(h,n)) + 
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, 0, 4, id_size)/MSS)) +
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, h, 4, id_size)/MSS)) +
    (request_rebound_data_packets(h,n) * floor(election_data_size(n, h, h, 4, id_size)/MSS)) +
    keys_exchange_packets(tally_group_size)
  return(packets)
}
