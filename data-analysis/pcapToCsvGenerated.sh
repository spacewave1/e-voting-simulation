n=8

rm -rf "data/e-vote-setup/n"$n"gen"
rm -rf "data/e-vote-exe/n"$n"gen"
rm -rf "data/did-e-vote-setup/n"$n"gen"
rm -rf "data/did-e-vote-exe/n"$n"gen"

mkdir "data/e-vote-setup/n"$n"gen"
mkdir "data/e-vote-exe/n"$n"gen"
mkdir "data/did-e-vote-setup/n"$n"gen"
mkdir "data/did-e-vote-exe/n"$n"gen"

for (( i=1; i <= n; ++i ))
do
  tshark -r "../examples/e-voting-setup-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/e-vote-setup/n"$n"gen/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/e-vote-setup/n"$n"gen/ethHost"$i".csv"
  tshark -r "../examples/e-voting-execution-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/e-vote-exe/n"$n"gen/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/e-vote-exe/n"$n"gen/ethHost"$i".csv"
  tshark -r "../examples/did-e-voting-setup-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/did-e-vote-setup/n"$n"gen/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/did-e-vote-setup/n"$n"gen/ethHost"$i".csv"
  tshark -r "../examples/did-e-voting-execution-n/run/results/ethHost"$i".ip.pcap" -E header=y > "data/did-e-vote-exe/n"$n"gen/ethHost"$i".csv"
  sed -i -e 's/   /;/g' "data/did-e-vote-exe/n"$n"gen/ethHost"$i".csv"
done