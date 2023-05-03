# Generate ini file
file="./run/omnetpp.ini"
n=$n
nRouters=4
tConnect=0.02
tConnectDelta=0.05
tSend=0.04
tListenStart=0.01
tSyncInit=0.05
tListenDownSync=0.04
tSyncDelta=0.05

echo "[Config PcapRecording]" > $file
echo "network = voting.Simulation" >> $file
echo "" >> $file
echo "# traffic settings" >> $file
echo "" >> $file

for (( i=1; i <= n; ++i ))
do
  echo "*.ethHost$i.numApps = 1" >> $file
  echo "*.ethHost$i.app[0].typename = \"voting.VotingSetupApp\"" >> $file
  echo "*.ethHost$i.app[0].active = true" >> $file
  echo "*.ethHost$i.app[0].localAddress = \"10.0.0.$((1+4*(i-1)))\"" >> $file
  echo "*.ethHost$i.app[0].localPort = 5555" >> $file
  if ((i < n)); then
    echo "*.ethHost$i.app[0].connectAddress = \"10.0.0.$((1+4*i))\"" >> $file
    unset connectTime
    connectTime=$(awk -v i="${i}" -v tConnect="${tConnect}" -v tDelta="${tConnectDelta}" 'BEGIN{print (tConnect+tDelta*(i-1))}')
    echo "*.ethHost$i.app[0].tConnect = ${connectTime}s" >> $file
  fi
  if [[ $i == 1 ]]; then
    unset sendTime
    sendTime=$(awk -v i="${i}" -v tSend="${tSend}" -v tDelta="${tConnectDelta}" 'BEGIN{print (tSend+tDelta*(i-1))}')
    echo "*.ethHost$i.app[0].tSend = ${sendTime}s" >> $file
    unset syncInitTime
    syncInitTime=$(awk -v i="${i}" -v n="${n}" -v tSyncInit="${tSyncInit}" -v tDelta="${tSyncDelta}" 'BEGIN{print (tSyncInit+n*0.05+tDelta*(i-1))}')
    echo "*.ethHost$i.app[0].tSyncInit = ${syncInitTime}s" >> $file
  else
    unset listenDownSyncTime
    listenDownSyncTime=$(awk -v i="${i}" -v n="${n}" -v tListenDownSync="${tListenDownSync}" -v tDelta="${tSyncDelta}" 'BEGIN{print (tListenDownSync+n*0.05)}')
    echo "*.ethHost$i.app[0].tListenDownSync = ${listenDownSyncTime}s" >> $file
  fi
  unset listenStartTime
  listenStartTime=$(awk -v i="${i}" -v n="${n}" -v tListenStart="${tListenStart}" -v tDelta="${tConnectDelta}" 'BEGIN{print (tListenStart+tDelta*((i-2+n)%n))}')
  echo "*.ethHost$i.app[0].tListenStart = ${listenStartTime}s" >> $file
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
echo "*.visualizer.*.transportConnectionVisualizer[*].displayTransportConnections = true" >> $file
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
echo -e '\t@display("'bgb=$((n*100+100)),$((n/2*100+100))'");' >> $ned_file
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
  c=$((i % 2))
  m=$((i / 2))
  if [ $c == 0 ]; then
    x=100
    y=$((100+100*(m-1)))
  elif [ $c == 1 ]; then
    x=700
    y=$((100+100*m))
  fi

  echo -e '\t\tethHost'$i': StandardHost {' >> $ned_file
  echo -e '\t\t\t@display("'p=$((x)),$((y))'");' >> $ned_file
  echo -e '\t\t}' >> $ned_file

done

for (( i=1; i <= nRouters; ++i ))
do
  x=0
  y=0
  c=$((i % 2))
  m=$((i / 2))
  if [ $c == 0 ]; then
    x=200
    y=$((100+100*(m-1)))
  elif [ $c == 1 ]; then
    x=600
    y=$((100+100*m))
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