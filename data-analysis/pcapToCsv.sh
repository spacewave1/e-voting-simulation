#mkdir data/e-vote-setup
#mkdir data/e-vote-setup/n4
#mkdir data/e-vote-exe
#mkdir data/e-vote-exe/n4
#mkdir data/did-e-vote-setup
#mkdir data/did-e-vote-setup/n4
#mkdir data/did-e-vote-exe
#mkdir data/did-e-vote-exe/n4

tshark -r ../examples/e-voting-setup/run/results/ethHost1.ip.pcap -E separator=, -E header=y > data/e-vote-setup/n4/ethHost1.csv
tshark -r ../examples/e-voting-setup/run/results/ethHost2.ip.pcap -E separator=, -E header=y > data/e-vote-setup/n4/ethHost2.csv
tshark -r ../examples/e-voting-setup/run/results/ethHost3.ip.pcap -E separator=, -E header=y > data/e-vote-setup/n4/ethHost3.csv
tshark -r ../examples/e-voting-setup/run/results/ethHost4.ip.pcap -E separator=, -E header=y > data/e-vote-setup/n4/ethHost4.csv

tshark -r ../examples/e-voting-execution/run/results/ethHost1.ip.pcap -E separator=, -E header=y > data/e-vote-exe/n4/ethHost1.csv
tshark -r ../examples/e-voting-execution/run/results/ethHost2.ip.pcap -E separator=, -E header=y > data/e-vote-exe/n4/ethHost2.csv
tshark -r ../examples/e-voting-execution/run/results/ethHost3.ip.pcap -E separator=, -E header=y > data/e-vote-exe/n4/ethHost3.csv
tshark -r ../examples/e-voting-execution/run/results/ethHost4.ip.pcap -E separator=, -E header=y > data/e-vote-exe/n4/ethHost4.csv

tshark -r ../examples/did-e-voting-setup/run/results/ethHost1.ip.pcap -E separator=, -E header=y > data/did-e-vote-setup/n4/ethHost1.csv
tshark -r ../examples/did-e-voting-setup/run/results/ethHost2.ip.pcap -E separator=, -E header=y > data/did-e-vote-setup/n4/ethHost2.csv
tshark -r ../examples/did-e-voting-setup/run/results/ethHost3.ip.pcap -E separator=, -E header=y > data/did-e-vote-setup/n4/ethHost3.csv
tshark -r ../examples/did-e-voting-setup/run/results/ethHost4.ip.pcap -E separator=, -E header=y > data/did-e-vote-setup/n4/ethHost4.csv

tshark -r ../examples/did-e-voting-execution/run/results/ethHost1.ip.pcap -E separator=, -E header=y > data/did-e-vote-exe/n4/ethHost1.csv
tshark -r ../examples/did-e-voting-execution/run/results/ethHost2.ip.pcap -E separator=, -E header=y > data/did-e-vote-exe/n4/ethHost2.csv
tshark -r ../examples/did-e-voting-execution/run/results/ethHost3.ip.pcap -E separator=, -E header=y > data/did-e-vote-exe/n4/ethHost3.csv
tshark -r ../examples/did-e-voting-execution/run/results/ethHost4.ip.pcap -E separator=, -E header=y > data/did-e-vote-exe/n4/ethHost4.csv


