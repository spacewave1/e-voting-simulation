#mkdir data/e-vote-setup
#mkdir data/e-vote-setup/n4
#mkdir data/e-vote-exe
#mkdir data/e-vote-exe/n4
#mkdir data/did-e-vote-setup
#mkdir data/did-e-vote-setup/n4
#mkdir data/did-e-vote-exe
#mkdir data/did-e-vote-exe/n4

for (( i=1; i <= 4; ++i ))
do
  dataId=$i
  if [ $i -lt 10 ]; then
    dataId="0$i"
  fi
  echo $dataId
  tshark -r "../examples/e-voting-setup-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/    /;/g' "data/e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/  /;/g' "data/e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/\([0-9]\)[ ]*[ ]\([0-9]*[0-9]*[0-9]*\.[0-9][0-9]\)/\1;\2/g' "data/e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/[ ]\([0-9][0-9][0-9][0-9];\)/;\1/g' "data/e-vote-setup/n4/ethHost"$i".csv"

  echo $dataId
  tshark -r "../examples/e-voting-execution-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/    /;/g' "data/e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/  /;/g' "data/e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/\([0-9]\)[ ]*[ ]\([0-9]*[0-9]*[0-9]*\.[0-9][0-9]\)/\1;\2/g' "data/e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/[ ]\([0-9][0-9][0-9][0-9];\)/;\1/g' "data/e-vote-exe/n4/ethHost"$i".csv"

  echo $dataId
  tshark -r "../examples/did-e-voting-setup-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/did-e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/    /;/g' "data/did-e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/did-e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/  /;/g' "data/did-e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/\([0-9]\)[ ]*[ ]\([0-9]*[0-9]*[0-9]*\.[0-9][0-9]\)/\1;\2/g' "data/did-e-vote-setup/n4/ethHost"$i".csv"
  sed -i -e 's/[ ]\([0-9][0-9][0-9][0-9];\)/;\1/g' "data/did-e-vote-setup/n4/ethHost"$i".csv"

  echo $dataId
  tshark -r "../examples/did-e-voting-execution-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/did-e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/    /;/g' "data/did-e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/did-e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/  /;/g' "data/did-e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/\([0-9]\)[ ]*[ ]\([0-9]*[0-9]*[0-9]*\.[0-9][0-9]\)/\1;\2/g' "data/did-e-vote-exe/n4/ethHost"$i".csv"
  sed -i -e 's/[ ]\([0-9][0-9][0-9][0-9];\)/;\1/g' "data/did-e-vote-exe/n4/ethHost"$i".csv"

done