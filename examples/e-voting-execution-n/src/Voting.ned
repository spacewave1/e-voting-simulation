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
		ethHost9: StandardHost {
			@display("p=700,500");
		}
		ethHost10: StandardHost {
			@display("p=100,500");
		}
		ethHost11: StandardHost {
			@display("p=700,600");
		}
		ethHost12: StandardHost {
			@display("p=100,600");
		}
		ethHost13: StandardHost {
			@display("p=700,700");
		}
		ethHost14: StandardHost {
			@display("p=100,700");
		}
		ethHost15: StandardHost {
			@display("p=700,800");
		}
		ethHost16: StandardHost {
			@display("p=100,800");
		}
		ethHost17: StandardHost {
			@display("p=700,900");
		}
		ethHost18: StandardHost {
			@display("p=100,900");
		}
		ethHost19: StandardHost {
			@display("p=700,1000");
		}
		ethHost20: StandardHost {
			@display("p=100,1000");
		}
		ethHost21: StandardHost {
			@display("p=700,1100");
		}
		ethHost22: StandardHost {
			@display("p=100,1100");
		}
		ethHost23: StandardHost {
			@display("p=700,1200");
		}
		ethHost24: StandardHost {
			@display("p=100,1200");
		}
		ethHost25: StandardHost {
			@display("p=700,1300");
		}
		ethHost26: StandardHost {
			@display("p=100,1300");
		}
		ethHost27: StandardHost {
			@display("p=700,1400");
		}
		ethHost28: StandardHost {
			@display("p=100,1400");
		}
		ethHost29: StandardHost {
			@display("p=700,1500");
		}
		ethHost30: StandardHost {
			@display("p=100,1500");
		}
		ethHost31: StandardHost {
			@display("p=700,1600");
		}
		ethHost32: StandardHost {
			@display("p=100,1600");
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
		ethHost9.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost10.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost11.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost12.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost13.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost14.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost15.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost16.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost17.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost18.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost19.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost20.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost21.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost22.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost23.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost24.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost25.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost26.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost27.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost28.ethg++ <--> Eth100M <--> router4.ethg++;
		ethHost29.ethg++ <--> Eth100M <--> router1.ethg++;
		ethHost30.ethg++ <--> Eth100M <--> router2.ethg++;
		ethHost31.ethg++ <--> Eth100M <--> router3.ethg++;
		ethHost32.ethg++ <--> Eth100M <--> router4.ethg++;

		router1.ethg++ <--> Eth100M <--> router2.ethg++;
		router1.ethg++ <--> Eth100M <--> router3.ethg++;
		router1.ethg++ <--> Eth100M <--> router4.ethg++;
		router2.ethg++ <--> Eth100M <--> router3.ethg++;
		router2.ethg++ <--> Eth100M <--> router4.ethg++;
		router3.ethg++ <--> Eth100M <--> router4.ethg++;
}
