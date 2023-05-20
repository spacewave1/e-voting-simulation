
connectionDataPackets <- function(h,n) {
  dataPackets = 0
  if(h == 1 || h == n ) {
    # received from connections
    dataPackets = dataPackets + 1
    # received from sync
  } else {
    # received from connections
    dataPackets = dataPackets + 2
    # received from sync
  }
  return(dataPackets)
}

didSyncAggregationDataPackageSize <- function(h) {
  didSyncPacketDataBytes <- 0
  type = 1 # Byte
  didDoc1 = 546 # Bytes
  didDoc2 = 713 # Bytes
  didRes = 158 # Bytes
  json_head = 37 # Bytes
  exit_sequence = 4 # Bytes
  if(h > 1) {
    storage = 0
    for(i in 1:(h-1)){
      storage = storage + didDoc2 + didRes
    }
    didSyncPacketDataBytes = type + storage + json_head + exit_sequence
  }
  return(didSyncPacketDataBytes)
}

didSyncReturnDataPackageSize <- function(h, n) {
  didSyncPacketDataBytes <- 0
  type = 1 # Byte
  didDoc1 = 546 # Bytes
  didDoc2 = 713 # Bytes
  didRes = 158 # Bytes
  json_head = 37 # Bytes
  
  exit_sequence = 4 # Bytes
  if(h < n) {
    storage = 0
    for(i in 1:(n-1)){
      storage = storage + didDoc2 + didRes + 1
    }
    storage = storage - 1 # Zwei kommas müssen weg
    didSyncPacketDataBytes = type + storage + didDoc1 + didRes + json_head + exit_sequence
  }
  return(didSyncPacketDataBytes)
}

syncAggregationDataPackageSize <- function(h) {
  syncPacketDataBytes <- 0
  type = 1 # Byte
  ipv4_string = 11 # Bytes
  json_head = 30 # Bytes
  exit_sequence = 4 # Bytes
  if(h > 1) {
    addresses = 0
    for(i in 1:(h-1)){
      addresses = addresses + (ipv4_string * 3 + 3)
    }
    addresses = addresses - 2 # Zwei kommas müssen weg
    syncPacketDataBytes = type + ipv4_string + addresses + json_head + exit_sequence
  }
  return(syncPacketDataBytes)
}

syncReturnDataPackageSize <- function(h, n) {
  syncPacketDataBytes <- 0
  type = 1 # Byte
  ipv4_string = 11 # Bytes
  json_head = 30 # Bytes
  exit_sequence = 4 # Bytes
  if(h < n) {
    addresses = 0
    for(i in 1:(n-1)){
      addresses = addresses + (ipv4_string * 3 + 3)
    }
    addresses = addresses - 2 # Zwei kommas müssen weg
    syncPacketDataBytes = type + ipv4_string + addresses + json_head + exit_sequence
  }
  return(syncPacketDataBytes)
}

didSyncDataPackets <- function(h,n) {
  MSS = 536
  dataPackets = 0
  if(h == 1 ) {
    # received from connections
    dataPackets = 1 + floor(didSyncReturnDataPackageSize(h,n)/MSS)+ 1
    # received from sync
  } else if(h == n){
    dataPackets = 1 + floor(didSyncAggregationDataPackageSize(n)/MSS)+ 1
  } else {
    dataPackets = 1 + floor(didSyncAggregationDataPackageSize(h)/MSS)+ 1 + 1 + floor(didSyncReturnDataPackageSize(h,n)/MSS)+ 1
  }
  return(dataPackets)
}


syncDataPackets <- function(h,n) {
  MSS = 536
  dataPackets = 0
  if(h == 1 ) {
    # received from connections
    dataPackets = 1 + floor(syncReturnDataPackageSize(h,n)/MSS)+ 1
    # received from sync
  } else if(h == n){
    dataPackets = 1 + floor(syncAggregationDataPackageSize(n)/MSS)+ 1
  } else {
    dataPackets = 1 + floor(syncAggregationDataPackageSize(n)/MSS)+ 1 + 1 + floor(syncReturnDataPackageSize(h,n)/MSS)+ 1
  }
  return(dataPackets)
}
