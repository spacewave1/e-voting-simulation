package voting;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.ethernet.Eth100M;
import inet.node.inet.AdhocHost;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.physicallayer.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.visualizer.integrated.IntegratedMultiVisualizer;


network Simulation
{
	@display("bgb=1600,800");
	submodules:
		configurator: Ipv4NetworkConfigurator {
			@display("p=80,50");
		}
		visualizer: IntegratedMultiVisualizer {
			@display("p=160,50");
		}
		ethHost1: StandardHost {
			@display("p=700,100");
		}
		ethHost2: StandardHost {
			@display("p=100,100");
		}
		ethHost3: StandardHost {
			@display("p=700,200");
		}
		ethHost4: StandardHost {
			@display("p=100,200");
		}
		ethHost5: StandardHost {
			@display("p=700,300");
		}
		ethHost6: StandardHost {
			@display("p=100,300");
		}
		ethHost7: StandardHost {
			@display("p=700,400");
		}
		ethHost8: StandardHost {
			@display("p=100,400");
		}
		router1: Router {
			@display("p=300,400");
		}
		router2: Router {
			@display("p=500,400");
		}
		router3: Router {
			@display("p=400,300");
		}
		router4: Router {
			@display("p=400,500");
		}
	connections:
		ethHost1.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost2.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost3.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost4.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost5.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost6.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost7.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost8.ethg++ <--> Eth100M <--> router4.ethg++;

		router1.ethg++ <--> Eth100M <--> router2.ethg++;
		router1.ethg++ <--> Eth100M <--> router3.ethg++;
		router1.ethg++ <--> Eth100M <--> router4.ethg++;
		router2.ethg++ <--> Eth100M <--> router3.ethg++;
		router2.ethg++ <--> Eth100M <--> router4.ethg++;
		router3.ethg++ <--> Eth100M <--> router4.ethg++;
}
