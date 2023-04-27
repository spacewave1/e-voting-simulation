# Generate ini file
file="./run/omnetpp.ini"
n=4
nRouters=4
tCreateElection=0.01
tPlaceVoteStart=1
tPlaceVoteDelta=2
tThreePReceive=0.01
tConfirmVoteStart=8
tConfirmVoteDelta=2
tRequestKeysStart=16
tRequestKeysDelta=2
tTallyStart=30
tTallyDelta=2

echo "[Config PcapRecording]" > $file
echo "network = voting.Simulation" >> $file
echo "" >> $file
echo "# traffic settings" >> $file
echo "" >> $file

for (( i=1; i <= n; ++i ))
do
  echo "*.ethHost$i.numApps = 1" >> $file
  echo "*.ethHost$i.app[0].typename = \"voting.DidVotingApp\"" >> $file
  echo "*.ethHost$i.app[0].active = true" >> $file
  echo "*.ethHost$i.app[0].localAddress = \"10.0.0.$((1+4*(i-1)))\"" >> $file
  echo "*.ethHost$i.app[0].tThreePReceive = ${tThreePReceive}s" >> $file
  if [[ $i == 1 ]]; then
    echo "*.ethHost$i.app[0].tCreateElection = ${tCreateElection}s" >> $file
  fi
  echo "*.ethHost$i.app[0].tPlaceVote = $((tPlaceVoteStart+(i-1)*tPlaceVoteDelta))s" >> $file
  echo "*.ethHost$i.app[0].tConfirmVote = $((tConfirmVoteStart+(i-1)*tConfirmVoteDelta))s" >> $file
  echo "*.ethHost$i.app[0].tRequestKeys = $((tRequestKeysStart+(i-1)*tRequestKeysDelta))s" >> $file
  echo "*.ethHost$i.app[0].tTallyAt = $((tTallyStart+(i-1)*tTallyDelta))s" >> $file
  echo "" >> $file
  echo "" >> $file
done

echo "# misc settings" >> $file
echo "**.crcMode = \"computed\"" >> $file
echo "**.fcsMode = \"computed\"" >> $file

for (( i=1; i <= n; ++i ))
do
  echo "*.ethHost$i.numPcapRecorders = 1" >> $file
  echo "*.ethHost$i.pcapRecorder[*].pcapLinkType = 101	# raw IP" >> $file
  echo "*.ethHost$i.pcapRecorder[*].pcapFile = \"results/ethHost$i.ip.pcap\"" >> $file
  echo "*.ethHost$i.pcapRecorder[*].moduleNamePatterns = \"ipv4\"" >> $file
  echo "*.ethHost$i.pcapRecorder[*].dumpProtocols = \"ipv4\"" >> $file
  echo "" >> $file
done

for (( i=1; i <= nRouters; ++i ))
do
  echo "*.router$i.numPcapRecorders = 2" >> $file
  echo "*.router$i.pcapRecorder[0].pcapLinkType = 204		# ppp" >> $file
  echo "*.router$i.pcapRecorder[0].moduleNamePatterns = \"ppp[*]\"" >> $file
  echo "*.router$i.pcapRecorder[0].pcapFile = \"results/router$i.ppp.pcap\"" >> $file
  echo "*.router$i.pcapRecorder[1].pcapLinkType = 1		# ethernet" >> $file
  echo "*.router$i.pcapRecorder[1].pcapFile = \"results/router$i.eth.pcap\"" >> $file
  echo "*.router$i.pcapRecorder[1].moduleNamePatterns = \"eth[*]\"" >> $file
  echo "" >> $file
done

echo "**.pcapRecorder[*].verbose = true	# is this needed? doesnt seem to work ok" >> $file
echo "**.pcapRecorder[*].alwaysFlush = true" >> $file

echo "# visualizer settings" >> $file
echo "*.visualizer.*.numDataLinkVisualizers = 2" >> $file
echo "*.visualizer.*.numInterfaceTableVisualizers = 2" >> $file
echo "*.visualizer.*.dataLinkVisualizer[0].displayLinks = true" >> $file
echo "*.visualizer.*.dataLinkVisualizer[0].packetFilter = \"not *ping*\"" >> $file
echo "*.visualizer.*.physicalLinkVisualizer[*].displayLinks = true" >> $file
echo "*.visualizer.*.interfaceTableVisualizer[0].displayInterfaceTables = true" >> $file
echo "*.visualizer.*.interfaceTableVisualizer[0].format = \"%N\"" >> $file
echo "*.visualizer.*.interfaceTableVisualizer[1].displayInterfaceTables = true" >> $file
echo "*.visualizer.*.interfaceTableVisualizer[1].format = \"%a\"" >> $file
echo "*.visualizer.*.interfaceTableVisualizer[1].displayWiredInterfacesAtConnections = false" >> $file
echo "*.visualizer.*.transportConnectionVisualizer[*].displayTransportConnections = false" >> $file
echo "*.visualizer.*.dataLinkVisualizer[1].displayLinks = true" >> $file
echo "*.visualizer.*.dataLinkVisualizer[1].packetFilter = \"*ping*\"" >> $file
echo "*.visualizer.*.dataLinkVisualizer[1].lineColor = \"red\"" >> $file
echo "*.visualizer.*.infoVisualizer[*].modules = \"*.*.pcapRecorder[*]\"" >> $file
echo "*.visualizer.*.infoVisualizer[*].format = \"%t\"" >> $file


# Generate Voting.ned file

ned_file="./src/Voting.ned"

echo "package voting;" > $ned_file
echo "" >> $ned_file
echo "import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;" >> $ned_file
echo "import inet.node.ethernet.Eth100M;" >> $ned_file
echo "import inet.node.inet.AdhocHost;" >> $ned_file
echo "import inet.node.inet.Router;" >> $ned_file
echo "import inet.node.inet.StandardHost;" >> $ned_file
echo "import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;" >> $ned_file
echo "import inet.visualizer.integrated.IntegratedMultiVisualizer;" >> $ned_file
echo "" >> $ned_file
echo "" >> $ned_file

echo "network Simulation" >> $ned_file
echo "{" >> $ned_file
echo -e '\t@display("bgb=1600,800");' >> $ned_file
echo -e '\tsubmodules:' >> $ned_file
echo -e '\t\tconfigurator: Ipv4NetworkConfigurator {' >> $ned_file
echo -e '\t\t\t@display("p=80,50");' >> $ned_file
echo -e '\t\t}' >> $ned_file
echo -e '\t\tvisualizer: IntegratedMultiVisualizer {' >> $ned_file
echo -e '\t\t\t@display("p=160,50");' >> $ned_file
echo -e '\t\t}' >> $ned_file

for (( i=1; i <= n; ++i ))
do
  x=0
  y=0
  c=$((i % nRouters))
  m=$((i / (nRouters + 1)))
  if [ $c == 1 ]; then
    x=100
    y=$((400+100*m))
  elif [ $c == 2 ]; then
    x=700
    y=$((400+100*m))
  elif [ $c == 3 ]; then
    x=$((400+200*m))
    y=200
  elif [ $c == 0 ]; then
    x=$((400+100*m))
    y=600
  fi

  echo -e '\t\tethHost'$i': StandardHost {' >> $ned_file
  echo -e '\t\t\t@display("'p=$((x)),$((y))'");' >> $ned_file
  echo -e '\t\t}' >> $ned_file

done

for (( i=1; i <= nRouters; ++i ))
do
  x=0
    y=0
    c=$((i % nRouters))
    if [ $c == 1 ]; then
      x=300
      y=400
    elif [ $c == 2 ]; then
      x=500
      y=400
    elif [ $c == 3 ]; then
      x=400
      y=300
    elif [ $c == 0 ]; then
      x=400
      y=500
    fi

  echo -e '\t\trouter'$i': Router {' >> $ned_file
  echo -e '\t\t\t@display("'p=$((x))','$((y))'");' >> $ned_file
  echo -e '\t\t}' >> $ned_file
done

echo -e '\tconnections:' >> $ned_file
for (( i=1; i <= n; ++i ))
do
  r=$(((i-1)%(nRouters)+1))
  echo -e '\t\tethHost'$i'.ethg++ <--> Eth100M <--> router'$r'.ethg++;' >> $ned_file
done

echo "" >> $ned_file

for (( r=1; r < nRouters; ++r ))
do
  for ((rpp=r+1; rpp <= nRouters; ++rpp ))
  do
    echo -e '\t\trouter'$r'.ethg++ <--> Eth100M <--> router'$rpp'.ethg++;' >> $ned_file
  done
done

echo -e '}' >> $ned_file